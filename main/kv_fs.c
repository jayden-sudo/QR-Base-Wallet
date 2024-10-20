/*********************
 *      INCLUDES
 *********************/
#include "kv_fs.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include "esp_littlefs.h"
#include <unistd.h>
#include <sys/stat.h>

/**********************
 *      VARIABLES
 **********************/
const char *TAG = "KV_FS";
const esp_vfs_littlefs_conf_t conf = {
    .base_path = "/littlefs",
    .partition_label = "littlefs",
    .format_if_mount_failed = true,
    .dont_mount = false,
};

/**********************
 *  STATIC PROTOTYPES
 **********************/
static int mount_littlefs(void);
static int unmount_littlefs(void);

/**********************
 * GLOBAL PROTOTYPES
 **********************/
int kv_save(const char *key, const char *hex, size_t len);
int kv_load(const char *key, char **hex, size_t *len);
int kv_delete(const char *key);

/**********************
 *   STATIC FUNCTIONS
 **********************/
static int mount_littlefs(void)
{
    esp_err_t ret = esp_vfs_littlefs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG, "Failed to find LittleFS partition");
        }
        else
        {
            ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
        }
    }
    return ret;
}

static int unmount_littlefs(void)
{
    esp_err_t ret = esp_vfs_littlefs_unregister(conf.partition_label);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to unmount LittleFS (%s)", esp_err_to_name(ret));
    }
    return ret;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int kv_save(const char *key, const char *hex, size_t len)
{
    if (mount_littlefs() != ESP_OK)
    {
        return ESP_FAIL;
    }

    size_t size = strlen(key) + 15 /* strlen("/littlefs/") */;
    char *path = malloc(size);

    sprintf(path, "/littlefs/%s", key);
    FILE *f = fopen(path, "wb");
    if (f == NULL)
    {
        ESP_LOGE(TAG, "Failed to create file");
        return ESP_FAIL;
    }
    fwrite(hex, sizeof(char), len, f);
    fclose(f);
    free(path);
    unmount_littlefs();
    return ESP_OK;
}

int kv_load(const char *key, char **hex, size_t *len)
{
    if (mount_littlefs() != ESP_OK)
    {
        return ESP_FAIL;
    }

    size_t size = strlen(key) + 15 /* strlen("/littlefs/") */;
    char *path = malloc(size);

    sprintf(path, "/littlefs/%s", key);

    struct stat st;
    if (stat(path, &st) == 0)
    {
        *hex = malloc(st.st_size);
        if (*hex == NULL)
        {
            ESP_LOGE(TAG, "Failed to allocate memory");
            goto done;
        }
        FILE *f = fopen(path, "rb");
        if (f == NULL)
        {
            ESP_LOGE(TAG, "Failed to open file for reading");
            free(*hex);
            *hex = NULL;
            goto done;
        }
        fread(*hex, sizeof(char), st.st_size, f);
        fclose(f);
        *len = st.st_size;
        goto done;
    }
    else
    {
        *len = 0;
        *hex = NULL;
        goto done;
    }

done:
    free(path);
    unmount_littlefs();
    return ESP_OK;
}

int kv_delete(const char *key)
{
    if (mount_littlefs() != ESP_OK)
    {
        return ESP_FAIL;
    }

    size_t size = strlen(key) + 15 /* strlen("/littlefs/") */;
    char *path = malloc(size);

    sprintf(path, "/littlefs/%s", key);
    struct stat st;
    if (stat(path, &st) == 0)
    {
        // Delete it if it exists
        if (unlink(path) != 0)
        {
            ESP_LOGE(TAG, "Failed to delete file 1");
            free(path);
            unmount_littlefs();
            return ESP_FAIL;
        }
        else
        {
            if (stat(path, &st) == 0)
            {
                ESP_LOGE(TAG, "Failed to delete file 2");
                free(path);
                unmount_littlefs();
                return ESP_FAIL;
            }
        }
    }
    free(path);
    unmount_littlefs();
    return ESP_OK;
}