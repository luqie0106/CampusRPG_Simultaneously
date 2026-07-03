cmake_minimum_required(VERSION 3.16)

# 项目名称与版本配置（对应你们的 2026 校园RPG大作业）
project(CampusRPG VERSION 1.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 要求支持 C++20 标准（Qt6 推荐至少 C++17，C++20 更佳）
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 自动处理 Qt 的元对象编译器 (MOC)、UI 编译器 (UIC) 和资源编译器 (RCC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 1. 查找 Qt6 的核心组件、图形界面组件和微件组件
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql)

# 2. 收集项目内所有的头文件与源文件
# 头文件包含路径
include_directories(include)

# 自动搜集 src 目录和根目录下的代码文件（根目录用非递归以防扫到 build 文件夹）
file(GLOB SRC_ROOT_FILES *.cpp *.h)
file(GLOB_RECURSE SRC_DIR_FILES src/*.cpp src/*.h)
set(SRC_FILES ${SRC_ROOT_FILES} ${SRC_DIR_FILES})

# 自动搜集 ui 文件和资源文件
file(GLOB UI_FILES *.ui)
file(GLOB QRC_FILES *.qrc)

# 3. 定义生成的可执行文件
add_executable(${PROJECT_NAME} ${SRC_FILES} ${UI_FILES} ${QRC_FILES})

# 4. 链接 Qt6 的库到你们的游戏项目中
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
)

# 5. 跨平台素材路径神器：将代码所在的真实目录传递给 C++
target_compile_definitions(${PROJECT_NAME} PRIVATE 
    PROJECT_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

# 针对 Windows 系统的特殊处理：防止运行时弹出一个空白的 CMD 命令行窗口
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
    )
endif()

# Force CMake reload for new files