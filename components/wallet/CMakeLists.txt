set(src "./wallet.cpp")
set(include "./")

idf_component_register(SRCS ${src}
    INCLUDE_DIRS ${include}
    REQUIRES uBitcoin transaction_factory
    PRIV_INCLUDE_DIRS ".")
