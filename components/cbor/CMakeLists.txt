file(GLOB_RECURSE src
    "src/*.c"
)
set(include "./src/")

idf_component_register(SRCS ${src}
    INCLUDE_DIRS ${include}
    PRIV_INCLUDE_DIRS ".")
