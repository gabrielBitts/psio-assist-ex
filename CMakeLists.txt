cmake_minimum_required(VERSION 3.5)

project(psio-assist LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 packages
find_package(Qt6 COMPONENTS Core Gui Widgets Sql Network REQUIRED)

# Add source files
add_executable(psio-assist
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/game.cpp
    src/game.h
    src/cuesheet.cpp
    src/cuesheet.h
    src/database.cpp
    src/database.h
    src/config.cpp
    src/config.h
    src/binfile.h
    src/preferencesdialog.h
    src/preferencesdialog.cpp
    src/binfile.cpp
)

# Link with Qt6 libraries
target_link_libraries(psio-assist PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
    Qt6::Network
) 