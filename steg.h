#include <stdint.h>
#include <stdbool.h>

uint64_t ensteg(void* carrier, uint64_t cSize, uint64_t cCount,
		bool littleEndian, uint64_t safeBits,
		void* secret, uint64_t sSize);

uint64_t desteg(void* carrier, uint64_t cSize, uint64_t cCount,
		bool littleEndian, uint64_t safeBits,
		void** secret, uint64_t* sSize);
