#ifndef INCLUDE_MAIN_XGL_DMA_H
#define INCLUDE_MAIN_XGL_DMA_H

/* Functions of src/main/xgl_dma.c that other files call. */

void xglDmaDirectNormal(unsigned int channel, unsigned int address, unsigned int count);
void xglDmaDirectSrcChain(unsigned int channel, unsigned int address);

#endif /* INCLUDE_MAIN_XGL_DMA_H */
