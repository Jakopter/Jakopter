/* Jakopter
 * Copyright © 2014 - 2015 Hector Labanca
 * Copyright © 2015 ALF@INRIA
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "video_dump.h"
#include "video.h"

/*file descriptor pointing to a location where we will dump raw decoded frames.*/
static int outfd;
/*how many frames at most do we want to dump to this FD ? -1 = unlimited*/
static int nbFramesToDump;
/*when we call the dump function for the first time, we may need to initialize some stuff.
Use this boolean to check.*/
static int isInitialized = 0;


int jakopter_dumpFrameToFile(uint8_t* buffer, int width, int height, int size)
{
	//NULL buffer = end of stream = clean our stuff
	if (buffer == NULL) {
		close(outfd);
		isInitialized = 0;
		return 0;
	}

	//first time called ? Open the output file descriptor.
	if (!isInitialized) {
		outfd = open(JAKO_FRAMEDUMP_FILENAME, O_WRONLY | O_TRUNC | O_CREAT, 0666);
		if (outfd < 0) {
			perror("[~] Framedump : Failed to open output file");
			return -1;
		}
		nbFramesToDump = JAKO_FRAMEDUMP_COUNT;
		isInitialized = 1;
		if (nbFramesToDump < 0) {
			printf("[*] Dumping frames to file %s...\n", JAKO_FRAMEDUMP_FILENAME);
			printf("[!] the frame count is unlimited ! The file is going to grow big real fast !\n");
		}
		else
			printf("[*] Dumping %d frames to file %s...\n", nbFramesToDump, JAKO_FRAMEDUMP_FILENAME);
	}

	//dump the frame into the file
	if (nbFramesToDump != 0) {
		write(outfd, buffer, size);
		nbFramesToDump--;
	}
	else {
		printf("[*] We're done dumping the stream\n");
		video_set_stopped();
	}

	return 0;
}
