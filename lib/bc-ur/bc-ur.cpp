#include "bc-ur.h"
#include <crc32.h>
#include <errno.h>

#define BYTEWORDS_NUM 256
#define BYTEWORD_LENGTH 4

static const char *bytewords = "ableacidalsoapexaquaarchatomauntawayaxisbackbaldbarnbeltbetabiasbluebodybragbrewbulbbuzzcalmcashcatschefcityclawcodecolacookcostcruxcurlcuspcyandarkdatadaysdelidicedietdoordowndrawdropdrumdulldutyeacheasyechoedgeepicevenexamexiteyesfactfairfernfigsfilmfishfizzflapflewfluxfoxyfreefrogfuelfundgalagamegeargemsgiftgirlglowgoodgraygrimgurugushgyrohalfhanghardhawkheathelphighhillholyhopehornhutsicedideaidleinchinkyintoirisironitemjadejazzjoinjoltjowljudojugsjumpjunkjurykeepkenokeptkeyskickkilnkingkitekiwiknoblamblavalazyleaflegsliarlimplionlistlogoloudloveluaulucklungmainmanymathmazememomenumeowmildmintmissmonknailnavyneednewsnextnoonnotenumbobeyoboeomitonyxopenovalowlspaidpartpeckplaypluspoempoolposepuffpumapurrquadquizraceramprealredorichroadrockroofrubyruinrunsrustsafesagascarsetssilkskewslotsoapsolosongstubsurfswantacotasktaxitenttiedtimetinytoiltombtoystriptunatwinuglyundouniturgeuservastveryvetovialvibeviewvisavoidvowswallwandwarmwaspwavewaxywebswhatwhenwhizwolfworkyankyawnyellyogayurtzapszerozestzinczonezoom";

void get_word(char result[BYTEWORD_LENGTH], int index)
{
    strncpy(result, bytewords + index * BYTEWORD_LENGTH, BYTEWORD_LENGTH);
}

void get_minimal_word(char result[2], int index)
{
    char word[BYTEWORD_LENGTH];
    get_word(word, index);
    result[0] = word[0];
    result[1] = word[BYTEWORD_LENGTH - 1];
}

int add_crc(uint8_t *source, size_t sourceLen, uint8_t *output, size_t outputLen)
{
    if (outputLen < sourceLen + 4)
    {
        return EINVAL;
    }

    uint32_t crc = crc32(0, source, sourceLen);
    memcpy(output, source, sourceLen);
    // padding crc to 4 bytes
    uint8_t crc_bytes[4];
    crc_bytes[0] = (crc >> 24) & 0xFF;
    crc_bytes[1] = (crc >> 16) & 0xFF;
    crc_bytes[2] = (crc >> 8) & 0xFF;
    crc_bytes[3] = crc & 0xFF;

    memcpy(output + sourceLen, crc_bytes, 4);
    return 0;
}

String encode_minimal(uint8_t *source, size_t sourceLen)
{
    uint8_t len = sourceLen + 4;
    uint8_t *appended = new uint8_t[len];
    int ret = add_crc(source, sourceLen, appended, len);
    if (ret != 0)
    {
        return String("");
    }
    uint8_t encoded_len = len * 2 + 1;
    uint8_t *encoded = new uint8_t[encoded_len];
    char buff[2];
    for (uint8_t i = 0; i < len; i++)
    {
        get_minimal_word(buff, appended[i]);
        encoded[i * 2] = buff[0];
        encoded[i * 2 + 1] = buff[1];
    }
    encoded[encoded_len - 1] = '\0';
    String result = String((char *)encoded);
    delete[] appended;
    delete[] encoded;
    return result;
}