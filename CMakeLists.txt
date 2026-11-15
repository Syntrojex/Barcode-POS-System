cmake_minimum_required(VERSION 3.21)
project(BarcodeHub VERSION 0.2.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)   # needed for Q_OBJECT (CameraWidget, POSWindow...)

# Qt and the application must use the same compiler ABI (MinGW kit).
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# Increment 2 needs the camera, so OpenCV 4 is required.
find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc imgcodecs videoio)
message(STATUS "OpenCV ${OpenCV_VERSION} found: ${OpenCV_DIR}")

# ---- Backend: every .cpp under Backend/src (billing, order, product, scanner) ----
# GLOB means a teammate adding a file never has to edit this list (fewer merge conflicts).
file(GLOB_RECURSE BACKEND_SOURCES CONFIGURE_DEPENDS Backend/src/*.cpp)
add_library(barcode_core STATIC ${BACKEND_SOURCES})
target_include_directories(barcode_core PUBLIC
    Backend/include
    Backend/include/billing
    Backend/include/order
    Backend/include/product
    Backend/include/scanner
    ${OpenCV_INCLUDE_DIRS})
target_link_libraries(barcode_core PUBLIC Qt${QT_VERSION_MAJOR}::Widgets ${OpenCV_LIBS})
if(WIN32)
    target_link_libraries(barcode_core PUBLIC ole32 oleaut32 strmiids)   # DirectShow camera
endif()

# ---- Frontend: every .cpp/.h under Frontend except main.cpp ----
file(GLOB_RECURSE FRONTEND_SOURCES CONFIGURE_DEPENDS Frontend/src/*.cpp Frontend/include/*.h)
list(FILTER FRONTEND_SOURCES EXCLUDE REGEX "[/\\\\]main\\.cpp$")
add_library(barcode_ui STATIC ${FRONTEND_SOURCES})
target_include_directories(barcode_ui PUBLIC Frontend/include Frontend/include/ui)
target_link_libraries(barcode_ui PUBLIC barcode_core)

# ---- The application ----
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/Frontend/src/main.cpp)
    message(FATAL_ERROR "Frontend/src/main.cpp is missing. It must create QApplication and show the main window.")
endif()
add_executable(BarcodeHub WIN32 Frontend/src/main.cpp)
target_link_libraries(BarcodeHub PRIVATE barcode_ui)

# Copy the Qt/OpenCV DLLs next to the .exe so it runs by double-click.
function(copy_runtime target)
    if(WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${target}> $<TARGET_FILE_DIR:${target}>
            COMMAND_EXPAND_LISTS VERBATIM)
    endif()
endfunction()
copy_runtime(BarcodeHub)

# Regression tests are built only if the file exists.
include(CTest)
if(BUILD_TESTING AND EXISTS ${CMAKE_SOURCE_DIR}/tests/regression.cpp)
    add_executable(barcode_tests tests/regression.cpp)
    target_link_libraries(barcode_tests PRIVATE barcode_ui)
    copy_runtime(barcode_tests)
    add_test(NAME barcode_regression COMMAND barcode_tests)
    set_tests_properties(barcode_regression PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 90)
endif()