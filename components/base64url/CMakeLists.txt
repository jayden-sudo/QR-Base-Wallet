set(src "./base64url.cpp")
set(include "./")

idf_component_register(SRCS ${src}
    INCLUDE_DIRS ${include}
    REQUIRES uBitcoin
    PRIV_INCLUDE_DIRS ".")
