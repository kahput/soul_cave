cmake_minimum_required(VERSION 3.11)
project(game)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    set(CONFIG $<CONFIG>)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
else()
    set(CONFIG ${CMAKE_BUILD_TYPE})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CONFIG})
endif()

# Dependencies
set(RAYLIB_VERSION 5.5)
find_package(raylib ${RAYLIB_VERSION} REQUIRED) # QUIET or REQUIRED

if (NOT raylib_FOUND) # If there's none, fetch and build raylib
  include(FetchContent)
  FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
  )
  FetchContent_GetProperties(raylib)
  if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(raylib)
  endif()
endif()

file(GLOB_RECURSE SOURCES "src/*.c" "src/*.h" )
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC "./src/")
target_link_libraries( ${PROJECT_NAME} raylib m)

if(MSVC)
    # /WX
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    # -Wpedantic
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable)
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    # Set source and destination directories
    set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")

    # Get all files in the assets directory
    file(GLOB_RECURSE ASSET_FILES "${ASSETS_DIR}/*.*")

    # Create custom command to copy each asset file
    foreach(ASSET_FILE ${ASSET_FILES})
        # Get relative path to preserve folder structure
        file(RELATIVE_PATH REL_PATH "${ASSETS_DIR}" "${ASSET_FILE}")
        set(DEST_FILE "${CMAKE_BINARY_DIR}/bin/${CONFIG}/assets/${REL_PATH}")

        add_custom_command(
            OUTPUT "${DEST_FILE}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ASSET_FILE}" "${DEST_FILE}"
            DEPENDS "${ASSET_FILE}"
            COMMENT "Copying asset: ${REL_PATH}"
            VERBATIM
        )

        list(APPEND ASSET_OUTPUTS "${DEST_FILE}")
    endforeach()

    # Create a custom target that depends on all copied assets
    add_custom_target(copy_assets ALL DEPENDS ${ASSET_OUTPUTS})
endif()
