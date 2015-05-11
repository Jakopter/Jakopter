// #include <ncurses.h>
// #include <string.h>

// #define WIDTH 30
// #define HEIGHT 10

// int startx = 0;
// int starty = 0;

// char *choices[] = { 	"Choice 1",
// 			"Choice 2",
// 			"Choice 3",
// 			"Choice 4",
// 			"Exit",
// 		  };

// int n_choices = sizeof(choices) / sizeof(char *);

// void print_menu(WINDOW *menu_win, int highlight);
// void report_choice(int mouse_x, int mouse_y, int *p_choice);

// int main()
// {	int c, choice = 0;
// 	WINDOW *menu_win;
// 	MEVENT event;

// 	/* Initialize curses */
// 	initscr();
// 	clear();
// 	noecho();
// 	cbreak();	//Line buffering disabled. pass on everything
// 	mousemask(ALL_MOUSE_EVENTS, NULL);

// 	/* Try to put the window in the middle of screen */
// 	startx = (80 - WIDTH) / 2;
// 	starty = (24 - HEIGHT) / 2;

// 	attron(A_REVERSE);
// 	mvprintw(23, 1, "Click on Exit to quit (Works best in a virtual console)");
// 	refresh();
// 	attroff(A_REVERSE);

// 	/* Print the menu for the first time */
// 	menu_win = newwin(HEIGHT, WIDTH, starty, startx);
// 	print_menu(menu_win, 1);
// 	/* Get all the mouse events */

// 	while(1)
// 	{
// 		keypad(stdscr, TRUE);
// 		c = getch();
// 		switch(c)
// 		{	case KEY_MOUSE:
// 			if(getmouse(&event) == OK)
// 			{	 When the user clicks left mouse button
// 				if(event.bstate & BUTTON1_CLICKED)
// 				{
// 					report_choice(event.x + 1, event.y + 1, &choice);
// 					if(choice == -1) //Exit chosen
// 						goto end;
// 					mvprintw(22, 1, "Choice made is : %d String Chosen is \"%10s\"", choice, choices[choice - 1]);
// 					refresh();
// 				}
// 			}
// 			print_menu(menu_win, choice);
// 			break;
// 		}
// 	}
// end:
// 	endwin();
// 	return 0;
// }


// void print_menu(WINDOW *menu_win, int highlight)
// {
// 	int x, y, i;

// 	x = 2;
// 	y = 2;
// 	box(menu_win, 0, 0);
// 	for(i = 0; i < n_choices; ++i)
// 	{	if(highlight == i + 1)
// 		{	wattron(menu_win, A_REVERSE);
// 			mvwprintw(menu_win, y, x, "%s", choices[i]);
// 			wattroff(menu_win, A_REVERSE);
// 		}
// 		else
// 			mvwprintw(menu_win, y, x, "%s", choices[i]);
// 		++y;
// 	}
// 	wrefresh(menu_win);
// }

// /* Report the choice according to mouse position */
// void report_choice(int mouse_x, int mouse_y, int *p_choice)
// {	int i,j, choice;

// 	i = startx + 2;
// 	j = starty + 3;

// 	for(choice = 0; choice < n_choices; ++choice)
// 		if(mouse_y == j + choice && mouse_x >= i && mouse_x <= i + strlen(choices[choice]))
// 		{	if(choice == n_choices - 1)
// 				*p_choice = -1;
// 			else
// 				*p_choice = choice + 1;
// 			break;
// 		}
// }

#include <stdio.h>
#include <stdlib.h>
#include <curses.h>

#define CMDFILENAME "/tmp/jakopter_cmd.txt"

int main()
{

    // int ch;

    // initscr();
    // noecho();
    // cbreak();
    // refresh();
    // mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

    // while(1)
    // {
    //     ch = getch();
    //     addch(ch);
    // };

    // return 0;
	MEVENT event;
	int c;
	int last_x = event.x;
	int last_y = event.y;
	FILE *cmd;

	initscr();
	clear();
	noecho();
	cbreak();

	mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);

	while (1) {
		keypad(stdscr, TRUE);
		c = getch();
		switch (c)
		{
		case KEY_MOUSE:
			printf("mouse\n");
			if (getmouse(&event) == OK) {
				if (event.bstate & BUTTON2_CLICKED) {
					c = 'a';
					printf("Button3\n");
				}
				else if (event.bstate & BUTTON3_CLICKED) {
					c = 'd';
					printf("Button2\n");
				}
				else if (event.bstate & BUTTON1_CLICKED) {
					c = 'u';
					printf("Button1\n");
				}
			}
			break;
		case 'm':
			if (getmouse(&event) == OK) {
				if (last_x < event.x) {
					printf("deplacement gauche\n");
					c = 67;
				}
				else if (last_x > event.x) {
					printf("deplacement droite\n");
					c = 68;
				}
				else if (last_y < event.y) {
					printf("deplacement avant\n");
					c = 102;
				}
				else if (last_y > event.y) {
					printf("deplacement apres\n");
					c = 98;
				}
			}
			break;
		case 's':
				endwin();
				return 0;
		}

		cmd = fopen(CMDFILENAME, "w");
		fprintf(cmd, "%c\n", (int) c);
		fclose(cmd);
	}

	return 0;
}