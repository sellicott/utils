#ifndef _WAV_PLAYER_H_
#define _WAV_PLAYER_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wavDefs.h"

int loadHeader(FILE *file, WaveHeaderChunk *hdr);
int loadInfo(FILE *file, WaveHeaderChunk *hdr);
int printInfo(FILE *file);
void freeInfo(WaveHeaderChunk *hdr);
int readData(FILE *file, WaveHeaderChunk *hdr, void* buff, int buff_len);

#endif