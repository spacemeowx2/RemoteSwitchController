#ifndef __F_PRO_H_
#define __F_PRO_H_

#include <linux/usb/composite.h>

struct f_opts {
	struct usb_function_instance func_inst;
};
struct f_switchpro;
int send_report(struct f_switchpro *sp, const void *data, int size);

#endif
