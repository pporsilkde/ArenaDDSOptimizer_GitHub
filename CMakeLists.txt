cmake_minimum_required(VERSION 3.21)
project(ArenaDDSOptimizer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

add_executable(ArenaDDSOptimizer
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/ddsinfo.cpp
    src/ddsinfo.h
    src/optimizerprofile.cpp
    src/optimizerprofile.h
)

target_link_libraries(ArenaDDSOptimizer PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if (MSVC)
    target_compile_options(ArenaDDSOptimizer PRIVATE /W4 /permissive-)
    target_compile_definitions(ArenaDDSOptimizer PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(ArenaDDSOptimizer PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (WIN32)
    set_target_properties(ArenaDDSOptimizer PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

install(TARGETS ArenaDDSOptimizer RUNTIME DESTINATION bin)
install(DIRECTORY presets/ DESTINATION share/ArenaDDSOptimizer/presets)
install(FILES README.md README_RU.md CHANGELOG.md DESTINATION share/ArenaDDSOptimizer)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    install(FILES LICENSE DESTINATION share/ArenaDDSOptimizer)
endif()
