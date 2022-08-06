// *************************************************************************
//
// Copyright (c) 2022 Andrei Gramakov. All rights reserved.
//
// This file is licensed under the terms of the MIT license.
// For a copy, see: https://opensource.org/licenses/MIT
//
// site:    https://agramakov.me
// e-mail:  mail@agramakov.me
//
// *************************************************************************

#include <string.h>
#include "decode_image.h"
#include "driver/spi_master.h"
#include "esp_system.h"
#include "faces.h"
#include "lcd.hpp"
#include "pinout.hpp"
#include "spi.hpp"


void init_spi()
{
    esp_err_t                     ret;
    spi_bus_config_t              buscfg = { .mosi_io_num     = PIN_NUM_MOSI,
                                             .miso_io_num     = PIN_NUM_MISO,
                                             .sclk_io_num     = PIN_NUM_CLK,
                                             .quadwp_io_num   = -1,
                                             .quadhd_io_num   = -1,
                                             .max_transfer_sz = PARALLEL_LINES * LCD_SIZE_PX_X * 2 + 8 };
    spi_device_interface_config_t devcfg = {
        .mode = 0,  //SPI mode 0
#ifdef CONFIG_LCD_OVERCLOCK
        .clock_speed_hz = 26 * 1000 * 1000,  //Clock out at 26 MHz
#else
        .clock_speed_hz = 10 * 1000 * 1000,  //Clock out at 10 MHz
#endif
        .spics_io_num = PIN_NUM_CS,                     //CS pin
        .queue_size   = 7,                              //We want to be able to queue 7 transactions at a time
        .pre_cb       = lcd_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
    };

    //Initialize the SPI bus
    ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    //Attach the LCD to the SPI bus
    ret = spi_bus_add_device(LCD_HOST, &devcfg, &dev_lcdSpi);
    ESP_ERROR_CHECK(ret);
}

void send_lines(spi_device_handle_t spi, int ypos, uint16_t y_lines_num, uint16_t *linedata)
{
    esp_err_t ret;
    int       x;
    //Transaction descriptors. Declared static so they're not allocated on the stack; we need this memory even when this
    //function is finished because the SPI driver needs access to it even while we're already calculating the next line.
    static spi_transaction_t trans[6];

    //In theory, it's better to initialize trans and data only once and hang on to the initialized
    //variables. We allocate them on the stack, so we need to re-init them each call.
    for (x = 0; x < 6; x++) {
        memset(&trans[x], 0, sizeof(spi_transaction_t));
        if ((x & 1) == 0) {
            //Even transfers are commands
            trans[x].length = 8;
            trans[x].user   = (void *) 0;
        } else {
            //Odd transfers are data
            trans[x].length = 8 * 4;
            trans[x].user   = (void *) 1;
        }
        trans[x].flags = SPI_TRANS_USE_TXDATA;
    }
    trans[0].tx_data[0] = 0x2A;                                 //Column Address Set
    trans[1].tx_data[0] = 0;                                    //Start Col High
    trans[1].tx_data[1] = 0;                                    //Start Col Low
    trans[1].tx_data[2] = (LCD_SIZE_PX_X) >> 8;                 //End Col High
    trans[1].tx_data[3] = (LCD_SIZE_PX_X) &0xff;                //End Col Low
    trans[2].tx_data[0] = 0x2B;                                 //Page address set
    trans[3].tx_data[0] = ypos >> 8;                            //Start page high
    trans[3].tx_data[1] = ypos & 0xff;                          //start page low
    trans[3].tx_data[2] = (ypos + y_lines_num) >> 8;            //end page high
    trans[3].tx_data[3] = (ypos + y_lines_num) & 0xff;          //end page low
    trans[4].tx_data[0] = 0x2C;                                 //memory write
    trans[5].tx_buffer  = linedata;                             //finally send the line data
    trans[5].length     = y_lines_num * LCD_SIZE_PX_X * 2 * 8;  //Data length, in bits
    trans[5].flags      = 0;                                    //undo SPI_TRANS_USE_TXDATA flag

    //Queue all transactions.
    for (x = 0; x < 6; x++) {
        ret = spi_device_queue_trans(dev_lcdSpi, &trans[x], portMAX_DELAY);
        assert(ret == ESP_OK);
    }

    //When we are here, the SPI driver is busy (in the background) getting the transactions sent. That happens
    //mostly using DMA, so the CPU doesn't have much to do here. We're not going to wait for the transaction to
    //finish because we may as well spend the time calculating the next line. When that is done, we can call
    //send_line_finish, which will wait for the transfers to be done and check their status.
}


void send_line_finish(spi_device_handle_t spi)
{
    spi_transaction_t *rtrans;
    esp_err_t          ret;
    //Wait for all 6 transactions to be done and get back the results.
    for (int x = 0; x < 6; x++) {
        ret = spi_device_get_trans_result(dev_lcdSpi, &rtrans, portMAX_DELAY);
        assert(ret == ESP_OK);
        //We could inspect rtrans now if we received any info back. The LCD is treated as write-only, though.
    }
}

static inline uint16_t get_bgnd_pixel(uint16_t **pixels, int x, int y)
{
    //Image has an 8x8 pixel margin, so we can also resolve e.g. [-3, 243]
    x += 8;
    y += 8;
    return pixels[y][x];
}

//Instead of calculating the offsets for each pixel we grab, we pre-calculate the valueswhenever a frame changes, then re-use
//these as we go through all the pixels in the frame. This is much, much faster.

//Calculate the pixel data for a set of lines (with implied line size of 320). Pixels go in dest, line is the Y-coordinate of the
//first line to be calculated, linect is the amount of lines to calculate. Frame increases by one every time the entire image
//is displayed; this is used to go to the next frame of animation.
static void prepare_lines(uint16_t **src_image_in_pixels, uint16_t *dest, int line, int y_lines_num)
{
    for (int y = line; y < line + y_lines_num; y++) {
        for (int x = 0; x < LCD_SIZE_PX_X; x++) { *dest++ = get_bgnd_pixel(src_image_in_pixels, x, y); }
    }
}

//Simple routine to generate some patterns and send them to the LCD. Don't expect anything too
//impressive. Because the SPI driver handles transactions in the background, we can calculate the next line
//while the previous one is being sent.
void send_image(spi_device_handle_t spi, const uint8_t *img_jpg)
{
    uint16_t *lines[2];

    //Allocate memory for the pixel buffers
    for (int i = 0; i < 2; i++) {
        lines[i] = static_cast<uint16_t *>(
                heap_caps_malloc(LCD_SIZE_PX_X * PARALLEL_LINES * sizeof(uint16_t), MALLOC_CAP_DMA));
        assert(lines[i] != NULL);
    }

    uint16_t **image_in_pixels;
    decode_image(&image_in_pixels, img_jpg);

    //Indexes of the line currently being sent to the LCD and the line we're calculating.
    int sending_line = -1;
    int calc_line    = 0;


    for (int y_cur = 0; y_cur < LCD_SIZE_PX_Y; y_cur += PARALLEL_LINES) {
        //Calculate a line.
        prepare_lines(image_in_pixels, lines[calc_line], y_cur, PARALLEL_LINES);

        //Finish up the sending process of the previous line, if any
        if (sending_line != -1) send_line_finish(dev_lcdSpi);

        //Swap sending_line and calc_line
        sending_line = calc_line;
        calc_line    = (calc_line == 1) ? 0 : 1;

        //Send the line we currently calculated.
        send_lines(dev_lcdSpi, y_cur, PARALLEL_LINES, lines[sending_line]);

        //The line set is queued up for sending now; the actual sending happens in the
        //background. We can go on to calculate the next line set as long as we do not
        //touch line[sending_line]; the SPI sending process is still reading from that.
    }
    send_line_finish(dev_lcdSpi);  // the last line


    // Clean up
    if (image_in_pixels != NULL) {
        for (int i = 0; i < IMAGE_H; i++) { free((image_in_pixels)[i]); }
        free(image_in_pixels);
    }
    for (int i = 0; i < 2; i++) { free(lines[i]); }
}

spi_device_handle_t dev_lcdSpi = nullptr;
