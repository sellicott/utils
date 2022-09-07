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

int totalframesr = 0;
int totalframesp = 0;

FILE* wav_file;
WaveHeaderChunk hdr;
struct CNFADriver * cnfa;
short buff[BUFF_SIZE];

int is_done;

void Callback( struct CNFADriver * sd, short * out, short * in, int framesp, int framesr )
{
	int* is_done_ptr = (int*) sd->opaque;
	const int output_channels = sd->channelsPlay;
	const int file_channels = hdr.fmt.num_channels;
	const int output_buff_sz = framesp * output_channels;
	int br = 0;

	// if we have already ended the file, then clear the buffer and exit function
	if(*is_done_ptr) {
		memset(out, 0, sizeof(short) * output_buff_sz);
		return;
	}

	totalframesr += framesr;
	totalframesp += framesp;

	if (output_channels == file_channels) {
		int read_buff_sz = framesp * sd->channelsPlay;
		br = readData(wav_file, &hdr, out, read_buff_sz);
	}
	else if (output_channels > file_channels) {
		int read_buff_sz = framesp;

		// exit loop if we are done filling the output buffer or we are at the end of file
		int samples_remaining = read_buff_sz;
		while (samples_remaining > 0 && br >= 0) {
			int read_sz = (samples_remaining > BUFF_SIZE) ? BUFF_SIZE : samples_remaining;
			br = readData(wav_file, &hdr, buff, read_sz);
			// duplicate data on left and right channels
			for (int i = 0; i < read_buff_sz; ++i){
				out[2*i]   = buff[i];
				out[2*i+1] = buff[i];
			}
			samples_remaining -= br;
		}
		
	}
	else {
		printf("what are you doing? mono sound output?\n");
	}

	// end of file
	if (br < 0) {
		printf("End of wave file: setting flag\n");
		*is_done_ptr = 1;
	}
}

int main (int nargs, char** args) {
	//const char* filename = "no8_aleg.wav";
	char* filename = "min&trio.wav";

	// if there is a file given on the command line play it.
	if(nargs >= 2) {
		filename = args[1];
	}
	wav_file = fopen(filename, "r");

	printInfo(wav_file);
	printf("\n\n");

	printf("loading file\n");
	loadHeader(wav_file, &hdr);

	printf("playing file\n");

	is_done = 0;
	cnfa = CNFAInit( 
		NULL,                // String, for the driver "PULSE", "WASAPI" (output only) - NULL means default. 
		"cnfa_example",      // Name of program to audio driver
		Callback,            // CNFA callback function handle
		hdr.fmt.sample_rate, // Requested samplerate for playback
		hdr.fmt.sample_rate, // Requested samplerate for record
		2,                   // Number of playback channels.
		2,                   // Number of record channels.
		1024,                // Buffer size in frames.
		NULL,                // String, for the selected input device - NULL means default.
		NULL,                // String, for the selected output device - NULL means default.
		&is_done             // pass an integer as an "opaque" object so that CNFA can close
	);

	int runtime = 0;
	const char* spin_glyph = "-\\|/";
	const char* glyph = spin_glyph;
	int i = 0;
	while (!is_done){
		sleep(1);
		++runtime;
		printf("\r %c ", *glyph++);
		fflush(stdout);
		if (!*glyph) {
			glyph = spin_glyph;
		}
	}

	CNFAClose(cnfa);
	fclose(wav_file);

	printf( "Received %d (%d per sec) frames\nSent %d (%d per sec) frames\n",
		totalframesr, totalframesr/runtime,   // recorded samples, recorded samples/sec
		totalframesp, totalframesp/runtime ); // outputted samples, outputted samples/sec

    return 0;
}