/* SPDX-License-Identifier: GPL-2.0*/
/**
 * These interrupts are used to prevent a nasty crash when initializing the
 * T300RS. Used in tminit_interrupts().
 */
static const u8 setup_0[] = { 0x42, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static const u8 setup_1[] = { 0x0a, 0x04, 0x90, 0x03, 0x00, 0x00, 0x00, 0x00 };
static const u8 setup_2[] = { 0x0a, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00 };
static const u8 setup_3[] = { 0x0a, 0x04, 0x12, 0x10, 0x00, 0x00, 0x00, 0x00 };
static const u8 setup_4[] = { 0x0a, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00 };
static const u8 *const setup_arr[] = { setup_0, setup_1, setup_2, setup_3, setup_4 };
static const unsigned int setup_arr_sizes[] = {
	ARRAY_SIZE(setup_0),
	ARRAY_SIZE(setup_1),
	ARRAY_SIZE(setup_2),
	ARRAY_SIZE(setup_3),
	ARRAY_SIZE(setup_4)
};
/**
 * This struct contains for each type of
 * Thrustmaster wheel
 *
 * Note: The values are stored in the CPU
 * endianness, the USB protocols always use
 * little endian; the macro cpu_to_le[BIT]()
 * must be used when preparing USB packets
 * and vice-versa
 */
struct tm_wheel_info {
	uint16_t wheel_type;

	/**
	 * See when the USB control out packet is prepared...
	 * @TODO The TMX seems to require multiple control codes to switch.
	 */
	uint16_t switch_value;

	char const *const wheel_name;
};

/**
 * Known wheels.
 * Note: TMX does not work as it requires 2 control packets
 */
static const struct tm_wheel_info tm_wheels_infos[] = {
	{0x0306, 0x0006, "Thrustmaster T150RS"},
	{0x0206, 0x0005, "Thrustmaster T300RS"},
	{0x0204, 0x0005, "Thrustmaster T300 Ferrari Alcantara Edition"},
	{0x0002, 0x0002, "Thrustmaster T500RS"},
	{0x0407, 0x0001, "Thrustmaster TMX"}
};

static const uint8_t tm_wheels_infos_length = 5;

/**
 * This structs contains (in little endian) the response data
 * of the wheel to the request 73
 *
 * A sufficient research to understand what each field does is not
 * beign conducted yet. The position and meaning of fields are a
 * just a very optimistic guess based on instinct....
 */
struct __packed tm_wheel_response
{
	/**
	 * Seems to be the type of packet
	 * - 0x0049 if is data.a (15 bytes)
	 * - 0x0047 if is data.b (7 bytes)
	 */
	uint16_t type;

	union {
		struct __packed {
			uint16_t field0;
			uint16_t field1;
			/**
			 * Seems to be the model code of the wheel
			 * Read table thrustmaster_wheels to values
			 */
			uint16_t model;

			uint16_t field2;
			uint16_t field3;
			uint16_t field4;
			uint16_t field5;
		} a;
		struct __packed {
			uint16_t field0;
			uint16_t field1;
			uint16_t model;
		} b;
	} data;
};

struct tm_wheel {
	struct usb_device *usb_dev;
	struct urb *urb;

	struct usb_ctrlrequest *model_request;
	struct tm_wheel_response *response;

	struct usb_ctrlrequest *change_request;
};

/** The control packet to send to wheel */
static const struct usb_ctrlrequest model_request = {
	.bRequestType = 0xc1,
	.bRequest = 73,
	.wValue = 0,
	.wIndex = 0,
	.wLength = cpu_to_le16(0x0010)
};

static const struct usb_ctrlrequest change_request = {
	.bRequestType = 0x41,
	.bRequest = 83,
	.wValue = 0, // Will be filled by the driver
	.wIndex = 0,
	.wLength = 0
};
