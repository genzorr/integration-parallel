cmake_minimum_required(VERSION 3.20)
project(lab2integration)

set(CMAKE_CXX_STANDARD 17)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
#target_link_libraries(my_app PRIVATE Threads::Threads)
# Linear program
#add_executable(lab2integration src/integration.cpp)

# Parallel program
add_executable(lab2integration src/integration_parallel.cpp)
target_link_libraries(lab2integration Threads::Threads)
