#include "qrcode_protocol.hpp"
#include "cbor.h"
#include "cborjson.h"
#include "cJSON.h"
#include "base64url.hpp"
#include <algorithm>
#include "esp_log.h"

#define TAG "QRCODE_PROTOCOL"
#define LOGI(...) ESP_LOGI(TAG, __VA_ARGS__)
#define LOGE(...) ESP_LOGE(TAG, __VA_ARGS__)
URType ur_type(const std::string &url)
{
    size_t _first_pos = url.find('/');
    if (_first_pos != std::string::npos)
    {
        size_t _second_pos = url.find('/', _first_pos + 1);
        if (_second_pos != std::string::npos)
            return URType::MultiPart;
        else
            return URType::SinglePart;
    }
    return URType::Invalid;
}

std::string generate_metamask_crypto_hdkey(Wallet *wallet)
{
    PublicKeyFingerprint publicKeyFingerprint = wallet->public_eth_key_fingerprint();
    uint32_t fingerprint = (publicKeyFingerprint.fingerprint[0] << 24) |
                           (publicKeyFingerprint.fingerprint[1] << 16) |
                           (publicKeyFingerprint.fingerprint[2] << 8) |
                           (publicKeyFingerprint.fingerprint[3]);

    size_t buf_len = 200;
    uint8_t *buf = new uint8_t[buf_len];
    CborError err;
    CborEncoder encoder, mapEncoder;
    cbor_encoder_init(&encoder, buf, buf_len, 0);
    err = cbor_encoder_create_map(&encoder, &mapEncoder, 3 /* 3,4,6 */);
    if (err)
    {
        printf("Error creating map encoder");
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

    std::vector<uint8_t, PSRAMAllocator<uint8_t>> vec(buf, buf + len);
    delete[] buf;
    ur::UR ur_crypto_hdkey(METAMASK_CRYPTO_HDKEY, vec);
    std::string encoded = ur::UREncoder::encode(ur_crypto_hdkey);
    return encoded;
}

static std::string cast_cbor_to_json(const uint8_t *output_payload, size_t output_payload_len)
{
    int json_flags = CborConvertDefaultFlags | CborConvertTagsToObjects | CborConvertStringifyMapKeys | CborConvertByteStringsToBase64Url;
    CborParser parser;
    CborValue value;
    CborError err = cbor_parser_init(output_payload, output_payload_len, 0, &parser, &value);
    if (!err)
    {
        size_t json_length = 0;
        char *json_result = NULL;
        FILE *memstream;
        memstream = open_memstream(&json_result, &json_length);
        if (memstream != NULL)
        {
            err = cbor_value_to_json_advance(memstream, &value, json_flags);
            if (fclose(memstream) != 0)
            {
                free(json_result);
                return "";
            }
            else
            {
                std::string json_string = std::string(json_result);
                free(json_result);
                return json_string;
            }
        }
    }
    return "";
}

#define KEY_REQUEST_ID "1"
#define KEY_SIGN_DATA "2"
#define KEY_DATA_TYPE "3"
#define KEY_CHAIN_ID "4"
#define KEY_DERIVATION_PATH "5"
#define KEY_ADDRESS "6"
#define TAG_UUID "tag37"
#define TAG_CRYPTO_KEYPATH "tag304"

int decode_metamask_eth_sign_request(ur::UR *ur, MetamaskEthSignRequest *request)
{
    // eth-sign-request
    if (ur->type() != METAMASK_ETH_SIGN_REQUEST)
    {
        return 1;
    }
    const uint8_t *payload_ptr = const_cast<uint8_t *>(ur->cbor().data());
    size_t payload_len = ur->cbor().size();
    // cbor decode
    std::string json_string = cast_cbor_to_json(payload_ptr, payload_len);
    if (json_string.empty())
    {
        return 2;
    }
    /*
        {
            "1": { // keys_requestId
                "tag37": "CgQTuuJcShSf9B3ptoq_dw" // RegistryType_uuid = 37
            },
            "2": "AvODqjangIRZaC8AhQy4MDcwglIIlI9j191qP1k4YW7wYBa78lvWAjMViAFjRXhdigAAgMA", // keys_signData
            "3": 4, // keys_dataType
            "4": 11155111, // keys_chainId
            "5": { // keys_derivationPath
                "tag304": { // RegistryType_crypto_keypath = 304;
                    "1": [
                        44,
                        true,
                        60,
                        true,
                        0,
                        true,
                        0,
                        false,
                        0,
                        false
                    ],
                    "2": 3911562418 // keys_sourceFingerprint
                }
            },
            "6": "n-I5XWdpeDa0n1I6UUGn-P_QB2o" // keys_address
        }
     */
    LOGI("json_string: %s", json_string.c_str());
    // cJSON *json = cJSON_Parse(json_string.c_str());
    std::unique_ptr<cJSON, decltype(&cJSON_Delete)> _json(cJSON_Parse(json_string.c_str()), cJSON_Delete);
    if (_json == nullptr)
    {
        return 3;
    }
    const cJSON *json = _json.get();

    const cJSON *keys_requestId_path_doc = cJSON_GetObjectItemCaseSensitive(json, KEY_REQUEST_ID);
    if (keys_requestId_path_doc == nullptr)
    {
        // cJSON_Delete(json);
        return 4;
    }
    // tag37
    const cJSON *tag37_doc = cJSON_GetObjectItemCaseSensitive(keys_requestId_path_doc, TAG_UUID);
    if (tag37_doc == nullptr)
    {
        return 5;
    }
    std::string uuid_base64url = tag37_doc->valuestring;
    const cJSON *sign_data_doc = cJSON_GetObjectItemCaseSensitive(json, KEY_SIGN_DATA);
    if (sign_data_doc == nullptr)
    {
        return 6;
    }
    std::string sign_data_base64 = sign_data_doc->valuestring;

    const cJSON *data_type_doc = cJSON_GetObjectItemCaseSensitive(json, KEY_DATA_TYPE);
    if (data_type_doc == nullptr)
    {
        return 7;
    }
    int data_type = data_type_doc->valueint;
    if (data_type != KEY_DATA_TYPE_SIGN_TRANSACTION && data_type != KEY_DATA_TYPE_SIGN_TYPED_DATA)
    {
        LOGI("data_type is not supported: %d", data_type);
        return 8;
    }

    uint64_t chain_id = 0; // optional
    const cJSON *chain_id_doc = cJSON_GetObjectItemCaseSensitive(json, KEY_CHAIN_ID);
    if (chain_id_doc != nullptr)
    {
        chain_id = chain_id_doc->valueint;
    }
    if (data_type == KEY_DATA_TYPE_SIGN_TRANSACTION)
    {
        if (chain_id == 0)
        {
            LOGE("chain_id is required for sign transaction");
            return 9;
        }
    }

    const cJSON *address_base64_doc = cJSON_GetObjectItemCaseSensitive(json, KEY_ADDRESS);
    if (address_base64_doc == nullptr)
    {
        return 10;
    }
    std::string address_base64 = address_base64_doc->valuestring;
    size_t hex_address_max_len = 20;
    uint8_t hex_address[hex_address_max_len];
    size_t hex_address_len = decode_base64url(address_base64, hex_address, hex_address_max_len);
    std::string address = "0x" + toHex(hex_address, hex_address_len);

    const cJSON *derivation_path_doc = cJSON_GetObjectItemCaseSensitive(json, KEY_DERIVATION_PATH);
    if (derivation_path_doc == nullptr)
    {
        return 11;
    }
    // tag304
    const cJSON *tag304_doc = cJSON_GetObjectItemCaseSensitive(derivation_path_doc, TAG_CRYPTO_KEYPATH);
    if (tag304_doc == nullptr)
    {
        return 12;
    }
    std::string crypto_keypath = "m/";
    const cJSON *components_path_array_doc = cJSON_GetObjectItemCaseSensitive(tag304_doc, "1");
    if (components_path_array_doc == nullptr)
    {
        return 13;
    }
    const cJSON *components_path_doc = NULL;
    size_t _index = 0;
    cJSON_ArrayForEach(components_path_doc, components_path_array_doc)
    {
        // if index%2 == 0 -> path (uint8_t)
        // if index%2 == 1 -> hardend (bool)
        if (_index % 2 == 0)
        {
            uint8_t _path = components_path_doc->valueint;
            crypto_keypath += std::to_string(_path);
        }
        else
        {
            bool _hardend = components_path_doc->valueint;
            if (_hardend)
            {
                crypto_keypath += "'";
            }
            crypto_keypath += "/";
        }
        _index++;
    }
    LOGI("crypto_keypath: %s", crypto_keypath.c_str());
    request->uuid_base64url = uuid_base64url;
    request->sign_data_base64url = sign_data_base64;
    request->data_type = data_type;
    request->chain_id = chain_id;
    request->derivation_path = crypto_keypath;
    request->address = address;
    return 0;
}

std::string generate_metamask_eth_signature(uint8_t *uuid, size_t uuid_len, uint8_t signature[65])
{
    size_t buf_len = uuid_len + 100;
    uint8_t *buf = new uint8_t[buf_len];

    CborError err;
    CborEncoder encoder, mapEncoder;
    cbor_encoder_init(&encoder, buf, buf_len, 0);
    err = cbor_encoder_create_map(&encoder, &mapEncoder, 2 /* uuid, signature */);
    if (err)
    {
        printf("Error creating map encoder");
        return "";
    }
    /* uuid */
    // key = '1'
    // tag = 37
    int key_uuid = 1;
    int tag_uuid = 37;
    cbor_encode_int(&mapEncoder, key_uuid);
    cbor_encode_tag(&mapEncoder, tag_uuid);
    cbor_encode_byte_string(&mapEncoder, uuid, uuid_len);

    /* signature */
    // key = '2'
    int key_signature = 2;
    cbor_encode_int(&mapEncoder, key_signature);
    cbor_encode_byte_string(&mapEncoder, signature, 65);

    cbor_encoder_close_container_checked(&encoder, &mapEncoder);
    size_t len = cbor_encoder_get_buffer_size(&encoder, buf);

    const char type[] = METAMASK_ETH_SIGNATURE;
    ur::ByteVector vec(buf, buf + len);
    ur::UR ur_signature(type, vec);
    std::string encoded = ur::UREncoder::encode(ur_signature);
    LOGI("encoded_ur: %s", encoded.c_str());
    delete[] buf;
    return encoded;
}