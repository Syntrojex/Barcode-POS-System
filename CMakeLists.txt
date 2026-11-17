cmake_minimum_required(VERSION 3.20)   # file(SIZE) needs 3.20; Qt ships a newer one
project(BarcodePOS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BACKEND_ONLY "Build only the Qt-free backend and its tests (no GUI)" OFF)
option(WITH_CAMERA  "Build the camera scanner + CameraWidget + POSWindow (needs OpenCV 4)" OFF)

# ---------------------------------------------------------------------------
# Windows: always use the Qt from C:/Qt, never the one from MSYS2.
# (Qt Creator's "CMake preset" kit has no Qt path, so CMake falls back to PATH and can pick
#  C:/msys64/ucrt64. Its uic.exe then dies with 0xc0000139 next to Qt's MinGW.)
# ---------------------------------------------------------------------------
set(QT_INSTALL_ROOT "C:/Qt" CACHE PATH "Folder that holds the Qt installs (6.x.x/mingw_64)")
if(WIN32)
    file(GLOB _qt_roots "${QT_INSTALL_ROOT}/6.*/mingw_64")
    if(_qt_roots)
        list(SORT _qt_roots COMPARE NATURAL ORDER DESCENDING)   # newest version first
        list(GET _qt_roots 0 _qt_root)

        # A Qt found earlier in MSYS2 is remembered in the cache: forget it.
        get_cmake_property(_cache_vars CACHE_VARIABLES)
        foreach(_v ${_cache_vars})
            if(_v MATCHES "^Qt6.*_DIR$" AND "${${_v}}" MATCHES "msys64")
                unset(${_v} CACHE)
            endif()
        endforeach()

        if(NOT Qt6_DIR AND NOT CMAKE_PREFIX_PATH)               # the kit did not give a Qt
            list(PREPEND CMAKE_PREFIX_PATH "${_qt_root}")
            message(STATUS "Qt: using ${_qt_root}")
        endif()
        list(APPEND CMAKE_IGNORE_PREFIX_PATH "C:/msys64" "C:/msys64/ucrt64" "C:/msys64/mingw64")
    endif()
endif()

# ===========================================================================
# BACKEND  (plain C++17: no Qt in here)
# ===========================================================================
# Two include styles are used by the modules, so both roots are exported:
#   Backend/include          ->  #include "order/OrderManager.h", "product/Product.h"
#   Backend/include/product  ->  #include "Product.h"   (Farzam's files include each other flat)

# --- Product (Farzam) ---------------------------------------------------------
file(GLOB PRODUCT_SOURCES CONFIGURE_DEPENDS Backend/src/product/*.cpp)
add_library(product_core STATIC ${PRODUCT_SOURCES})
target_include_directories(product_core PUBLIC Backend/include Backend/include/product)

# --- Order (Mustafa): independent of Product on purpose (talks via ProductSnapshot) ---
file(GLOB ORDER_SOURCES CONFIGURE_DEPENDS Backend/src/order/*.cpp)
add_library(order_core STATIC ${ORDER_SOURCES})
target_include_directories(order_core PUBLIC Backend/include)

# --- Billing (Ali): empty for now, so it only becomes a library once it has code ---
file(GLOB BILLING_SOURCES CONFIGURE_DEPENDS Backend/src/billing/*.cpp)
set(BILLING_REAL_SOURCES "")
foreach(f ${BILLING_SOURCES})
    file(SIZE ${f} _bytes)
    if(_bytes GREATER 0)
        list(APPEND BILLING_REAL_SOURCES ${f})
    endif()
endforeach()
if(BILLING_REAL_SOURCES)
    add_library(billing_core STATIC ${BILLING_REAL_SOURCES})
    target_include_directories(billing_core PUBLIC Backend/include Backend/include/billing)
endif()

# --- Integration glue: Product -> ProductSnapshot -> Order ---------------------
add_library(integration_core STATIC Backend/src/integration/ProductAdapter.cpp)
target_link_libraries(integration_core PUBLIC product_core order_core)

# --- One name for the frontend to link ---------------------------------------
add_library(backend_core INTERFACE)
target_link_libraries(backend_core INTERFACE product_core order_core integration_core)
if(TARGET billing_core)
    target_link_libraries(backend_core INTERFACE billing_core)
endif()

# --- Camera scanner (Shauzab): needs OpenCV, so opt-in ------------------------
if(WITH_CAMERA)
    foreach(candidate
            ${CMAKE_SOURCE_DIR}/build/deps/opencv/x64/mingw/lib
            ${CMAKE_SOURCE_DIR}/build/deps/opencv)
        if(EXISTS ${candidate}/OpenCVConfig.cmake)
            set(OpenCV_DIR ${candidate} CACHE PATH "OpenCV 4 location" FORCE)
            break()
        endif()
    endforeach()
    find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc imgcodecs videoio)

    file(GLOB SCANNER_SOURCES CONFIGURE_DEPENDS Backend/src/scanner/*.cpp)
    add_library(scanner_core STATIC ${SCANNER_SOURCES})
    target_include_directories(scanner_core PUBLIC Backend/include/scanner ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(scanner_core PUBLIC ${OpenCV_LIBS})
    if(WIN32)
        target_link_libraries(scanner_core PUBLIC ole32 oleaut32 strmiids)   # DirectShow camera
    endif()
endif()

# --- Backend tests (no Qt, no camera) -----------------------------------------
add_executable(integration_test tests/integration_test.cpp)
target_link_libraries(integration_test PRIVATE backend_core)

if(EXISTS ${CMAKE_SOURCE_DIR}/tests/product_tests.cpp)
    add_executable(product_tests tests/product_tests.cpp)
    target_link_libraries(product_tests PRIVATE product_core)
    if(EXISTS ${CMAKE_SOURCE_DIR}/tests/data/Product.sample.txt)
        add_custom_command(TARGET product_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/tests/data/Product.sample.txt
                $<TARGET_FILE_DIR:product_tests>/Product.sample.txt)
    endif()
endif()

enable_testing()
add_test(NAME integration COMMAND integration_test)

if(BACKEND_ONLY)
    message(STATUS "BACKEND_ONLY=ON: Qt frontend skipped")
    return()
endif()

# ===========================================================================
# FRONTEND  (Qt Widgets)
# ===========================================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

file(GLOB UI_SOURCES CONFIGURE_DEPENDS Frontend/src/ui/*.cpp)
file(GLOB UI_HEADERS CONFIGURE_DEPENDS Frontend/include/ui/*.h)

# main() never goes into the library (the executable owns it). Frontend/src/ui/Main.cpp is the old
# camera-only demo: it is skipped here too, it must not be compiled without OpenCV.
list(FILTER UI_SOURCES EXCLUDE REGEX "[/\\\\][Mm]ain\\.cpp$")   # Main.cpp AND main.cpp (REGEX is case-sensitive)

# These two need OpenCV, so they are only compiled when the camera is switched on.
if(NOT WITH_CAMERA)
    list(FILTER UI_SOURCES EXCLUDE REGEX "CameraWidget|POSWindow")
    list(FILTER UI_HEADERS EXCLUDE REGEX "CameraWidget|POSWindow")
endif()

add_library(barcode_ui STATIC ${UI_SOURCES} ${UI_HEADERS})
target_include_directories(barcode_ui PUBLIC Frontend/include/ui Frontend/include)
target_link_libraries(barcode_ui PUBLIC Qt${QT_VERSION_MAJOR}::Widgets backend_core)
if(WITH_CAMERA)
    target_link_libraries(barcode_ui PUBLIC scanner_core)
    target_compile_definitions(barcode_ui PUBLIC WITH_CAMERA)   # lets the UI use #ifdef WITH_CAMERA
endif()

# --- Stand-alone camera test window ------------------------------------------
if(WITH_CAMERA AND EXISTS ${CMAKE_SOURCE_DIR}/tests/camera_demo.cpp)
    add_executable(CameraDemo WIN32 tests/camera_demo.cpp)
    target_link_libraries(CameraDemo PRIVATE barcode_ui)
endif()

# --- Main app -------------------------------------------------------------------
if(EXISTS ${CMAKE_SOURCE_DIR}/Frontend/src/main.cpp)
    set(APP_MAIN Frontend/src/main.cpp)
elseif(EXISTS ${CMAKE_SOURCE_DIR}/Frontend/src/ui/main.cpp)
    set(APP_MAIN Frontend/src/ui/main.cpp)
else()
    message(FATAL_ERROR "No main.cpp found in Frontend/src or Frontend/src/ui")
endif()
add_executable(BarcodeHub WIN32 ${APP_MAIN})
target_link_libraries(BarcodeHub PRIVATE barcode_ui)

# --- Windows: DLLs + data next to the exe ---------------------------------------
if(WIN32)
    set(_apps BarcodeHub)
    if(TARGET CameraDemo)
        list(APPEND _apps CameraDemo)
    endif()

    # windeployqt copies the Qt DLLs *and* the plugins (platforms/qwindows.dll ...) next to the exe.
    # Without the plugins the exe says: Could not find the Qt platform plugin "windows".
    if(TARGET Qt${QT_VERSION_MAJOR}::windeployqt)
        get_target_property(WINDEPLOYQT_EXECUTABLE Qt${QT_VERSION_MAJOR}::windeployqt IMPORTED_LOCATION)
    else()
        find_program(WINDEPLOYQT_EXECUTABLE windeployqt
            HINTS "${Qt${QT_VERSION_MAJOR}_DIR}/../../../bin")
    endif()
    if(NOT WINDEPLOYQT_EXECUTABLE)
        message(WARNING "windeployqt not found: BarcodeHub.exe will not start from Explorer/Qt Creator")
    endif()
    get_filename_component(_cxx_bin "${CMAKE_CXX_COMPILER}" DIRECTORY)   # MinGW runtime DLLs live here

    foreach(app ${_apps})
        add_custom_command(TARGET ${app} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${app}> $<TARGET_FILE_DIR:${app}>
            COMMAND_EXPAND_LISTS VERBATIM)
        if(WITH_CAMERA AND EXISTS ${OpenCV_DIR}/../bin)
            add_custom_command(TARGET ${app} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${OpenCV_DIR}/../bin $<TARGET_FILE_DIR:${app}>)
        endif()
        if(WINDEPLOYQT_EXECUTABLE)
            add_custom_command(TARGET ${app} POST_BUILD
                COMMAND ${WINDEPLOYQT_EXECUTABLE} --force --no-translations $<TARGET_FILE:${app}>)   # --force: replace stale DLLs of another Qt version
        endif()
        foreach(_dll libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)   # MinGW runtime
            if(EXISTS "${_cxx_bin}/${_dll}")
                add_custom_command(TARGET ${app} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_cxx_bin}/${_dll}" $<TARGET_FILE_DIR:${app}>)
            endif()
        endforeach()
    endforeach()

    if(EXISTS ${CMAKE_SOURCE_DIR}/data)   # Product.txt, Order.txt ... next to the exe
        add_custom_command(TARGET BarcodeHub POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:BarcodeHub>/data)
    endif()
endif()
