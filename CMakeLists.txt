cmake_minimum_required(VERSION 3.10)
project(DexRewrite)

set(CMAKE_CXX_STANDARD 17)
add_subdirectory(external/libbase)
add_subdirectory(external/libdex)
include_directories(external)

#dex src
aux_source_directory(./ src)

add_executable(DexRewrite
        ${src}
        repairdex.cpp repairdex.h baserepair.cpp baserepair.h)




target_link_libraries(DexRewrite z base dex)