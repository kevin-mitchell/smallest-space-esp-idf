#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "house_control.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "wifi_connect.h"
#include "hue_http.h"

// Enabled / Green LED
#define ENABLED_PIN 6

// Disabled / Red LED
#define DISABLED_PIN 7

void print_memory()
{
    ESP_LOGI("memory", "stack %d, total ram %d, internal memory %d, external memory %d\n",
             uxTaskGetStackHighWaterMark(NULL), heap_caps_get_free_size(MALLOC_CAP_8BIT),
             heap_caps_get_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}

void update_lights(void *light_display_queue)
{
    while (true)
    {
        house_lights_status_t lights_status;
        xQueueReceive(light_display_queue, &lights_status, portMAX_DELAY);
        set_house_lights_struct(&lights_status);
    }
}

void update_lights_from_event(void *event_grouped_lights_updates_queue)
{
    while (true)
    {
        hue_grouped_lights_status_t grouped_lights_status;
        xQueueReceive(event_grouped_lights_updates_queue, &grouped_lights_status, portMAX_DELAY);
        ESP_LOGI("UPDATE", "Received grouped_lights_status from queue with %d updates...", grouped_lights_status.grouped_light_count);
        // set_house_lights_struct(&lights_status);
    }
}

void check_for_switch_updates(void *params)
{
    /**
     * HOUSE v0.0.1 pin mapping
     * 0 - 1 - SW7
     * 1 - 2 - SW8
     * 2 - 3 - SW9
     * 3 - 4 - SW4
     * 4 - 5 - SW5
     * 5 - 6 - SW6
     * 6 - 7 - SW1
     * 7 - 8 - SW2
     *
     * 10 - 1 - D7
     * 11 - 2 - D8
     * 12 - 3 - D5
     * 13 - 4 - D6
     * 14 - 5 - D3
     * 15 - 6 - D4
     * 16 - 7 - D1
     * 17 - 8 - D2
     */

    while (true)
    {
        house_switches_status_t switches_status = get_house_switch_statuses_struct();

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void app_main(void)
{
    QueueHandle_t light_display_queue = xQueueCreate(5, sizeof(house_lights_status_t));
    QueueHandle_t event_grouped_lights_updates_queue = xQueueCreate(10, sizeof(hue_grouped_lights_status_t));
    house_lights_status_t lights_status = {0};

    print_memory();
    // TODO: if I make this bigger than 10 or 12 rooms there is a stack overflow?
    //       but the size of this is only 1004 bytes apparently?
    hue_house_status_t *house_status;
    house_status = malloc(sizeof(hue_house_status_t));
    if (house_status == NULL)
    {
        ESP_LOGE("ERROR", "Failed to allocate memory for house_status");
        return;
    }
    memset(house_status, 0, sizeof(hue_house_status_t));
    house_status->room_count = 0;

    print_memory();

    nvs_flash_init();

    gpio_reset_pin(DISABLED_PIN);
    gpio_reset_pin(ENABLED_PIN);
    gpio_set_direction(ENABLED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(DISABLED_PIN, GPIO_MODE_OUTPUT);

    house_control_init();

    set_house_lights(0xFF);

    wifi_connect_init();
    ESP_ERROR_CHECK(wifi_connect_sta("Michigan", "", 10000));

    print_memory();

    hue_get_full_house_status(house_status);

    print_memory();

    // print out the house_status
    for (int i = 0; i < house_status->room_count; i++)
    {
        ESP_LOGI("MAIN", "Room %d: %s, %s, %s", i, house_status->rooms[i].room_name, house_status->rooms[i].room_id, house_status->rooms[i].grouped_light_id);
    }

    update_display_lights_from_house_status(house_status, &lights_status);
    ESP_LOGI("MAIN", "lights_status: %d %d %d %d %d %d %d %d", lights_status.light_1, lights_status.light_2, lights_status.light_3, lights_status.light_4, lights_status.light_5, lights_status.light_6, lights_status.light_7, lights_status.light_8);

    ESP_LOGI("MAIN", "We are at the end...");

    xTaskCreate(&hue_http_start, "hue_http_start", 4096, event_grouped_lights_updates_queue, 5, NULL);
    xTaskCreate(&update_lights, "update_lights", 2024, light_display_queue, 5, NULL);
    xTaskCreate(&update_lights_from_event, "update_lights_from_event", 4024, event_grouped_lights_updates_queue, 5, NULL);
    xTaskCreate(&check_for_switch_updates, "check_for_switch_updates", 2024, NULL, 5, NULL);

    xQueueSend(light_display_queue, &lights_status, pdMS_TO_TICKS(20));

    free(house_status);
    house_status = NULL;

    // wifi_disconnect();
}
