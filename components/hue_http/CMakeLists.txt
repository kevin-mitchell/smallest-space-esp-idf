idf_component_register(
    SRCS 
        "hue_http.c"
        "hue_data_parsers.c"
    INCLUDE_DIRS 
        "."
    REQUIRES
        "esp_netif"
        "esp_wifi"
        "log"
        "esp_http_client"
        "json"
)
