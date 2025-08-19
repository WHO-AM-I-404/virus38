cmake_minimum_required(VERSION 3.20)
project(VIRUS38)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/Os /Oy /GT /GF /Gy /GS- /GR- /EHa-)
    add_link_options(/DEBUG:NONE /OPT:REF /OPT:ICF /LTCG)
endif()

find_package(Gdiplus REQUIRED)

set(SOURCES
    virus38.cpp
)

include_directories(
    ${Gdiplus_INCLUDE_DIRS}
)

add_executable(VIRUS38 ${SOURCES})

target_link_libraries(VIRUS38
    ${Gdiplus_LIBRARIES}
    winmm
    ntdll
)

install(TARGETS VIRUS38 DESTINATION bin)
