file(GLOB OSAL_GENERIC_SOURCE_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
)

set(OSAL_GENERIC_HEADER_FILES
    ${CMAKE_CURRENT_LIST_DIR}
)

include(${CMAKE_CURRENT_LIST_DIR}/../cmake/find_directory.cmake)
if(DEFINED APPKIT_OSAL_SYSTEM)
    if(APPKIT_OSAL_SYSTEM STREQUAL "LINUX")
        recurse_find_include_directory(${CMAKE_CURRENT_LIST_DIR}/linux OSAL_INCLUDE_DIRS)
        file(GLOB OSAL_SOURCE_FILES
            ${CMAKE_CURRENT_LIST_DIR}/linux/*.cpp
        )
    elseif(APPKIT_OSAL_SYSTEM STREQUAL "FREERTOS")
        recurse_find_include_directory(${CMAKE_CURRENT_LIST_DIR}/freertos OSAL_INCLUDE_DIRS)
        file(GLOB OSAL_SOURCE_FILES
            ${CMAKE_CURRENT_LIST_DIR}/freertos/*.cpp
        )
    else()
        message(FATAL_ERROR "Unsupported OSAL system: ${APPKIT_OSAL_SYSTEM}")
    endif()

    message(STATUS "Enable system OSAL: ${APPKIT_OSAL_SYSTEM}")
else()
    message(FATAL_ERROR "APPKIT_OSAL_SYSTEM is not defined.")
endif()

set(OSAL_SOURCE_FILES
    ${OSAL_GENERIC_SOURCE_FILES}
    ${OSAL_SOURCE_FILES}
    PARENT_SCOPE
)

set(OSAL_INCLUDE_DIRS
    ${OSAL_GENERIC_HEADER_FILES}
    ${OSAL_INCLUDE_DIRS}
    PARENT_SCOPE
)