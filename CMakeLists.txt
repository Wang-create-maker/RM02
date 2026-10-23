cmake_minimum_required(VERSION 3.10)

project(rm_lightbar_detection LANGUAGES CXX)

# C++ 标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 默认 Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
#为什么用 CACHE STRING ... FORCE？
#  CACHE：写进 CMakeCache.txt，后续 cmake .. 会复用，不会每次重新判断；
#  FORCE：在 if 成立时强制写入缓存，否则普通 set 只设当前作用域，缓存里没有，下次配置又会进 if。
#  Release 会加 -O3 -DNDEBUG，处理视频时明显比 Debug 快；Debug 加 -g，方便 gdb。任务二是逐帧处理视频，默认 Release 合理。#



# OpenCV 安装路径：根据任务一安装位置设置
# 如果安装在其他路径，修改这里，或在 cmake 时传入 -DOpenCV_DIR=...
set(OpenCV_DIR "/usr/local/opencv4.5.4/lib/cmake/opencv4"
    CACHE PATH "OpenCV config directory")

# 搜索 OpenCV
# 必做灯条识别主要需要 core/imgproc/highgui/videoio/imgcodecs
# 选做 PnP 位姿解算需要 calib3d
set(OpenCV_DIR "/usr/local/opencv4.5.4/lib/cmake/opencv4")
find_package(OpenCV 4.5 REQUIRED COMPONENTS
    core
    imgproc
    highgui
    videoio
    imgcodecs
    calib3d
)

message(STATUS "OpenCV_VERSION       = ${OpenCV_VERSION}")
message(STATUS "OpenCV_INCLUDE_DIRS  = ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV_LIBS          = ${OpenCV_LIBS}")

# 可执行文件
# 如果项目拆成多个 cpp，继续在下面添加，例如 src/lightbar_detector.cpp
add_executable(${PROJECT_NAME}
    src/main.cpp
)

# 指定 OpenCV 头文件搜索路径
target_include_directories(${PROJECT_NAME} PRIVATE
    ${OpenCV_INCLUDE_DIRS}
)

# 链接 OpenCV 库
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OpenCV_LIBS}
)

# 生成的可执行文件放到 build/bin 下，方便查找
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)