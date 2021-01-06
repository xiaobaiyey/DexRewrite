set(CMAKE_CXX_STANDARD 17)
aux_source_directory(./ libbase_src)

if (shared_library)
    add_library(base SHARED ${libbase_src})
else ()
    add_library(base STATIC ${libbase_src} )
endif ()
