set(src "./app.cpp" "./app_peripherals.c")
set(include "./include")

idf_component_register(SRCS ${src}
    INCLUDE_DIRS ${include}
    REQUIRES nvs_flash spi_flash console esp_timer uBitcoin wallet aes base64url bc-ur cbor crc32 qrcode_protocol qrcodegen rlp esp-code-scanner esp32-camera
    PRIV_INCLUDE_DIRS ".")