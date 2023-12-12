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

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(${ROGUE_EXECUTABLE_NAME}
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
                  devices_status.c
)

set_property(TARGET ${ROGUE_EXECUTABLE_NAME} PROPERTY C_STANDARD 17)

target_link_libraries(${ROGUE_EXECUTABLE_NAME} PRIVATE Threads::Threads -levdev -ludev -lconfig -lm)

set_target_properties(${ROGUE_EXECUTABLE_NAME} PROPERTIES LINKER_LANGUAGE C)

install(TARGETS ${ROGUE_EXECUTABLE_NAME} DESTINATION bin)

set_property(TARGET ${STRAY_EXECUTABLE_NAME} PROPERTY C_STANDARD 17)

target_link_libraries(${STRAY_EXECUTABLE_NAME} PRIVATE Threads::Threads -levdev -ludev -lconfig -lm)

set_target_properties(${STRAY_EXECUTABLE_NAME} PROPERTIES LINKER_LANGUAGE C)

install(TARGETS ${STRAY_EXECUTABLE_NAME} DESTINATION bin)