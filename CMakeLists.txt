cmake_minimum_required(VERSION 3.26)
project(MHY_Scanner LANGUAGES CXX OBJCXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OBJCXX_STANDARD 23)

# macOS 部署目标
set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "Minimum OS X deployment version" FORCE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(MAJOR_VERSION 1)
set(MINOR_VERSION 1)
set(MICRO_VERSION 15)
set(_VERSION_ "v${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}")

option(USE_OPENSSL "OPENSSL" ON)
option(DEV "DEV" OFF)

message(STATUS "Platform: macOS")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Apple Clang: 忽略废弃警告
add_compile_options(-Wno-deprecated-declarations)

# ========================================
# 依赖查找
# ========================================

# Qt 6 组件（只包含实际使用的）
set(QT_COMPONENTS
    Core
    Widgets
    Gui
    Multimedia
    MultimediaWidgets
)

find_package(Qt6 REQUIRED COMPONENTS ${QT_COMPONENTS})

# vcpkg 依赖 (非 Qt 依赖)
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui)
find_package(FFmpeg REQUIRED)
find_package(cpr REQUIRED)
find_package(nlohmann_json REQUIRED)

if(USE_OPENSSL)
    find_package(OpenSSL REQUIRED)
endif()

# ========================================
# 本地第三方库
# ========================================

# nayuki-qr-code-generator (通过 vcpkg)
# 根据架构确定静态库 triplet
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm64|aarch64")
    set(STATIC_TRIPLET "arm64-osx")
else()
    set(STATIC_TRIPLET "x64-osx")
endif()

# 查找静态库
find_library(QRCODEGEN_LIB
    NAMES nayuki-qr-code-generator
    PATHS
        "$ENV{VCPKG_ROOT}/installed/${STATIC_TRIPLET}/lib"
    NO_DEFAULT_PATH
)

if(QRCODEGEN_LIB)
    message(STATUS "Using nayuki-qr-code-generator from ${QRCODEGEN_LIB}")
    include_directories("$ENV{VCPKG_ROOT}/installed/${STATIC_TRIPLET}/include")
else()
    message(FATAL_ERROR "nayuki-qr-code-generator not found. Please install via vcpkg: vcpkg install nayuki-qr-code-generator --triplet=${STATIC_TRIPLET}")
endif()

# abogus_cpp (抖音 a_bogus 算法，保留本地)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/abogus_cpp/abogus.cpp)
    add_library(abogus_lib STATIC ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/abogus_cpp/abogus.cpp)
    target_include_directories(abogus_lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/abogus_cpp)
    target_link_libraries(abogus_lib PRIVATE nlohmann_json::nlohmann_json)
else()
    set(abogus_lib "")
endif()

# src/Core 头文件路径 (UI 目标需要访问 Core 头文件)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/Core)

# ========================================
# 添加子目录
# ========================================
add_subdirectory(src/UI)
add_subdirectory(src/Core)

# ========================================
# 项目链接库配置
# ========================================

# 基础库
set(PROJECT_LIBRARIES
    MHY_Scanner_Core
    ${QRCODEGEN_LIB}
    ${abogus_lib}
)

# Qt 6 组件
list(APPEND PROJECT_LIBRARIES
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Multimedia
    Qt6::MultimediaWidgets
)

# vcpkg 库
list(APPEND PROJECT_LIBRARIES ${OpenCV_LIBS})

# FFmpeg 库
if(TARGET FFmpeg::avcodec)
    list(APPEND PROJECT_LIBRARIES FFmpeg::avcodec)
endif()
if(TARGET FFmpeg::avdevice)
    list(APPEND PROJECT_LIBRARIES FFmpeg::avdevice)
endif()
if(TARGET FFmpeg::avfilter)
    list(APPEND PROJECT_LIBRARIES FFmpeg::avfilter)
endif()
if(TARGET FFmpeg::avformat)
    list(APPEND PROJECT_LIBRARIES FFmpeg::avformat)
endif()
if(TARGET FFmpeg::avutil)
    list(APPEND PROJECT_LIBRARIES FFmpeg::avutil)
endif()
if(TARGET FFmpeg::swresample)
    list(APPEND PROJECT_LIBRARIES FFmpeg::swresample)
endif()
if(TARGET FFmpeg::swscale)
    list(APPEND PROJECT_LIBRARIES FFmpeg::swscale)
else()
    # vcpkg 可能没有提供 swscale target，直接添加库名
    list(APPEND PROJECT_LIBRARIES swscale)
endif()

    # cpr
    list(APPEND PROJECT_LIBRARIES cpr::cpr)

# OpenSSL
if(USE_OPENSSL)
    list(APPEND PROJECT_LIBRARIES OpenSSL::SSL OpenSSL::Crypto)
endif()

# 链接库到主目标
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_LIBRARIES})

# macOS 特定属性
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/src/Resources/Info.plist
)

# 定义版本号
target_compile_definitions(${PROJECT_NAME} PRIVATE SCAN_VER="${_VERSION_}")

if(NOT DEV)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TESTSPEED SHOW)
endif()

# ========================================
# 安装配置
# ========================================
set(Install_Directory "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}_${_VERSION_}")

install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION ${Install_Directory})

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/abogus_cpp/abogus_js/get_abogus)
    install(FILES 3rdparty/abogus_cpp/abogus_js/get_abogus
        DESTINATION ${Install_Directory}/abogus_cpp/abogus_js
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    )
endif()
