# hid-tminit
Linux driver to properly initialize Thrustmaster Wheels, based on [this](https://github.com/Kimplul/hid-tmff2) driver written by Kimplul

When connected to the machine, the Thrustmaster wheels appear as
a «generic» hid gamepad called `Thrustmaster FFB Wheel`.

When in this mode not every functionality of the wheel, like the force feedback,
are available. To enable all functionalities of a Thrustmaster wheel we have to send
to it a specific USB CONTROL request with a code different for each wheel.

This driver tries to understand which model of Thrustmaster wheel the generic
`Thrustmaster FFB Wheel` really is and then sends the appropriate control code.

## Wheels supported
You can check the table of wheels supported in `hid-tminit.c`, they are stored in the array `th_wheels_infos[]`

## Recapitulatory table
This table may be outdated.

My new guess is the control packet with `usb.setup.bRequest == 73`, is suspiciously near the control packet used to switch the wheel mode and the response of the wheel seems to remain the same after the upgrade

|Model| Response| bytes +6, +7|
|--------------------------------|---------------------------------------------------------|---|
|T150 (same both my and @Kimplul)| 0x49002100000006030000000000000000| 0x0603|
|T300 (@Kimplul)                 | 0x49000301010006021300000005010000| 0x0602|
|T300 (cap from Gitlab)          | 0x49002100000006020000000000000000| 0x0602|
|T300 Ferrari Alcantara Edition  | 0x49000200010004021300000005010000| 0x0402|
|TMX                             | 0x4700410000000704                | 0x0704 |
|T500                            | 0x4700030000000200                | 0x0200 |

[Original discussion](https://github.com/Kimplul/hid-tmff2/issues/3)
