file(GLOB _HEADERS "include/*.h")

include_directories(include)

#include_directories(${CMAKE_INSTALL_INC_PREFIX}/arch)
#include_directories(${CMAKE_INSTALL_INC_PREFIX}/common)
#include_directories(${CMAKE_INSTALL_INC_PREFIX}/kernel)
#include_directories(${CMAKE_INSTALL_INC_PREFIX}/hal/uart)
#include_directories(${CMAKE_INSTALL_INC_PREFIX}/hal/config)
#include_directories(${CMAKE_INSTALL_INC_PREFIX}/hal/pinmux)

install(FILES ${_HEADERS} DESTINATION include/driver/fast_image)
