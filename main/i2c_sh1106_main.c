#include <string.h>
#include <stdio.h>

#include "sdkconfig.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c_master.h"

#include "i2c_sh1106.h"

// Define the SDA and SCL pinouts
#define SCL_IO_PIN CONFIG_I2C_MASTER_SCL
#define SDA_IO_PIN CONFIG_I2C_MASTER_SDA

// Define the master frequency, use the freq supported by slave.
#define MASTER_FREQUENCY CONFIG_I2C_MASTER_FREQUENCY

// I don't understand why the port number is -1 and lenght is 48. 
#define PORT_NUMBER CONFIG_I2C_PORT_NUMBER

void app_main() {
    i2c_master_bus_config_t i2c_master_config = {
        .i2c_port = PORT_NUMBER,
        .sda_io_num = SDA_IO_PIN,
        .scl_io_num = SCL_IO_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    /*  Create an instance of the i2c bus, add the a new master device with i2c_master_config to the i2c bus    */
    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_master_config, &bus_handle));

    i2c_sh1106_config_t sh1106_t = {
      .device_address = 0x3C,
      .supported_freq = MASTER_FREQUENCY,
      .width = 128,
      .height = 64,
      .contrast_level = 0x7F,
    };

    ESP_ERROR_CHECK(i2c_sh1106_init(bus_handle, &sh1106_t));

}