file(GLOB_RECURSE src
    "./*.c"

    # "./*.cpp"
)
set(include "./include" "./")

idf_component_register(SRCS ${src}
    INCLUDE_DIRS ${include}
    PRIV_INCLUDE_DIRS ".")

# lvgl_port_create_c_image("images/esp_logo.png" "images/" "ARGB8888" "NONE")
# lvgl_port_add_images(${COMPONENT_LIB} "images/")