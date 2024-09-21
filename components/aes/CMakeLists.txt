set(src "./aes.c")
set(include "./")

idf_component_register(SRCS ${src}
    INCLUDE_DIRS ${include}
    REQUIRES mbedtls
    PRIV_INCLUDE_DIRS ".")
