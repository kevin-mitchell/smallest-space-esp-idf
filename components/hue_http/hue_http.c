
#include <stdio.h>
#include <string.h>
#include <cJSON.h>
#include "hue_http.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "hue_data_parsers.h"

#define TAG "HUE_HTTP"

#define MAX_HTTP_RECV_BUFFER 500

typedef struct
{
    char *buffer;
    int index;
} http_data_t;

esp_err_t on_client_data(esp_http_client_event_t *evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
    {
        http_data_t *http_data = evt->user_data;
        http_data->buffer = realloc(http_data->buffer, http_data->index + evt->data_len + 1);
        memcpy(&http_data->buffer[http_data->index], (uint8_t *)evt->data, evt->data_len);
        http_data->index = http_data->index + evt->data_len;
        http_data->buffer[http_data->index] = 0;
    }
    break;

    default:
        break;
    }
    return ESP_OK;
}

void hue_get_full_house_status(hue_house_status_t *house_status)
{
    // get all of the rooms
    // https://192.168.11.10/clip/v2/resource/room
    // get all of the group_lights
    // https://192.168.11.10/clip/v2/resource/grouped_light

    http_data_t http_data = {0};

    esp_http_client_config_t esp_http_client_config = {
        // TODO: make dynamic and fetch at startup
        .url = "https://192.168.11.10/clip/v2/resource/room",
        .method = HTTP_METHOD_GET,
        .event_handler = on_client_data,
        .user_data = &http_data};
    esp_http_client_handle_t client = esp_http_client_init(&esp_http_client_config);
    esp_http_client_set_header(client, "hue-application-key", ".-.");
    esp_http_client_set_header(client, "Contnet-Type", "application/json");
    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        return;
    }

    parse_hue_rooms(http_data.buffer, house_status);

    esp_http_client_cleanup(client);

    if (http_data.buffer != NULL)
    {
        free(http_data.buffer);
    }

    memset(&http_data, 0, sizeof(http_data_t));

    esp_http_client_config_t esp_http_client_config2 = {
        // TODO: make dynamic and fetch at startup
        .url = "https://192.168.11.10/clip/v2/resource/grouped_light",
        .method = HTTP_METHOD_GET,
        .event_handler = on_client_data,
        .user_data = &http_data};
    client = esp_http_client_init(&esp_http_client_config2);
    esp_http_client_set_header(client, "hue-application-key", ".-.");
    esp_http_client_set_header(client, "Contnet-Type", "application/json");
    err = esp_http_client_perform(client);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        return;
    }

    parse_hue_grouped_lights(http_data.buffer, house_status);

    esp_http_client_cleanup(client);

    if (http_data.buffer != NULL)
    {
        free(http_data.buffer);
    }
}

void hue_http_start(void *event_grouped_lights_updates_queue)
{
    char *buffer = malloc(MAX_HTTP_RECV_BUFFER + 1);
    char *json_data = malloc(10000);

    if (buffer == NULL)
    {
        ESP_LOGE(TAG, "Cannot malloc http receive buffer");
        return;
    }

    http_data_t http_data = {0};
    hue_grouped_lights_status_t grouped_lights_status = {0};
    http_data.index = 0;

    esp_http_client_config_t config = {
        .url = "https://192.168.11.10/eventstream/clip/v2",
        .method = HTTP_METHOD_GET,
        .keep_alive_enable = true,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Accept", "text/event-stream");
    esp_http_client_set_header(client, "hue-application-key", ".-.");

    ESP_LOGI(TAG, "Opening Connection...");

    esp_err_t err;
    if ((err = esp_http_client_open(client, 0)) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        free(buffer);
        return;
    }

    ESP_LOGI(TAG, "Open Connection...");

    int content_length = esp_http_client_fetch_headers(client);
    if (content_length > 0)
    {
        ESP_LOGW(TAG, "Unexpected Content-Length: %d", content_length);
    }
    int read_length = -1;

    esp_http_client_set_timeout_ms(client, 50);
    while (true)
    {
        read_length = esp_http_client_read(client, buffer, MAX_HTTP_RECV_BUFFER);
        if (read_length > 0)
        {
            http_data.index += read_length;
            http_data.buffer = realloc(http_data.buffer, http_data.index + 1);
            strncpy(&http_data.buffer[http_data.index - read_length], buffer, read_length);
            http_data.buffer[http_data.index] = '\0';

            char *start = strstr(http_data.buffer, "data: [");

            while (start)
            {
                char *end = strstr(start, "]\n\n");
                if (end)
                {
                    start += 6; // Move past the "data: ["
                    end += 1;   // Move past the "]\n\n"
                    int json_length = end - start;
                    // char *json_data = malloc(json_length + 1);
                    memcpy(json_data, start, json_length);
                    json_data[json_length] = '\0';

                    /**
                     * Do something with the json_data
                     */

                    // check if "grouped_light" is in the json because if not for now we dont' care about it
                    // TODO: this is where if we need to care about individual light updates to roll up to a room
                    //        ourselves we'd need to parse other messages as well
                    if (strstr(json_data, "grouped_light"))
                    {
                        ESP_LOGI(TAG, "Received grouped_light update: %s", json_data);

                        parse_hue_event_update(json_data, &grouped_lights_status);
                        if (grouped_lights_status.grouped_light_count > 0)
                        {
                            ESP_LOGI(TAG, "Sending grouped_lights_status to queue with %d updates...", grouped_lights_status.grouped_light_count);
                            xQueueSend(event_grouped_lights_updates_queue, &grouped_lights_status, pdMS_TO_TICKS(20));
                        }
                    }

                    int remaining_length = http_data.index - (end - http_data.buffer);
                    memmove(http_data.buffer, end, remaining_length);

                    http_data.index = remaining_length;
                    http_data.buffer = realloc(http_data.buffer, http_data.index + 1);
                    http_data.buffer[http_data.index] = '\0';

                    start = strstr(http_data.buffer, "data: [");
                }
                else
                {
                    break;
                }
            }

            // ESP_LOGI(TAG, "Data so far:\n%s", http_data.buffer);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
    ESP_LOGI(TAG, "HTTP Stream reader Status = %d, content_length = %" PRId64,
             esp_http_client_get_status_code(client),
             esp_http_client_get_content_length(client));
    // esp_http_client_close(client);
    // esp_http_client_cleanup(client);
    free(buffer);
}
