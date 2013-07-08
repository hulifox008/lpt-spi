#include <stdio.h>
#include <assert.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include "lptspi.h"

#define PAGE_SIZE   256

int mxic_read_id(u_int8_t *manufacturer, u_int8_t *dev_id)
{
    assert(NULL!=manufacturer);
    assert(NULL!=dev_id);

    unsigned char buf_out[3] = {0x9F, 0x00, 0x00};
    unsigned char buf_in[3] = {0};

    if(spi_transfer(buf_out, buf_in, 3)){
        return -1;
    }

    *manufacturer = buf_in[1];
    *dev_id = buf_in[2];
    return 0;
}

int mxic_write_enable(int enable)
{
    unsigned char buf_out[1] = {4};
    unsigned char buf_in[1] = {0};

    if(enable){
        buf_out[0] = 6;
    }
        
    if(spi_transfer(buf_out, buf_in, 1)){
        return -1;
    }

    return 0;
}

int mxic_read_status(u_int8_t *status)
{
    assert(NULL!=status);

    unsigned char buf_out[2] = {0x05, 0xFF};
    unsigned char buf_in[2] = {0xFF, 0xFF};

    if(spi_transfer(buf_out, buf_in, 2)){
        return -1;
    }

    *status = buf_in[1];
    return 0;
}

int mxic_wait_busy()
{
    u_int8_t status;
    mxic_read_status(&status);
    while(status & 0x01){
        usleep(10000);
        mxic_read_status(&status);
    }

    return 0;
}

int mxic_chip_erase()
{
    unsigned char buf_out[1] = {0xC7};
    unsigned char buf_in[1] = {0};

    if(spi_transfer(buf_out, buf_in, 1)){
        return -1;
    }

    mxic_wait_busy();
    return 0;
}

int mxic_page_program(u_int32_t address, u_int8_t *page)
{
    assert(NULL!=page);

    unsigned char buf_out[5+PAGE_SIZE];
    unsigned char buf_in[5+PAGE_SIZE];

    mxic_write_enable(1);

    buf_out[0] = 0x02;
    buf_out[1] = address>>24 & 0xFF;
    buf_out[2] = address>>16 & 0xFF;
    buf_out[3] = address>>8 & 0xFF;
    buf_out[4] = address & 0xFF;
    memcpy(&buf_out[5], page, PAGE_SIZE);

    if(spi_transfer(buf_out, buf_in, 5+PAGE_SIZE)){
        return -1;
    }
    mxic_wait_busy();
    return 0;
}

int mxic_page_read(u_int32_t address, u_int8_t *page)
{
    assert(NULL!=page);

    unsigned char buf_out[5+PAGE_SIZE];
    unsigned char buf_in[5+PAGE_SIZE];

    buf_out[0] = 0x03;
    buf_out[1] = address>>24 & 0xFF;
    buf_out[2] = address>>16 & 0xFF;
    buf_out[3] = address>>8 & 0xFF;
    buf_out[4] = address & 0xFF;
    memcpy(&buf_out[5], page, PAGE_SIZE);

    if(spi_transfer(buf_out, buf_in, 5+PAGE_SIZE)){
        return -1;
    }
    memcpy(page, &buf_in[5], PAGE_SIZE);
    return 0;
}

int mxic_write_status(u_int8_t status)
{
    unsigned char buf_out[2] = {0x1, 0x00};
    unsigned char buf_in[2] = {0};

    buf_out[1] = status;

    if(spi_transfer(buf_out, buf_in, 2)){
        return -1;
    }
    return 0;
}

void hex_dump(u_int8_t *data, int size)
{
    int i;
    for(i=0;i<size;i++)
    {
        printf("%02X ", data[i]);
        if(!((i+1)%32))
            printf("\n");
    }
    printf("\n");
}

int program_file(const char *file)
{
    assert(NULL!=file);

    u_int8_t write_buf[PAGE_SIZE], read_buf[PAGE_SIZE];
    unsigned int page = 0;
    
    FILE *fp = fopen(file, "r");
    if(NULL==fp){
        perror("Cannot open file");
        return -1;
    }


    memset(write_buf, 0xFF, PAGE_SIZE);
    
    while(fread(write_buf, PAGE_SIZE, 1, fp)){
        printf("Programing page %d ...\n", page);
        mxic_page_program(page*PAGE_SIZE, write_buf);

        mxic_page_read(page*PAGE_SIZE, read_buf);

        if(memcmp(write_buf, read_buf, PAGE_SIZE)){
            printf("Verification failed!\n");
            fclose(fp);
            return -1;
        }
        memset(write_buf, 0xFF, PAGE_SIZE);
        page++;
    }
    fclose(fp);
    return 0;
}

int main(int argc, char* argv[])
{
    u_int8_t manufacturer, dev_id;

    if(argc!=2) {   
        fprintf(stderr, "Usage: lptavr <filename>\n");
        return 1;
    }

    spi_init();
    usleep(100000);

    mxic_read_id(&manufacturer, &dev_id);
    printf("Manufacturer: %02X  Device: %02X\n", manufacturer, dev_id);
    if(0xC2 != manufacturer
    || 0x20 != dev_id){
        printf("Unsupported device!\n");
        return -1;
    }

    u_int8_t status;
    mxic_read_status(&status);
    printf("Status: %02X\n", status);

    printf("Erasing device ...\n");
    mxic_write_enable(1);
    mxic_chip_erase();

    program_file(argv[1]);

    return 0;
}
