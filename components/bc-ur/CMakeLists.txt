set(src "./bc-ur.cpp")
set(include "./")

idf_component_register(SRCS ${src}
    INCLUDE_DIRS ${include}
    REQUIRES crc32
    PRIV_INCLUDE_DIRS ".")
