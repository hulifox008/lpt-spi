/* Using LPT port to program AVR. Using SPI serial programing mode. 
 *
 * Connection:
 *
 *     LPT              AVR
 *     Bit0             MOSI
 *     Bit1             SCK
 *     Bit2             CS 
 *     _BUSY            MISO
 *
 */

#include <sys/io.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#define LPT_BASE    0x378

#define PAGE_SIZE   32      /* 32bytes (16 words) */

inline void MOSI_hi()
{
    outb(inb(LPT_BASE) | 0x01, LPT_BASE);
}

inline void MOSI_lo()
{
    outb(inb(LPT_BASE) & ~0x01, LPT_BASE);
}

inline void SCK_hi()
{
    outb(inb(LPT_BASE) | 0x02, LPT_BASE);
}

inline void SCK_lo()
{
    outb(inb(LPT_BASE) & ~0x02, LPT_BASE);
}

inline void CS_hi()
{
    outb(inb(LPT_BASE) | 0x04, LPT_BASE);
}

inline void CS_lo()
{
    outb(inb(LPT_BASE) & ~0x4, LPT_BASE);
}

inline int  MISO()
{
    return !(inb(LPT_BASE+1) & 0x80);
}


int spi_transfer(unsigned char *out, unsigned char *in, int len)
{
    int i, b;
    unsigned c;

    CS_lo();
    for(i=0;i<len;i++)
    {
        c = out[i];
        unsigned char r=0;
        for(b=0;b<8;b++)
        {
            r=r<<1;
            if(MISO())
                r=r|0x01;

            if(c&0x80)
                MOSI_hi();
            else
                MOSI_lo();
            c=c<<1;

            SCK_hi();
            SCK_lo();
        }

        in[i] = r;
    }

    CS_hi();
    return 0;
}

void spi_init()
{
    iopl(3);
    CS_hi();
    SCK_lo();
}
