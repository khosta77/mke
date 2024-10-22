cmake_minimum_required(VERSION 3.0)
project(MKE)

find_package(Eigen3 REQUIRED)

# Выставить версию с++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Добавление исполняемого файла с именем main.out
add_executable(
        main.out
        ./main.cpp
    )

target_link_libraries(main.out Eigen3::Eigen)
