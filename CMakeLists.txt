cmake_minimum_required(VERSION 3.14)
project(thermal_test LANGUAGES CXX)

# 1) find your deps
find_package(OpenCV REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB   REQUIRED libusb-1.0)
pkg_check_modules(CONFIGPP REQUIRED libconfig++)

# 2) point at your vendored SDK
set(I3_ROOT    "${CMAKE_SOURCE_DIR}/i3system")
set(I3_INCLUDE "${I3_ROOT}/include")
set(I3_LIBDIR  "${I3_ROOT}/lib")

# 3) include paths for your wrapper + SDK + OpenCV + libusb/config++
include_directories(
  ${CMAKE_SOURCE_DIR}/include         # your ThermalCamera.h
  ${I3_INCLUDE}                       # i3system headers
  ${OpenCV_INCLUDE_DIRS}
  ${LIBUSB_INCLUDE_DIRS}
  ${CONFIGPP_INCLUDE_DIRS}
)

# 4) tell the linker where to find the SDK .so
link_directories(${I3_LIBDIR})

# 5) build your test executable
add_executable(thermal_test
  src/ThermalCamera.cpp
  main.cpp
)

# 6) link against everything
target_link_libraries(thermal_test PRIVATE
  ${OpenCV_LIBS}
  ${LIBUSB_LIBRARIES}
  ${CONFIGPP_LIBRARIES}
  udev
  i3system_te_64
  i3system_usb_64
  i3system_imgproc_impl_64
)

# 7) embed an RPATH so it finds the .so at runtime
set_target_properties(thermal_test PROPERTIES
  BUILD_RPATH "$ORIGIN/i3system/lib"
)
