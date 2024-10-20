/*********************
 *      INCLUDES
 *********************/
#include "sha256_str.h"
#include <esp_log.h>
#include "mbedtls/md.h"
#include <string.h>

#ifndef MBEDTLS_MD_C
#error "not implemented"
#endif

/*********************
 *      DEFINES
 *********************/
static const char *TAG = "sha256";

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void sha256_str(const char *str, uint8_t output[32]);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void sha256_str(const char *str, uint8_t output[32])
{
	mbedtls_md_context_t ctx;
	const size_t len = strlen(str);
	mbedtls_md_init(&ctx);
	mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
	mbedtls_md_update(&ctx, (unsigned char *)str, len);
	mbedtls_md_finish(&ctx, output);
	mbedtls_md_free(&ctx);
}