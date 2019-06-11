cmake_minimum_required(VERSION 3.5)
project(carbon-examples C)

add_executable(examples-error examples-error.c ${LIB_SOURCES})

set_target_properties(examples-error PROPERTIES EXCLUDE_FROM_ALL TRUE)
