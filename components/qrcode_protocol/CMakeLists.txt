set(src "./qrcode_protocol.cpp")
set(include "./")

idf_component_register(SRCS ${src}
    INCLUDE_DIRS ${include}
    REQUIRES wallet bc-ur base64url cbor cJSON
    PRIV_INCLUDE_DIRS ".")
