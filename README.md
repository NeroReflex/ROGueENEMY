# ROGueENEMY
Collects ROG Ally input events to a single (active) virtual controller in linux to allow the use of the gyroscope and every button but also maximize controller compatibility with multiple games.

## Usage
On steam head for settings for the emulated PS4 controller and remove the default deadzone for both left and right joystick: the ROG ally joystics appears to be way better than DS4 ones.

On steam disable *Nintendo buttons layout* and rely on the proper configuration option on this software to accomplish what you seek.

Tweak ff_rumble to a value between 0 and 100 to configure the strenght for rumble output.

Remember to look /etc/ROGueENEMY/config.cfg for additional configurations.

__WARNING:__ If steam does not recognise a DualSense controller use the udev rule provided in this repo.

## Compilation
To compile from source you need CMake and make. After the usual git clone and cd inside the cloned directory to use CMake do:

```sh
mkdir build
cd build
cmake ..
cmake --build .
```

__Notes__: This project should be compiled with the following flags: *-O3 -march=znver4 -flto=full*

## Design
This software is meant to be run all the time in background and avoid busy wait, as well as quick reaction time from user input are both a design goal as well as ensuring reliable operation across many linux distributions in different conditions.

## Contributions
The following is a (probably incomplete) list of contributions this project had.

As this project was met in a great way by the community contributions were many:
  - __ashtopeth101__ for providing capture data of a real DS4 controller, for his precious testing and in general his time
  - __143mailliw__ for spotting and fixing a bug with the timestamp and the following time required to ensure proper operation
  __jlobue10__ for suggestions and allowing easy end-user usage of this software in fedora-based distros
  - everybody else testing and providing feedback

If I have forgotten someone please tell me and/or send a pull request.
