cmake_minimum_required(VERSION 3.16)
project(chromack LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core DBus Widgets)
find_package(LayerShellQt QUIET)

qt_add_executable(chromack
    src/main.cpp
    src/ChromackControl.cpp
    src/ChromackConfig.cpp
    src/ChromackPanel.cpp
)

target_link_libraries(chromack PRIVATE
    Qt6::Core
    Qt6::DBus
    Qt6::Widgets
)

if(LayerShellQt_FOUND)
    target_link_libraries(chromack PRIVATE LayerShellQt::Interface)
    target_compile_definitions(chromack PRIVATE CHROMACK_HAS_LAYERSHELLQT=1)
else()
    target_compile_definitions(chromack PRIVATE CHROMACK_HAS_LAYERSHELLQT=0)
endif()

install(TARGETS chromack RUNTIME DESTINATION bin)
