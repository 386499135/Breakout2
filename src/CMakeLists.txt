cmake_minimum_required(VERSION 3.10)
project(Breakout)

set(CMAKE_CXX_STANDARD 17)

# 查找raylib
find_package(raylib REQUIRED)

# 查找nlohmann_json
find_package(nlohmann_json REQUIRED)

# 源文件
set(SOURCES
    main.cpp
    Ball.cpp
    Paddle.cpp
    Brick.cpp
    PowerUp.cpp
)

# 头文件
set(HEADERS
    Ball.h
    Paddle.h
    Brick.h
    PowerUp.h
)

# 创建可执行文件
add_executable(breakout ${SOURCES} ${HEADERS})

# 链接库
target_link_libraries(breakout raylib nlohmann_json::nlohmann_json)

# 包含目录
target_include_directories(breakout PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
