# Set the minimum version of CMake that can be used
# To find the cmake version run
# $ cmake --version
cmake_minimum_required(VERSION 3.5)

# Set the project name
project (client)

# Set C Flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -D_GNU_SOURCE")

# Create a sources variable with a link to all cpp files to compile
set(SOURCES
    src/helpers.c
    src/main.c
)

# Add an executable with the above sources
add_executable(client ${SOURCES})

# Set EVL library path
set(CMAKE_PREFIX_PATH "/usr/evl/lib")

# Find libraries
find_library(EVL_LIB evl)

# Link libraries
target_link_libraries(client PRIVATE "${EVL_LIB}")
target_link_libraries(client PRIVATE rt)
target_link_libraries(client PRIVATE pthread)
target_link_libraries(client PRIVATE m)

# Set the directories that should be included in the build command for this target
# when running g++ these will be included as -I/directory/path/
target_include_directories(client
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/include
        /usr/evl/include
)
