#ifndef HUE_DATA_PARSERS_H
#define HUE_DATA_PARSERS_H

void parse_hue_rooms(char *get_room_response, hue_house_status_t *house_status);
void parse_hue_grouped_lights(char *http_get_grouped_lights_response, hue_house_status_t *house_status);
void parse_hue_event_update(char *http_event_data, hue_grouped_lights_status_t *grouped_lights_status);

#endif