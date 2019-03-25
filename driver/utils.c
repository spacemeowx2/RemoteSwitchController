#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/usb/composite.h>
#include <linux/err.h>
#include "f_pro.h"
#include "switch_pro.h"

struct usb_request *alloc_ep_req(struct usb_ep *ep, int len)
{
	struct usb_request *req;

	req = usb_ep_alloc_request(ep, GFP_ATOMIC);
	if (req) {
		req->length = len;
		req->buf = kmalloc(req->length, GFP_ATOMIC);
		if (!req->buf) {
			usb_ep_free_request(ep, req);
			req = NULL;
		}
	}
	return req;
}

static inline void free_ep_req(struct usb_ep *ep, struct usb_request *req)
{
	kfree(req->buf);
	usb_ep_free_request(ep, req);
}
