#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <sys/poll.h>

#define CMDFILENAME "/tmp/jakopter_cmd.txt"
#define INPUT_INTERVAL (1/30)*1000

int main(){

	struct pollfd keyb_poll;
	//by default, 'a' = stay/don't move
	char c = 'a';
	FILE *cmd;
	initscr();

	//initialize the poll structure so that it checks keyboard input
	keyb_poll.fd = stdin;
	keyb_poll.events = POLLIN;
	
	while (c != 's') {
		
		//put the input back to "neutral" (no movement)
		c = 'a';
		
		//check keyboard input
		if (poll(&keyb_poll, 1, INPUT_INTERVAL) > 0
			&& keyb_poll.revents & POLLIN) {
			c = getch();
		}
		
		//print the new value to the input file
		cmd = fopen(CMDFILENAME, "w");
		fprintf(cmd, "%c\n", (int) c);
		fclose(cmd);
	}


}
