file(GLOB _SOURCES "src/*.c")
file(GLOB _HEADERS "include/*.h")

add_compile_definitions(MIPI_IF)
add_compile_definitions(SUBLVDS_IF)
add_compile_definitions(HISPI_IF)
add_compile_definitions(DVP_IF)
add_compile_definitions(BT1120_IF)
add_compile_definitions(BT656_IF)
add_compile_definitions(BT601_IF)
add_compile_definitions(BT_DEMUX_IF)
add_compile_definitions(CUSTOM0_IF)

include_directories(include)
include_directories(${DRIVER_GPIO_DIR}/include)
include_directories(${DRIVER_SENSOR_DIR}/include)

include_directories(${CMAKE_INSTALL_INC_PREFIX}/arch)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/common)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/kernel)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/hal/cif)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/hal/config)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/hal/pinmux)

add_library(cif OBJECT ${_SOURCES})

install(FILES ${_HEADERS} DESTINATION include/driver/cif)
