idf_component_register(SRCS "esp32p4_lcd.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_ledc esp_lcd_touch_gt911 esp_lcd_jd9365)
