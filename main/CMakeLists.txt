FILE(GLOB_RECURSE app_sources *.cpp)

idf_component_register( SRCS ${app_sources} 
                    INCLUDE_DIRS ../include
                    )
#PRIV_REQUIRES nvs_flash vfs esp_netif mdns esp_http_server json

# idf_component_register(SRCS "station_example_main.c"
#                 INCLUDE_DIRS ".")
