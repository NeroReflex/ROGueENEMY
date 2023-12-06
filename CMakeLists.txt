cmake_minimum_required(VERSION 3.1...3.27)

project(
  ROGueENEMY
  VERSION 1.0
  LANGUAGES C
)

set(EXECUTABLE_NAME "rogue-enemy")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Adding executable - Output name matches target name
add_executable(${EXECUTABLE_NAME} dev_iio.c input_dev.c logic.c main.c output_dev.c platform.c queue.c settings.c virt_ds4.c virt_ds5.c)

# Linking static libraries
# Assuming the static libraries are installed and findable by CMake
# Specify the full path to the static libraries if necessary
find_library(LIBEVDEV_STATIC NAMES libevdev.a evdev)
find_library(LIBCONFIG_STATIC NAMES libconfig.a config)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE Threads::Threads ${LIBEVDEV_STATIC} ${LIBCONFIG_STATIC})

set_target_properties(${EXECUTABLE_NAME} PROPERTIES LINKER_LANGUAGE C)

install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin)
