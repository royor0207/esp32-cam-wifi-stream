idf_component_register(SRCS "TCP_Server.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi esp_netif esp_event nvs_flash
                    PRIV_REQUIRES WiFi_AP
                    )
