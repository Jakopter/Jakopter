#include "video_process.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

//file descriptor pointing to a location where we will dump raw decoded frames.
static int outfd;
//how many frames at most do we want to dump to this FD ? -1 = unlimited
static int nbFramesToDump;
/*when we call the dump function for the first time, we may need to initialize some stuff.
Use this boolean to check.*/
static int isInitialized = 0;


int jako_dumpFrameToFile(uint8_t* buffer, int width, int height, int size) {
	//NULL buffer = end of stream = clean our stuff
	if(buffer == NULL) {
		close(outfd);
		isInitialized = 0;
		return 0;
	}
	
	//first time called ? Open the output file descriptor. 
	if(!isInitialized) {
		outfd = open(JAKO_FRAMEDUMP_FILENAME, O_WRONLY | O_TRUNC | O_CREAT, 0666);
		if(outfd < 0) {
			perror("Error Framedump : Failed to open output file");
			return -1;
		}
		nbFramesToDump = JAKO_FRAMEDUMP_COUNT;
		isInitialized = 1;
	}
	
	//dump the frame into the file
	if(nbFramesToDump != 0) {
		write(outfd, buffer, size);
		nbFramesToDump--;
	}
	
	return 0;
}
