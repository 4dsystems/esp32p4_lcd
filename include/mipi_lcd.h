#pragma once

#include "esp_lcd_jd9365.h"
#include "esp_lcd_types.h"
#include "driver/gpio.h"

#ifndef __ESP32P4_4D_MIPI_LCD__
#define __ESP32P4_4D_MIPI_LCD__

#if defined(__cplusplus)
extern "C" {
#endif

// Resolution and bits per pixel for 4D Systems ESP32-P4 MIPI LCD models
#define LCD_BITS_PER_PIXEL      16      // RGB565 over the 16-bit parallel bus

// Pin definitions for 4D Systems' ESP32-P4-MIPI Rev 1.4
#define LCD_BL_GPIO_NUM         GPIO_NUM_22    // backlight control (PWM)
#define LCD_RST_GPIO_NUM        GPIO_NUM_23
#define LCD_BOARD_SDA_GPIO_NUM  GPIO_NUM_7
#define LCD_BOARD_SCL_GPIO_NUM  GPIO_NUM_8
#define LCD_TOUCH_INT_GPIO_NUM  GPIO_NUM_5
#define LCD_TOUCH_RST_GPIO_NUM  GPIO_NUM_4

// Timing parameters are panel-size specific and come from the LCD's own
// datasheet, not this schematic - see the README's 4DLCD-xxxxxx datasheet
// links. Selecting the wrong one will not "mostly work"; it'll be a blank,
// torn, or rolling image, since it's the video timing itself that's wrong.
#if defined(CONFIG_ESP32P4_LCD_70) || defined(CONFIG_ESP32P4_LCD_80) || defined(CONFIG_ESP32P4_LCD_90) || defined(CONFIG_ESP32P4_LCD_101)
// All rectangular MIPI-interface panels (7.0", 8.0", 9.0" and 10.1") share
// the same 800x1280 portrait native resolution, and timing parameters,
// only the physical size/density and init code differ.
#define LCD_DSI_PHY_LDO_ID          3
#define LCD_DSI_PHY_LDO_MV          2500
#define LCD_DSI_LANE_NUM            2
#define LCD_DSI_LANE_BIT_RATE       1200
#define LCD_DPI_CLK_MHZ             60
#define LCD_HSYNC_PULSE_WIDTH       20
#define LCD_HSYNC_BACK_PORCH        20
#define LCD_HSYNC_FRONT_PORCH       40
#define LCD_VSYNC_PULSE_WIDTH       4
#define LCD_VSYNC_BACK_PORCH        12
#define LCD_VSYNC_FRONT_PORCH       30
#define LCD_TOUCH_SWAP_MAX_XY       1
#define LCD_TOUCH_SWAP_XY           1
#define LCD_TOUCH_MIRROR_X          0
#define LCD_TOUCH_MIRROR_Y          1
#elif defined(CONFIG_ESP32P4_LCD_34R) || defined(CONFIG_ESP32P4_LCD_40R)
// All round MIPI-interface panels (3.4" and 4.0") share the same timing
// parameters, only the physical size/density, resolution and init code differ.
#define LCD_DSI_PHY_LDO_ID          3
#define LCD_DSI_PHY_LDO_MV          2500
#define LCD_DSI_LANE_NUM            2
#define LCD_DSI_LANE_BIT_RATE       1200
#define LCD_DPI_CLK_MHZ             60
#define LCD_HSYNC_PULSE_WIDTH       20
#define LCD_HSYNC_BACK_PORCH        20
#define LCD_HSYNC_FRONT_PORCH       40
#define LCD_VSYNC_PULSE_WIDTH       4
#define LCD_VSYNC_BACK_PORCH        12
#define LCD_VSYNC_FRONT_PORCH       24
#define LCD_TOUCH_SWAP_MAX_XY       0
#define LCD_TOUCH_SWAP_XY           0
#define LCD_TOUCH_MIRROR_X          0
#define LCD_TOUCH_MIRROR_Y          0
#else
#error "No valid 4D Systems MIPI LCD model defined"
#endif

#define LCD_BL_PWM_FREQ_HZ       25000    // PWM frequency (25kHz)

#if defined(__cplusplus)
}
#endif

#endif // __ESP32P4_4D_MIPI_LCD__