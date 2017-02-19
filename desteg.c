#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sndfile.h>
#include "steg.h"

typedef struct DataMode DataMode;
struct DataMode
{
    uint64_t subtype;
    sf_count_t(*read)(SNDFILE*,void*,sf_count_t);
    size_t size;
    uint64_t stegBits;
};

const uint64_t subtypeMask = 0x7;
#define nModes 7
DataMode modeMap[nModes] =
{
    {SF_FORMAT_PCM_16, sf_read_short, sizeof(short), 12},
    {SF_FORMAT_PCM_32, sf_read_int, sizeof(int), 24},
    {SF_FORMAT_FLOAT, sf_read_float, sizeof(double), 24},
    {SF_FORMAT_DOUBLE, sf_read_double, sizeof(double), 48}
};

int main(int argc, char** argv)
{
    // arguments
    if(argc < 3)
    {
	fprintf(stderr, "usage: desteg <input.wav> <secret>\n");
	exit(1);
    }
    char* inputFilename = argv[1];
    char* secretFilename = argv[2];

    // open the file and get format information
    SF_INFO inputInfo;
    SNDFILE* inputFile = sf_open(inputFilename, SFM_READ, &inputInfo);
    if(!inputFile)
    {
	fprintf(stderr, "couldn't open file %s\n", inputFilename);
	exit(1);
    }

    // find out what mode to use for this input file
    uint64_t subtype = inputInfo.format & subtypeMask;
    DataMode* mode = NULL;
    for(int i = 0; i < nModes; i++)
    {
	DataMode* toCheck = &(modeMap[i]);
	if(toCheck->subtype == subtype)
	{
	    mode = toCheck;
	    break;
	}
    }
    if(mode == NULL)
    {
	fprintf(stderr, "couldn't get data mode for subtype %llu\n", subtype);
	sf_close(inputFile);
	exit(1);
    }

    // read the audio data and close the file
    sf_count_t nData = inputInfo.frames * inputInfo.channels;
    void* data = malloc(mode->size * nData);
    sf_count_t success = mode->read(inputFile, data, nData);
    if(success < nData)
    {
	fprintf(stderr, "short read from file %s: %lld/%lld items\n",
		inputFilename, success, nData);
	sf_close(inputFile);
	exit(1);
    }
    int errcode;
    if(errcode = sf_close(inputFile))
    {
	fprintf(stderr, "error closing file %s: code=%d\n",
		inputFilename, errcode);
	exit(1);
    }

    // stegasaurus
    void* secret = NULL;
    uint64_t secretLen = 0;
    desteg((void*)data, mode->size, nData, true,
	   mode->stegBits, &secret, &secretLen);
    if(secret == NULL)
    {
	fprintf(stderr, "received NULL data from desteg\n");
	exit(1);
    }

    FILE* secretFile = fopen(secretFilename, "w");
    if(!secretFile)
    {
	fprintf(stderr, "couldn't open file %s\n", secretFilename);
	exit(1);
    }

    success = fwrite(secret, sizeof(uint8_t), secretLen, secretFile);
    fclose(secretFile);
    free(secret);
    if(success != secretLen)
    {
	fprintf(stderr, "short write to %s: %llu/%llu bytes\n",
		secretFilename, success, secretLen);
	exit(1);
    }

    return 0;
}
