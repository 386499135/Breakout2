cmake_minimum_required(VERSION 3.15)
project(BreakoutGame VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 raylib
find_package(raylib REQUIRED)

# 查找 nlohmann_json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.2
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# 源文件
set(SOURCES
    src/main.cpp
    src/Ball.cpp
    src/Paddle.cpp
    src/Brick.cpp
    src/PowerUp.cpp
    src/ParticleSystem.cpp
    src/SaveManager.cpp
    src/AsyncLoader.cpp
    src/Leaderboard.cpp
    src/NetworkManager.cpp
)

# 创建可执行文件
add_executable(breakout ${SOURCES})

# 包含目录
target_include_directories(breakout PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 链接库
target_link_libraries(breakout 
    raylib 
    nlohmann_json::nlohmann_json
    pthread
)

# 复制资源文件
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/levels)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/levels/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/levels)

# 设置输出目录
set_target_properties(breakout PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
