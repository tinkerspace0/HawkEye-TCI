cmake_minimum_required(VERSION 3.14)
project(thermal_app VERSION 1.0 LANGUAGES CXX)

# 1) Find your normal dependencies
find_package(OpenCV REQUIRED)

# If you need libusb/libconfig++ from the system:
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
pkg_check_modules(CONFIGPP REQUIRED libconfig++)

# 2) Point at your vendored SDK
set(I3_ROOT       "${CMAKE_SOURCE_DIR}/i3system")
set(I3_INCLUDE    "${I3_ROOT}/include")
set(I3_LIB_DIR    "${I3_ROOT}/lib")

# 3) Declare IMPORTED targets for each .so
#    so consumers can just link to i3system::te, ::usb, ::imgproc
add_library(i3system::te       SHARED IMPORTED GLOBAL)
add_library(i3system::usb      SHARED IMPORTED GLOBAL)
add_library(i3system::imgproc  SHARED IMPORTED GLOBAL)

set_target_properties(i3system::te       PROPERTIES
  IMPORTED_LOCATION         "${I3_LIB_DIR}/libi3system_te_64.so"
  INTERFACE_INCLUDE_DIRECTORIES "${I3_INCLUDE}"
)
set_target_properties(i3system::usb      PROPERTIES
  IMPORTED_LOCATION         "${I3_LIB_DIR}/libi3system_usb_64.so"
  INTERFACE_INCLUDE_DIRECTORIES "${I3_INCLUDE}"
)
set_target_properties(i3system::imgproc  PROPERTIES
  IMPORTED_LOCATION         "${I3_LIB_DIR}/libi3system_imgproc_impl_64.so"
  INTERFACE_INCLUDE_DIRECTORIES "${I3_INCLUDE}"
)

# 4) Build your ThermalCamera wrapper (as shared or static)
add_library(ThermalCamera SHARED
  src/ThermalCamera.cpp
)
target_include_directories(ThermalCamera
  PUBLIC include
)
target_link_libraries(ThermalCamera
  PUBLIC
    i3system::te
    i3system::usb
    i3system::imgproc
    ${OpenCV_LIBS}
    ${LIBUSB_LIBRARIES}
    ${CONFIGPP_LIBRARIES}
    udev
)
target_include_directories(ThermalCamera
  PUBLIC
    ${LIBUSB_INCLUDE_DIRS}
    ${CONFIGPP_INCLUDE_DIRS}
)

# 5) Ensure at runtime the loader finds the vendored .so
#    $ORIGIN expands to the directory of the executable or .so
set_target_properties(ThermalCamera PROPERTIES
  BUILD_RPATH "\$ORIGIN/../i3system/lib"
)

# 6) (Optional) build a small test executable
add_executable(thermal_test src/main.cpp)
target_link_libraries(thermal_test PRIVATE ThermalCamera)

# 7) Install & export so downstream can do find_package(ThermalCamera)
install(TARGETS ThermalCamera thermal_test
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)

install(EXPORT ThermalCameraTargets
  FILE ThermalCameraTargets.cmake
  NAMESPACE ThermalCamera::
  DESTINATION lib/cmake/ThermalCamera
)
include(CMakePackageConfigHelpers)
configure_package_config_file(
  cmake/ThermalCameraConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/ThermalCameraConfig.cmake
  INSTALL_DESTINATION lib/cmake/ThermalCamera
)
write_basic_package_version_file(
  cmake/ThermalCameraConfigVersion.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/ThermalCameraConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/ThermalCameraConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/ThermalCameraConfigVersion.cmake
  DESTINATION lib/cmake/ThermalCamera
)
