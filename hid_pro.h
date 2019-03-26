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

struct usb_config_descriptor prog_config_desc = {
  .bLength             = USB_DT_CONFIG_SIZE,
  .bDescriptorType     = USB_DT_CONFIG,
  // .wTotalLength        = cpu_to_le16(sizeof(struct pro_config_descriptor)),
  .bNumInterfaces      = 1,
  .bConfigurationValue = 1,
  .iConfiguration      = IDX_NULL,
  .bmAttributes        = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_WAKEUP,
  .bMaxPower           = 250,  // x 2mA
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
  .iInterface          = IDX_NULL,
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

struct usb_endpoint_descriptor prog_in_ep_desc = {
    .bLength             = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType     = USB_DT_ENDPOINT,
    .bEndpointAddress    = USB_DIR_IN,      // will be overriden
    .bmAttributes        =
      USB_ENDPOINT_XFER_INT | USB_ENDPOINT_SYNC_NONE | USB_ENDPOINT_USAGE_DATA,
    .wMaxPacketSize      = 64,
    .bInterval           = 8,
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

static const struct usb_descriptor_header *prog_descriptors[] = {
	(struct usb_descriptor_header *)&prog_interface_desc,
	(struct usb_descriptor_header *)&prog_desc,
	(struct usb_descriptor_header *)&prog_in_ep_desc,
	(struct usb_descriptor_header *)&prog_out_ep_desc,
	NULL,
};

union SwitchController {
  struct {
    // byte1
    unsigned int Y : 1;
    unsigned int B : 1;
    unsigned int A : 1;
    unsigned int X : 1;
    unsigned int L : 1;
    unsigned int R : 1;
    unsigned int ZL : 1;
    unsigned int ZR : 1;

    // byte2
    unsigned int minus : 1;
    unsigned int plus : 1;
    unsigned int lclick : 1;
    unsigned int rclick : 1;
    unsigned int home : 1;
    unsigned int capture : 1;
    unsigned int : 2; // unused bits

    // byte3 it should be 0-8 direction not bit enabled
    unsigned int dpad_up : 1;
    unsigned int dpad_down : 1;
    unsigned int dpad_right : 1;
    unsigned int dpad_left : 1;
    unsigned int : 4; // unused bits
    // byte4
    uint8_t LX : 8;

    // byte5
    uint8_t LY : 8;

    // byte6
    uint8_t RX : 8;

    // byte7
    uint8_t RY : 8;

    // byte8
    unsigned int : 8; // vendor specific
  } data;
  uint8_t bytes[8];
};

static const char* pro_get_string(int idx) {
  switch (idx) {
    case IDX_MANUFACTURER:
      return "Nintendo Co., Ltd.";
    case IDX_PRODUCT:
      return "Pro Controller";
    case IDX_SERIAL_NO:
      return "000000000001";
    default:
      break;
  }
  return NULL;
}

static int pro_setup(struct usb_gadget* gadget, const struct usb_ctrlrequest* r) {
  struct driver_data* data = get_gadget_data(gadget);
  int type = r->bRequestType & USB_TYPE_MASK;
  // printk("pro_setup %x %x %x\n", le16_to_cpu(r->wValue), type, r->bRequest);
  if (type == USB_TYPE_CLASS && r->bRequest == HID_REQ_GET_REPORT) {
    switch(le16_to_cpu(r->wValue)) {
      default:
        printk("%s: report page: %04x\n", pro_driver_name, r->wValue);
        break;
    }
  }
  return -EOPNOTSUPP;
}