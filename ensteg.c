#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <sndfile.h>
#include "steg.h"

typedef struct DataMode DataMode;
struct DataMode
{
    uint64_t old_subtype;
    uint64_t new_subtype;
    sf_count_t(*read)(SNDFILE*,void*,sf_count_t);
    sf_count_t(*write)(SNDFILE*,void*,sf_count_t);
    size_t size;
    uint64_t stegBits;
};

const uint64_t subtypeMask = 0x7;
const uint64_t nModes = 7;
DataMode modeMap[nModes] =
{
    {SF_FORMAT_PCM_U8, SF_FORMAT_PCM_32,
     sf_read_int, sf_write_int, sizeof(int), 24},
    {SF_FORMAT_PCM_16, SF_FORMAT_PCM_32,
     sf_read_int, sf_write_int, sizeof(int), 24},
    {SF_FORMAT_PCM_24, SF_FORMAT_PCM_32,
     sf_read_int, sf_write_int, sizeof(int), 24},
    {SF_FORMAT_PCM_32, SF_FORMAT_PCM_32,
     sf_read_int, sf_write_int, sizeof(int), 24},
    {SF_FORMAT_PCM_S8, SF_FORMAT_PCM_32,
     sf_read_int, sf_write_int, sizeof(int), 24},
    {SF_FORMAT_FLOAT, SF_FORMAT_DOUBLE,
     sf_read_double, sf_write_double, sizeof(double), 48},
    {SF_FORMAT_DOUBLE, SF_FORMAT_DOUBLE,
     sf_read_double, sf_write_double, sizeof(double), 48}
};

int main(int argc, char** argv)
{
    // arguments
    if(argc < 4)
    {
	fprintf(stderr, "usage: ensteg <input.wav> <secret> <output.wav>\n");
	exit(1);
    }
    char* inputFilename = argv[1];
    char* secretFilename = argv[2];
    char* outputFilename = argv[3];

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
	if(toCheck->old_subtype == subtype)
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

    // load the secret text
    FILE* secretFile = fopen(secretFilename, "r");
    if(!secretFile)
    {
	fprintf(stderr, "couldn't open file %s\n", secretFilename);
	exit(1);
    }
    if(fseek(secretFile, 0, SEEK_END))
    {
	fprintf(stderr, "failed to seek end of file %s\n", secretFilename);
	fclose(secretFile);
	exit(1);
    }
    long int secretLen = ftell(secretFile);
    if(secretLen < 0)
    {
	fprintf(stderr, "failed to get length of file %s\n", secretFilename);
	fclose(secretFile);
	exit(1);
    }
    uint8_t* secret = (uint8_t*)malloc(1 + sizeof(uint8_t) * secretLen);

    rewind(secretFile);
    success = fread((void*)secret, sizeof(uint8_t), secretLen, secretFile);
    fclose(secretFile);

    if(success != secretLen)
    {
	fprintf(stderr, "short read on file %s: %lld/%ld items\n",
		secretFilename, success, secretLen);
	exit(1);
    }

    // stegasaurus
    ensteg((void*)data, mode->size, nData, true,
	   mode->stegBits, (void*)secret, secretLen);
    free(secret);

    // correct our format settings to increase data size
    inputInfo.format &= ~subtypeMask;
    inputInfo.format |= mode->new_subtype;

    // write to the output file with corrected settings
    SNDFILE* outputFile = sf_open(outputFilename, SFM_WRITE, &inputInfo);
    if(!outputFile)
    {
	fprintf(stderr, "couldn't open file %s\n", outputFilename);
	exit(1);
    }
    success = mode->write(outputFile, data, nData);
    if(success != nData)
    {
	fprintf(stderr, "short write to file %s: %lld/%lld items\n",
		outputFilename, success, nData);
	sf_close(outputFile);
	exit(1);
    }
    free(data);
    if(errcode = sf_close(outputFile))
    {
	fprintf(stderr, "error closing file %s: code=%d\n",
		outputFilename, errcode);
	exit(1);
    }

    return 0;
}
