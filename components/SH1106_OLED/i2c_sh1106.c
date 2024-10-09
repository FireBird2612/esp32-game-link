#include <string.h>
#include <stdio.h>

#include "esp_types.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c_master.h"

#include "i2c_sh1106.h"

static const char TAG[] = "i2c-sh1106";

esp_err_t i2c_sh1106_init(i2c_master_bus_handle_t bus_handle, i2c_sh1106_config_t *sh1106_config){
    esp_err_t ret = ESP_OK;

    /*  Configure the sh1106 in slave and add it to the i2c bus  */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_7,
        .device_address = sh1106_config->device_address,
        .scl_speed_hz = sh1106_config->supported_freq,
    };
    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    // Ready the buffer to write 0s to all the pixels.
    for (int pg_start = 0; pg_start < 8; pg_start++){
        for (int seg_start = 0; seg_start < 128; seg_start++){
            sh1106_config->page[pg_start].page_seg[seg_start] = 0;
        }
    }

    /*  display initialization commands */
    uint8_t display_off_cmd[] = {OLED_CNT_BYTE_CMD_STREAM, OLED_SET_DISPLAY_OFF};
    uint8_t clk_div_cmd[] = {OLED_CNT_BYTE_CMD_STREAM, OLED_SET_DISPLAY_CLK_DIV, 0x80};
    uint8_t multiplex_ratio[] = {OLED_CNT_BYTE_CMD_STREAM, OLED_SET_MULTIPLEX_RATIO, 0x3F};
    uint8_t display_offset[] = {OLED_CNT_BYTE_CMD_STREAM, OLED_SET_DISPLAY_OFFSET, 0x00};
    uint8_t display_start_line[] = {OLED_CNT_BYTE_CMD_STREAM, OLED_SET_DISPLAY_START_L};
    uint8_t unknown_cmd_1[] = {OLED_CNT_BYTE_CMD_STREAM, 0x8D, 0x14};
    uint8_t unknown_cmd_2[] = {OLED_CNT_BYTE_CMD_STREAM, 0x20, 0x00};
    uint8_t segment_remap[] = {OLED_CNT_BYTE_CMD_STREAM, OLED_SET_SEG_REMAP_H};
    uint8_t scan_dir[] = {OLED_CNT_BYTE_CMD_STREAM, OLED_SET_COM_OP_SCAN_V};
    uint8_t com_hw_pad[] = {OLED_CNT_BYTE_CMD_STREAM, OLED_SET_COM_PAD_HW, 0x12};
    uint8_t contrast[] = {OLED_CNT_BYTE_CMD_STREAM, OLED_SET_CONTRAST, 0xCF};
    uint8_t precharge[] = {OLED_CNT_BYTE_CMD_STREAM, OLED_SET_PRECHARGE, 0xF1};
    uint8_t vcom[] = {OLED_CNT_BYTE_CMD_STREAM, OLED_SET_VCOM_LEVEL, 0x40};
    uint8_t unknown_cmd_3[] = {OLED_CNT_BYTE_CMD_STREAM, 0x2E};
    uint8_t display_style[] = {OLED_CNT_BYTE_CMD_STREAM, OLED_SET_ENT_DISPLAY_RAM};
    uint8_t display_style1[] = {OLED_CNT_BYTE_CMD_STREAM, OLED_SET_DISPLAY_NORMAL};
    uint8_t setpage[] = {OLED_CNT_BYTE_CMD_STREAM, 0x10, 0x2, OLED_SET_PAGE_ADR};

    sh1106_i2c_transaction_t init_cmds[] = {
        {.command = display_off_cmd, .command_length = sizeof(display_off_cmd)},
        {.command = clk_div_cmd, .command_length = sizeof(clk_div_cmd)},
        {.command = multiplex_ratio, .command_length = sizeof(multiplex_ratio)},
        {.command = display_offset, .command_length = sizeof(display_offset)},
        {.command = display_start_line, .command_length = sizeof(display_start_line)},
        {.command = unknown_cmd_1, .command_length = sizeof(unknown_cmd_1)},
        {.command = unknown_cmd_2, .command_length = sizeof(unknown_cmd_2)},
        {.command = segment_remap, .command_length = sizeof(segment_remap)},
        {.command = scan_dir, .command_length = sizeof(scan_dir)},
        {.command = com_hw_pad, .command_length = sizeof(com_hw_pad)},
        {.command = contrast, .command_length = sizeof(contrast)},
        {.command = precharge, .command_length = sizeof(precharge)},
        {.command = vcom, .command_length = sizeof(vcom)},
        {.command = unknown_cmd_3, .command_length = sizeof(unknown_cmd_3)},
        {.command = display_style, .command_length = sizeof(display_style)},
        {.command = display_style1, .command_length = sizeof(display_style1)},
//        {.command = display_start_line, .command_length = sizeof(display_start_line)},
//        {.command = setpage, .command_length = sizeof(setpage)},
    };

    /* Point to the start of the buffer */
    sh1106_i2c_transaction_t *start_pointer = init_cmds;

    uint8_t i2c_tranx = sizeof(init_cmds) / sizeof(init_cmds[0]);
    while (i2c_tranx--)
    {
        i2c_master_transmit(dev_handle, start_pointer->command, start_pointer->command_length, 100);
        ++start_pointer;
    }
    
    // create a buffer to transmit the 0s
    uint8_t buff_fill_zeros[25] = {0};
    buff_fill_zeros[0] = 0x40;

    uint8_t *write_bytes = NULL;
    uint8_t total_seg_batch = 6;
    uint8_t count = 0;
    for (uint8_t page_start = 0; page_start < 8; page_start++)
    {
        //write_bytes = sh1106_config->page[page_start].page_seg;

        setpage[3] = OLED_SET_PAGE_ADR + count++;
        i2c_master_transmit(dev_handle, display_start_line, 2, 100);
        i2c_master_transmit(dev_handle, setpage, 4, 100);

        for (uint8_t seg_batch = 1; seg_batch <= total_seg_batch; seg_batch++){
            uint8_t seg_count;
            if (seg_batch == 6){
                seg_count = 9;
            }
            else{
                seg_count = 25;
            }

            i2c_master_transmit(dev_handle, buff_fill_zeros, seg_count, 100);
            write_bytes = write_bytes + seg_count;

        }
    }

    uint8_t disp_on[] = {OLED_CNT_BYTE_CMD_STREAM, OLED_SET_DISPLAY_ON};
    i2c_master_transmit(dev_handle, disp_on, 2, 100);

    ESP_LOGI(TAG, "initialized successfully!");

    // turn on the 3rd pixel in every page of 5th column
    // buff_fill_zeros[6] |= 1 << 4;

    return ret;
}
