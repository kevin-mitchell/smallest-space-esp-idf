#include <string.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "TCA9535.h"
#include "house_control.h"

/**
 * TODO:
 * 1. Should ideally have some sort of debounce function or state management to tell when the switch state changes
 * 2. Along with above consider method for sending a message when something changes vs requiring polling
 */

static gpio_num_t i2c_gpio_sda = 4;
static gpio_num_t i2c_gpio_scl = 5;

esp_err_t house_control_init(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = i2c_gpio_sda,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = i2c_gpio_scl,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };

    // Init i2c driver
    esp_err_t ret = TCA9535Init(&conf);

    if (ret == ESP_OK)
    {
        ESP_LOGI(I2C_LOG_TAG, "Initialization done");
    }
    else
    {
        ESP_LOGI(I2C_LOG_TAG, "Initialization failed");
    }

    // Configure TCA9535's PORT0 as input port
    TCA9535WriteSingleRegister(TCA9535_CONFIG_REG0, 0xFF);

    // Configure TCA9535's PORT1 as output port.. I think?
    TCA9535WriteSingleRegister(TCA9535_CONFIG_REG1, 0x00);

    return ret;
}

void set_house_lights(unsigned short lightState)
{
    TCA9535WriteSingleRegister(TCA9535_OUTPUT_REG1, lightState);
}

uint8_t get_house_switch_statuses()
{
    uint8_t reg_data = TCA9535ReadSingleRegister(TCA9535_INPUT_REG0);

    return reg_data;
}

void set_house_lights_struct(house_lights_status_t *target_light_status)
{
    unsigned short lightState = 0;

    lightState |= (!target_light_status->light_1) << 0;
    lightState |= (!target_light_status->light_2) << 1;
    lightState |= (!target_light_status->light_3) << 2;
    lightState |= (!target_light_status->light_4) << 3;
    lightState |= (!target_light_status->light_5) << 4;
    lightState |= (!target_light_status->light_6) << 5;
    lightState |= (!target_light_status->light_7) << 6;
    lightState |= (!target_light_status->light_8) << 7;

    set_house_lights(lightState);
}

house_switches_status_t get_house_switch_statuses_struct()
{
    house_switches_status_t switches_status;
    uint8_t reg_data = get_house_switch_statuses();

    // Note: this is backwards / being flipped because the switches are active low
    switches_status.switch_1 = !(reg_data & 0x01);
    switches_status.switch_2 = !(reg_data & 0x02);
    switches_status.switch_3 = !(reg_data & 0x04);
    switches_status.switch_4 = !(reg_data & 0x08);
    switches_status.switch_5 = !(reg_data & 0x10);
    switches_status.switch_6 = !(reg_data & 0x20);
    switches_status.switch_7 = !(reg_data & 0x40);
    switches_status.switch_8 = !(reg_data & 0x80);

    return switches_status;
}

void update_display_lights_from_house_status(hue_house_status_t *house_status, house_lights_status_t *lights_status)
{
    lights_status->light_1 = false;
    lights_status->light_2 = false;
    lights_status->light_3 = false;
    lights_status->light_4 = false;
    lights_status->light_5 = false;
    lights_status->light_6 = false;
    lights_status->light_7 = false;
    lights_status->light_8 = false;

    /**
     * I need to have some sort of lookup that matches a case insensitive room name to a light number,
     * so for example "livingroom" should map to lights_status->light_1, "kitchen" to lights_status->light_2, etc.
     */
    // Define a mapping from room names to light numbers
    struct room_light_mapping
    {
        const char *room_name;
        int light_number;
    };

    struct room_light_mapping room_light_map[] = {
        {"Living room", 0},
        {"Kitchen", 1},
        {"bedroom", 2},
        {"bathroom", 3},
        {"diningroom", 4},
        {"garage", 5},
        {"office", 6},
        {"hallway", 7},
    };

    // Iterate through each room in the house status
    for (int i = 0; i < house_status->room_count; i++)
    {
        for (int j = 0; j < sizeof(room_light_map) / sizeof(room_light_map[0]); j++)
        {
            ESP_LOGI("UPDATE", "Comparing %s to %s", house_status->rooms[i].room_name, room_light_map[j].room_name);
            ESP_LOGI("UPDATE", "Result: %d", (strcasecmp(house_status->rooms[i].room_name, room_light_map[j].room_name) == 0));
            if (strcasecmp(house_status->rooms[i].room_name, room_light_map[j].room_name) == 0)
            {
                // Set the corresponding light status
                switch (room_light_map[j].light_number)
                {
                case 0:
                    lights_status->light_1 = house_status->rooms[i].grouped_lights_on;
                    break;
                case 1:
                    lights_status->light_2 = house_status->rooms[i].grouped_lights_on;
                    break;
                case 2:
                    lights_status->light_3 = house_status->rooms[i].grouped_lights_on;
                    break;
                case 3:
                    lights_status->light_4 = house_status->rooms[i].grouped_lights_on;
                    break;
                case 4:
                    lights_status->light_5 = house_status->rooms[i].grouped_lights_on;
                    break;
                case 5:
                    lights_status->light_6 = house_status->rooms[i].grouped_lights_on;
                    break;
                case 6:
                    lights_status->light_7 = house_status->rooms[i].grouped_lights_on;
                    break;
                case 7:
                    lights_status->light_8 = house_status->rooms[i].grouped_lights_on;
                    break;
                default:
                    break;
                }
                break;
            }
        }
    }
}