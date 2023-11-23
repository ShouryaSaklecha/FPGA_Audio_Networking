/** 22T3 COMP3601 Design Project A
 * File name: axi_dma.c
 * Description: Userspace driver for AXI DMA. This file memory maps the DMA device and
 * 	configures it to transfer data from FPGA to software program memory.
 *
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

#include "axi_dma.h"

#include <stdio.h> // todo: remove this once debugging is finished
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

/*
 * AXI DMA general function
 */
int32_t axi_dma_init(axi_dma_t *device, uint32_t baseaddr, uint32_t dst_addr, uint32_t size) {
    // Open /dev/mem for memory mapping
    int32_t dev_fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (dev_fd < 0) {
        return -1;
    }

    // Init device
    device->size = size;
    device->p_baseaddr = baseaddr;
    device->v_baseaddr = (uint32_t *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, baseaddr);
    if (device->v_baseaddr == MAP_FAILED) {
        close(dev_fd);
        return -1;
    }

    if (dma_s2mm_sg_active(device)) {
        close(dev_fd);
        return -1;
    }
    dma_s2mm_reset(device);

    // Init buffers
    device->p_dst_addr = dst_addr;
    device->v_dst_addr = (void *) mmap(NULL, 0xffff, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, dst_addr);
    if (device->v_dst_addr == MAP_FAILED) {
        munmap(device->v_baseaddr, device->size);
        close(dev_fd);
        return -1;
    }

    close(dev_fd);
    return 0;
}

void axi_dma_release(axi_dma_t *device) {
    munmap(device->v_baseaddr, device->size);
    munmap(device->v_dst_addr, device->size);
}

void axi_dma_s2mm_transfer(axi_dma_t *device, uint32_t size) {
    // Clearup
    dma_s2mm_reset(device);
    dma_s2mm_stop(device);

    // Config and start
    dma_s2mm_set_dst_addr(device, device->p_dst_addr);
    _reg_set(device->v_baseaddr, AXI_DMA_S2MM_CR, 0xf001);
    _reg_set(device->v_baseaddr, AXI_DMA_S2MM_LENGTH, size);
    
    dma_s2mm_busy_wait(device);
}

void dma_s2mm_busy_wait(axi_dma_t *device) {
    volatile uint32_t s2mm_sr = _reg_get(device->v_baseaddr, AXI_DMA_S2MM_SR);
    while (!(s2mm_sr & (1 << AXI_DMA_SR_IDLE))) {
        s2mm_sr = _reg_get(device->v_baseaddr, AXI_DMA_S2MM_SR);
    }
}

/*
 * DMA status
 */

void dma_s2mm_status(axi_dma_t *device) {
    uint32_t status = _reg_get(device->v_baseaddr, AXI_DMA_S2MM_SR);
    printf("Stream to Memory-mapped status (0x%08x@0x%02x):", status, AXI_DMA_S2MM_SR);
    if (status & (1 << AXI_DMA_SR_HALTED))
        printf(" halted");
    else
        printf(" running");
    if (status & (1 << AXI_DMA_SR_IDLE))
        printf(" idle");
    if (status & (1 << AXI_DMA_SR_SG_ACT))
        printf(" SGIncld");
    if (status & (1 << AXI_DMA_SR_DMA_INT_ERR))
        printf(" DMAIntErr");
    if (status & (1 << AXI_DMA_SR_DMA_SLV_ERR))
        printf(" DMASlvErr");
    if (status & (1 << AXI_DMA_SR_DMA_DEC_ERR))
        printf(" DMADecErr");
    if (status & (1 << AXI_DMA_SR_SG_INT_ERR))
        printf(" SGIntErr");
    if (status & (1 << AXI_DMA_SR_SG_SLV_ERR))
        printf(" SGSlvErr");
    if (status & (1 << AXI_DMA_SR_SG_DEC_ERR))
        printf(" SGDecErr");
    if (status & (1 << AXI_DMA_SR_IOC_IRQ))
        printf(" IOC_Irq");
    if (status & (1 << AXI_DMA_SR_DLY_IRQ))
        printf(" Dly_Irq");
    if (status & (1 << AXI_DMA_SR_ERR_IRQ))
        printf(" Err_Irq");
    printf("\n");
}

void axi_dma_read_data(void *address, int byte_length) {
    int *addr = (int *) address;
    int reg_offset;
    printf("\t[read_data] data at destination address 0x%p:", address); 
    for (reg_offset = 0; reg_offset < byte_length/4; reg_offset+=4) {
        if (reg_offset % 10 == 0) {
            printf("\n\t");
        }
        printf("%x ", *addr);
        addr++;
    }
    printf("\n");
}

uint32_t dma_s2mm_sr(axi_dma_t *device) {
    return _reg_get(device->v_baseaddr, AXI_DMA_S2MM_SR);
}

////////////////////////////////////////////////////////////////////////////////
// AXI DMA Setters
////////////////////////////////////////////////////////////////////////////////

void dma_s2mm_reset(axi_dma_t *device) {
    _reg_set(device->v_baseaddr, AXI_DMA_S2MM_CR, 1 << AXI_DMA_CR_RESET);
}

void dma_s2mm_run(axi_dma_t *device) {
    uint32_t cr = _reg_get(device->v_baseaddr, AXI_DMA_S2MM_CR);
    _reg_set(device->v_baseaddr, AXI_DMA_S2MM_CR, cr | (1 << AXI_DMA_CR_RS));
}

void dma_s2mm_stop(axi_dma_t *device) {
    uint32_t cr = _reg_get(device->v_baseaddr, AXI_DMA_S2MM_CR);
    _reg_set(device->v_baseaddr, AXI_DMA_S2MM_CR, cr & ~(1 << AXI_DMA_CR_RS));
}

void dma_s2mm_IOC_IRQ_EN(axi_dma_t *device) {
    uint32_t cr = _reg_get(device->v_baseaddr, AXI_DMA_S2MM_CR);
    _reg_set(device->v_baseaddr, AXI_DMA_S2MM_CR, cr | (1 << AXI_DMA_CR_IOC_IRQ_EN));
}

void dma_s2mm_IOC_IRQ_DIS(axi_dma_t *device) {
    uint32_t cr = _reg_get(device->v_baseaddr, AXI_DMA_S2MM_CR);
    _reg_set(device->v_baseaddr, AXI_DMA_S2MM_CR, cr & ~(1 << AXI_DMA_CR_IOC_IRQ_EN));
}

void dma_s2mm_DLY_IRO_EN(axi_dma_t *device) {
    uint32_t cr = _reg_get(device->v_baseaddr, AXI_DMA_S2MM_CR);
    _reg_set(device->v_baseaddr, AXI_DMA_S2MM_CR, cr | (1 << AXI_DMA_CR_DLY_IRQ_EN));
}

void dma_s2mm_DLY_IRO_DIS(axi_dma_t *device) {
    uint32_t cr = _reg_get(device->v_baseaddr, AXI_DMA_S2MM_CR);
    _reg_set(device->v_baseaddr, AXI_DMA_S2MM_CR, cr & ~(1 << AXI_DMA_CR_DLY_IRQ_EN));
}

void dma_s2mm_ERR_IRQ_EN(axi_dma_t *device) {
    uint32_t cr = _reg_get(device->v_baseaddr, AXI_DMA_S2MM_CR);
    _reg_set(device->v_baseaddr, AXI_DMA_S2MM_CR, cr | (1 << AXI_DMA_CR_ERR_IRQ_EN));
}

void dma_s2mm_ERR_IRQ_DIS(axi_dma_t *device) {
    uint32_t cr = _reg_get(device->v_baseaddr, AXI_DMA_S2MM_CR);
    _reg_set(device->v_baseaddr, AXI_DMA_S2MM_CR, cr & ~(1 << AXI_DMA_CR_ERR_IRQ_EN));
}


// Sets the number of bytes (length) for the mm2s transfer
// Writing to this register will start the transfer

void dma_s2mm_set_dst_addr(axi_dma_t *device, uint32_t addr) {
    _reg_set(device->v_baseaddr, AXI_DMA_S2MM_DST_ADDR, addr);
}

void dma_s2mm_set_dst_addr_msb(axi_dma_t *device, uint32_t addr) {
    _reg_set(device->v_baseaddr, AXI_DMA_S2MM_DST_ADDR_MSB, addr);
}

// Sets the number of bytes (length) for the s2mm transfer
// Writing to this register will start the transfer
void dma_s2mm_set_length(axi_dma_t *device, uint32_t length) {
    _reg_set(device->v_baseaddr, AXI_DMA_S2MM_LENGTH, length);
}


////////////////////////////////////////////////////////////////////////////////
// AXI DMA Getters
////////////////////////////////////////////////////////////////////////////////

uint8_t dma_s2mm_halted(axi_dma_t *device) {
    return (_reg_get(device->v_baseaddr, AXI_DMA_S2MM_SR) & (1 << AXI_DMA_SR_HALTED)) ? 1 : 0;
}

uint8_t dma_s2mm_idle(axi_dma_t *device) {
    return (_reg_get(device->v_baseaddr, AXI_DMA_S2MM_SR) & (1 << AXI_DMA_SR_IDLE)) ? 1 : 0;
}

uint8_t dma_s2mm_busy(axi_dma_t *device) {
    uint32_t sr = _reg_get(device->v_baseaddr, AXI_DMA_S2MM_SR);
    return (!(sr & (1 << AXI_DMA_SR_IOC_IRQ)) && !(sr & (1 << AXI_DMA_SR_IDLE))) ? 1 : 0;
}

uint8_t dma_s2mm_sg_active(axi_dma_t *device) {
    return (_reg_get(device->v_baseaddr, AXI_DMA_S2MM_SR) & (1 << AXI_DMA_SR_SG_ACT)) ? 1 : 0;
}

uint8_t dma_s2mm_dma_internal_error(axi_dma_t *device) {
    return (_reg_get(device->v_baseaddr, AXI_DMA_S2MM_SR) & (1 << AXI_DMA_SR_DMA_INT_ERR)) ? 1 : 0;
}

uint8_t dma_s2mm_dma_slave_error(axi_dma_t *device) {
    return (_reg_get(device->v_baseaddr, AXI_DMA_S2MM_SR) & (1 << AXI_DMA_SR_DMA_SLV_ERR)) ? 1 : 0;
}

uint8_t dma_s2mm_dma_decode_error(axi_dma_t *device) {
    return (_reg_get(device->v_baseaddr, AXI_DMA_S2MM_SR) & (1 << AXI_DMA_SR_DMA_DEC_ERR)) ? 1 : 0;
}

uint8_t dma_s2mm_IOC_IRQ(axi_dma_t *device) {
    return (_reg_get(device->v_baseaddr, AXI_DMA_S2MM_SR) & (1 << AXI_DMA_SR_IOC_IRQ)) ? 1 : 0;
}

uint8_t dma_s2mm_DLY_IRQ(axi_dma_t *device) {
    return (_reg_get(device->v_baseaddr, AXI_DMA_S2MM_SR) & (1 << AXI_DMA_SR_DLY_IRQ)) ? 1 : 0;
}

uint8_t dma_s2mm_ERR_IRQ(axi_dma_t *device) {
    return (_reg_get(device->v_baseaddr, AXI_DMA_S2MM_SR) & (1 << AXI_DMA_SR_ERR_IRQ)) ? 1 : 0;
}
