# ROGueENEMY
Collects ROG Ally input events to a single (active) virtual controller in linux to allow the use of the gyroscope and every button but also maximize controller compatibility with multiple games.

## Steam Configuration
In the emulated PS4 controller remember to remove the default deadzone for both left and right joystick: the ROG ally joystics appears to be way better than DS4 ones.

## Compilation
To compile from source you need CMake and make. After the usual git clone and cd inside the cloned directory to use CMake do:

```sh
mkdir build
cd build
cmake ..
cmake --build .
```

__WARNING__: CMake is currently __broken__. Use Make.