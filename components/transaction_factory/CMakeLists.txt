set(src "./transaction_factory.cpp")
set(include "./")

idf_component_register(SRCS ${src}
    INCLUDE_DIRS ${include}
    REQUIRES rlp uBitcoin
    PRIV_INCLUDE_DIRS ".")
