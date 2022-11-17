FILE(GLOB_RECURSE app_sources *.cpp)

idf_component_register( SRCS ${app_sources} 
                    INCLUDE_DIRS ../include
                    PRIV_REQUIRES vfs nvs_flash esp_netif mdns esp_http_server json led_strip dht
                    )
