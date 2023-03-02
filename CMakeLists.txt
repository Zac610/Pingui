cmake_minimum_required(VERSION 3.13)

project(Pingui VERSION 0.1)

add_library(zac_build_flags INTERFACE)
target_compile_features(zac_build_flags INTERFACE cxx_std_11)

add_executable(${CMAKE_PROJECT_NAME} src/main.cpp)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC inc)
if (WIN32)
	target_compile_options(zac_build_flags INTERFACE -DPLATFORM_WIN32)
	target_link_directories(${CMAKE_PROJECT_NAME} PUBLIC lib)
endif()
if (UNIX)
	target_compile_options(zac_build_flags INTERFACE -DPLATFORM_LINUX -DHAVE_PTHREAD_H)
	target_link_libraries(zac_build_flags INTERFACE fltk pthread)
endif()

target_link_libraries(${CMAKE_PROJECT_NAME} zac_build_flags)
