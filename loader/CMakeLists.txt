cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

set(SHORT_NAME baremetal)
project(${SHORT_NAME})
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Baremetal")
set(VITA_TITLEID  "BAREMETAL")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")

link_directories(
  ${CMAKE_BINARY_DIR}/kernel_stubs
  ${CMAKE_BINARY_DIR}/user_stubs
)

add_executable(${SHORT_NAME}
  main.c
)

target_link_libraries(${SHORT_NAME}
  taihen_stub
  SceDisplay_stub
  SceCtrl_stub
  SceGxm_stub
  SceAppUtil_stub
  SceCommonDialog_stub
)

add_dependencies(${SHORT_NAME} user_stubs)

vita_create_self(${SHORT_NAME}.self ${SHORT_NAME} UNSAFE)

vita_create_vpk(${SHORT_NAME}.vpk ${VITA_TITLEID} ${SHORT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
)
