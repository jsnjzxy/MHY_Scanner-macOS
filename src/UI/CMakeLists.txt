cmake_minimum_required(VERSION 3.26)

# 使用 qt_add_executable 而不是 add_executable，以启用 Qt 自动插件部署
qt_add_executable(${PROJECT_NAME}
    "${CMAKE_SOURCE_DIR}/src/Resources/MHY_Scanner.qrc"
    "Main.cpp"
    "Main/MainWindow.cpp"
    "Main/MainWindow.h"
    "Main/MainWindow.ui"
    "ScanThreads/StreamScanThread.mm"
    "ScanThreads/ScreenScanThread.cpp"
    "About/AboutDialog.cpp"
    "Login/LoginTab.cpp"
    "Login/SmsLoginTab.cpp"
    "Login/QrCodeLoginTab.cpp"
    "Login/CookieLoginTab.cpp"
    "Login/BiliLoginTab.cpp"
    "Login/LoginDialog.cpp"
    "Widgets/StyleManager.h"
    "Widgets/StyleManager.cpp"
    "Widgets/DraggableTableWidget.h"
)

# 设置 Objective-C++ 源文件的属性
set_source_files_properties(ScanThreads/StreamScanThread.mm PROPERTIES
    COMPILE_FLAGS "-x objective-c++"
)

# Qt6 核心组件
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
)

# Qt Multimedia
find_package(Qt6 REQUIRED COMPONENTS
    Multimedia
    MultimediaWidgets
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    MHY_Scanner_Core
)

# 添加当前源码目录到包含路径（用于 ui_*.h 找到自定义控件头文件）
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# macOS 框架
target_link_libraries(${PROJECT_NAME} PRIVATE
    "-framework CoreGraphics"
    "-framework CoreVideo"
    "-framework Quartz"
    "-framework AVFoundation"
    "-framework CoreMedia"
)

# 禁用 OpenCV 调试窗口
target_compile_definitions(${PROJECT_NAME} PRIVATE SHOW)

# 设置应用包属性
set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/src/Resources/Info.plist"
)

# 添加应用图标
set(APP_ICON "${CMAKE_SOURCE_DIR}/src/Resources/MHY_Scanner.icns")
if(EXISTS ${APP_ICON})
    set_source_files_properties(${APP_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(${PROJECT_NAME} PRIVATE ${APP_ICON})
endif()

# 复制 get_abogus 可执行文件到 Resources 目录
if(EXISTS ${CMAKE_SOURCE_DIR}/3rdparty/abogus_cpp/abogus_js/get_abogus)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/abogus_js"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/3rdparty/abogus_cpp/abogus_js/get_abogus"
            "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/abogus_js/get_abogus"
        COMMAND chmod +x "$<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/Resources/abogus_js/get_abogus"
        COMMENT "Copying get_abogus executable to app bundle..."
    )
endif()

# Debug 模式：允许调试器附加（get-task-allow）
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.mhy.scanner"
    )
    # 创建临时 entitlements 文件用于调试
    set(DEBUG_ENTITLEMENTS "${CMAKE_CURRENT_BINARY_DIR}/debug_entitlements.plist")
    file(WRITE "${DEBUG_ENTITLEMENTS}"
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
<plist version=\"1.0\">
<dict>
    <key>com.apple.security.get-task-allow</key>
    <true/>
</dict>
</plist>
")
    # 添加 get-task-allow 权限以允许 Instruments 附加
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND codesign --force --deep -s - --entitlements "${DEBUG_ENTITLEMENTS}" "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>"
        COMMENT "Signing with get-task-allow entitlement for debugging..."
    )
endif()
