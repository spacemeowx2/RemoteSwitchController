static char opg_driver_name[] = "Gadget HID Driver for Nintendo Switch";

#define OPG_VENDOR_ID 0x0f0d
#define OPG_PRODUCT_ID 0x00c1

static char opg_hid_report[] = {
  0x05, 0x01, /* USAGE_PAGE (Generic Desktop)         */
  0x09, 0x05, /* USAGE (Gamepad)                      */
  0xa1, 0x01, /* COLLECTION (Application)             */
  0x15, 0x00, /*     LOGICAL_MINIMUM (0)              */
  0x25, 0x01, /*     LOGICAL_MAXIMUM (1)              */
  0x35, 0x00, /*     PHYSICAL_MINIMUM (0)             */
  0x45, 0x01, /*     PHYSICAL_MAXIMUM (1)             */
  0x75, 0x01, /*     REPORT_SIZE (1)                  */
  0x95, 0x0e, /*     REPORT_COUNT (14)                */
  0x05, 0x09, /*     USAGE_PAGE (Buttons)             */
  0x19, 0x01, /*     USAGE_MINIMUM (Button 1)         */
  0x29, 0x0e, /*     USAGE_MAXIMUM (Button 14)        */
  0x81, 0x02, /*     INPUT (Data,Var,Abs)             */
  0x95, 0x02, /*     REPORT_COUNT (2)                 */
  0x81, 0x01, /*     INPUT (Data,Var,Abs)             */
  0x05, 0x01, /*     USAGE_PAGE (Generic Desktop Ctr) */
  0x25, 0x07, /*     LOGICAL_MAXIMUM (7)              */
  0x46, 0x3b, 0x01, /*     PHYSICAL_MAXIMUM (315)     */
  0x75, 0x04, /*     REPORT_SIZE (4)                  */
  0x95, 0x01, /*     REPORT_COUNT (1)                 */
  0x65, 0x14, /*     UNIT (20)                        */
  0x09, 0x39, /*     USAGE (Hat Switch)               */
  0x81, 0x42, /*     INPUT (Data,Var,Abs)             */
  0x65, 0x00, /*     UNIT (0)                         */
  0x95, 0x01, /*     REPORT_COUNT (1)                 */
  0x81, 0x01, /*     INPUT (Data,Var,Abs)             */
  0x26, 0xff, 0x00, /*     LOGICAL_MAXIMUM (255)      */
  0x46, 0xff, 0x00, /*     PHYSICAL_MAXIMUM (255)     */
  0x09, 0x30, /*     USAGE (Direction-X)              */
  0x09, 0x31, /*     USAGE (Direction-Y)              */
  0x09, 0x32, /*     USAGE (Direction-Z)              */
  0x09, 0x35, /*     USAGE (Rotate-Z)                 */
  0x75, 0x08, /*     REPORT_SIZE (8)                  */
  0x95, 0x04, /*     REPORT_COUNT (4)                 */
  0x81, 0x02, /*     INPUT (Data,Var,Abs)             */
  0x75, 0x08, /*     REPORT_SIZE (8)                  */
  0x95, 0x01, /*     REPORT_COUNT (1)                 */
  0x81, 0x01, /*     INPUT (Data,Var,Abs)             */
  0xc0,       /*   END_COLLECTION                     */
};

struct opg_config_descriptor {
  struct usb_config_descriptor config;
  struct usb_interface_descriptor interface;
  struct usb_hid_descriptor hid;
  struct usb_short_endpoint_descriptor ep_out;
  struct usb_short_endpoint_descriptor ep_in;
} __attribute__ ((packed)) opg_config_desc = {
  .config = {
    .bLength             = USB_DT_CONFIG_SIZE,
    .bDescriptorType     = USB_DT_CONFIG,
    .wTotalLength        = cpu_to_le16(sizeof(struct opg_config_descriptor)), // it's zero
    .bNumInterfaces      = 1,
    .bConfigurationValue = 1,
    .iConfiguration      = IDX_NULL,
    .bmAttributes        = USB_CONFIG_ATT_ONE,
    .bMaxPower           = 250,  // x 2mA
  },
  .interface = {
    .bLength             = USB_DT_INTERFACE_SIZE,
    .bDescriptorType     = USB_DT_INTERFACE,
    .bInterfaceNumber    = 0,
    .bAlternateSetting   = 0,
    .bNumEndpoints       = 2,
    .bInterfaceClass     = USB_CLASS_HID,
    .bInterfaceSubClass  = 0x00,
    .bInterfaceProtocol  = 0x00,
    .iInterface          = IDX_NULL,
  },
  .hid = {
    .bLength             = sizeof(struct usb_hid_descriptor),
    .bDescriptorType     = USB_DT_HID,
    .bcdHID              = cpu_to_le16(0x0111),
    .bCountryCode        = 0,
    .bNumReports         = 1,
    .bReportType         = USB_DT_HID_REPORT,
    .wReportLength       = cpu_to_le16(sizeof(opg_hid_report)),
  },
  .ep_out = {
    .bLength             = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType     = USB_DT_ENDPOINT,
    .bEndpointAddress    = USB_DIR_OUT | 3,  // will be overriden
    .bmAttributes        =
      USB_ENDPOINT_XFER_INT | USB_ENDPOINT_SYNC_NONE | USB_ENDPOINT_USAGE_DATA,
    .wMaxPacketSize      = 64,
    .bInterval           = 5,
  },
  .ep_in = {
    .bLength             = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType     = USB_DT_ENDPOINT,
    .bEndpointAddress    = USB_DIR_IN | 4,  // will be overriden
    .bmAttributes        =
      USB_ENDPOINT_XFER_INT | USB_ENDPOINT_SYNC_NONE | USB_ENDPOINT_USAGE_DATA,
    .wMaxPacketSize      = 64,
    .bInterval           = 5,
  },
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

    // byte3
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

static const char* opg_get_string(int idx) {
  switch (idx) {
    case IDX_MANUFACTURER:
      return "HORI CO.,LTD.";
    case IDX_PRODUCT:
      return "HORIPAD S";
    default:
      break;
  }
  return NULL;
}
