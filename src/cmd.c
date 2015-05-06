#include <stdio.h>
#include <stdlib.h>
#include <curses.h>

#define CMDFILENAME "/tmp/jakopter_cmd.txt"

int main(){
	char c;
	int i = 100;
	FILE *cmd;
	initscr();

	while (1) {
		c = getch();

		if (c == 's') {
			endwin();
			return 0;
		}

		cmd = fopen(CMDFILENAME, "w");
		fprintf(cmd, "%c\n", (int) c);
		fclose(cmd);
	}

	i--;

	if (i <0) {
		endwin();
		return 0;
	}

	return 0;
}
