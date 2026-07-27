#include "app_config.h"

#include <string.h>

#define APP_CONFIG_MAGIC        0x4D455452UL
#define APP_CONFIG_VERSION      1UL
#define APP_CONFIG_FLASH_ADDR   0x080E0000UL
#define APP_CONFIG_FLASH_SECTOR FLASH_SECTOR_11

static AppConfig g_app_config;

static uint32_t AppConfig_Crc32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFUL;
    uint8_t bit;

    while (len-- > 0U) {
        crc ^= *data++;
        for (bit = 0U; bit < 8U; bit++) {
            if ((crc & 1UL) != 0UL) {
                crc = (crc >> 1U) ^ 0xEDB88320UL;
            } else {
                crc >>= 1U;
            }
        }
    }

    return ~crc;
}

static uint32_t AppConfig_CalcConfigCrc(const AppConfig *config)
{
    AppConfig temp;

    memcpy(&temp, config, sizeof(temp));
    temp.crc32 = 0UL;
    return AppConfig_Crc32((const uint8_t *)&temp, sizeof(temp));
}

void AppConfig_LoadDefaults(void)
{
    uint8_t i;

    memset(&g_app_config, 0, sizeof(g_app_config));
    g_app_config.magic = APP_CONFIG_MAGIC;
    g_app_config.version = APP_CONFIG_VERSION;

    g_app_config.d0_temp.target_temp = 37.0f;
    g_app_config.d0_temp.kp = 10.0f;
    g_app_config.d0_temp.ki = 0.5f;
    g_app_config.d0_temp.kd = 1.0f;

    g_app_config.d1_temp.target_temp = 30.0f;
    g_app_config.d1_temp.kp = 10.0f;
    g_app_config.d1_temp.ki = 0.5f;
    g_app_config.d1_temp.kd = 1.0f;

    g_app_config.co2.target_ppm = 50000UL;
    g_app_config.co2.kp = 1.0f;
    g_app_config.co2.ki = 0.05f;
    g_app_config.co2.control_enabled = 0U;

    for (i = 0U; i < APP_CONFIG_MOTOR_COUNT; i++) {
        g_app_config.motor[i].direction = 1U;
        g_app_config.motor[i].subdivision = 64U;
        g_app_config.motor[i].frequency_hz = 9000UL;
        g_app_config.motor[i].flow_k = 1.0f;
        g_app_config.motor[i].flow_b = 0.0f;
    }

    g_app_config.crc32 = AppConfig_CalcConfigCrc(&g_app_config);
}

void AppConfig_Init(void)
{
    const AppConfig *stored = (const AppConfig *)APP_CONFIG_FLASH_ADDR;

    if (stored->magic == APP_CONFIG_MAGIC &&
        stored->version == APP_CONFIG_VERSION &&
        stored->crc32 == AppConfig_CalcConfigCrc(stored)) {
        memcpy(&g_app_config, stored, sizeof(g_app_config));
    } else {
        AppConfig_LoadDefaults();
    }
}

HAL_StatusTypeDef AppConfig_Save(void)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef erase;
    uint32_t sector_error = 0U;
    uint32_t addr = APP_CONFIG_FLASH_ADDR;
    uint32_t *src = (uint32_t *)&g_app_config;
    uint32_t words = (sizeof(g_app_config) + 3U) / 4U;
    uint32_t i;

    g_app_config.magic = APP_CONFIG_MAGIC;
    g_app_config.version = APP_CONFIG_VERSION;
    g_app_config.crc32 = AppConfig_CalcConfigCrc(&g_app_config);

    HAL_FLASH_Unlock();

    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    erase.Sector = APP_CONFIG_FLASH_SECTOR;
    erase.NbSectors = 1U;

    status = HAL_FLASHEx_Erase(&erase, &sector_error);
    if (status == HAL_OK) {
        for (i = 0U; i < words; i++) {
            status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, src[i]);
            if (status != HAL_OK) {
                break;
            }
            addr += 4U;
        }
    }

    HAL_FLASH_Lock();
    return status;
}

const AppConfig *AppConfig_Get(void)
{
    return &g_app_config;
}

AppConfig *AppConfig_Mutable(void)
{
    return &g_app_config;
}
