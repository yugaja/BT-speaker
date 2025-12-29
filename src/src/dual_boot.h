#ifndef _DUAL_BOOT_H
#define _DUAL_BOOT_H

#include "Arduino.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_partition.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Structure stored in otadata partition
 */
typedef struct {
    uint8_t active_partition;  /**< 0 for ota_0, 1 for ota_1 */
    uint8_t next_partition;    /**< 0 for ota_0, 1 for ota_1 */
    uint8_t reserved[2];       /**< Reserved bytes */
} ota_data_t;

/**
 * @brief Reboot device into BT firmware (OTA_1)
 */
void reboot_to_bt(void);

/**
 * @brief Reboot device into WiFi firmware (OTA_0)
 */
void reboot_to_wifi(void);

#ifdef __cplusplus
}
#endif

#endif /* _DUAL_BOOT_H */
