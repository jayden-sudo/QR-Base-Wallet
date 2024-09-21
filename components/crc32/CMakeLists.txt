set(src "./crc32.c")
set(include "./")

idf_component_register(SRCS ${src}
    INCLUDE_DIRS ${include}
    PRIV_INCLUDE_DIRS ".")
