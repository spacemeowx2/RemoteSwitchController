#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/usb/composite.h>
#include <linux/err.h>
#include "utils.c"
#include "f_pro.h"
#include "switch_pro.h"

#define BULK_QLEN 5
#define BULK_BUF_SIZE 64

struct f_switchpro {
	struct usb_function	function;
	struct usb_ep		*in_ep;
	struct usb_ep		*out_ep;
    int                 cur_alt;
};

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
	(struct usb_descriptor_header *)&prog_out_ep_desc,
	(struct usb_descriptor_header *)&prog_in_ep_desc,
	NULL,
};

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
			// TODO: read something here
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
	int i, size, qlen, status = 0;

    ep = is_in ? sp->in_ep : sp->out_ep;
    qlen = BULK_QLEN;
    size = BULK_BUF_SIZE;

	for (i = 0; i < qlen; i++) {
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

	result = switch_pro_start_ep(sp, true);
	if (result < 0) {
fail:
		ep = sp->in_ep;
		usb_ep_disable(ep);
		return result;
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

	return 0;
}

static void switchpro_free_func(struct usb_function *f)
{
	struct f_opts *opts;

	opts = container_of(f->fi, struct f_opts, func_inst);

	usb_free_all_descriptors(f);
	kfree(func_to_sp(f));
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
	u16	w_index = le16_to_cpu(ctrl->wIndex);
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u16 w_length = le16_to_cpu(ctrl->wLength);

	req->length = USB_COMP_EP0_BUFSIZ;

	switch (ctrl->bRequest) {
	default:
		VDBG(c->cdev,
			"unknown control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
	}

	/* respond with data transfer or status phase? */
	if (value >= 0) {
		VDBG(c->cdev, "source/sink req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest,
			w_value, w_index, w_length);
		req->zero = 0;
		req->length = value;
		value = usb_ep_queue(c->cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0)
			ERROR(c->cdev, "source/sink response, err %d\n",
					value);
	}

	/* device either stalls (value < 0) or reports success */
	return value;
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
