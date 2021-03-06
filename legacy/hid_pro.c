#include <linux/hid.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <linux/kthread.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/socket.h>
#include <net/sock.h>

#include "usb_common.h"
#include "hid_pro.h"

static DEFINE_SPINLOCK(switch_controller_lock);
static union SwitchController switch_controller;
MODULE_LICENSE("Dual BSD/GPL");

#define USB_BUFSIZ 1024

struct usb_device_descriptor device_desc = {
  .bLength            = USB_DT_DEVICE_SIZE,
  .bDescriptorType    = USB_DT_DEVICE,
  .bcdUSB             = cpu_to_le16(0x200),
  .bDeviceClass       = USB_CLASS_PER_INTERFACE,
  .bDeviceSubClass    = 0,
  .bDeviceProtocol    = 0,
  .bMaxPacketSize0    = 64,
  .idVendor           = cpu_to_le16(PRO_VENDOR_ID),
  .idProduct          = cpu_to_le16(PRO_PRODUCT_ID),
  .bcdDevice          = cpu_to_le16(0x0200),
  .iManufacturer      = IDX_MANUFACTURER,
  .iProduct           = IDX_PRODUCT,
  .iSerialNumber      = IDX_SERIAL_NO,
  .bNumConfigurations = 1,
};

struct usb_string_descriptor string_desc_lang = {
  .bLength         = 4,
  .bDescriptorType = USB_DT_STRING,
  .wData           = { cpu_to_le16(0x0409) },
};

static void update_report(void) {
  // do nothing
}

static void free_ep_req(struct usb_ep *ep, struct usb_request *req) {
	WARN_ON(req->buf == NULL);
  if (req->buf) {
    kfree(req->buf);
  }
	req->buf = NULL;
	usb_ep_free_request(ep, req);
}

static int get_descriptor(
    struct usb_gadget* gadget, const struct usb_ctrlrequest* r) {
  struct driver_data* data = get_gadget_data(gadget);
  u16 w_value = le16_to_cpu(r->wValue);
  u8 type = w_value >> 8;
  u8 index = w_value & 0xff;

  switch (type) {
    case USB_DT_DEVICE:
      if (device_desc.bMaxPacketSize0 > gadget->ep0->maxpacket) {
        device_desc.bMaxPacketSize0 = gadget->ep0->maxpacket;
        printk("%s: shrink ep0 packet size %d\n",
            pro_driver_name, device_desc.bMaxPacketSize0);
      }
      memcpy(data->ep0_request->buf, &device_desc, sizeof(device_desc));
      return sizeof(device_desc);
    case USB_DT_CONFIG:
      return usb_gadget_config_buf(&prog_config_desc, data->ep0_request->buf, USB_BUFSIZ, prog_descriptors);
    case USB_DT_STRING:
      if (index) {
        struct usb_string_descriptor* buf = data->ep0_request->buf;
        const char* string = pro_get_string(index);
        size_t length;
        if (!string) {
          printk("%s: unknown string index %d\n", pro_driver_name, index);
          break;
        }
        buf->bDescriptorType = USB_DT_STRING;
        for (length = 0; string[length]; ++length)
          buf->wData[length] = cpu_to_le16(string[length]);
        buf->bLength = length * 2 + 2;
        return buf->bLength;
      }
      memcpy(data->ep0_request->buf, &string_desc_lang,
          string_desc_lang.bLength);
      return string_desc_lang.bLength;
    case USB_DT_HID_REPORT:
      memcpy(data->ep0_request->buf, pro_hid_report, sizeof(pro_hid_report));
      return sizeof(pro_hid_report);
    default:
      printk("%s: unknown descriptor %02x, ignoring\n", pro_driver_name, type);
      break;
  }
  return -EOPNOTSUPP;
}

static void setup_complete(struct usb_ep* ep, struct usb_request* r) {
  struct driver_data* data = ep->driver_data;
  if (r->status) {
    printk("%s: failed on setup; status=%d, bRequestType=%02x, bRequest=%02x\n",
        pro_driver_name, r->status, data->last_request_type,
        data->last_request);
  }
}

static void report_complete(struct usb_ep* ep, struct usb_request* req) {
  int result;

  switch (req->status) {
	case 0:				/* normal completion */
    update_report();
    spin_lock(&switch_controller_lock);
    memcpy(req->buf, switch_controller.bytes, sizeof(switch_controller.bytes));
    spin_unlock(&switch_controller_lock);
    req->length = sizeof(switch_controller.bytes);
		break;
	/* this endpoint is normally active while we're configured */
	case -ECONNABORTED:		/* hardware forced ep reset */
	case -ECONNRESET:		/* request dequeued */
	case -ESHUTDOWN:		/* disconnect from host */
		printk("%s: %s gone (%d), %d/%d\n", pro_driver_name,
      ep->name, req->status,
      req->actual, req->length);
		return;

	case -EOVERFLOW:		/* buffer overrun on read means that
					 * we didn't provide a big enough
					 * buffer.
					 */
	default:
#if 1
		printk("%s: %s complete --> %d, %d/%d\n", pro_driver_name, ep->name,
				req->status, req->actual, req->length);
#endif
	case -EREMOTEIO:		/* short read */
		break;
  }
  if (req->status) {
    printk("%s: failed to send a report, suspending\n", pro_driver_name);
    return;
  }

  result = usb_ep_queue(ep, req, GFP_ATOMIC);
  if (result < 0)
    printk("%s: failed to queue a report\n", pro_driver_name);
}

static void input_complete(struct usb_ep* ep, struct usb_request* r) {
  if (r->status) {
    printk("%s: failed to recv a input, suspending\n", pro_driver_name);
    return;
  }
  printk("%s: input_complete", pro_driver_name);
}

static int setup(struct usb_gadget* gadget, const struct usb_ctrlrequest* r) {
  struct driver_data* data = get_gadget_data(gadget);
  u16 w_length = le16_to_cpu(r->wLength);
  int value = -EOPNOTSUPP;

  data->ep0_request->zero = 0;
  data->ep0_request->complete = setup_complete;
  data->ep0_request->length = 0;
  data->ep0_request->status = 0;

  value = pro_setup(gadget, r);
  if (value == -EOPNOTSUPP) {
    int type = r->bRequestType & USB_TYPE_MASK;
    if (type == USB_TYPE_STANDARD) switch (r->bRequest) {
      case USB_REQ_GET_DESCRIPTOR:
        value = get_descriptor(gadget, r);
        break;
      case USB_REQ_SET_CONFIGURATION:
        if (data->ep_in && !data->ep_in_request) {
          data->ep_in_request = usb_ep_alloc_request(data->ep_in, GFP_KERNEL);
          if (data->ep_in_request) {
            data->ep_in_request->buf =
              kmalloc(data->ep_in->desc->wMaxPacketSize, GFP_KERNEL);
            if (data->ep_in_request->buf)
              usb_ep_enable(data->ep_in);
          }
          if (data->ep_in_request && data->ep_in_request->buf) {
            data->ep_in_request->status = 0;
            data->ep_in_request->zero = 0;
            data->ep_in_request->complete = report_complete;
            data->ep_in_request->length = data->ep_in->desc->wMaxPacketSize;
            report_complete(data->ep_in, data->ep_in_request);
            value = w_length;
          } else {
            printk("%s: failed to setup endpoints\n", pro_driver_name);
            value = -ENOMEM;
          }
        } else {
          value = w_length;
        }
        if (value >= 0 && data->ep_out && !data->ep_out_request) {
          data->ep_out_request = usb_ep_alloc_request(data->ep_out, GFP_KERNEL);
          if (data->ep_out_request) {
            data->ep_out_request->buf =
              kmalloc(data->ep_out->desc->wMaxPacketSize, GFP_KERNEL);
            if (data->ep_out_request->buf)
              usb_ep_enable(data->ep_out);
          }
          if (data->ep_out_request && data->ep_out_request->buf) {
            data->ep_out_request->status = 0;
            data->ep_out_request->zero = 0;
            data->ep_out_request->complete = input_complete;
            data->ep_out_request->length = data->ep_out->desc->wMaxPacketSize;
            input_complete(data->ep_out, data->ep_out_request);
            value = w_length;
          } else {
            printk("%s: failed to setup endpoints\n", pro_driver_name);
            value = -ENOMEM;
          }
        }
        break;
      default:
        printk("%s: standard setup not impl: %02x-%02x\n",
            pro_driver_name, r->bRequestType, r->bRequest);
        break;
    } else if (type == USB_TYPE_CLASS) switch (r->bRequest) {
      case HID_REQ_GET_REPORT:
        printk("HID_REQ_GET_REPORT\n");
        update_report();
        spin_lock(&switch_controller_lock);
        memcpy(data->ep0_request->buf, switch_controller.bytes, sizeof(switch_controller.bytes));
        spin_unlock(&switch_controller_lock);
        value = sizeof(switch_controller.bytes);
        break;
      case HID_REQ_SET_REPORT:
      case HID_REQ_SET_IDLE:
        value = w_length;
        break;
      default:
        printk("%s: hid class setup not impl: %02x-%02x\n",
            pro_driver_name, r->bRequestType, r->bRequest);
        break;
    } else {
        printk("%s: setup not impl: %02x-%02x\n",
            pro_driver_name, r->bRequestType, r->bRequest);
    }
  }

  if (value >= 0) {
    data->ep0_request->length = min((u16)value, w_length);
    data->ep0_request->zero = value < w_length;
    data->last_request_type = r->bRequestType;
    data->last_request = r->bRequest;
    value = usb_ep_queue(gadget->ep0, data->ep0_request, GFP_ATOMIC);
    if (value < 0)
      printk("%s: usb_ep_queue returns a negative value\n", pro_driver_name);
  }

  return value;
}

static int bind(struct usb_gadget* gadget, struct usb_gadget_driver *driver) {
  struct driver_data* data = kzalloc(sizeof(struct driver_data), GFP_KERNEL);
  if (!data)
    return -ENOMEM;
  data->gadget = gadget;

  set_gadget_data(gadget, data);

  // Initialize EP0 for setup.
  data->ep0_request = usb_ep_alloc_request(gadget->ep0, GFP_KERNEL);
  if (!data->ep0_request)
    return -ENOMEM;
  data->ep0_request->buf = kmalloc(USB_BUFSIZ, GFP_KERNEL);
  if (!data->ep0_request->buf)
    return -ENOMEM;
  gadget->ep0->driver_data = data;

  // Claim endpoints for interrupt transfer.
  data->ep_in = usb_ep_autoconfig(gadget, &prog_in_ep_desc);
  if (data->ep_in) {
    data->ep_in->driver_data = data;
    data->ep_in->desc = &prog_in_ep_desc;
    prog_in_ep_desc.bEndpointAddress = data->ep_in->address;
  } else {
    printk("%s: failed to allocate ep-in\n", pro_driver_name);
    return -EOPNOTSUPP;
  }

  data->ep_out = usb_ep_autoconfig(gadget, &prog_out_ep_desc);
  if (data->ep_out) {
    data->ep_out->driver_data = data;
    data->ep_out->desc = &prog_out_ep_desc;
    prog_out_ep_desc.bEndpointAddress = data->ep_out->address;
  } else {
    printk("%s: failed to allocate ep-out, ignoring\n", pro_driver_name);
  }

  return 0;
}

static void unbind(struct usb_gadget* gadget) {
  struct driver_data* data = get_gadget_data(gadget);
  if (!data)
    return;

  if (data->ep_in) {
    usb_ep_disable(data->ep_in);
    data->ep_in->driver_data = NULL;
    data->ep_in->desc = NULL;
    free_ep_req(data->ep_in, data->ep_in_request);
    data->ep_in_request = NULL;
  }
  if (data->ep_out) {
    usb_ep_disable(data->ep_out);
    data->ep_out->driver_data = NULL;
    data->ep_out->desc = NULL;
    free_ep_req(data->ep_out, data->ep_out_request);
    data->ep_out_request = NULL;
  }

  free_ep_req(gadget->ep0, data->ep0_request);
  data->ep0_request = NULL;

  kfree(data);
  set_gadget_data(gadget, NULL);
}

static void disconnect(struct usb_gadget* gadget) {
  struct driver_data* data = get_gadget_data(gadget);
  if (!data)
    return;

  printk("%s: disconnect\n", pro_driver_name);

  if (data->ep_in_request) {
    free_ep_req(data->ep_in, data->ep_in_request);
    data->ep_in_request = NULL;
  }
  if (data->ep_out_request) {
    free_ep_req(data->ep_out, data->ep_out_request);
    data->ep_out_request = NULL;
  }
  if (data->ep_in) {
    usb_ep_disable(data->ep_in);
    data->ep_in->driver_data = NULL;
    data->ep_in->desc = NULL;
    data->ep_in = NULL;
  }
  if (data->ep_out) {
    usb_ep_disable(data->ep_out);
    data->ep_out->driver_data = NULL;
    data->ep_out->desc = NULL;
    data->ep_out = NULL;
  }
  // TODO: finalize endpoints for interrupt in/out.
}

static void suspend(struct usb_gadget* gadget) {
  printk("%s: suspend not impl\n", pro_driver_name);
}

static void resume(struct usb_gadget* gadget) {
  printk("%s: resume not impl\n", pro_driver_name);
}

static struct usb_gadget_driver driver = {
  .function   = "USB Gadget Test Driver",
  .max_speed  = USB_SPEED_HIGH,
  .bind       = bind,
  .unbind     = unbind,
  .setup      = setup,
  .disconnect = disconnect,
  .suspend    = suspend,
  .resume     = resume,
  .driver = { .owner = THIS_MODULE },
};

struct task_struct* recv_task;
static int recv_func(void *unused)
{
  // must larger than sizeof(switch_controller)
  const int BUF_LEN = 64;
  struct socket v_socket;
  struct socket *sock = &v_socket;
  struct sockaddr_in s_addr;
  unsigned short portnum = 0x8888;
  int ret = 0;
  int opt;
  struct timeval tv;
  char recvbuf[BUF_LEN];
  struct kvec vec;
  struct msghdr msg;

  memset(&s_addr, 0, sizeof(s_addr));
  s_addr.sin_family = AF_INET;
  s_addr.sin_port = htons(portnum);
  s_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  // sock = (struct socket *)kmalloc(sizeof(struct socket), GFP_KERNEL);

  /*create a socket*/
  ret = sock_create_kern(&init_net, PF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock);
  if (ret) {
    printk("server:socket_create error!\n");
    return -1;
  }
  printk("server:socket_create ok!\n");

  opt = 1;
  ret = kernel_setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
  if (ret) {
    printk("SO_REUSEADDR error %d\n", ret);
    return ret;
  }

  tv.tv_sec = 1;
  tv.tv_usec = 0; // 10 * 1000; // 10ms
  ret = kernel_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));
  if (ret) {
    printk("SO_RCVTIMEO error %d\n", ret);
    return ret;
  }

  /*bind the socket*/
  ret = sock->ops->bind(sock, (struct sockaddr *)&s_addr, sizeof(s_addr));
  if (ret < 0) {
    printk("server: bind error\n");
    return ret;
  }
  printk("server:bind ok!\n");

  while (!kthread_should_stop()) {
    memset(recvbuf, 0, BUF_LEN);
    memset(&vec, 0, sizeof(vec));
    memset(&msg, 0, sizeof(msg));
    vec.iov_base = recvbuf;
    vec.iov_len = BUF_LEN;
    ret = kernel_recvmsg(sock, &msg, &vec, 1, BUF_LEN, 0); /*receive message*/
    if (ret > 0) {
      spin_lock(&switch_controller_lock);
      memcpy(switch_controller.bytes, recvbuf, sizeof(switch_controller.bytes));
      spin_unlock(&switch_controller_lock);
    }
  }

  /*release socket*/
  sock_release(sock);
  // kfree(sock);
  return 0;
}

static int __init init(void) {
  driver.function = pro_driver_name;
  recv_task = kthread_run(recv_func, NULL, "recv_task");
  return usb_gadget_probe_driver(&driver);
}
module_init(init);

static void __exit cleanup(void) {
  kthread_stop(recv_task);
  usb_gadget_unregister_driver(&driver);
}
module_exit(cleanup);