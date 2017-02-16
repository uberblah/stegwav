#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

void printBit(uint8_t bit)
{
    if(bit) printf("1");
    else printf("0");
}

void printByteBits(uint8_t byte)
{
    printBit(byte & 0x80);
    printBit(byte & 0x40);
    printBit(byte & 0x20);
    printBit(byte & 0x10);
    
    printf(":");
    
    printBit(byte & 0x08);
    printBit(byte & 0x04);
    printBit(byte & 0x02);
    printBit(byte & 0x01);
}

void printBytes(void* data, size_t size)
{
    uint8_t* bytes = (uint8_t*)data;
    for(size_t i = size - 1; i < size; i--)
    {
	printByteBits(bytes[i]);
	printf(" ");
    }
    printf("\n");
}

int main(int argc, char** argv)
{
    uint32_t stuff = 0xdeadbeef;
    printBytes((void*)&stuff, sizeof(stuff));
    for(int i = 0; i < sizeof(stuff) * 8; i++)
    {
	stuff = stuff >> 1;
	printBytes((void*)&stuff, sizeof(stuff));
    }
    return 0;
}
