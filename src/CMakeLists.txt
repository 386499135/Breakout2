cmake_minimum_required(VERSION 3.10)
project(Breakout)

set(CMAKE_CXX_STANDARD 17)

# 查找raylib
find_package(raylib REQUIRED)

# 查找nlohmann_json
find_package(nlohmann_json REQUIRED)

# 查找ENet
find_package(enet QUIET)
if(NOT enet_FOUND)
    # 如果系统没有安装ENet，从源码构建
    include(FetchContent)
    FetchContent_Declare(
        enet
        GIT_REPOSITORY https://github.com/lsalzman/enet.git
        GIT_TAG v1.3.18
    )
    FetchContent_MakeAvailable(enet)
endif()

# 源文件
set(SOURCES
    main.cpp    
    main_networked.cpp
    Ball.cpp
    Paddle.cpp
    Brick.cpp
    PowerUp.cpp
    NetworkManager.cpp
)

# 头文件
set(HEADERS
    Ball.h
    Paddle.h
    Brick.h
    PowerUp.h
    NetworkManager.h
    NetworkProtocol.h
)

# 创建可执行文件
add_executable(breakout ${SOURCES} ${HEADERS})

# 链接库
if(enet_FOUND)
    target_link_libraries(breakout raylib nlohmann_json::nlohmann_json enet::enet)
else()
    target_link_libraries(breakout raylib nlohmann_json::nlohmann_json enet)
endif()

# 包含目录
target_include_directories(breakout PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
