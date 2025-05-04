cmake_minimum_required(VERSION 3.1)
project(ThermalCamera VERSION 1.0.0 LANGUAGES CXX)

# Find dependencies
find_package(OpenCV REQUIRED)

# Build static and shared libraries
add_library(ThermalCamera_static STATIC src/ThermalCamera.cpp)
add_library(ThermalCamera_shared SHARED src/ThermalCamera.cpp)

# Public include directory
target_include_directories(ThermalCamera_static PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)
target_include_directories(ThermalCamera_shared PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
)

# Link against the SDK and other libs
foreach(lib IN ITEMS ThermalCamera_static ThermalCamera_shared)
  target_link_libraries(${lib} PUBLIC
    i3system_TE
    usb-1.0
    config++
    udev
    ${OpenCV_LIBS}
  )
endforeach()

# Installation rules
install(TARGETS ThermalCamera_static ThermalCamera_shared
  EXPORT ThermalCameraTargets
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)

# Export CMake package
install(EXPORT ThermalCameraTargets
  NAMESPACE ThermalCamera::
  DESTINATION lib/cmake/ThermalCamera
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/ThermalCameraConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)
configure_package_config_file(
  cmake/ThermalCameraConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/ThermalCameraConfig.cmake"
  INSTALL_DESTINATION lib/cmake/ThermalCamera
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/ThermalCameraConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/ThermalCameraConfigVersion.cmake"
  DESTINATION lib/cmake/ThermalCamera
)