cmake_minimum_required(VERSION 3.10)
project(OJ_Project)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 生成 compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 1. 查找依赖
find_package(PkgConfig REQUIRED)
pkg_check_modules(MYSQL mysqlclient REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

# 2. 设置包含目录
include_directories(
    include 
    deps/imgui
    deps/imgui/backends
    ${MYSQL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
)

# 3. 收集源文件
file(GLOB SOURCES "src/*.cpp")
file(GLOB IMGUI_SOURCES "deps/imgui/*.cpp")
list(APPEND SOURCES ${IMGUI_SOURCES})
list(APPEND SOURCES "deps/imgui/backends/imgui_impl_glfw.cpp")
list(APPEND SOURCES "deps/imgui/backends/imgui_impl_opengl3.cpp")

# 4. 生成可执行文件
add_executable(oj_app ${SOURCES})

# 5. 链接库
target_link_libraries(oj_app 
    PRIVATE 
    ${MYSQL_LIBRARIES}
    OpenSSL::Crypto
    Threads::Threads
    glfw
    OpenGL::GL
)

# 打印信息用于调试
message(STATUS "Source files: ${SOURCES}")
message(STATUS "MySQL libraries: ${MYSQL_LIBRARIES}")
message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")
