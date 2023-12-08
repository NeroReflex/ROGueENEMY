# Almost all CMake files should start with this
# You should always specify a range with the newest
# and oldest tested versions of CMake. This will ensure
# you pick up the best policies.
cmake_minimum_required(VERSION 3.1...3.27)

project(
  ROGueENEMY
  VERSION 1.0
  LANGUAGES C)

set(EXECUTABLE_NAME "rogue-enemy")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Adding something we can run - Output name matches target name
add_executable(${EXECUTABLE_NAME}
                  dev_evdev.c
                  dev_iio.c
                  dev_in.c
                  dev_out.c
                  main.c
                  platform.c
                  settings.c
                  virt_ds4.c
                  virt_ds5.c
                  virt_mouse_kbd.c
                  virt_evdev.c
                  devices_status.c
                  rog_ally.c
                  xbox360.c
)

set_property(TARGET ${EXECUTABLE_NAME} PROPERTY C_STANDARD 17)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE Threads::Threads -levdev -ludev -lconfig -lm)

set_target_properties(${EXECUTABLE_NAME} PROPERTIES LINKER_LANGUAGE C)

install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin)