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

#include "decode_image.h"
#include <string.h>
#include "esp_log.h"
#include "esp_rom_tjpgd.h"
#include "faces.h"

const char *TAG = "ImageDec";

//Data that is passed from the decoder function to the infunc/outfunc functions.
typedef struct {
    const unsigned char *inData;   //Pointer to jpeg data
    uint16_t             inPos;    //Current position in jpeg data
    uint16_t           **outData;  //Array of IMAGE_H pointers to arrays of IMAGE_W 16-bit pixel values
    int                  outW;     //Width of the resulting file
    int                  outH;     //Height of the resulting file
} JpegDev;

//Input function for jpeg decoder. Just returns bytes from the inData field of the JpegDev structure.
static uint32_t infunc(esp_rom_tjpgd_dec_t *decoder, uint8_t *buf, uint32_t len)
{
    //Read bytes from input file
    JpegDev *jd = (JpegDev *) decoder->device;
    if (buf != NULL) { memcpy(buf, jd->inData + jd->inPos, len); }
    jd->inPos += len;
    return len;
}

//Output function. Re-encodes the RGB888 data from the decoder as big-endian RGB565 and
//stores it in the outData array of the JpegDev structure.
static uint32_t outfunc(esp_rom_tjpgd_dec_t *decoder, void *bitmap, esp_rom_tjpgd_rect_t *rect)
{
    JpegDev *jd = (JpegDev *) decoder->device;
    uint8_t *in = (uint8_t *) bitmap;
    for (int y = rect->top; y <= rect->bottom; y++) {
        for (int x = rect->left; x <= rect->right; x++) {
            //We need to convert the 3 bytes in `in` to a rgb565 value.
            uint16_t v = 0;
            v |= ((in[0] >> 3) << 11);
            v |= ((in[1] >> 2) << 5);
            v |= ((in[2] >> 3) << 0);
            //The LCD wants the 16-bit value in big-endian, so swap bytes
            v                 = (v >> 8) | (v << 8);
            jd->outData[y][x] = v;
            in += 3;
        }
    }
    return 1;
}

//Size of the work space for the jpeg decoder.
#define WORKSZ 3100

//Decode the embedded image into pixel lines that can be used with the rest of the logic.
esp_err_t decode_image(uint16_t ***pixels, const uint8_t *image_array)
{
    char               *work = NULL;
    int                 r;
    esp_rom_tjpgd_dec_t decoder;
    JpegDev             jd;
    *pixels       = NULL;
    esp_err_t ret = ESP_OK;

    //Alocate pixel memory. Each line is an array of IMAGE_W 16-bit pixels; the `*pixels` array itself contains pointers to these lines.
    *pixels = calloc(IMAGE_H, sizeof(uint16_t *));
    if (*pixels == NULL) {
        ESP_LOGE(TAG, "Error allocating memory for lines");
        ret = ESP_ERR_NO_MEM;
        goto err;
    }
    for (int i = 0; i < IMAGE_H; i++) {
        (*pixels)[i] = malloc(IMAGE_W * sizeof(uint16_t));
        if ((*pixels)[i] == NULL) {
            ESP_LOGE(TAG, "Error allocating memory for line %d", i);
            ret = ESP_ERR_NO_MEM;
            goto err;
        }
    }

    //Allocate the work space for the jpeg decoder.
    work = calloc(WORKSZ, 1);
    if (work == NULL) {
        ESP_LOGE(TAG, "Cannot allocate workspace");
        ret = ESP_ERR_NO_MEM;
        goto err;
    }

    //Populate fields of the JpegDev struct.
    jd.inData  = image_array;
    jd.inPos   = 0;
    jd.outData = *pixels;
    jd.outW    = IMAGE_W;
    jd.outH    = IMAGE_H;

    //Prepare and decode the jpeg.
    r = esp_rom_tjpgd_prepare(&decoder, infunc, work, WORKSZ, (void *) &jd);
    if (r != JDR_OK) {
        ESP_LOGE(TAG, "Image decoder: jd_prepare failed (%d)", r);
        ret = ESP_ERR_NOT_SUPPORTED;
        goto err;
    }
    r = esp_rom_tjpgd_decomp(&decoder, outfunc, 0);
    if (r != JDR_OK && r != JDR_FMT1) {
        ESP_LOGE(TAG, "Image decoder: jd_decode failed (%d)", r);
        ret = ESP_ERR_NOT_SUPPORTED;
        goto err;
    }

    //All done! Free the work area (as we don't need it anymore) and return victoriously.
    free(work);
    return ret;
err:
    //Something went wrong! Exit cleanly, de-allocating everything we allocated.
    if (*pixels != NULL) {
        for (int i = 0; i < IMAGE_H; i++) { free((*pixels)[i]); }
        free(*pixels);
    }
    free(work);
    return ret;
}
