// SPDX-License-Identifier: GPL-2.0
/**
 * When connected to the machine, the Thrustmaster wheels appear as
 * a «generic» hid gamepad called "Thrustmaster FFB Wheel".
 * 
 * When in this mode not every functionality of the wheel, like the force feedback,
 * are available. To enable all functionalities of a Thrustmaster wheel we have to send
 * to it a specific USB CONTROL request with a code different for each wheel.
 *
 * This driver tries to understand which model of Thrustmaster wheel the generic
 * "Thrustmaster FFB Wheel" really is and then sends the appropriate control code.
 */
#include <linux/hid.h>
#include <linux/usb.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/module.h>
#include "hid-tminit.h"

/**
 * On some setups initializing the T300RS crashes the kernel,
 * these interrupts fix that particular issue. So far they haven't caused any
 * adverse effects in other wheels.
 */
static void tminit_interrupts(struct hid_device *hdev){
	int ret, trans, i, b_ep;
	u8 *send_buf = kmalloc(256, GFP_KERNEL);
	struct usb_host_endpoint *ep;
	struct device *dev = &hdev->dev;
	struct usb_interface *usbif = to_usb_interface(dev->parent);
	struct usb_device *usbdev = interface_to_usbdev(usbif);

	if(!send_buf){
		hid_err(hdev, "failed allocating send buffer\n");
		return;
	}

	ep = &usbif->cur_altsetting->endpoint[1];
	b_ep = ep->desc.bEndpointAddress;

	for(i = 0; i < ARRAY_SIZE(setup_arr); ++i){
		memcpy(send_buf, setup_arr[i], setup_arr_sizes[i]);

		ret = usb_interrupt_msg(usbdev,
			usb_sndintpipe(usbdev, b_ep),
			send_buf,
			setup_arr_sizes[i],
			&trans,
			USB_CTRL_SET_TIMEOUT);

		if(ret){
			hid_err(hdev, "setup data couldn't be sent\n");
			return;
		}
	}

	kfree(send_buf);
}

static void tminit_change_handler(struct urb *urb)
{
	struct hid_device *hdev = urb->context;

	// The wheel seems to kill himself before answering the host and therefore is violating the USB protocol...
	if(urb->status == 0 || urb->status == -EPROTO || urb->status == -EPIPE)
		hid_info(hdev, "Success?! The wheel should have been initialized!\n");
	else
		hid_warn(hdev, "URB to change wheel mode seems to have failed with error %d\n", urb->status);
}



/**
 * Called by the USB subsystem when the wheel respons to our request
 * to get [what it seems to be] the wheel's model.
 * 
 * If the model id is recognized then we send an opportune USB CONTROL REQUEST
 * to switch the wheel to its full capabilities
 */
static void tminit_model_handler(struct urb *urb)
{
	struct hid_device *hdev = urb->context;
	struct tm_wheel *tm_wheel = hid_get_drvdata(hdev);
	uint16_t model = 0;
	int i, ret;
	const struct th_wheel_info *twi = 0;

	if(urb->status)
	{
		hid_err(hdev, "URB to get model id failed with error %d\n", urb->status);
		return;
	}

	if(tm_wheel->response->type == cpu_to_le16(0x49))
		model = le16_to_cpu(tm_wheel->response->data.a.model);
	else if(tm_wheel->response->type == cpu_to_le16(0x47))
		model = le16_to_cpu(tm_wheel->response->data.b.model);
	else
	{
		hid_err(hdev, "Unknow packet type 0x%x, unable to proceed further with wheel init\n", tm_wheel->response->type);
		return;
	}

	for(i = 0; i < th_wheels_infos_length && !twi; i++)
		if(th_wheels_infos[i].wheel_type == model)
			twi = th_wheels_infos + i;
	
	if(twi)
		hid_info(hdev, "Wheel with model id 0x%x is a %s\n", model, twi->wheel_name);
	else
	{
		hid_err(hdev, "Unknown wheel's model id 0x%x, unable to proceed further with wheel init\n",model);
		return;
	}
	
	tm_wheel->change_request->wValue = cpu_to_le16(twi->switch_value);
	usb_fill_control_urb(
		tm_wheel->urb,
		tm_wheel->usb_dev,
		usb_sndctrlpipe(tm_wheel->usb_dev, 0),
		(char*)tm_wheel->change_request,
		0, 0, // We do not expect any response from the wheel
		tminit_change_handler,
		hdev
	);

	ret = usb_submit_urb(tm_wheel->urb, GFP_ATOMIC);
	if(ret)
		hid_err(hdev, "Error %d while submitting the change URB. I am unable to initialize this wheel...\n", ret);
}

static void tminit_remove(struct hid_device *hdev)
{
	struct tm_wheel *tm_wheel = hid_get_drvdata(hdev);

	usb_kill_urb(tm_wheel->urb);

	kfree(tm_wheel->response);
	kfree(tm_wheel->model_request);
	usb_free_urb(tm_wheel->urb);
	kfree(tm_wheel);

	hid_hw_stop(hdev);
}

/**
 * Function called by HID when a hid Thrustmaster FFB wheel is connected to the host.
 * This function starts the hid dev, tries to allocate the tm_wheel data structure and
 * finally send an USB CONTROL REQUEST to the wheel to get [what it seems to be] its
 * model type. 
 */
static int tminit_probe(struct hid_device *hdev, const struct hid_device_id *id){
	int ret = 0;
	struct tm_wheel *tm_wheel = 0;

	ret = hid_parse(hdev);
	if(ret)
	{
		hid_err(hdev, "parse failed with error %d\n", ret);
		goto error0;
	}

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT & ~HID_CONNECT_FF);
	if(ret)
	{
		hid_err(hdev, "hw start failed with error %d\n", ret);
		goto error0;
	}

	// Now we allocate the tm_wheel
	tm_wheel = kzalloc(sizeof(struct tm_wheel), GFP_KERNEL);
	if(!tm_wheel)
	{
		ret = -ENOMEM;
		goto error1;
	}

	tm_wheel->urb = usb_alloc_urb(0, GFP_ATOMIC);
	if(!tm_wheel->urb)
	{
		ret = -ENOMEM;
		goto error2;
	}

	tm_wheel->model_request = kzalloc(sizeof(struct usb_ctrlrequest), GFP_KERNEL);
	if(! tm_wheel->model_request)
	{
		ret = -ENOMEM;
		goto error3;
	}
	memcpy(tm_wheel->model_request, &model_request, sizeof(struct usb_ctrlrequest));

	tm_wheel->response = kzalloc(sizeof(struct tm_wheel_response), GFP_KERNEL);
	if(! tm_wheel->response)
	{
		ret = -ENOMEM;
		goto error4;
	}

	tm_wheel->change_request = kzalloc(sizeof(struct usb_ctrlrequest), GFP_KERNEL);
	if(! tm_wheel->model_request)
	{
		ret = -ENOMEM;
		goto error5;
	}
	memcpy(tm_wheel->change_request, &change_request, sizeof(struct usb_ctrlrequest));

	tm_wheel->usb_dev = interface_to_usbdev(to_usb_interface(hdev->dev.parent));
	hid_set_drvdata(hdev, tm_wheel);

	tminit_interrupts(hdev);

	usb_fill_control_urb(
		tm_wheel->urb,
		tm_wheel->usb_dev,
		usb_rcvctrlpipe(tm_wheel->usb_dev, 0),
		(char*)tm_wheel->model_request,
		tm_wheel->response,
		sizeof(struct tm_wheel_response),
		tminit_model_handler,
		hdev
	);

	ret = usb_submit_urb(tm_wheel->urb, GFP_ATOMIC);
	if(ret)
		hid_err(hdev, "Error %d while submitting the URB. I am unable to initialize this wheel...\n", ret);

	return ret;

error5: kfree(tm_wheel->response);
error4:	kfree(tm_wheel->model_request);
error3:	usb_free_urb(tm_wheel->urb);
error2:	kfree(tm_wheel);
error1:	hid_hw_stop(hdev);
error0:
	return ret; 
}

static const struct hid_device_id tminit_devices[] = {
	{ HID_USB_DEVICE(0x044f, 0xb65d)},
	{}
};

MODULE_DEVICE_TABLE(hid, tminit_devices);

static struct hid_driver tminit_driver = {
	.name = "hid-tminit",
	.id_table = tminit_devices,
	.probe = tminit_probe,
	.remove = tminit_remove,
};

static int __init tminit_init(void)
{
	int errno;

	errno = hid_register_driver(&tminit_driver);

	if(errno)
		printk(KERN_ERR "hid-tminit: error %d while registering the hid driver\n", errno);

	return errno;
}

static void __exit tminit_exit(void)
{
	hid_unregister_driver(&tminit_driver);
}

module_init(tminit_init);
module_exit(tminit_exit);

MODULE_AUTHOR("Dario Pagani <>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Driver to initialize Thrustmaster\'s wheels");

