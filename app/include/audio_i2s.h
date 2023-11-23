/** 22T3 COMP3601 Design Project A
 * File name: audio_i2s.h
 * Description: Header file for the audio_i2s driver.
 *
 * Distributed under the MIT license.
 * Copyright (c) 2022 Elton Shih
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef AUDIO_I2S_H
#define AUDIO_I2S_H

#include <stdint.h>
#include "axi_dma.h"

// AXI Stream definitions
#define AXI_DMA_S2MM_PADDR          0xa0000000 // Match this with your AXI DMA Control bus
#define AXI_DMA_S2MM_SIZE           0x0000ffff
#define AXI_DMA_RECV_BUFFER_PADDR   0x0f000000
#define AXI_DMA_RECV_BUFFER_SIZE    65535
#define TRANSFER_LEN 256

// audio_i2s module definitions
#define AUDIO_I2S_PADDR             0xa0010000 // Match this wiht your Control bus for your module
#define AUDIO_I2S_SIZE              0xffff
#define AUDIO_I2S_CR                0 << 2
#define AUDIO_I2S_SR                1 << 2
#define AUDIO_I2S_KEY               2 << 2
#define AUDIO_I2S_GAIN              3 << 2
#define AUDIO_I2S_PRESERVED_0       4 << 2
#define AUDIO_I2S_PRESERVED_1       5 << 2

typedef struct audio_i2s_t {
    axi_dma_t s2mm;

    uint32_t *v_baseaddr;
    uint32_t p_baseaddr;
    uint32_t size;
} audio_i2s_t;

int audio_i2s_init(audio_i2s_t *config);
void audio_i2s_release(audio_i2s_t *config);

uint32_t audio_i2s_set_reg(audio_i2s_t *config, uint32_t reg, uint32_t value);
uint32_t audio_i2s_get_reg(audio_i2s_t *config, uint32_t reg);

uint32_t audio_i2s_get_key(audio_i2s_t *config);
int32_t* audio_i2s_recv(audio_i2s_t *config); // Currently set to size 256

#endif // AUDIO_I2S_H
