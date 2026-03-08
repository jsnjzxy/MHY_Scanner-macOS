cmake_minimum_required(VERSION 3.26)

# Objective-C++ 源文件列表
set(OBJCPP_SOURCES
    Scanner/QRScanner.mm  # Objective-C++ 文件用于 Vision 框架
    Scanner/ScreenCapture/ScreenCapture.mm  # Objective-C++ 文件用于屏幕捕获
    Scanner/ScreenCapture/ScreenCaptureKitWrapper.mm  # ScreenCaptureKit 封装（macOS 12.3+）

    # 新架构：统一扫码框架
    Scanner/UnifiedFrameBuffer.mm  # 环形帧缓冲区（零拷贝）
    Scanner/VisionProcessor.mm  # Vision 处理器（VNRequest 复用）
    Scanner/UnifiedScanner.mm  # 统一扫码器
    Scanner/GPUCopyHelper.mm  # GPU → GPU 拷贝工具（断开 VTDecoder 关联）
)

add_library(MHY_Scanner_Core STATIC
    Utils/CryptoUtils.cpp
    Network/HttpClient.cpp
    Network/Api/LiveStreamApi.cpp
    SDK/MihoyoSDK.cpp
    ${OBJCPP_SOURCES}
    Config/ConfigManager.cpp
    Network/Api/MihoyoApi.hpp
    Network/Api/BilibiliApi.hpp
    Utils/UUIDUtil.hpp
    Scanner/ScannerBase.hpp
)

# 设置 Objective-C++ 源文件的属性
set_source_files_properties(${OBJCPP_SOURCES} PROPERTIES
    COMPILE_FLAGS "-x objective-c++"
)

# 添加 include 路径 (必须在 add_library 之后)
target_include_directories(MHY_Scanner_Core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_SOURCE_DIR}/3rdparty/boost
)

# OpenSSL 链接 (通过 vcpkg)
if(USE_OPENSSL)
    target_link_libraries(MHY_Scanner_Core PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(MHY_Scanner_Core PRIVATE USE_OPENSSL)
endif()

# OpenCV 链接 (通过 vcpkg)
if(OpenCV_FOUND)
    target_link_libraries(MHY_Scanner_Core PRIVATE ${OpenCV_LIBS})
    # vcpkg 的 OpenCV 头文件在 opencv4/opencv2/ 下
    target_include_directories(MHY_Scanner_Core PRIVATE ${OpenCV_INCLUDE_DIRS})
endif()

# macOS 框架
target_link_libraries(MHY_Scanner_Core PRIVATE
    "-framework Vision"
    "-framework CoreVideo"
    "-framework CoreImage"
    "-framework ScreenCaptureKit"
    Qt6::Core
)

target_compile_definitions(MHY_Scanner_Core PRIVATE USE_SCREEN_CAPTURE_KIT)

# 添加 abogus.cpp 到核心库
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/abogus_cpp/abogus.cpp)
    target_sources(MHY_Scanner_Core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/abogus_cpp/abogus.cpp)
    target_include_directories(MHY_Scanner_Core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/abogus_cpp)
endif()
