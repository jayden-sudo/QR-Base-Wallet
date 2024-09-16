#include <qrcode_protocol.h>
#include <bc-ur.h>
#include <cbor.h>
#include <cborjson.h>
#include <ArduinoJson.h>
#include <base64url.h>

int decode_url(const String url, String *type, String *payload)
{
    // UR:ETH-SIGN-REQUEST/OLADTPDAGDBKAABWRDVOHHGEBBNEWKCAWLRPLERSKTAOHDECAOWFLSPKENOSLALRHKISDLAELPBNRODYEMDYLFGMAYMWMYIATSUTIMFHHKETHSJTWTHNCMRKWZHPTBAOEOBZLOADIAFEKSHLLEAEAELARTAXAAAACYAEPKENOSAHTAADDYOEADLECSDWYKCSFNYKAEYKAEWKAEWKAOCYWLDAQZPRAMGHNEVOESHLIOINKSENQZNEGMFTGYFPOSYAZMTIATIMFGVLDWFW
    int index = url.indexOf('/');
    if (index == -1)
    {
        return 1;
    }
    *type = url.substring(0, index);
    (*type).toLowerCase();
    (*type).replace("ur:", "");
    *payload = url.substring(index + 1);
    (*payload).toLowerCase();
    return 0;
}

String encode_url(const String type, const String payload)
{
    String encoded = "ur:" + type + "/" + payload;
    encoded.toUpperCase();
    return encoded;
}

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
    String encoded_payload = encode_minimal(buf, len);

    String encoded = encode_url(METAMASK_CRYPTO_HDKEY, encoded_payload);
    // uint8_t *decoded_payload = nullptr;
    // size_t decoded_payload_len = decode_minimal(encoded_payload, &decoded_payload);
    return encoded;
}

static String cast_cbor_to_json(uint8_t *output_payload, size_t output_payload_len)
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
                String json_string = String(json_result);
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

int decode_metamask_eth_sign_request(String qrcode, MetamaskEthSignRequest *request)
{
    uint8_t *output_payload = nullptr;
    size_t output_payload_len = decode_minimal(qrcode, &output_payload);
    // cbor decode
    String json_string = cast_cbor_to_json(output_payload, output_payload_len);
    delete[] output_payload;
    if (json_string.isEmpty())
    {
        return 1;
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
    JsonDocument doc;
    deserializeJson(doc, json_string);

    JsonDocument keys_requestId_path_doc = doc[String(KEY_REQUEST_ID)];
    if (keys_requestId_path_doc.isNull())
    {
        return 1;
    }
    // tag37
    String uuid_base64url = keys_requestId_path_doc[String(TAG_UUID)];

    String sign_data_base64 = doc[String(KEY_SIGN_DATA)];

    uint32_t data_type = doc[String(KEY_DATA_TYPE)];
    if (data_type != 4)
    {
        return 1;
    }

    uint64_t chain_id = doc[String(KEY_CHAIN_ID)];

    String address_base64 = doc[String(KEY_ADDRESS)];
    size_t hex_address_max_len = 20;
    uint8_t hex_address[hex_address_max_len];
    size_t hex_address_len = decode_base64url(address_base64, hex_address, hex_address_max_len);
    String address = "0x" + toHex(hex_address, hex_address_len);

    JsonDocument derivation_path_doc = doc[String(KEY_DERIVATION_PATH)];
    if (derivation_path_doc.isNull())
    {
        return 1;
    }
    // tag304
    JsonDocument tag304_doc = derivation_path_doc[String(TAG_CRYPTO_KEYPATH)];
    if (tag304_doc.isNull())
    {
        return 1;
    }

    String crypto_keypath = "m/";
    JsonArray path_array = tag304_doc[String("1")].as<JsonArray>();
    if (path_array.size() % 2 != 0)
    {
        return 1;
    }
    for (int i = 0; i < path_array.size(); i += 2)
    {
        uint8_t _path = path_array[i];
        bool _hardend = path_array[i + 1];
        crypto_keypath += String(_path);
        if (_hardend)
        {
            crypto_keypath += "'";
        }
        crypto_keypath += "/";
    }

    request->uuid_base64url = uuid_base64url;
    request->sign_data_base64url = sign_data_base64;
    request->data_type = data_type;
    request->chain_id = chain_id;
    request->derivation_path = crypto_keypath;
    request->address = address;

    return 0;
}

String generate_metamask_eth_signature(uint8_t *uuid, size_t uuid_len, uint8_t signature[65])
{
    size_t buf_len = uuid_len + 100;
    uint8_t *buf = new uint8_t[buf_len];

    CborError err;
    CborEncoder encoder, mapEncoder;
    cbor_encoder_init(&encoder, buf, buf_len, 0);
    err = cbor_encoder_create_map(&encoder, &mapEncoder, 2 /* uuid, signature */);
    if (err)
    {
          Serial.println("Error creating map encoder");
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
    String encoded_payload = encode_minimal(buf, len);
    String encoded = encode_url(METAMASK_ETH_SIGNATURE, encoded_payload);

    delete[] buf;

    return encoded;
}