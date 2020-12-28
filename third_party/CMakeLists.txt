cmake_minimum_required(VERSION 3.4.1)

project(third_party LANGUAGES C CXX)

# gfx-math
set(GFX_MATH_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gfx-math)

set(GFX_MATH_INCLUDE_DIRS
    ${GFX_MATH_DIR}/include
)

# libktx
set(KTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/KTX-Software)

set(KTX_SOURCES
    ${KTX_DIR}/lib/texture.c
    ${KTX_DIR}/lib/hashlist.c
    ${KTX_DIR}/lib/checkheader.c
    ${KTX_DIR}/lib/swap.c
    ${KTX_DIR}/lib/memstream.c
    ${KTX_DIR}/lib/filestream.c
)

set(KTX_INCLUDE_DIRS
    ${KTX_DIR}/include
    ${KTX_DIR}/lib
    ${KTX_DIR}/other_include
)

add_library(ktx ${KTX_SOURCES})

target_include_directories(ktx PUBLIC ${KTX_INCLUDE_DIRS})

set_property(TARGET ktx PROPERTY FOLDER "third_party")
