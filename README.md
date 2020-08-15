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
You can check the table of wheels supported in `hid-tminit.h`, they are stored in the array `th_wheels_infos[]`
