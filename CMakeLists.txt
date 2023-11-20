# Almost all CMake files should start with this
# You should always specify a range with the newest
# and oldest tested versions of CMake. This will ensure
# you pick up the best policies.
cmake_minimum_required(VERSION 3.1...3.27)

project(
  ROGueENEMY
  VERSION 1.0
  LANGUAGES C CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Adding something we can run - Output name matches target name
add_executable(rogue_enemy dev_iio.c input_dev.c logic.c main.c output_dev.c platform.c queue.c virt_ds4.c)

target_link_libraries(rogue_enemy PRIVATE Threads::Threads)

set_target_properties(rogue_enemy PROPERTIES LINKER_LANGUAGE C)
