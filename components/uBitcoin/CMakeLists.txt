file(GLOB_RECURSE src
    "src/*.c"
    "src/*.cpp"
)
set(include "./src/")

idf_component_register(SRCS ${src}
    INCLUDE_DIRS ${include}
    REQUIRES esp_system mbedtls
    PRIV_INCLUDE_DIRS ".")
