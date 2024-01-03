# Almost all CMake files should start with this
# You should always specify a range with the newest
# and oldest tested versions of CMake. This will ensure
# you pick up the best policies.
cmake_minimum_required(VERSION 3.1...3.27)

project(
  ROGueENEMY
  VERSION 1.0
  LANGUAGES C)

set(ROGUE_EXECUTABLE_NAME "rogue-enemy")
set(STRAY_EXECUTABLE_NAME "stray-ally")
set(ALLINONE_EXECUTABLE_NAME "allynone")

find_package(PkgConfig REQUIRED) # Include functions provided by PkgConfig module.

pkg_check_modules(DBUS REQUIRED dbus-1) # This calls pkgconfig with appropriate arguments
# Use results of pkg_check_modules() call.
include_directories(${DBUS_INCLUDE_DIRS})
link_directories(${DBUS_LIBRARY_DIRS})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(${ROGUE_EXECUTABLE_NAME}
                  dev_timer.c
                  dev_evdev.c
                  dev_iio.c
                  dev_hidraw.c
                  dev_in.c
                  main.c
                  settings.c
                  rog_ally.c
                  legion_go.c
                  xbox360.c
                  rogue_enemy.c
)

add_executable(${STRAY_EXECUTABLE_NAME}
                  dev_out.c
                  stray_ally.c
                  settings.c
                  virt_ds4.c
                  virt_ds5.c
                  virt_mouse.c
                  virt_kbd.c
                  devices_status.c
                  rogue_enemy.c
)

add_executable(${ALLINONE_EXECUTABLE_NAME}
                  dev_out.c
                  allynone.c
                  settings.c
                  virt_ds4.c
                  virt_ds5.c
                  virt_mouse.c
                  virt_kbd.c
                  devices_status.c
                  dev_timer.c
                  dev_evdev.c
                  dev_iio.c
                  dev_hidraw.c
                  dev_in.c
                  settings.c
                  rog_ally.c
                  legion_go.c
                  xbox360.c
                  rogue_enemy.c
)

set_property(TARGET ${ALLINONE_EXECUTABLE_NAME} PROPERTY C_STANDARD 17)

target_link_libraries(${ALLINONE_EXECUTABLE_NAME} PRIVATE Threads::Threads ${DBUS_LIBRARIES} -levdev -ludev -lconfig -lm -lz)

set_target_properties(${ALLINONE_EXECUTABLE_NAME} PROPERTIES LINKER_LANGUAGE C)

install(TARGETS ${ALLINONE_EXECUTABLE_NAME} DESTINATION bin)

set_property(TARGET ${ROGUE_EXECUTABLE_NAME} PROPERTY C_STANDARD 17)

target_link_libraries(${ROGUE_EXECUTABLE_NAME} PRIVATE Threads::Threads ${DBUS_LIBRARIES} -levdev -ludev -lconfig -lm)

set_target_properties(${ROGUE_EXECUTABLE_NAME} PROPERTIES LINKER_LANGUAGE C)

install(TARGETS ${ROGUE_EXECUTABLE_NAME} DESTINATION bin)

set_property(TARGET ${STRAY_EXECUTABLE_NAME} PROPERTY C_STANDARD 17)

target_link_libraries(${STRAY_EXECUTABLE_NAME} PRIVATE Threads::Threads -levdev -ludev -lconfig -lm -lz)

set_target_properties(${STRAY_EXECUTABLE_NAME} PROPERTIES LINKER_LANGUAGE C)

install(TARGETS ${STRAY_EXECUTABLE_NAME} DESTINATION bin)