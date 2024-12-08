#ifndef HUE_HTTP_H
#define HUE_HTTP_H

#define MAX_ROOMS 10

typedef struct
{
    char grouped_light_id[37];
    int grouped_lights_on;
} hue_grouped_light_status_t;

typedef struct
{
    char room_name[15];
    // todo: we may not NEED this assuming we're OK assuming grouped_light_id is 1:1 with a room_id
    //        though it's likely if we need to roll up an individual light to a room we'll need this
    char room_id[37];
    char grouped_light_id[37];
    int grouped_lights_on;
} hue_room_status_t;
typedef struct
{
    hue_room_status_t rooms[MAX_ROOMS];
    int room_count;
} hue_house_status_t;
typedef struct
{
    hue_grouped_light_status_t rooms[MAX_ROOMS];
    int grouped_light_count;
} hue_grouped_lights_status_t;

void hue_get_full_house_status(hue_house_status_t *house_status);

void hue_http_start(void *params);

#endif