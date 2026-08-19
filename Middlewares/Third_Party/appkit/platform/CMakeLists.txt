file(GLOB PLATFORM_GENERIC_SOURCE_FILES
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
)

set(PLATFORM_GENERIC_HEADER_FILES
    ${CMAKE_CURRENT_LIST_DIR}
)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/find_directory.cmake)
if(DEFINED APPKIT_PLATFORM)
    if(APPKIT_PLATFORM STREQUAL "LINUX")
        recurse_find_include_directory(${CMAKE_CURRENT_LIST_DIR}/linux PLATFORM_INCLUDE_DIRS)
        file(GLOB_RECURSE PLATFORM_SOURCE_FILES
            ${CMAKE_CURRENT_LIST_DIR}/linux/*.cpp
        )
    elseif(APPKIT_PLATFORM STREQUAL "STM32")
        recurse_find_include_directory(${CMAKE_CURRENT_LIST_DIR}/stm32 PLATFORM_INCLUDE_DIRS)
        file(GLOB_RECURSE PLATFORM_SOURCE_FILES
            ${CMAKE_CURRENT_LIST_DIR}/stm32/*.cpp
        )
    else()
        message(FATAL_ERROR "Unsupported platform: ${APPKIT_PLATFORM}")
    endif()

    message(STATUS "Enable platform: ${APPKIT_PLATFORM}")
else()
    message(FATAL_ERROR "APPKIT_PLATFORM is not defined.")
endif()

set(PLATFORM_SOURCE_FILES
    ${PLATFORM_GENERIC_SOURCE_FILES}
    ${PLATFORM_SOURCE_FILES}
    PARENT_SCOPE
)

set(PLATFORM_INCLUDE_DIRS
    ${PLATFORM_GENERIC_HEADER_FILES}
    ${PLATFORM_INCLUDE_DIRS}
    PARENT_SCOPE
)