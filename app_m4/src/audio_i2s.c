/** 22T3 COMP3601 Design Project A
 * File name: audio_i2s.c
 * Description: Audio I2S driver. This is an example userspace driver for the FPGA microphone system.
 * 	It wraps around the AXI DMA driver (for PL to PS data transfer) and memory maps the control bus
 * 	interface in the FPGA design (AXI-Lite) to access registers in your design. 
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

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "misc.h"
#include "audio_i2s.h"


/**
 * @brief Initalize the I2S audio interface. Mmaps the physical address.
 * 
 * @param config 
 * @return int 
 */
int audio_i2s_init(audio_i2s_t *config) {
    int32_t ret = axi_dma_init(&config->s2mm, AXI_DMA_S2MM_PADDR, AXI_DMA_RECV_BUFFER_PADDR, AXI_DMA_RECV_BUFFER_SIZE);
    if (ret < 0) {
        return ret;
    }
    memset(config->s2mm.v_dst_addr, 0, AXI_DMA_RECV_BUFFER_SIZE);
    
    int32_t dev_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (dev_fd < 0) {
        axi_dma_release(&config->s2mm);
        return -1;
    }

    config->size = AUDIO_I2S_SIZE;
    config->p_baseaddr = AUDIO_I2S_PADDR;
    config->v_baseaddr = (uint32_t *) mmap(NULL, AUDIO_I2S_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, AUDIO_I2S_PADDR);
    if (config->v_baseaddr == MAP_FAILED) {
        close(dev_fd);
        axi_dma_release(&config->s2mm);
        return -1;
    }

    return 0;
}

void audio_i2s_release(audio_i2s_t *config) {
    axi_dma_release(&config->s2mm);
    munmap(config->v_baseaddr, config->size);
}


uint32_t audio_i2s_set_reg(audio_i2s_t *config, uint32_t reg, uint32_t value) {
    return _reg_set(config->v_baseaddr, reg, value);
}

uint32_t audio_i2s_get_reg(audio_i2s_t *config, uint32_t reg) {
    return _reg_get(config->v_baseaddr, reg);
}

uint32_t audio_i2s_get_key(audio_i2s_t *config) {
    return _reg_get(config->v_baseaddr, AUDIO_I2S_KEY);
}

int32_t* audio_i2s_recv(audio_i2s_t *config){  //, void *buffer) { // Currently set to size 256
    axi_dma_s2mm_transfer(&config->s2mm, TRANSFER_LEN*sizeof(uint32_t));
    // memcpy(buffer, (void*) config->s2mm->v_dst_addr, TRANSFER_LEN*sizeof(uint32_t));
    return (int32_t*) config->s2mm.v_dst_addr;
}
