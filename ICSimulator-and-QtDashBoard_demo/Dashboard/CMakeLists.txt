cmake_minimum_required(VERSION 3.14)

project(dashboard LANGUAGES CXX)

# Enable Qt's automatic MOC, UIC, and RCC processing
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt5 packages
find_package(QT NAMES Qt5 REQUIRED COMPONENTS Core Gui Qml Quick)
find_package(Qt5 REQUIRED COMPONENTS Core Gui Qml Quick)

# Find nlohmann_json package (required for JSON parsing)
find_package(nlohmann_json 3.9.1 REQUIRED)

# Add Qt resource file (resources.qrc)
qt5_add_resources(QRCS resources.qrc)

# Define the executable and its sources
add_executable(dashboard
    src/main.cpp
    src/CanReceiver.cpp
    src/CanReceiver.h
    src/LinReceiver.cpp
    src/LinReceiver.h
    src/UdpReceiver.cpp
    src/UdpReceiver.h
    src/FlexrayReceiver.cpp
    src/FlexrayReceiver.h
    src/TcpSignalReceiver.h
    src/TcpSignalReceiver.cpp
    ${QRCS}
)

# Link libraries
target_link_libraries(dashboard
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Quick
    nlohmann_json::nlohmann_json
)

# Include directories (for custom headers)
target_include_directories(dashboard PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Installation rules
include(GNUInstallDirs)
install(TARGETS dashboard
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)