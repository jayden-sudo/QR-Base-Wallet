#include <qrcode_protocol.h>

String generate_metamask_crypto_hdkey(Wallet *wallet)
{
    PublicKeyFingerprint publicKeyFingerprint = wallet->public_eth_key_fingerprint();
    uint32_t fingerprint = (publicKeyFingerprint.fingerprint[0] << 24) |
                           (publicKeyFingerprint.fingerprint[1] << 16) |
                           (publicKeyFingerprint.fingerprint[2] << 8) |
                           (publicKeyFingerprint.fingerprint[3]);

    uint8_t buf[200] = {0};
    CborError err;
    CborEncoder encoder, mapEncoder;
    cbor_encoder_init(&encoder, buf, sizeof(buf), 0);
    err = cbor_encoder_create_map(&encoder, &mapEncoder, 3 /* 3,4,6 */);
    if (err)
    {
        Serial.println("Error creating map encoder");
        return "";
    }
    /* Key data */
    int key_key_data = 3;
    cbor_encode_int(&mapEncoder, key_key_data);
    cbor_encode_byte_string(&mapEncoder, publicKeyFingerprint.public_key, sizeof(publicKeyFingerprint.public_key));

    /* Chain code */
    int key_chain_code = 4;
    cbor_encode_int(&mapEncoder, key_chain_code);
    cbor_encode_byte_string(&mapEncoder, publicKeyFingerprint.chain_code, sizeof(publicKeyFingerprint.chain_code));

    /* Origin */
    int key_origin = 6;
    int RegistryType_crypto_keypath = 304;
    int keys_source_fingerprint = 2;
    int key_components = 1;
    cbor_encode_int(&mapEncoder, key_origin);
    cbor_encode_tag(&mapEncoder, RegistryType_crypto_keypath);

    /* Components path */
    CborEncoder originMapEncoder;
    cbor_encoder_create_map(&mapEncoder, &originMapEncoder, 2 /* components_path,sourceFingerprint */);
    char components_path[] = {44, 60, 0};
    cbor_encode_int(&originMapEncoder, key_components); // index 1 : components_path
    CborEncoder componentsArrayEncoder;
    cbor_encoder_create_array(&originMapEncoder, &componentsArrayEncoder, sizeof(components_path) * 2);
    for (int i = 0; i < sizeof(components_path); i++)
    {
        cbor_encode_int(&componentsArrayEncoder, components_path[i]);
        cbor_encode_boolean(&componentsArrayEncoder, 1 /* hardened = true */);
    }
    cbor_encoder_close_container_checked(&originMapEncoder, &componentsArrayEncoder);
    cbor_encode_int(&originMapEncoder, keys_source_fingerprint); // index 2 : sourceFingerprint
    cbor_encode_int(&originMapEncoder, fingerprint);
    cbor_encoder_close_container_checked(&mapEncoder, &originMapEncoder);
    cbor_encoder_close_container_checked(&encoder, &mapEncoder);
    size_t len = cbor_encoder_get_buffer_size(&encoder, buf);
    String encoded = "ur:crypto-hdkey/" + encode_minimal(buf, len);
    return encoded;
}