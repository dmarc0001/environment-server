FILE(GLOB_RECURSE app_sources *.cpp)

# spiffs_create_partition_image( measure ../www FLASH_IN_PROJECT )

idf_component_register( SRCS ${app_sources} 
                    INCLUDE_DIRS ../include
                    PRIV_REQUIRES esp32_wifi_manager nvs_flash json led_strip dht ds18x20 spiffs 
                    )
