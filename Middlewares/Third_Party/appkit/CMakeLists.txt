cmake_minimum_required(VERSION 3.22)

project(AppKit LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/find_directory.cmake)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/osal)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/platform)

recurse_find_include_directory(${CMAKE_CURRENT_LIST_DIR}/common COMMON_INCLUDE_DIRS)
recurse_find_include_directory(${CMAKE_CURRENT_LIST_DIR}/driver DRIVER_INCLUDE_DIRS)
recurse_find_include_directory(${CMAKE_CURRENT_LIST_DIR}/math MATH_INCLUDE_DIRS)
recurse_find_include_directory(${CMAKE_CURRENT_LIST_DIR}/middleware MIDDLEWARE_INCLUDE_DIRS)
recurse_find_include_directory(${CMAKE_CURRENT_LIST_DIR}/structure STRUCTURE_INCLUDE_DIRS)

set(APPKIT_INCLUDE_DIRS
    ${COMMON_INCLUDE_DIRS}
    ${DRIVER_INCLUDE_DIRS}
    ${MATH_INCLUDE_DIRS}
    ${MIDDLEWARE_INCLUDE_DIRS}
    ${OSAL_INCLUDE_DIRS}
    ${PLATFORM_INCLUDE_DIRS}
    ${STRUCTURE_INCLUDE_DIRS}
    ${THIRD_PARTY_INCLUDE_DIRS}
)

file(GLOB_RECURSE APPKIT_SOURCE_FILES
    ${CMAKE_CURRENT_LIST_DIR}/common/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/driver/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/math/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/middleware/*.cpp
    ${OSAL_SOURCE_FILES}
    ${PLATFORM_SOURCE_FILES}
    ${CMAKE_CURRENT_LIST_DIR}/structure/*.cpp
    ${THIRD_PARTY_SOURCE_FILES}
)

if(${PROJECT_IS_TOP_LEVEL})
    message(STATUS "Building AppKit as a standalone project.")

    set(CMAKE_CXX_STANDARD 23)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS ON)

    enable_testing()
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/test)

    add_executable(appkit)

    target_sources(appkit PRIVATE ${APPKIT_SOURCE_FILES} ${APPKIT_TEST_SOURCE_FILES})
    target_include_directories(appkit PRIVATE ${APPKIT_INCLUDE_DIRS} ${APPKIT_TEST_INCLUDE_DIRS})
    target_link_libraries(appkit PRIVATE ${THIRD_PARTY_LIBRARIES} ${APPKIT_TEST_LIBRARIES})

    target_compile_definitions(appkit PRIVATE 
        -DAPPKIT_PRINTF_BUFFER_SIZE=1024
        -DAPPKIT_LOGGER_BUFFER_SIZE=512
        -DAPPKIT_RAMFS_MAX_BLOCK_NAME_LEN=64
        -DAPPKIT_LOGGER_LEVEL=DEBUG
        -DAPPKIT_LOGGER_OUTPUT_LEVEL=DEBUG
    )

    # 定义文件名宏
    include(${CMAKE_CURRENT_LIST_DIR}/cmake/define_file_name.cmake)
    redefine_file_macro(appkit)
else()
    message(STATUS "Including AppKit as a subproject.")

    # 检查C++23支持
    target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE cxx_std_23)

    set(APPKIT_SOURCE_FILES ${APPKIT_SOURCE_FILES} PARENT_SCOPE)
    set(APPKIT_INCLUDE_DIRS ${APPKIT_INCLUDE_DIRS} PARENT_SCOPE)
    set(THIRD_PARTY_LIBRARIES ${THIRD_PARTY_LIBRARIES} PARENT_SCOPE)
endif()
