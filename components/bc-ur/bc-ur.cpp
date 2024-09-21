#include "bc-ur.hpp"
#include <algorithm>

#define BYTEWORDS_NUM 256
#define BYTEWORD_LENGTH 4

static const char *bytewords = "ableacidalsoapex...zonezoom";

struct lookup_table
{
    uint8_t index;
    uint8_t word[2];
};

static bool __lookup_table_initialized;
static struct lookup_table __lookup_table[BYTEWORDS_NUM];

static void _lookup_table(void)
{
    int i;

    if (__lookup_table_initialized)
        return;

    for (i = 0; i < BYTEWORDS_NUM; i++)
    {
        __lookup_table[i].index = i;
        __lookup_table[i].word[0] = bytewords[i * BYTEWORD_LENGTH];
        __lookup_table[i].word[1] = bytewords[((i + 1) * BYTEWORD_LENGTH) - 1];
    }
    __lookup_table_initialized = true;
}

void get_minimal_word(char result[2], int index)
{
    _lookup_table();
    result[0] = __lookup_table[index].word[0];
    result[1] = __lookup_table[index].word[1];
}

uint8_t from_minimal_word(uint8_t char1, uint8_t char2)
{
    int i;

    _lookup_table();
    for (i = 0; i < BYTEWORDS_NUM; i++)
    {
        if (__lookup_table[i].word[0] == char1 &&
            __lookup_table[i].word[1] == char2)
            return __lookup_table[i].index;
    }
    return 0;
}

static void calc_crc(uint8_t *source, size_t source_len, uint8_t crc_bytes[4])
{
    uint32_t crc = crc32(0, source, source_len);

    crc_bytes[0] = (crc >> 24) & 0xFF;
    crc_bytes[1] = (crc >> 16) & 0xFF;
    crc_bytes[2] = (crc >> 8) & 0xFF;
    crc_bytes[3] = crc & 0xFF;
}

int add_crc(uint8_t *source, size_t source_len, uint8_t *output, size_t output_len)
{
    uint8_t crc_bytes[4];

    if (output_len < source_len + 4)
        return -EINVAL;

    memcpy(output, source, source_len);
    calc_crc(source, source_len, crc_bytes);
    memcpy(output + source_len, crc_bytes, 4);
    return 0;
}

std::string encode_minimal(uint8_t *source, size_t sourceLen)
{
    size_t len = sourceLen + 4;
    uint8_t *appended = new uint8_t[len];
    int ret = add_crc(source, sourceLen, appended, len);
    if (ret != 0)
    {
        delete[] appended;
        return std::string("");
    }

    size_t encoded_len = len * 2;
    uint8_t *encoded = new uint8_t[encoded_len];
    char buff[2];
    for (size_t i = 0; i < len; i++)
    {
        get_minimal_word(buff, appended[i]);
        encoded[i * 2] = buff[0];
        encoded[i * 2 + 1] = buff[1];
    }

    delete[] appended;

    std::string result = std::string(reinterpret_cast<char *>(encoded), encoded_len);
    delete[] encoded;
    return result;
}

int decode_minimal(
    std::string source,
    uint8_t **output_payload)
{
    // source to lower case
    std::transform(source.begin(), source.end(), source.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });

    size_t sourceLen = source.length();
    if (sourceLen % 2 != 0)
    {
        return 1;
    }
    *output_payload = new uint8_t[sourceLen / 2];
    for (size_t i = 0; i < sourceLen; i += 2)
    {
        int index = from_minimal_word(source[i], source[i + 1]);
        (*output_payload)[i / 2] = index;
    }
    size_t payload_len = (sourceLen / 2) - 4 /* crc32 */;
    // check crc
    uint8_t crc_bytes[4];
    calc_crc(*output_payload, payload_len, crc_bytes);
    uint8_t a, b;
    for (int i = 0; i < 4; i++)
    {
        a = (*output_payload)[payload_len + i];
        b = crc_bytes[i];
        if (a != b)
        {
            delete[] *output_payload;
            return 0;
        }
    }
    return payload_len;
}