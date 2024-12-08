#ifndef HOUSE_CONTROL_H
#define HOUSE_CONTROL_H

#include "hue_http.h"

/**
 * @brief house control lights
 */
typedef struct
{
    bool light_1;
    bool light_2;
    bool light_3;
    bool light_4;
    bool light_5;
    bool light_6;
    bool light_7;
    bool light_8;
} house_lights_status_t;

/**
 * @brief house control switches
 */
typedef struct
{
    bool switch_1;
    bool switch_2;
    bool switch_3;
    bool switch_4;
    bool switch_5;
    bool switch_6;
    bool switch_7;
    bool switch_8;
} house_switches_status_t;

esp_err_t house_control_init(void);

void set_house_lights(unsigned short lightState);

uint8_t get_house_switch_statuses();

void set_house_lights_struct(house_lights_status_t *target_light_status);

void update_display_lights_from_house_status(hue_house_status_t *house_status, house_lights_status_t *lights_status);

house_switches_status_t get_house_switch_statuses_struct();

#endif