cmake_minimum_required(VERSION 3.26)
project(fiddle C)

### Settings ------------------------------------------------------------------

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)

### Dependencies --------------------------------------------------------------

set(RAYLIB_VERSION 4.5.0)
find_package(raylib ${RAYLIB_VERSION} QUIET) # QUIET or REQUIRED
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
        FetchContent_Populate(raylib)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
        add_subdirectory(${raylib_SOURCE_DIR} ${raylib_BINARY_DIR})
    endif()
endif()

### Executable ----------------------------------------------------------------

add_executable(${PROJECT_NAME}
        src/main.c
        include/common.h
        include/ui.h
)

target_include_directories(${PROJECT_NAME} PRIVATE include)

### Library linkage -----------------------------------------------------------

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

if (APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework OpenGL")
endif()

### Web build via emscripten --------------------------------------------------

###
### TODO - figure out how to setup the emscripten build directly in cmake
### TODO - ideally we could also FetchContent for the emsdk package, but not sure if that can work
###
#add_custom_target(${PROJECT_NAME}-web
# NOTE - don't think \ multiline is actually supported
#        COMMAND ${CMAKE_COMMAND} -E env "EMSDK=$ENV{TOOLS_PATH}/emsdk" \
#            emcmake ${CMAKE_COMMAND} ${CMAKE_SOURCE_DIR} \
#            -DPLATFORM=Web -DCMAKE_BUILD_TYPE=Release \
#            -DCMAKE_EXE_LINKER_FLAGS="-s USE_GLFW=3" \
#            -DCMAKE_EXECUTABLE_SUFFIX=".html"
#        COMMAND ${CMAKE_COMMAND} --build . --config Release
#        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/web_build
#)

###
### TODO - is this needed if we can get the custom command above to work?
###
#if (DEFINED PLATFORM)
#    if (${PLATFORM} STREQUAL "Web")
#        # Tell Emscripten to build an example.html file.
#        set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".html")
#    endif()
#endif()
