#ifndef __SWITCH_PRO_H_
#define __SWITCH_PRO_H_

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

static char pro_driver_name[] = "Gadget HID Driver for Nintendo Switch";

#define PRO_VENDOR_ID 0x057E
#define PRO_PRODUCT_ID 0x2009

static char pro_hid_report[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x15, 0x00,        // Logical Minimum (0)
  0x09, 0x04,        // Usage (Joystick)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x30,        //   Report ID (48)
  0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
  0x05, 0x09,        //   Usage Page (Button)
  0x19, 0x01,        //   Usage Minimum (0x01)
  0x29, 0x0A,        //   Usage Maximum (0x0A)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x0A,        //   Report Count (10)
  0x55, 0x00,        //   Unit Exponent (0)
  0x65, 0x00,        //   Unit (None)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x05, 0x09,        //   Usage Page (Button)
  0x19, 0x0B,        //   Usage Minimum (0x0B)
  0x29, 0x0E,        //   Usage Maximum (0x0E)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x04,        //   Report Count (4)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x02,        //   Report Count (2)
  0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x0B, 0x01, 0x00, 0x01, 0x00,  //   Usage (0x010001)
  0xA1, 0x00,        //   Collection (Physical)
  0x0B, 0x30, 0x00, 0x01, 0x00,  //     Usage (0x010030)
  0x0B, 0x31, 0x00, 0x01, 0x00,  //     Usage (0x010031)
  0x0B, 0x32, 0x00, 0x01, 0x00,  //     Usage (0x010032)
  0x0B, 0x35, 0x00, 0x01, 0x00,  //     Usage (0x010035)
  0x15, 0x00,        //     Logical Minimum (0)
  0x27, 0xFF, 0xFF, 0x00, 0x00,  //     Logical Maximum (65534)
  0x75, 0x10,        //     Report Size (16)
  0x95, 0x04,        //     Report Count (4)
  0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              //   End Collection
  0x0B, 0x39, 0x00, 0x01, 0x00,  //   Usage (0x010039)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x07,        //   Logical Maximum (7)
  0x35, 0x00,        //   Physical Minimum (0)
  0x46, 0x3B, 0x01,  //   Physical Maximum (315)
  0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
  0x75, 0x04,        //   Report Size (4)
  0x95, 0x01,        //   Report Count (1)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x05, 0x09,        //   Usage Page (Button)
  0x19, 0x0F,        //   Usage Minimum (0x0F)
  0x29, 0x12,        //   Usage Maximum (0x12)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x04,        //   Report Count (4)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x34,        //   Report Count (52)
  0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
  0x85, 0x21,        //   Report ID (33)
  0x09, 0x01,        //   Usage (0x01)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x3F,        //   Report Count (63)
  0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x85, 0x81,        //   Report ID (-127)
  0x09, 0x02,        //   Usage (0x02)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x3F,        //   Report Count (63)
  0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x85, 0x01,        //   Report ID (1)
  0x09, 0x03,        //   Usage (0x03)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x3F,        //   Report Count (63)
  0x91, 0x83,        //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
  0x85, 0x10,        //   Report ID (16)
  0x09, 0x04,        //   Usage (0x04)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x3F,        //   Report Count (63)
  0x91, 0x83,        //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
  0x85, 0x80,        //   Report ID (-128)
  0x09, 0x05,        //   Usage (0x05)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x3F,        //   Report Count (63)
  0x91, 0x83,        //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
  0x85, 0x82,        //   Report ID (-126)
  0x09, 0x06,        //   Usage (0x06)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x3F,        //   Report Count (63)
  0x91, 0x83,        //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Volatile)
  0xC0,              // End Collection
};

#endif