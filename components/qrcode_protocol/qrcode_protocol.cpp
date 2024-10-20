/*********************
 *      INCLUDES
 *********************/
#include "qrcode_protocol.h"
#include "cbor.h"
#include "cborjson.h"
#include "cJSON.h"
#include "base64url.h"
#include <algorithm>
#include "esp_log.h"
#include <string>
#include "wallet.h"
#include "bc-ur.hpp"
#include <Bitcoin.h>

/*********************
 *      DEFINES
 *********************/
#define TAG "QRCODE_PROTOCOL"
#define LOGI(...) ESP_LOGI(TAG, __VA_ARGS__)
#define LOGE(...) ESP_LOGE(TAG, __VA_ARGS__)

#define KEY_REQUEST_ID "1"
#define KEY_SIGN_DATA "2"
#define KEY_DATA_TYPE "3"
#define KEY_CHAIN_ID "4"
#define KEY_DERIVATION_PATH "5"
#define KEY_ADDRESS "6"
#define TAG_UUID "tag37"
#define TAG_CRYPTO_KEYPATH "tag304"

/**********************
 *  STATIC VARIABLES
 **********************/
static std::unordered_map<int, std::shared_ptr<ur::UR>> shared_ur_ptr_map;
static std::unordered_map<int, std::shared_ptr<ur::URDecoder>> shared_ur_decoder_ptr_map;

extern "C"
{
    /**********************
     *  STATIC PROTOTYPES
     **********************/
    static std::string cast_cbor_to_json(const uint8_t *output_payload, size_t output_payload_len);

    /**********************
     * GLOBAL PROTOTYPES
     **********************/
    void free_metamask_sign_request(MetamaskSignRequest *request);
    void generate_metamask_crypto_hdkey(Wallet *wallet, char **output);
    void generate_metamask_eth_signature(uint8_t *uuid_str, uint8_t signature[65], char **output);
    int decode_metamask_sign_request(UR ur, MetamaskSignRequest *request);

    URType ur_type(const char *url);
    void qrcode_protocol_bc_ur_init(qrcode_protocol_bc_ur_data_t *data);
    void qrcode_protocol_bc_ur_free(qrcode_protocol_bc_ur_data_t *data);
    bool qrcode_protocol_bc_ur_receive(qrcode_protocol_bc_ur_data_t *data, const char *receiveStr);
    bool qrcode_protocol_bc_ur_is_complete(qrcode_protocol_bc_ur_data_t *data);
    bool qrcode_protocol_bc_ur_is_success(qrcode_protocol_bc_ur_data_t *data);
    const char *qrcode_protocol_bc_ur_type(qrcode_protocol_bc_ur_data_t *data);

    /**********************
     *   STATIC FUNCTIONS
     **********************/
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

    /**********************
     *   GLOBAL FUNCTIONS
     **********************/
    void free_metamask_sign_request(MetamaskSignRequest *request)
    {
        if (request == nullptr)
        {
            return;
        }
        if (request->uuid_base64url != nullptr)
        {
            free(request->uuid_base64url);
            request->uuid_base64url = nullptr;
        }
        if (request->sign_data_base64url != nullptr)
        {
            free(request->sign_data_base64url);
            request->sign_data_base64url = nullptr;
        }
        if (request->derivation_path != nullptr)
        {
            free(request->derivation_path);
            request->derivation_path = nullptr;
        }
        if (request->address != nullptr)
        {
            free(request->address);
            request->address = nullptr;
        }
        free(request);
    }
    void generate_metamask_crypto_hdkey(Wallet *wallet, char **output)
    {
        HDPrivateKey *_wallet = (HDPrivateKey *)wallet;
        PublicKeyFingerprint publicKeyFingerprint;
        wallet_eth_key_fingerprint(_wallet, &publicKeyFingerprint);

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
            *output = (char *)malloc(1);
            (*output)[0] = '\0';
            return;
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
        *output = (char *)malloc(encoded.length() + 1);
        strcpy(*output, encoded.c_str());
    }
    void generate_metamask_eth_signature(uint8_t *uuid_str, uint8_t signature[65], char **output)
    {
        size_t uuid_len = strlen((char *)uuid_str);
        size_t buf_len = uuid_len + 100;
        uint8_t *buf = new uint8_t[buf_len];

        CborError err;
        CborEncoder encoder, mapEncoder;
        cbor_encoder_init(&encoder, buf, buf_len, 0);
        err = cbor_encoder_create_map(&encoder, &mapEncoder, 2 /* uuid, signature */);
        if (err)
        {
            printf("Error creating map encoder");
            *output = (char *)malloc(1);
            (*output)[0] = '\0';
            return;
        }
        /* uuid */
        // key = '1'
        // tag = 37
        int key_uuid = 1;
        int tag_uuid = 37;
        cbor_encode_int(&mapEncoder, key_uuid);
        cbor_encode_tag(&mapEncoder, tag_uuid);
        cbor_encode_byte_string(&mapEncoder, (uint8_t *)uuid_str, uuid_len);

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
        *output = (char *)malloc(encoded.length() + 1);
        strcpy(*output, encoded.c_str());
    }
    int decode_metamask_sign_request(UR _ur, MetamaskSignRequest *request)
    {
        ur::UR *ur = (ur::UR *)_ur;
        // eth-sign-request
        if (strcmp(ur->type().c_str(), METAMASK_ETH_SIGN_REQUEST) != 0)
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
        if (
            data_type != KEY_DATA_TYPE_SIGN_TYPED_TRANSACTION &&
            data_type != KEY_DATA_TYPE_SIGN_PERSONAL_MESSAGE &&
            data_type != KEY_DATA_TYPE_SIGN_TYPED_DATA)
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
        if (data_type == KEY_DATA_TYPE_SIGN_TYPED_TRANSACTION)
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
        size_t address_max_len = strlen(address_base64.c_str());
        uint8_t *hex_address = (uint8_t *)malloc(address_max_len);
        decode_base64url(address_base64.c_str(), hex_address, address_max_len);
        std::string address = "0x" + toHex(hex_address, 20);
        free(hex_address);

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
        char *uuid_base64url_cstr = (char *)malloc(uuid_base64url.length() + 1);
        strcpy(uuid_base64url_cstr, uuid_base64url.c_str());
        request->uuid_base64url = uuid_base64url_cstr;

        ESP_LOGI(TAG, "sign_data_base64: %s", sign_data_base64.c_str());
        char *sign_data_base64_cstr = (char *)malloc(sign_data_base64.length() + 1);
        strcpy(sign_data_base64_cstr, sign_data_base64.c_str());
        ESP_LOGI(TAG, "sign_data_base64_cstr: %s", sign_data_base64_cstr);
        request->sign_data_base64url = sign_data_base64_cstr;
        request->data_type = data_type;
        request->chain_id = chain_id;
        char *derivation_path_cstr = (char *)malloc(crypto_keypath.length() + 1);
        strcpy(derivation_path_cstr, crypto_keypath.c_str());
        request->derivation_path = derivation_path_cstr;
        char *address_cstr = (char *)malloc(address.length() + 1);
        strcpy(address_cstr, address.c_str());
        request->address = address_cstr;
        return 0;
    }
    URType ur_type(const char *url)
    {
        std::string _url = std::string(url);
        size_t _first_pos = _url.find('/');
        if (_first_pos != std::string::npos)
        {
            size_t _second_pos = _url.find('/', _first_pos + 1);
            if (_second_pos != std::string::npos)
                return URType::MultiPart;
            else
                return URType::SinglePart;
        }
        return URType::Invalid;
    }
    void qrcode_protocol_bc_ur_init(qrcode_protocol_bc_ur_data_t *data)
    {
        if (data == nullptr)
        {
            ESP_LOGE(TAG, "qrcode_protocol_bc_ur_data_t *data is nullptr");
            return;
        }
        data->ur_type = URType::Invalid;
        data->ur = nullptr;
        data->ur_decoder = nullptr;
    }
    void qrcode_protocol_bc_ur_free(qrcode_protocol_bc_ur_data_t *data)
    {
        if (data != nullptr)
        {
            if (data->ur != nullptr)
            {
                int ptr = (int)data->ur;
                if (shared_ur_ptr_map.find(ptr) != shared_ur_ptr_map.end())
                {
                    shared_ur_ptr_map.erase(ptr);
                }
            }
            if (data->ur_decoder != nullptr)
            {
                int ptr = (int)data->ur_decoder;
                if (shared_ur_decoder_ptr_map.find(ptr) != shared_ur_decoder_ptr_map.end())
                {
                    shared_ur_decoder_ptr_map.erase(ptr);
                }
            }
        }
    }
    bool qrcode_protocol_bc_ur_receive(qrcode_protocol_bc_ur_data_t *data, const char *receiveStr)
    {
        if (data == nullptr)
        {
            ESP_LOGE(TAG, "qrcode_protocol_bc_ur_data_t *data is nullptr");
            return false;
        }
        if (data->ur_type == URType::Invalid)
        {
            URType _urtype_internal = ur_type(receiveStr);
            if (_urtype_internal == URType::SinglePart)
            {
                data->ur_type = URType::SinglePart;
            }
            else if (_urtype_internal == URType::MultiPart)
            {
                data->ur_type = URType::MultiPart;
                auto ur_decoder = std::make_shared<ur::URDecoder>();
                int ptr = (int)ur_decoder.get();
                shared_ur_decoder_ptr_map[ptr] = ur_decoder;
                data->ur_decoder = (URDecoder)ptr;
            }
            else
            {
                return false;
            }
        }
        URType urtype_internal = ur_type(receiveStr);
        if (urtype_internal != data->ur_type)
        {
            // change ur type
            // #TODO
            return false;
        }
        if (data->ur_type == URType::SinglePart)
        {
            ur::UR decoded_ur = ur::URDecoder::decode(receiveStr);
            if (decoded_ur.is_valid())
            {
                std::shared_ptr<ur::UR> _ur = std::make_shared<ur::UR>(decoded_ur);
                auto _ur_ptr = _ur.get();
                int ptr = (int)_ur_ptr;
                shared_ur_ptr_map[ptr] = _ur;
                data->ur = (UR)_ur_ptr;
                return true;
            }
            return false;
        }
        else if (data->ur_type == URType::MultiPart)
        {
            auto ur_decoder = (ur::URDecoder *)data->ur_decoder;
            if (ur_decoder->receive_part(receiveStr))
            {
                if (ur_decoder->is_complete())
                {
                    if (ur_decoder->is_success())
                    {
                        std::shared_ptr<ur::UR> _ur = std::make_shared<ur::UR>(ur_decoder->result_ur());
                        auto _ur_ptr = _ur.get();
                        int ptr = (int)_ur_ptr;
                        shared_ur_ptr_map[ptr] = _ur;
                        data->ur = (UR)_ur_ptr;
                    }
                }
                return true;
            }
            return false;
        }
        return false;
    }
    bool qrcode_protocol_bc_ur_is_complete(qrcode_protocol_bc_ur_data_t *data)
    {
        if (data == nullptr)
        {
            ESP_LOGE(TAG, "qrcode_protocol_bc_ur_data_t *data is nullptr");
            return false;
        }
        if (data->ur_type == URType::SinglePart)
        {
            return data->ur != nullptr;
        }
        else if (data->ur_type == URType::MultiPart)
        {
            if (data->ur_decoder != nullptr)
            {
                auto ur_decoder = (ur::URDecoder *)data->ur_decoder;
                return ur_decoder->is_complete();
            }
        }
        return false;
    }
    bool qrcode_protocol_bc_ur_is_success(qrcode_protocol_bc_ur_data_t *data)
    {
        bool is_complete = qrcode_protocol_bc_ur_is_complete(data);
        if (is_complete)
        {
            if (data->ur_type == URType::SinglePart)
            {
                return true;
            }
            else if (data->ur_type == URType::MultiPart)
            {
                auto ur_decoder = (ur::URDecoder *)data->ur_decoder;
                return ur_decoder->is_success();
            }
        }
        return false;
    }
    const char *qrcode_protocol_bc_ur_type(qrcode_protocol_bc_ur_data_t *data)
    {
        bool is_success = qrcode_protocol_bc_ur_is_success(data);
        if (!is_success)
        {
            return NULL;
        }
        ur::UR *ur = (ur::UR *)data->ur;
        return ur->type().c_str();
    }
}
