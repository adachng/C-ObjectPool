add_library(c_objectpool
    c_objectpool/ObjectPool.c c_objectpool/ObjectPool.h
)

target_include_directories(c_objectpool
    INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
