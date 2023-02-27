CMAKE_MINIMUM_REQUIRED  (VERSION 3.14)
PROJECT                 (rmi-hxi-query LANGUAGES CXX)

# packages
SET                     (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake_modules/")
FIND_PACKAGE            (HIDAPI REQUIRED)

# includes and paths/directories
SET                     (CMAKE_INCLUDE_CURRENT_DIR on)
INCLUDE_DIRECTORIES     (${PROJECT_SOURCE_DIR}/sources)
SET                     (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
SET                     (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/plugin)
SET                     (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

# compiler settings
IF                      (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    MESSAGE             ("--> building on Linux")
    SET                 (CMAKE_CXX_FLAGS "-std=c++20 -pthread -flto -pedantic -W -Wall -Wextra -Os")
    SET                 (CMAKE_C_FLAGS "-std=c17 -pthread -flto -pedantic -W -Wall -Wextra -Os")
ELSE                    ()
    MESSAGE             (FATAL_ERROR "--> unsupported build host: ${CMAKE_SYSTEM_NAME}")
ENDIF                   ()

# sources
ADD_SUBDIRECTORY        (sources)
