cmake_minimum_required(VERSION 3.20)
project(VIRUS38)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set compiler flags untuk ukuran kecil
if(MSVC)
    add_compile_options(/Os /Oy /GT /GF /Gy /GS- /GR- /EHa-)
    add_link_options(/DEBUG:NONE /OPT:REF /OPT:ICF /LTCG)
endif()

# Find packages
find_package(Gdiplus REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/effects.cpp
    src/mbr.cpp
    src/system.cpp
)

# Include directories
include_directories(
    include
    ${Gdiplus_INCLUDE_DIRS}
)

# Create executable
add_executable(VIRUS38 ${SOURCES})

# Link libraries
target_link_libraries(VIRUS38
    ${Gdiplus_LIBRARIES}
    winmm
    ntdll
)

# Install instructions
install(TARGETS VIRUS38 DESTINATION bin)
