#ifndef __LPTSPI_INCLUDED__
#define __LPTSPI_INCLUDED__


void spi_init();

int spi_transfer(unsigned char *out, unsigned char *in, int len);


#endif /* __LPTSPI_INCLUDED__ */
