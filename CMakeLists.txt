cmake_minimum_required(VERSION 3.20)
project(BarCodePOS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6
find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)
# OpenCV (for camera capture)
find_package(OpenCV REQUIRED)
# ZBar (barcode decoding)
find_package(ZBar REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/CameraService.cpp
    src/BarcodeEngine.cpp
    src/FileIO.cpp
    src/OrderManager.cpp
    src/BillGenerator.cpp
    src/Models.cpp
    gui/MainWindow.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/gui)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets ${OpenCV_LIBS} ZBar::zbar)
