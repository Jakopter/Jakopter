/* Jakopter
 * Copyright © 2014 - 2015 Thibaud Hulin, Hector Labanca, Jérémy Yziquel
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

/** NCurses interface for keyboard handling */

#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <fcntl.h>
#include <unistd.h>

#define CMDFILENAME "/tmp/jakopter_user_input.sock"

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

		if (c == '\t') {
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
