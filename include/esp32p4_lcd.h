
/*
 * 4D Systems Pty Ltd
 * www.4dsystems.com.au
 *
 * SPDX-FileCopyrightText: 
 *   - 4D Systems Pty Ltd
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#include "esp_check.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

#include "driver/i2c_master.h"

#include "mipi_lcd.h"

#if !defined(CONFIG_SPIRAM)
#error "This board requires PSRAM. Enable CONFIG_SPIRAM."
#endif

#if defined(CONFIG_ESP32P4_LCD_DOUBLE_BUFFER)
#define LCD_FB_COUNT    2
#else
#define LCD_FB_COUNT    1
#endif

#if defined(CONFIG_ESP32P4_LCD_I2C0)
#define LCD_I2C_NUM         I2C_NUM_0
#define LCD_I2C_CLK_SRC     I2C_CLK_SRC_DEFAULT
#elif defined(CONFIG_ESP32P4_LCD_I2C1)
#define LCD_I2C_NUM         I2C_NUM_1
#define LCD_I2C_CLK_SRC     I2C_CLK_SRC_DEFAULT
#else // CONFIG_ESP32P4_LCD_I2C0_LP
#define LCD_I2C_NUM         LP_I2C_NUM_0
#define LCD_I2C_CLK_SRC     LP_I2C_SCLK_DEFAULT
#endif

#define LCD_TOUCH_AREA_MAX_X        (CONFIG_ESP32P4_4D_LCD_WIDTH - 1)
#define LCD_TOUCH_AREA_MAX_Y        (CONFIG_ESP32P4_4D_LCD_HEIGHT - 1)

/**
 * @brief I2C bus configuration structure for onboard I2C components
 *
 */
#define ESP32P4_LCD_ONBOARD_I2C_CONFIG()                \
    {                                                   \
        .clk_source = LCD_I2C_CLK_SRC,                  \
        .i2c_port = LCD_I2C_NUM,                        \
        .scl_io_num = LCD_BOARD_SCL_GPIO_NUM,           \
        .sda_io_num = LCD_BOARD_SDA_GPIO_NUM,           \
        .glitch_ignore_cnt = 7,                         \
        .flags.enable_internal_pullup = true,           \
    }

/**
 * @brief MIPI-DSI bus configuration structure
 *
 */
#define ESP32P4_LCD_MIPI_DSI_CONFIG()                   \
    {                                                   \
        .bus_id = 0,                                    \
        .num_data_lanes = LCD_DSI_LANE_NUM,             \
        .phy_clk_src = MIPI_DSI_PHY_CLK_SRC_DEFAULT,    \
        .lane_bit_rate_mbps = LCD_DSI_LANE_BIT_RATE,    \
    }

/**
 * @brief MIPI-DBI panel IO configuration structure
 *
 */
#define ESP32P4_LCD_MIPI_DBI_CONFIG()                   \
    {                                                   \
        .virtual_channel = 0,                           \
        .lcd_cmd_bits = 8,                              \
        .lcd_param_bits = 8,                            \
    }

/**
 * @brief MIPI DPI configuration structure
 *
 * @note  refresh_rate = (dpi_clock_freq_mhz * 1000000) / (h_res + hsync_pulse_width + hsync_back_porch + hsync_front_porch)
 *                                                      / (v_res + vsync_pulse_width + vsync_back_porch + vsync_front_porch)
 */
#define ESP32P4_LCD_MIPI_DPI_CONFIG()                   \
    {                                                   \
        .dpi_clk_src = MIPI_DSI_DPI_CLK_SRC_DEFAULT,    \
        .dpi_clock_freq_mhz = LCD_DPI_CLK_MHZ,          \
        .virtual_channel = 0,                           \
        .num_fbs = LCD_FB_COUNT,                        \
        .in_color_format = LCD_COLOR_FMT_RGB565,        \
        .out_color_format = LCD_COLOR_FMT_RGB565,       \
        .video_timing = {                               \
            .h_size = CONFIG_ESP32P4_4D_LCD_WIDTH,      \
            .v_size = CONFIG_ESP32P4_4D_LCD_HEIGHT,     \
            .hsync_back_porch = LCD_HSYNC_BACK_PORCH,   \
            .hsync_pulse_width = LCD_HSYNC_PULSE_WIDTH, \
            .hsync_front_porch = LCD_HSYNC_FRONT_PORCH, \
            .vsync_back_porch = LCD_VSYNC_BACK_PORCH,   \
            .vsync_pulse_width = LCD_VSYNC_PULSE_WIDTH, \
            .vsync_front_porch = LCD_VSYNC_FRONT_PORCH, \
        },                                              \
    }

/**
 * @brief Initialize the LCD backlight PWM control hardware.
 * 
 * Configures the required LED channel / output pin to control the display's
 * backlight LED driver. Automatically called at least once by esp32p4_lcd_backlight_set()
 * if not used separately.
 *
 * @return ESP_OK on success, an error code if `ledc` channel configuration fails.
 */
esp_err_t esp32p4_lcd_backlight_init(void);

/**
 * @brief Set the LCD backlight brightness level.
 * 
 * Applies a new duty cycle to the initialized backlight hardware. 
 * The valid range is 0 to 255
 *
 * @param[in] brightness Desired brightness level.
 * @return ESP_OK on success, an error code if `ledc` fails to set or update duty cycle
 */
esp_err_t esp32p4_lcd_backlight_set(uint8_t brightness);

/**
 * @brief Initialize the full LCD display panel and return its handle.
 * 
 * Handles complete panel setup including:
 *   - MIPI Interface configuration
 *   - Panel controller initialization sequence
 *   - DMA & framebuffer allocation
 *   - Sending display-specific init commands
 *
 * @param[out] ret_panel Pointer to store the initialized panel handle on success.
 *                        Pass the address of your `esp_lcd_panel_handle_t` variable (e.g., &panel).
 * @return ESP_OK on success, an error code indicating failure reason (e.g., I/O error, init sequence failure).
 */
esp_err_t esp32p4_lcd_full_init(esp_lcd_panel_handle_t *ret_panel);

/**
 * @brief Initialize the touch panel controller via I2C.
 * 
 * Configures the touch IC on the provided I2C bus, allocates a touch handle, and
 * registers required interrupt/event handling setup. The returned handle is used with
 * `esp_lcd_touch_get_data()` or callback registration.
 *
 * @param[in] i2c_bus   Already-configured I2C master bus handle where the touch IC resides.
 * @param[out] tp       Pointer to store the initialized touch panel handle on success.
 * @return ESP_OK on success, an error code if I2C communication fails or controller is not detected.
 */
esp_err_t esp32p4_lcd_touch_init(i2c_master_bus_handle_t i2c_bus, esp_lcd_touch_handle_t *tp);
