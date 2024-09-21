set(src "./cJSON.c")
set(include "./")

idf_component_register(SRCS ${src}
    INCLUDE_DIRS ${include}
    PRIV_INCLUDE_DIRS ".")
