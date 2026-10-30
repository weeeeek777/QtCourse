cmake_minimum_required(VERSION 3.21)

project(Calculator
    VERSION 1.0.0
    DESCRIPTION "带键盘事件的计算器（Qt Widgets）"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

# 界面与计算逻辑封装成静态库，便于主程序、单元测试、截图工具复用
qt_add_library(calculatorlib STATIC
    src/calculatorengine.h
    src/calculatorengine.cpp
    src/calculator.h
    src/calculator.cpp
    src/calculator.ui
)

target_include_directories(calculatorlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(calculatorlib PUBLIC Qt6::Widgets)

# 源码含中文与 UTF-8 符号，MSVC 需要显式指定编码
if(MSVC)
    target_compile_options(calculatorlib PRIVATE /utf-8 /W4 /permissive-)
else()
    target_compile_options(calculatorlib PRIVATE -Wall -Wextra)
endif()
qt_add_resources(calculatorlib "app_style"
    PREFIX "/"
    BASE "src"
    FILES src/style.qss
)

qt_add_executable(Calculator WIN32
    src/main.cpp
)
target_link_libraries(Calculator PRIVATE calculatorlib)

# 运行所需的 Qt 运行时由 windeployqt 复制到可执行文件目录（可选目标）
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT6_INSTALL_PREFIX}/bin")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_target(deploy
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations --no-compiler-runtime
                    "$<TARGET_FILE:Calculator>"
            DEPENDS Calculator
            COMMENT "使用 windeployqt 收集 Qt 运行时"
        )
    endif()
endif()

option(BUILD_TESTING "构建单元测试" ON)
if(BUILD_TESTING)
    enable_testing()
    find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
    qt_add_executable(tst_calculatorengine
        tests/tst_calculatorengine.cpp
    )
    target_link_libraries(tst_calculatorengine PRIVATE calculatorlib Qt6::Test)
    add_test(NAME tst_calculatorengine COMMAND tst_calculatorengine)
endif()
