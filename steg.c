#include <stdlib.h>
#include <stdio.h>

#include "steg.h"

const static uint8_t masks[9] =
{
    0x00,
    0x80,
    0xc0,
    0xe0,
    0xf0,
    0xf8,
    0xfc,
    0xfe,
    0xff
};

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

static void printUnknown(uint8_t byte)
{
    if(byte > 60) printf("%c", byte);
    else printf("%02x", byte);
}

static uint8_t maskCopy(uint8_t src, uint8_t dst,
			uint8_t srcIdx, uint8_t dstIdx, uint8_t count)
{
    /*
    printf("maskCopy(");
    printByteBits(src);
    printf(", ");
    printByteBits(dst);
    printf(", %u, %u, %u)\n", srcIdx, dstIdx, count);
    */
    
    // get a mask for the first 'count' bits
    uint8_t mask = masks[count];
    /*
    printf("mask = ");
    printByteBits(mask);
    printf("\n");
    */
    // clear the target bits to zeros
    dst &= ~(mask >> dstIdx);
    /*
    printf("cleared = ");
    printByteBits(dst);
    printf("\n");
    */
    // get the override bits
    uint8_t override = (src & (mask >> srcIdx)) << srcIdx;
    /*
    printf("override = ");
    printByteBits(override);
    printf("\n");
    */
    // apply the override bits
    dst |= (override >> dstIdx);

    /*
    printf("-> ");
    printByteBits(dst);
    printf("\n");
    fflush(stdout);
    */

    return dst;
}

uint64_t ensteg_raw(void* carrier, uint64_t cSize, uint64_t cCount,
		    bool littleEndian, uint64_t safeBits,
		    void* secret, uint64_t sSize)
{
    uint64_t eByteIdxStart = littleEndian ? 0 : cCount - 1;
    uint64_t eByteIdxDelta = littleEndian ? 1 : -1;
    uint64_t sByteIdx = 0;
    uint8_t sBitIdx = 0;

    for(uint64_t elemIdx = 0; elemIdx < cCount; elemIdx++)
    {
	uint64_t eBitsProcessed = 0;
	uint8_t* element = ((uint8_t*)carrier) + (elemIdx * cSize);

	for(uint64_t eByteIdx = eByteIdxStart;
	    eByteIdx <= cSize && eBitsProcessed < safeBits;
	    eByteIdx += eByteIdxDelta)
	{
	    /*
	    printf("writing new byte from element\n");
	    fflush(stdout);
	    */
	    uint64_t eBitsLeft = safeBits - eBitsProcessed;
	    uint8_t bBitsLeft = eBitsLeft < 8 ? eBitsLeft : 8;
	    uint8_t src = ((uint8_t*)secret)[sByteIdx];
	    uint8_t dst = element[eByteIdx];
	    
	    while(bBitsLeft > 0)
	    {
		uint8_t sBitsLeft = 8 - sBitIdx;
		uint8_t nCopy =
		    bBitsLeft < sBitsLeft ?
		    bBitsLeft : sBitsLeft;

		dst = maskCopy(src, dst, sBitIdx, 8 - bBitsLeft, nCopy);
		element[eByteIdx] = dst;

		bBitsLeft -= nCopy;
		sBitIdx += nCopy;
		eBitsProcessed += nCopy;
		
		if(sBitIdx >= 8)
		{
		    /*
		    printf("switching bytes in secret\n");
		    fflush(stdout);
		    */
		    sBitIdx = 0;
		    sByteIdx++;
		    src = ((uint8_t*)secret)[sByteIdx];
		    if(sByteIdx >= sSize) break;
		}
	    }
	    if(sByteIdx >= sSize) break;
	}
	if(sByteIdx >= sSize) return elemIdx + 1;
    }

    return cCount;
}

uint64_t desteg_raw(void* carrier, uint64_t cSize, uint64_t cCount,
		    bool littleEndian, uint64_t safeBits,
		    void* secret, uint64_t sSize)
{
    uint64_t eByteIdxStart = littleEndian ? 0 : cCount - 1;
    uint64_t eByteIdxDelta = littleEndian ? 1 : -1;
    uint64_t sByteIdx = 0;
    uint8_t sBitIdx = 0;

    for(uint64_t elemIdx = 0; elemIdx < cCount; elemIdx++)
    {
	uint64_t eBitsProcessed = 0;
	uint8_t* element = ((uint8_t*)carrier) + (elemIdx * cSize);

	for(uint64_t eByteIdx = eByteIdxStart;
	    eByteIdx <= cSize && eBitsProcessed < safeBits;
	    eByteIdx += eByteIdxDelta)
	{
	    uint64_t eBitsLeft = safeBits - eBitsProcessed;
	    uint8_t bBitsLeft = eBitsLeft < 8 ? eBitsLeft : 8;
	    uint8_t dst = ((uint8_t*)secret)[sByteIdx];
	    uint8_t src = element[eByteIdx];
	    
	    while(bBitsLeft > 0)
	    {
		uint8_t sBitsLeft = 8 - sBitIdx;
		uint8_t nCopy =
		    bBitsLeft < sBitsLeft ?
		    bBitsLeft : sBitsLeft;
		
		dst = maskCopy(src, dst, 8 - bBitsLeft, sBitIdx, nCopy);
		((uint8_t*)secret)[sByteIdx] = dst;

		bBitsLeft -= nCopy;
		sBitIdx += nCopy;
		eBitsProcessed += nCopy;
		
		if(sBitIdx >= 8)
		{
		    sBitIdx = 0;
		    sByteIdx++;
		    dst = ((uint8_t*)secret)[sByteIdx];
		    if(sByteIdx >= sSize) break;
		}
	    }
	    if(sByteIdx >= sSize) break;
	}
	if(sByteIdx >= sSize) return elemIdx + 1;
    }

    return cCount;
}

uint64_t ensteg(void* carrier, uint64_t cSize, uint64_t cCount,
		bool littleEndian, uint64_t safeBits,
		void* secret, uint64_t sSize)
{
    // ensteg the length of this chunk
    uint64_t offset = ensteg_raw(carrier, cSize, cCount,
				 littleEndian, safeBits,
				 (void*)(&sSize), sizeof(uint64_t));

    uint8_t* subcarrier = ((uint8_t*)carrier) + (offset * cSize);
    
    // ensteg the chunk itself
    return ensteg_raw((void*)subcarrier,
		      cSize, cCount - offset,
		      littleEndian, safeBits,
		      secret, sSize) + offset;
}

uint64_t desteg(void* carrier, uint64_t cSize, uint64_t cCount,
		bool littleEndian, uint64_t safeBits,
		void** secret, uint64_t* sSize)
{
    // desteg the length of this chunk
    uint64_t size;
    uint64_t offset = desteg_raw(carrier, cSize, cCount,
				 littleEndian, safeBits,
				 (void*)(&size), sizeof(uint64_t));

    *sSize = size;
    void* rawsec = malloc(sizeof(uint8_t) * size);
    uint8_t* subcarrier = ((uint8_t*)carrier) + (offset * cSize);

    // desteg the chunk itself
    uint64_t result =  desteg_raw((void*)subcarrier,
				  cSize, cCount - offset,
				  littleEndian, safeBits,
				  rawsec, size) + offset;

    *secret = rawsec;
    
    return result;
}
