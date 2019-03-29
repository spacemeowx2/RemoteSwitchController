#ifndef __F_PRO_H_
#define __F_PRO_H_

#include <linux/usb/composite.h>

enum {
	RBEGIN_REPORT_STANDARD = 0x100
};
struct f_opts {
	struct usb_function_instance func_inst;
};
typedef unsigned int bit;
struct pro_report_30 {
	u8 report_id;
	u8 timer;
	u8 battery_conn;
	struct {
		// byte1
		bit Y : 1;
		bit X : 1;
		bit B : 1;
		bit A : 1;
		bit RSR : 1;
		bit RSL : 1;
		bit R : 1;
		bit ZR : 1;
		// byte2
		bit Minus : 1;
		bit Plus : 1;
		bit RStick : 1;
		bit LStick : 1;
		bit Home : 1;
		bit Capture : 1;
		bit _reversed : 1;
		bit ChargingGrip : 1;
		// byte3
		bit Down : 1;
		bit Up : 1;
		bit Right : 1;
		bit Left : 1;
		bit LSR : 1;
		bit LSL : 1;
		bit L : 1;
		bit ZL : 1;
	} btns;

	u8 left_stick[3];
	u8 right_stick[3];
};
struct f_switchpro;
int send_report(struct f_switchpro *sp, const void *data, int size);

#endif
