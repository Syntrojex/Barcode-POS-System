cmake_minimum_required(VERSION 3.21)
project(BarcodeHub VERSION 0.2.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)   # needed for Q_OBJECT (CameraWidget, POSWindow...)

# Turn ON (-DSCANNER_ONLY=ON) to build just the scanner + CameraWidget, e.g. while
# another module is still unfinished. Default OFF = build everything.
option(SCANNER_ONLY "Build only the scanner and CameraWidget (to test one module)" OFF)

# ---- Qt (must use the same MinGW compiler as the kit) ----
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# ---- OpenCV 4 ----
# Prefer the copy built by scripts/build-windows.ps1 (tries the exact folder first).
foreach(candidate
        ${CMAKE_SOURCE_DIR}/build/deps/opencv/x64/mingw/lib
        ${CMAKE_SOURCE_DIR}/build/deps/opencv)
    if(EXISTS ${candidate}/OpenCVConfig.cmake)
        set(OpenCV_DIR ${candidate} CACHE PATH "OpenCV 4 location" FORCE)
        break()
    endif()
endforeach()
find_package(OpenCV 4 QUIET COMPONENTS core imgproc imgcodecs videoio)
if(NOT OpenCV_FOUND)
    message(FATAL_ERROR
        "OpenCV 4 (MinGW build) was not found.\n"
        "Run scripts/build-windows.ps1 once, or pass -DOpenCV_DIR=<folder with OpenCVConfig.cmake>.")
endif()
message(STATUS "OpenCV ${OpenCV_VERSION} found: ${OpenCV_DIR}")

# ---- Backend: every .cpp under Backend/src (billing, order, product, scanner) ----
# GLOB means a teammate adding a file never has to edit this list (fewer merge conflicts).
file(GLOB_RECURSE BACKEND_SOURCES CONFIGURE_DEPENDS Backend/src/*.cpp)
list(FILTER BACKEND_SOURCES EXCLUDE REGEX "[/\\\\][Mm]ain\\.cpp$")   # a stray main would clash
if(SCANNER_ONLY)
    list(FILTER BACKEND_SOURCES INCLUDE REGEX "Backend/src/scanner/")
endif()
if(NOT BACKEND_SOURCES)
    message(FATAL_ERROR "No .cpp files found under Backend/src.")
endif()
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
list(FILTER FRONTEND_SOURCES EXCLUDE REGEX "[/\\\\][Mm]ain\\.cpp$")
if(SCANNER_ONLY)
    list(FILTER FRONTEND_SOURCES EXCLUDE REGEX "POSWindow")
endif()
add_library(barcode_ui STATIC ${FRONTEND_SOURCES})
target_include_directories(barcode_ui PUBLIC Frontend/include Frontend/include/ui)
target_link_libraries(barcode_ui PUBLIC barcode_core)

# ---- The application: use the first NON-EMPTY main.cpp from these places ----
set(APP_MAIN "")
foreach(candidate Frontend/src/ui/main.cpp Frontend/src/main.cpp main.cpp)
    if(EXISTS ${CMAKE_SOURCE_DIR}/${candidate})
        file(SIZE ${CMAKE_SOURCE_DIR}/${candidate} main_size)
        if(main_size GREATER 0)
            set(APP_MAIN ${candidate})
            break()
        endif()
    endif()
endforeach()
if(NOT APP_MAIN)
    message(FATAL_ERROR "No non-empty main.cpp found (looked in Frontend/src/ui, Frontend/src and the root).")
endif()
message(STATUS "Application entry point: ${APP_MAIN}")
add_executable(BarcodeHub WIN32 ${APP_MAIN})
target_link_libraries(BarcodeHub PRIVATE barcode_ui)

# ---- Make the .exe runnable by double-click: copy Qt and OpenCV DLLs next to it ----
function(copy_runtime target)
    if(WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${target}> $<TARGET_FILE_DIR:${target}>
            COMMAND_EXPAND_LISTS VERBATIM)
        # Safety net: also copy the OpenCV DLLs directly if their folder exists.
        if(EXISTS ${OpenCV_DIR}/../bin)
            add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${OpenCV_DIR}/../bin $<TARGET_FILE_DIR:${target}>)
        endif()
    endif()
endfunction()
copy_runtime(BarcodeHub)

# ---- Regression tests: built only if the file exists ----
include(CTest)
if(BUILD_TESTING AND EXISTS ${CMAKE_SOURCE_DIR}/tests/regression.cpp)
    add_executable(barcode_tests tests/regression.cpp)
    target_link_libraries(barcode_tests PRIVATE barcode_ui)
    copy_runtime(barcode_tests)
    add_test(NAME barcode_regression COMMAND barcode_tests)
    set_tests_properties(barcode_regression PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 90)
endif()