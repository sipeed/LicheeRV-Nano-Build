file(GLOB _SOURCES "common/src/*.c")
file(GLOB _HEADERS "common/include/*.h")

include_directories(include)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/arch)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/common)
include_directories(${CMAKE_INSTALL_INC_PREFIX}/kernel)
include_directories(../common/inc)
include_directories(../rtos_cmdqu/include)
include_directories(./common/include)
include_directories(./chip/${CHIP}/inc)
include_directories(../../task/comm/include)

add_library(rgn OBJECT ${_SOURCES})

install(FILES ${_HEADERS} DESTINATION include/driver/rgn)
