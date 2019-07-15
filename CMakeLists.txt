cmake_minimum_required(VERSION 3.14)
project(DexRewrite)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)

#dex src
aux_source_directory(src/libdex DEX_SRC)

aux_source_directory(src/libutil/ UTIL_SRC)



aux_source_directory(./ test_src)

add_executable(DexRewrite
        ${DEX_SRC}
        ${test_src}
        ${UTIL_SRC}
        )






find_library(lib-z z)

target_link_libraries(DexRewrite ${lib-z})