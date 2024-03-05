file(GLOB _SOURCES "src/*.c")
file(GLOB _HEADERS "include/*.h")

# file(GLOB _ISP_HEADERS "include/isp/*.h")
# list(APPEND _HEADERS ${_ISP_HEADERS})

if(CONFIG_SENSOR_SONY_IMX327 STREQUAL "y")
add_compile_definitions(SENSOR_SONY_IMX327)
file(GLOB _SENSOR_IMX327 "src/sony_imx327/*.c")
list(APPEND _SOURCES ${_SENSOR_IMX327})
endif()

if(CONFIG_SENSOR_GCORE_GC4653 STREQUAL "y")
add_compile_definitions(SENSOR_GCORE_GC4653)
file(GLOB _SENSOR_GC4653 "src/gcore_gc4653/*.c")
list(APPEND _SOURCES ${_SENSOR_GC4653})
endif()

include_directories(include)

include_directories(${DRIVER_GPIO_DIR}/include)
# include_directories(${DRIVER_ISP_DIR}/include)
include_directories(${DRIVER_CIF_DIR}/include)

include_directories(${CMAKE_INSTALL_INC_PREFIX}/arch)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/common)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/kernel)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/hal/snsr_i2c)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/hal/config)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/hal/cif)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/driver/vi)

#include_directories(include/isp)

add_library(sensor OBJECT ${_SOURCES})

install(FILES ${_HEADERS} DESTINATION include/driver/sensor)
