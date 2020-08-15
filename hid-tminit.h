/**
 * These interrupts are used to prevent a nasty crash when initializing the
 * T300RS. Used in tminit_interrupts().
 */
u8 setup_0[] = { 0x42, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
u8 setup_1[] = { 0x0a, 0x04, 0x90, 0x03, 0x00, 0x00, 0x00, 0x00 };
u8 setup_2[] = { 0x0a, 0x04, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00 };
u8 setup_3[] = { 0x0a, 0x04, 0x12, 0x10, 0x00, 0x00, 0x00, 0x00 };
u8 setup_4[] = { 0x0a, 0x04, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00 };
u8 *setup_arr[] = { setup_0, setup_1, setup_2, setup_3, setup_4 };
unsigned int setup_arr_sizes[] = { 
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
 * endianess, the USB protocols always use
 * little endian; the macro cpu_to_le[BIT]()
 * must be used when preparing USB packets 
 * and vice-versa
 */
struct th_wheel_info
{
	uint16_t wheel_type;

	/** See when the USB control out packet is prepared... 
	 * @TODO The TMX seems to require to control codes to switch.
	 * Probabilly this field needs to be converted to an array,
	 */
	uint16_t switch_value;

	char const *const wheel_name;
};

/**
 * All wheel I know. TO BE TESTED
 * I can't find of a clever way to store them, but I do
 * not think a O(n) cycle for each wheel attached to the
 * machine is too bad...
 */
static const struct th_wheel_info th_wheels_infos[] =
{
	{0x0306, 0x0006, "Thrustmaster T150RS"},
	{0x0206, 0x0005, "Thrustmaster T300RS"},
	{0x0002, 0x0002, "Thrustmaster T500RS"},
	{0x0407, 0x0001, "Thrustmaster TMX"}
};

const uint8_t th_wheels_infos_length = 4;

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

	union
	{
		struct __packed
		{
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
		struct __packed
		{
			uint16_t field0;
			uint16_t field1;
			uint16_t model;
		} b;
	} data;
};

struct tm_wheel
{
	struct usb_device *usb_dev;
	struct urb *urb;

	struct usb_ctrlrequest *model_request;
	struct tm_wheel_response *response;

	struct usb_ctrlrequest *change_request;
};

/**
 * The control packet to send to wheel */
struct usb_ctrlrequest model_request =
{
	.bRequestType = 0xc1,
	.bRequest = 73,
	.wValue = 0,
	.wIndex = 0,
	.wLength = cpu_to_le16(0x0010)
};

struct usb_ctrlrequest change_request = 
{
	.bRequestType = 0x41,
	.bRequest = 83,
	.wValue = 0, // Will be filled by the driver
	.wIndex = 0,
	.wLength = 0
};
