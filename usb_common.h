
enum {
  IDX_NULL,
  IDX_MANUFACTURER,
  IDX_PRODUCT,
  IDX_SERIAL_NO,

  IDX_USER,
};

#define USB_DT_HID 0x21
#define USB_DT_HID_REPORT 0x22

struct usb_short_endpoint_descriptor {
  __u8   bLength;
  __u8   bDescriptorType;
  __u8   bEndpointAddress;
  __u8   bmAttributes;
  __le16 wMaxPacketSize;
  __u8   bInterval;
} __attribute__ ((packed));

struct usb_hid_descriptor {
  __u8   bLength;
  __u8   bDescriptorType;
  __le16 bcdHID;
  __u8   bCountryCode;
  __u8   bNumReports;
  __u8   bReportType;
  __le16 wReportLength;
} __attribute__ ((packed));

struct driver_data {
  u8 last_request_type;
  u8 last_request;
  struct usb_gadget* gadget;
  struct usb_request* ep0_request;
  struct usb_ep* ep_in;
  struct usb_request* ep_in_request;
  struct usb_ep* ep_out;
  struct usb_request* ep_out_request;
};
