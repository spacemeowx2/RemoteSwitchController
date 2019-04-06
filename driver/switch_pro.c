// #include "debug.h"
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/usb/composite.h>
#include "switch_pro.h"
#include "f_pro.c"

MODULE_LICENSE("Dual BSD/GPL");

static struct usb_function *func_sp;
static struct usb_function_instance *func_inst_sp;

#define STRING_DESCRIPTION_IDX		USB_GADGET_FIRST_AVAIL_IDX
static struct usb_string strings_dev[] = {
	[USB_GADGET_MANUFACTURER_IDX].s = "Nintendo Co., Ltd.",
	[USB_GADGET_PRODUCT_IDX].s = "Pro Controller",
	[USB_GADGET_SERIAL_IDX].s = "000000000001",
	[STRING_DESCRIPTION_IDX].s = NULL,
	{  } /* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static struct usb_device_descriptor device_desc = {
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
  /* .iManufacturer = DYNAMIC */
  /* .iProduct = DYNAMIC */
  /* .iSerialNumber = IDX_SERIAL_NO */
  .bNumConfigurations = 1,
};

static struct usb_configuration pro_config_driver = {
    .label                  = pro_driver_name,
    .bConfigurationValue    = 1,
    /* .iConfiguration = DYNAMIC */
	.bmAttributes	        = USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_WAKEUP,
    .MaxPower               = 500,
};

static int gsp_bind(struct usb_composite_dev *cdev) {
    int status;

    status = usb_string_ids_tab(cdev, strings_dev);
	if (status < 0)
		goto fail;
	device_desc.iManufacturer = strings_dev[USB_GADGET_MANUFACTURER_IDX].id;
	device_desc.iProduct = strings_dev[USB_GADGET_PRODUCT_IDX].id;
	device_desc.iSerialNumber = strings_dev[USB_GADGET_SERIAL_IDX].id;
	cdev->desc.bcdDevice = device_desc.bcdDevice;

    status = strings_dev[STRING_DESCRIPTION_IDX].id;
	pro_config_driver.iConfiguration = status;

    func_inst_sp = switch_pro_alloc_inst();
	if (IS_ERR(func_inst_sp))
		return PTR_ERR(func_inst_sp);

    func_sp = switch_pro_alloc_func(func_inst_sp);
	if (IS_ERR(func_sp)) {
		status = PTR_ERR(func_sp);
		goto err_put_func_inst_sp;
	}

	usb_add_config_only(cdev, &pro_config_driver);

	status = usb_add_function(&pro_config_driver, func_sp);
	if (status)
		goto err_put_func_ss;

    usb_ep_autoconfig_reset(cdev->gadget);

    return 0;

err_put_func_ss:
	func_sp->free_func(func_sp);
	func_sp = NULL;
err_put_func_inst_sp:
	func_inst_sp->free_func_inst(func_inst_sp);
	func_inst_sp = NULL;

fail:
	return status;
}


static int gsp_unbind(struct usb_composite_dev *cdev) {
	if (!IS_ERR_OR_NULL(func_sp)) {
		func_sp->free_func(func_sp);
		func_sp = NULL;
	}
	func_inst_sp->free_func_inst(func_inst_sp);
    return 0;
}

static struct usb_composite_driver gswitch_pro_driver = {
	.name		= "g_switch_pro",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.max_speed	= USB_SPEED_HIGH,
	.bind		= gsp_bind,
	.unbind		= gsp_unbind,
};

static int __init init(void)
{
	return usb_composite_probe(&gswitch_pro_driver);
}
module_init(init);

static void __exit cleanup(void)
{
	usb_composite_unregister(&gswitch_pro_driver);
}
module_exit(cleanup);
