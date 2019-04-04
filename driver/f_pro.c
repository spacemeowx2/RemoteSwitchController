#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/usb/composite.h>
#include <linux/err.h>
#include "utils.h"
#include "f_pro.h"
#include "switch_pro.h"

#define BULK_BUF_SIZE 64
#define COUNTER_INTERVAL 3

enum report_mode {
	report_none = 0,
	report_standard = 0x30,
};

uint8_t input_reply_30[] = {
	0x30, 0x00, 0x91, 0x00, 0x80, 0x00, 0x19, 0x68, 0x73, 0x3e, 0xa8, 0x73, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

struct f_switchpro {
	struct usb_function	function;
	struct usb_ep				*in_ep;
	struct usb_ep				*out_ep;
	int                 cur_alt;
	enum report_mode    mode;
	int                 counter;
	struct task_struct* recv_task;
	struct task_struct* report_task;
	spinlock_t 					report_lock;
	u8 									report_data[64];
};

#include "pro_reply.c"

struct usb_interface_descriptor prog_interface_desc = {
  .bLength             = USB_DT_INTERFACE_SIZE,
  .bDescriptorType     = USB_DT_INTERFACE,
  .bInterfaceNumber    = 0,
  .bAlternateSetting   = 0,
  .bNumEndpoints       = 2,
  .bInterfaceClass     = USB_CLASS_HID,
  .bInterfaceSubClass  = 0x00,
  .bInterfaceProtocol  = 0x00,
  /* .iInterface          = DYNAMIC */
};
struct hid_descriptor prog_desc = {
  .bLength                      = sizeof prog_desc,
  .bDescriptorType              = HID_DT_HID,
  .bcdHID                       = cpu_to_le16(0x0111),
  .bCountryCode                 = 0,
  .bNumDescriptors              = 1,
  .desc[0].bDescriptorType      = HID_DT_REPORT,
  .desc[0].wDescriptorLength    = cpu_to_le16(sizeof(pro_hid_report)),
};

struct usb_endpoint_descriptor prog_out_ep_desc = {
    .bLength             = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType     = USB_DT_ENDPOINT,
    .bEndpointAddress    = USB_DIR_OUT,     // will be overriden
    .bmAttributes        =
      USB_ENDPOINT_XFER_INT | USB_ENDPOINT_SYNC_NONE | USB_ENDPOINT_USAGE_DATA,
    .wMaxPacketSize      = 64,
    .bInterval           = 8,
};

struct usb_endpoint_descriptor prog_in_ep_desc = {
    .bLength             = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType     = USB_DT_ENDPOINT,
    .bEndpointAddress    = USB_DIR_IN,      // will be overriden
    .bmAttributes        =
      USB_ENDPOINT_XFER_INT | USB_ENDPOINT_SYNC_NONE | USB_ENDPOINT_USAGE_DATA,
    .wMaxPacketSize      = 64,
    .bInterval           = 8,
};

static struct usb_descriptor_header *prog_descriptors[] = {
	(struct usb_descriptor_header *)&prog_interface_desc,
	(struct usb_descriptor_header *)&prog_desc,
	(struct usb_descriptor_header *)&prog_in_ep_desc,
	(struct usb_descriptor_header *)&prog_out_ep_desc,
	NULL,
};

static int switch_pro_start_ep(struct f_switchpro *sp, bool is_in);

static void send_report_complete(struct usb_ep *ep, struct usb_request *req) {
	struct usb_composite_dev	*cdev;
	struct f_switchpro *sp = ep->driver_data;
	int status = req->status;

	/* driver_data will be null if ep has been disabled */
	if (!sp)
		return;

	cdev = sp->function.config->cdev;

	switch (status) {
	case 0:				/* normal completion */
		break;

	/* this endpoint is normally active while we're configured */
	case -ECONNABORTED:		/* hardware forced ep reset */
	case -ECONNRESET:		/* request dequeued */
	case -ESHUTDOWN:		/* disconnect from host */
		VDBG(cdev, "%s gone (%d), %d/%d\n", ep->name, status,
				req->actual, req->length);
		break;

	case -EOVERFLOW:		/* buffer overrun on read means that
					 * we didn't provide a big enough
					 * buffer.
					 */
	default:
#if 1
		DBG(cdev, "%s complete --> %d, %d/%d\n", ep->name,
				status, req->actual, req->length);
#endif
	case -EREMOTEIO:		/* short read */
		break;
	}

	free_ep_req(ep, req);
}

void print_report(const u8* data, int size){
	int i;
	char buf[64] = { 0 };
	
	for (i = 0; i < size; i++) {
		sprintf(buf + strlen(buf), "%02x ", data[i]);
		if ((i % 16) == 15) {
			printk("%s\n", buf);
			buf[0] = 0;
		}
	}
	if (strlen(buf)) {
		printk("%s\n", buf);
	}
}
int send_report(struct f_switchpro *sp, const void *data, int size)
{
	struct usb_request *req = alloc_ep_req(sp->in_ep, prog_in_ep_desc.wMaxPacketSize);
	struct usb_composite_dev *cdev = sp->function.config->cdev;
	struct usb_ep *ep = sp->in_ep;
	u8 *view = req->buf;
	int status;

	if (req == NULL) {
		ERROR(cdev, "send_report alloc_ep_req == NULL\n");
		return -ENOMEM;
	}
	memset(req->buf, 0x00, prog_in_ep_desc.wMaxPacketSize);
	memcpy(req->buf, data, size);
	if (view[0] != 0x81) {
		view[1] = sp->counter;
		sp->counter += COUNTER_INTERVAL;
	}

	if (view[0] != 0x30) {
		print_report(view, prog_in_ep_desc.wMaxPacketSize);
	}

	req->complete = send_report_complete;

	status = usb_ep_queue(ep, req, GFP_ATOMIC);
	if (status) {
		ERROR(cdev, "kill %s:  resubmit %d bytes --> %d\n",
				ep->name, req->length, status);
		usb_ep_set_halt(ep);
		/* FIXME recover later ... somehow */
	}

	return status;
}

static inline struct f_switchpro *func_to_sp(struct usb_function *f)
{
	return container_of(f, struct f_switchpro, function);
}

static void disable_ep(struct usb_composite_dev *cdev, struct usb_ep *ep)
{
	int			value;

	value = usb_ep_disable(ep);
	if (value < 0)
		DBG(cdev, "disable %s --> %d\n", ep->name, value);
}

static void switch_pro_set_mode(struct f_switchpro *sp, enum report_mode mode)
{
	enum report_mode old_mode = sp->mode;
	sp->mode = mode;

	if (old_mode == report_none) {
		switch_pro_start_ep(sp, true);
	}
}

static void switch_pro_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct usb_composite_dev	*cdev;
	struct f_switchpro *sp = ep->driver_data;
	int status = req->status;

	/* driver_data will be null if ep has been disabled */
	if (!sp)
		return;

	cdev = sp->function.config->cdev;

	switch (status) {
	case 0:				/* normal completion */
		if (ep == sp->out_ep) {
			u8 *buf = req->buf;
			ERROR(cdev, "%02x %02x (%02x)\n", buf[0], buf[1], buf[10]);
			status = handle_pro_input(buf, req->length, sp);
			// ERROR(cdev, "handle_pro_input %02x %02x %d", buf[0], buf[1], status);
			switch (status) {
				case RBEGIN_REPORT_STANDARD:
					switch_pro_set_mode(sp, report_standard);
					break;
			}
			if (status == -EOPNOTSUPP) {
				ERROR(cdev, "%02x %02x (%02x) is not supported\n", buf[0], buf[1], buf[10]);
			} else if (status) {
				ERROR(cdev, "%02x %02x error %d\n", buf[0], buf[1], status);
			}
		} else if (ep == sp->in_ep) {
			// ERROR(cdev, "in_ep\n");
		}
		break;

	/* this endpoint is normally active while we're configured */
	case -ECONNABORTED:		/* hardware forced ep reset */
	case -ECONNRESET:		/* request dequeued */
	case -ESHUTDOWN:		/* disconnect from host */
		VDBG(cdev, "%s gone (%d), %d/%d\n", ep->name, status,
				req->actual, req->length);
		free_ep_req(ep, req);
		return;

	case -EOVERFLOW:		/* buffer overrun on read means that
					 * we didn't provide a big enough
					 * buffer.
					 */
	default:
#if 1
		DBG(cdev, "%s complete --> %d, %d/%d\n", ep->name,
				status, req->actual, req->length);
#endif
	case -EREMOTEIO:		/* short read */
		break;
	}

	status = usb_ep_queue(ep, req, GFP_ATOMIC);
	if (status) {
		ERROR(cdev, "kill %s:  resubmit %d bytes --> %d\n",
				ep->name, req->length, status);
		usb_ep_set_halt(ep);
		/* FIXME recover later ... somehow */
	}
}

static int switch_pro_start_ep(struct f_switchpro *sp, bool is_in)
{
	struct usb_ep *ep;
	struct usb_request *req;
	int size, status = 0;

	ep = is_in ? sp->in_ep : sp->out_ep;
	size = BULK_BUF_SIZE;

	req = alloc_ep_req(ep, size);
	if (!req)
		return -ENOMEM;

	req->complete = switch_pro_complete;
	memset(req->buf, 0x0, req->length);

	status = usb_ep_queue(ep, req, GFP_ATOMIC);
	if (status) {
		struct usb_composite_dev	*cdev;

		cdev = sp->function.config->cdev;
		ERROR(cdev, "start %s %s --> %d\n",
					is_in ? "IN" : "OUT",
					ep->name, status);
		free_ep_req(ep, req);
		return status;
	}

	return status;
}

static void disable_switch_pro(struct f_switchpro *sp)
{
	struct usb_composite_dev *cdev;

	cdev = sp->function.config->cdev;
    
	disable_ep(cdev, sp->in_ep);
	disable_ep(cdev, sp->out_ep);
	VDBG(cdev, "%s disabled\n", sp->function.name);
}

static int
enable_switch_pro(struct usb_composite_dev *cdev, struct f_switchpro *sp,
		int alt)
{
	int result = 0;
	struct usb_ep *ep;

	/* one bulk endpoint writes (sources) zeroes IN (to the host) */
	ep = sp->in_ep;
	result = config_ep_by_speed(cdev->gadget, &(sp->function), ep);
	if (result)
		return result;
	result = usb_ep_enable(ep);
	if (result < 0)
		return result;
	ep->driver_data = sp;

	if (sp->mode != report_none) {
		result = switch_pro_start_ep(sp, true);
		if (result < 0) {
	fail:
			ep = sp->in_ep;
			usb_ep_disable(ep);
			return result;
		}
	}

	/* one bulk endpoint reads (sinks) anything OUT (from the host) */
	ep = sp->out_ep;
	result = config_ep_by_speed(cdev->gadget, &(sp->function), ep);
	if (result)
		goto fail;
	result = usb_ep_enable(ep);
	if (result < 0)
		goto fail;
	ep->driver_data = sp;

	result = switch_pro_start_ep(sp, false);
	if (result < 0) {
		ep = sp->out_ep;
		usb_ep_disable(ep);
		goto fail;
	}

	sp->cur_alt = alt;

	DBG(cdev, "%s enabled, alt intf %d\n", sp->function.name, alt);
	return result;
}

static int switchpro_report_func(void *data) {
	struct f_switchpro *sp = data;

	while (!kthread_should_stop()) {
		usleep_range(16000, 16666);

		if (sp->mode == report_standard) {
			// printk("report");
      spin_lock(&sp->report_lock);
			input_reply_30[3] += 1;
			memcpy(sp->report_data, input_reply_30, sizeof(sp->report_data));
			send_report(sp, sp->report_data, sizeof(sp->report_data));
      spin_unlock(&sp->report_lock);
		}
	}

	return 0;
}

static int switchpro_recv_func(void *data) {
	struct f_switchpro *sp = data;
  const int BUF_LEN = 64;
  struct socket *sock = NULL;
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

  /*create a socket*/
  ret = sock_create_kern(&init_net, PF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock);
  if (ret) {
    printk("server:socket_create error!\n");
    return -1;
  }
  printk("server: socket_create ok!\n");

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
  printk("server: bind ok!\n");

  while (!kthread_should_stop()) {
    memset(recvbuf, 0, BUF_LEN);
    memset(&vec, 0, sizeof(vec));
    memset(&msg, 0, sizeof(msg));
    vec.iov_base = recvbuf;
    vec.iov_len = BUF_LEN;
    ret = kernel_recvmsg(sock, &msg, &vec, 1, vec.iov_len, 0); /*receive message*/
    if (ret > 0) {
      spin_lock(&sp->report_lock);
      memcpy(sp->report_data, recvbuf, sizeof(sp->report_data));
      spin_unlock(&sp->report_lock);
    }
  }

  /*release socket*/
  sock_release(sock);
  return 0;
}

static int switchpro_bind(struct usb_configuration *c, struct usb_function *f) {
	struct usb_composite_dev *cdev = c->cdev;
	struct f_switchpro *sp = func_to_sp(f);
	int	id;
	int ret;

	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	prog_interface_desc.bInterfaceNumber = id;

	sp->out_ep = usb_ep_autoconfig(cdev->gadget, &prog_out_ep_desc);
	if (!sp->out_ep) {
autoconf_fail:
		ERROR(cdev, "%s: can't autoconfigure on %s\n",
			f->name, cdev->gadget->name);
		return -ENODEV;
	}

	sp->in_ep = usb_ep_autoconfig(cdev->gadget, &prog_in_ep_desc);
	if (!sp->in_ep)
		goto autoconf_fail;

	ret = usb_assign_descriptors(f,
        prog_descriptors, prog_descriptors,
        NULL, NULL);

	if (ret)
		return ret;

	sp->mode = report_none;

	return 0;
}

static int switchpro_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
{
	struct f_switchpro  *sp = func_to_sp(f);
	struct usb_composite_dev *cdev = f->config->cdev;

	disable_switch_pro(sp);
	return enable_switch_pro(cdev, sp, alt);
}

static int switchpro_get_alt(struct usb_function *f, unsigned intf)
{
	struct f_switchpro *sp = func_to_sp(f);

	return sp->cur_alt;
}

static void switchpro_disable(struct usb_function *f)
{
	struct f_switchpro *sp = func_to_sp(f);

	disable_switch_pro(sp);
}

static int switchpro_setup(struct usb_function *f,
		const struct usb_ctrlrequest *ctrl)
{
	struct usb_configuration *c = f->config;
	struct usb_request *req = c->cdev->req;
	int value = -EOPNOTSUPP;
	int type = ctrl->bRequestType & USB_TYPE_MASK;

	u16	w_index = le16_to_cpu(ctrl->wIndex);
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u16 w_length = le16_to_cpu(ctrl->wLength);

	req->length = USB_COMP_EP0_BUFSIZ;

	if (type == USB_TYPE_STANDARD) switch (ctrl->bRequest) {
	case USB_REQ_GET_DESCRIPTOR:
		ERROR(c->cdev, "switchpro USB_REQ_GET_DESCRIPTOR, len %d\n", w_value >> 8);
		switch (w_value >> 8) {
		case HID_DT_HID:
		{
			w_length = min_t(unsigned short, w_length, prog_desc.bLength);
			memcpy(req->buf, &prog_desc, w_length);
			ERROR(c->cdev, "switchpro HID_DT_HID, len %d\n", w_length);
			goto respond;
			break;
		}
		case HID_DT_REPORT:
			w_length = min_t(unsigned short, w_length, sizeof(pro_hid_report));
			memcpy(req->buf, pro_hid_report, w_length);
			ERROR(c->cdev, "switchpro HID_DT_REPORT, len %d\n", w_length);
			goto respond;
			break;
		default:
			VDBG(c->cdev, "Unknown descriptor request 0x%x\n",
				 w_value >> 8);
			goto stall;
			break;
		}
		break;
	default:
		VDBG(c->cdev,
			"unknown control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
		goto stall;
		break;
	}

stall:
	return -EOPNOTSUPP;

respond:
	/* respond with data transfer or status phase? */
	VDBG(c->cdev, "switchpro req%02x.%02x v%04x i%04x l%d\n",
		ctrl->bRequestType, ctrl->bRequest,
		w_value, w_index, w_length);
	req->zero = 0;
	req->length = w_length;
	value = usb_ep_queue(c->cdev->gadget->ep0, req, GFP_ATOMIC);
	if (value < 0)
		ERROR(c->cdev, "switchpro response, err %d\n",
				value);

	/* device either stalls (value < 0) or reports success */
	return value;
}

static void switchpro_free_func(struct usb_function *f)
{
	struct f_opts *opts;
	struct f_switchpro *sp = func_to_sp(f);

	printk("switchpro_free_func\n");

	opts = container_of(f->fi, struct f_opts, func_inst);

	kthread_stop(sp->report_task);
	kthread_stop(sp->recv_task);

	usb_free_all_descriptors(f);
	kfree(sp);
}

static struct usb_function *switch_pro_alloc_func(
		struct usb_function_instance *fi)
{
	struct f_switchpro *sp;
	struct f_opts	*opts;

	sp = kzalloc(sizeof(*sp), GFP_KERNEL);
	if (!sp)
		return NULL;

	opts = container_of(fi, struct f_opts, func_inst);

	sp->function.name = "switch/pro";
	sp->function.bind = switchpro_bind;
	sp->function.set_alt = switchpro_set_alt;
	sp->function.get_alt = switchpro_get_alt;
	sp->function.disable = switchpro_disable;
	sp->function.setup = switchpro_setup;

	sp->function.free_func = switchpro_free_func;

	spin_lock_init(&sp->report_lock);
	sp->recv_task = kthread_run(switchpro_recv_func, sp, "recv_func");
	sp->report_task = kthread_run(switchpro_report_func, sp, "report_func");

	return &sp->function;
}

static void switch_pro_free_instance(struct usb_function_instance *fi)
{
	struct f_opts *opts;

	opts = container_of(fi, struct f_opts, func_inst);
	kfree(opts);
}

static struct usb_function_instance *switch_pro_alloc_inst(void)
{
	struct f_opts *opts;

	opts = kzalloc(sizeof(*opts), GFP_KERNEL);
	if (!opts)
		return ERR_PTR(-ENOMEM);

	opts->func_inst.free_func_inst = switch_pro_free_instance;

	return &opts->func_inst;
}
