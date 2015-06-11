/** NCurses interface for keyboard handling */

#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <fcntl.h>
#include <unistd.h>

#define CMDFILENAME "/tmp/jakopter_cmd.txt"

int main(){
	char c;
	FILE *cmd;
	initscr();
	// if (mkfifo(CMDFILENAME, 0644) < 0) {
	// 	perror("mkfifo failed()");
	// 	return -1;
	// }

	// int fdpipe = open(CMDFILENAME, O_WRONLY|O_NONBLOCK);
	// if (fdpipe < 0) {
	// 	perror("Jakopter not listening");
	// 	return -1;
	// }
	while (1) {
		c = getch();

		if (c == 's') {
			endwin();
			// close(fdpipe);
			return 0;
		}

		// fprintf(fdpipe, "%c\n", (int) c);

		cmd = fopen(CMDFILENAME, "w");
		fprintf(cmd, "%c\n", (int) c);
		fclose(cmd);
	}

	// close(fdpipe);
	unlink(CMDFILENAME);

	return 0;
}
