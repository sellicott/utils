#include "wav_player.h"

static WaveHeaderChunk wav_data;

int printInfo(FILE *file){
	//clear pointers
	wav_data.info.creation_date=0;
	wav_data.info.genre=0;
	wav_data.info.artist=0;
	wav_data.info.title=0;
	
	if(loadHeader(file, &wav_data)!=0){
		printf("file invalid\n\r");
		return 1;
	}
	
	if(loadInfo(file, &wav_data)==0){//if there is data show it
		printf("Track name: %s \n\r", wav_data.info.title);//print data
		printf("Artist: %s \n\r", wav_data.info.artist);//print data
		printf("Genre: %s \n\r", wav_data.info.genre);//print data
		printf("Creation date: %s \n\r", wav_data.info.creation_date);//print data
	}
	

	//print data
	printf("\n");
	printf("Audio Format: %i \n", wav_data.fmt.audio_format);
	printf("Channels: %u \n", wav_data.fmt.num_channels);
	printf("Sample Rate: %lu \n", wav_data.fmt.sample_rate);
	printf("Block Alignment (bytes): %u \n", wav_data.fmt.block_allign);
	printf("Bits-per-Sample: %u \n", wav_data.fmt.bits_per_sample);

	printf("size of Wave Header %i bytes \n", sizeof(WaveHeaderChunk));

	//free the info tag heap data
	if(wav_data.info.title){
		free(wav_data.info.title);
		wav_data.info.title=NULL;
		
	}
	
	if(wav_data.info.artist){
		free(wav_data.info.artist);
		wav_data.info.artist=NULL;
	}
	
	if(wav_data.info.genre){
		free(wav_data.info.genre);
		wav_data.info.genre=NULL;
	}
	
	if(wav_data.info.creation_date){
		free(wav_data.info.creation_date);
		wav_data.info.creation_date=NULL;
	}
	
	return 0;
}

int loadHeader(FILE *file, WaveHeaderChunk *hdr){
	char chunk_id[5];
	uint32_t chunk_offset=0;//keeps track of position in file - referenced to the chunk id
	uint32_t chunk_len=0;//the length of the current chunk
	uint32_t br;

	//check if the file is valid first
	if(file==NULL){
		printf("Could not open file \n");
		return 1;
	}
	//look for RIFF file header
	fseek(file, CHUNK_ID, SEEK_SET);
	fgets(chunk_id, 5, file);
	if(strncmp(chunk_id, "RIFF", 4)!=0){
		printf("File is not a wav file\n");
		return 1;
	}
	
	//check if WAVE file
	fseek(file, FORMAT, SEEK_SET);
	fgets(chunk_id, 5, file);
	if(strncmp(chunk_id, "WAVE", 4)!=0){
		printf("File is not a wav file \n");
		return 1;		
	}
	
	chunk_offset+=CHUNK_DATA+4;//add the file offset of the begining of the first chunk
	
	//must be WAVE look for chunks
	do{
		//get the chunk id
		fseek(file, chunk_offset+CHUNK_ID, SEEK_SET);
		fgets(chunk_id, 5, file);

		//get the chunk length
		fseek(file, chunk_offset+CHUNK_SIZE, SEEK_SET);
		br = fread(&chunk_len, 4, 1, file);//get length
	
		//printf("%s \n", chunk_id);//print the chunk id

		//check for fmt chunk

		if(strncmp(chunk_id, "fmt ", 4)==0){//if format section
	
			//get the format section of the file
			fseek(file, chunk_offset+AUDIO_FORMAT, SEEK_SET);
			br = fread(&hdr->fmt.audio_format, 2, 1, file);
			
			//number of channels
			fseek(file, chunk_offset+NUM_CHANNELS, SEEK_SET);
			br = fread(&hdr->fmt.num_channels, 1, 1, file);

			//sample rate
			fseek(file, chunk_offset+SAMPLE_RATE, SEEK_SET);
			br = fread(&hdr->fmt.sample_rate, 2, 1, file);

			//bits per channel tells if mono, stero, or hifi stereo
			fseek(file, chunk_offset+BLOCK_ALLIGN, SEEK_SET);
			br = fread(&hdr->fmt.block_allign, 1, 1, file);

			//bits per channel 8 or 16
			fseek(file, chunk_offset+BITS_PER_SAMPLE, SEEK_SET);
			br = fread(&hdr->fmt.bits_per_sample, 1, 1, file);

			hdr->fmt.bytes_per_sample=hdr->fmt.bits_per_sample/8;
		}
		else if(strncmp(chunk_id, "data", 4)==0){//if chunk is of data type

			//grab required hdr
			hdr->data.data_offset=chunk_offset;
			hdr->data.data_size=chunk_len;
			hdr->data.num_samples=(uint16_t) chunk_len/hdr->fmt.bytes_per_sample;
			hdr->data.samples_left=hdr->data.num_samples;
			hdr->data.current_offset=chunk_offset+CHUNK_DATA;

		}
		else if(strncmp(chunk_id, "LIST", 4)==0){//if chunk is of LIST type
	
			//check if it is of info type
			fseek(file, chunk_offset+CHUNK_DATA, SEEK_SET);
			fgets(chunk_id, 5, file);
			if(strncmp(chunk_id, "INFO", 4)==0){
				hdr->info.is_info=1;//set info flag
		
				//set the info offset from the chunk offset
				hdr->info.info_offset=chunk_offset+4;
				//set the chunk lenght
				hdr->info.info_len=chunk_len;
			}
			else{
				hdr->info.is_info=0;//clear info flag
			}	

		}
		else if(strncmp(chunk_id, "data", 4)!=0){
			//get length of chunk
			fseek(file, chunk_offset+CHUNK_SIZE, SEEK_SET);
			br = fread(&chunk_len, 2, 1, file);//get length

		}

		//done reading skip chunk
		chunk_offset=chunk_offset+chunk_len+CHUNK_DATA;
		fseek(file, chunk_offset, SEEK_SET);
		
	} while(fgets(chunk_id, 5, file)!=NULL);//loop until the end of the file is reached

	return 0;
}


/*
 * reads the info chunk if availible and puts data into the WaveHeader structure provided
 */
int loadInfo(FILE *file, WaveHeaderChunk *hdr){
	char chunk_id[5];
	uint32_t chunk_offset=hdr->info.info_offset+CHUNK_DATA;//current file position
	uint32_t chunk_len=0;//the length of the current chunk
	uint32_t tag_len=0;
	uint32_t br;

	if(hdr->info.is_info==0){//if no data
		printf("No artist information is avilible for this wav file \n");
		return 1;//no data
	}
	
	fseek(file, chunk_offset, SEEK_SET);//go to chunk data

	while(chunk_offset-4 - hdr->info.info_offset < hdr->info.info_len){//while in info chunk
		
		//get the chunk id
		fseek(file, chunk_offset+CHUNK_ID, SEEK_SET);
		fgets(chunk_id, 5, file);

		//get the chunk length
		fseek(file, chunk_offset+CHUNK_SIZE, SEEK_SET);
		br = fread(&chunk_len, sizeof(int), 1, file);//get length

		//make sure the length of the tag isn't to big
		if(chunk_len > MAX_TAG_SIZE){//if too big
				tag_len=MAX_TAG_SIZE;
		}
		else{
			tag_len=chunk_len;
		}
		
		//go to tag information	
		fseek(file, chunk_offset+CHUNK_DATA, SEEK_SET);

		if(strncmp(chunk_id, "INAM", 4)==0){//if is name chunk
			
			hdr->info.title=malloc(tag_len+1);//allocate memory for tag
			 fgets(hdr->info.title, tag_len, file);//read tag
			
		}
		else if(strncmp(chunk_id, "IART", 4)==0){//if is artist chunk

			hdr->info.artist=malloc(tag_len+1);//allocate memory for tag
			 fgets(hdr->info.artist, tag_len, file);//read tag
			
		}
		else if(strncmp(chunk_id, "IGNR", 4)==0){//if is genre chunk

			hdr->info.genre=malloc(tag_len+1);//allocate memory for tag
			 fgets(hdr->info.genre, tag_len, file);//read tag
			
		}
		else if(strncmp(chunk_id, "ICRD", 4)==0){//if is creation date chunk

			hdr->info.creation_date=malloc(tag_len+1);//allocate memory for tag
			 fgets(hdr->info.creation_date, tag_len, file);//read tag
			
		}
		
		//done reading skip chunk
		chunk_offset=chunk_offset+chunk_len+CHUNK_DATA;
		fseek(file, chunk_offset, SEEK_SET);
	}

	return 0;
	 
}

int readData(FILE *file, WaveHeaderChunk *hdr, void* buff, int buff_len){
	uint32_t br;
	int bytes_to_read = hdr->fmt.bytes_per_sample*buff_len;

	if(!hdr) {
		printf("Error: No valid header\n");
		return -1;
	}

	fseek(file, hdr->data.current_offset, SEEK_SET);
	br = fread(buff, hdr->fmt.bytes_per_sample, buff_len, file);


	int bytes_read = hdr->fmt.bytes_per_sample*br;

	//printf("samples read: %u\n", br);

	hdr->data.samples_left -= br;
	hdr->data.current_offset += bytes_read;

	if(feof(file)){
		br = -1;
	}

	return br;
}
