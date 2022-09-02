#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "wav_player.h"

#define BUFF_SIZE 2048
// If using the shared library, don't define CNFA_IMPLEMENTATION 
// (it's already in the library).
#ifndef USE_SHARED
#define CNFA_IMPLEMENTATION
#endif
#include "CNFA/CNFA.h"

#define RUNTIME 5000

double omega = 0;
int totalframesr = 0;
int totalframesp = 0;
int16_t buff[BUFF_SIZE];

FILE* wav_file;
WaveHeaderChunk hdr;
struct CNFADriver * cnfa;

int is_done;

void Callback( struct CNFADriver * sd, short * out, short * in, int framesp, int framesr )
{
	int* is_done_ptr = (int*) sd->opaque;
	int sample_buff_sz = framesp * sd->channelsPlay;

	// if we have already ended the file, then clear the buffer and exit function
	if(*is_done_ptr) {
		memset(out, 0, sample_buff_sz);
		return;
	}

	totalframesr += framesr;
	totalframesp += framesp;


	int br = readData(wav_file, &hdr, out, sample_buff_sz);

	// end of file
	if (br < 0) {
		printf("End of wave file: setting flag\n");
		*is_done_ptr = 1;
	}
}

int main (void) {
	//const char* filename = "no8_aleg.wav";
	const char* filename = "min&trio.wav";

	wav_file = fopen(filename, "r");

	printInfo(wav_file);
	printf("\n\n");

	fclose(wav_file);
	wav_file = fopen(filename, "r");

	printf("loading file\n");
	loadHeader(wav_file, &hdr);

	printf("playing file\n");
	int br = 0;

	is_done = 0;

	cnfa = CNFAInit( 

		"PULSE",//You can select a plaback driver, or use 0 for default.
		//0, //default
		"cnfa_example", Callback, 
		44100, //Requested samplerate for playback
		44100, //Requested samplerate for record
		2, //Number of playback channels.
		2, //Number of record channels.
		1024, //Buffer size in frames.
		0, //Could be a string, for the selected input device - but 0 means default.
		0,  //Could be a string, for the selected output device - but 0 means default.
		&is_done // pass an integer as an "opaque" object so that CNFA can close
	 );

	while (!is_done){
		sleep(1);
	}

	CNFAClose(cnfa);

	printf( "Received %d (%d per sec) frames\nSent %d (%d per sec) frames\n", totalframesr, totalframesr/RUNTIME, totalframesp, totalframesp/RUNTIME );

    return 0;
}