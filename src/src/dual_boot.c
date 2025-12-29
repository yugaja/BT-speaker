#include "dual_boot.h"

/**
 * @brief Update OTA data partition with active and next firmware
 * @param active_partition Currently active partition (0=OTA_0, 1=OTA_1)
 * @param next_partition Next partition to boot
 */
void update_otadata(uint8_t active_partition, uint8_t next_partition) {
    const esp_partition_t* otadata = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA,
        ESP_PARTITION_SUBTYPE_DATA_OTA,
        NULL
    );

    if (otadata) {
        ota_data_t ota_data;
        ota_data.active_partition = active_partition;
        ota_data.next_partition = next_partition;
        memset(ota_data.reserved, 0, sizeof(ota_data.reserved));

        esp_err_t err = esp_partition_write(otadata, 0, &ota_data, sizeof(ota_data));
        if (err != ESP_OK) {
            ESP_LOGE("OTA", "Failed to write otadata: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI("OTA", "otadata successfully updated");
        }
    } else {
        ESP_LOGE("OTA", "otadata partition not found");
    }
}

/**
 * @brief Reboot into BT firmware
 */
void reboot_to_bt(void) {
    const esp_partition_t* bt_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP,
        ESP_PARTITION_SUBTYPE_APP_OTA_1,
        NULL
    );

    if (bt_partition) {
        update_otadata(1, 0);
        esp_ota_set_boot_partition(bt_partition);
        esp_restart();
    } else {
        ESP_LOGE("OTA", "BT partition not found");
    }
}

/**
 * @brief Reboot into WiFi firmware
 */
void reboot_to_wifi(void) {
    const esp_partition_t* wifi_partition = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP,
        ESP_PARTITION_SUBTYPE_APP_OTA_0,
        NULL
    );

    if (wifi_partition) {
        update_otadata(0, 1);
        esp_ota_set_boot_partition(wifi_partition);
        esp_restart();
    } else {
        ESP_LOGE("OTA", "WiFi partition not found");
    }
}
