cmake_minimum_required(VERSION 3.16)

project(EvolutionWORLD LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "Build raylib games" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(PLATFORM Desktop CACHE STRING "raylib platform" FORCE)
set(OPENGL_VERSION 1.1 CACHE STRING "raylib OpenGL version" FORCE)

add_subdirectory(build/external/raylib-master EXCLUDE_FROM_ALL)

add_executable(EvolutionWORLD
    include/miniz.c
    include/tinyfiledialogs.c
    src/Button.cpp
    src/Creature.cpp
    src/Helper.cpp
    src/InputField.cpp
    src/main.cpp
    src/PercentileGraph.cpp
    src/SpeciesGraph.cpp
    src/Slider.cpp
    src/World.cpp
)

target_include_directories(EvolutionWORLD PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/build/external/raylib-master/src"
)

target_link_libraries(EvolutionWORLD PRIVATE raylib)

if(WIN32)
    target_compile_definitions(EvolutionWORLD PRIVATE
        _WINSOCK_DEPRECATED_NO_WARNINGS
        _CRT_SECURE_NO_WARNINGS
    )
    target_link_libraries(EvolutionWORLD PRIVATE comdlg32 ole32)
endif()

if(APPLE)
    target_link_libraries(EvolutionWORLD PRIVATE
        "-framework IOKit"
        "-framework Cocoa"
        "-framework OpenGL"
    )
endif()

add_custom_command(TARGET EvolutionWORLD POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/resources"
            "$<TARGET_FILE_DIR:EvolutionWORLD>/resources"
)

install(TARGETS EvolutionWORLD RUNTIME DESTINATION .)
install(DIRECTORY resources DESTINATION .)
