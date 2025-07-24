cmake_minimum_required(VERSION 3.14)

project(ICSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# Build ICSimulator
add_executable(ICSimulator
    src/ICSimulator.cpp
)

# Build UDPSimulator
add_executable(UDPSimulator
    src/UDPSimulator.cpp
)

# Build UDPSimulator
add_executable(Sender
    src/Sender.cpp
)

include(GNUInstallDirs)
install(TARGETS ICSimulator UDPSimulator Sender
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

target_link_libraries(ICSimulator pthread)

