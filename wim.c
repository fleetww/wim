/*
 *
 *	wim - Will's Immitation Vim
 *
 *	William Fleetwood
 *
 */

/*** includes ***/
#include "proto.h"


/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)
#define MOUSE_EVENT(flag) ((state & flag))
#define MOUSE_FLAGS (BUTTON1_CLICKED | BUTTON4_PRESSED | BUTTON5_PRESSED)
#define TAB_SIZE 2

/*** Data ***/
/*
 *	Represents one line in the open file
 */
typedef struct Line {
	unsigned int len;
	char *text;
} Line;

/*
 *	Represents the open file, line by line
 */
typedef struct FileData {
	unsigned int numLines;
	Line *lines;
} FileData;

/*
 *	Holds general global data like number of open windows
 */
typedef struct GlobalState {
	FileData *data;
	unsigned int maxX;
	unsigned int maxY;
	unsigned int lineOffset;
	unsigned int colOffset;
	unsigned int cursX;
	unsigned int cursY;
	bool dirtyEditor;
} GlobalState;

GlobalState GS;

/*** ncurses management ***/

/*
 *	Makes sure to always end ncurses, otherwise the terminal emulator will
 *	break
 */
void closingTime() {
	endwin();

	return;
}

/*
 *	Probably going to call it a lot, ;)
 */
void fatalErr(const char *str, int eval) {
	perror(str);
	exit(eval);

	return;
}

/*
 *	Sets up the initial environment and allocating starting space
 */
void init() {
	atexit(closingTime);
	putenv("NCURSES_NO_HARD_TABS=false");
	setlocale(LC_ALL, "");
	initscr();
	meta(stdscr, true);
	keypad(stdscr, true);
	raw();
	nonl();
	noecho();
	timeout(0);

	errno = 0;

	if (!(GS.data = malloc(sizeof(FileData)))) {
		fatalErr("malloc", 1);
	}

	GS.data->numLines = 0;
	GS.maxX = 0;
	GS.maxY = 0;
	GS.lineOffset = 0;
	GS.cursX = 0;
	GS.cursY = 0;
	GS.colOffset = 0;
	GS.dirtyEditor = false;

	return;
}

/*** File I/O ***/
void dataAppendLine(char *newtext, size_t length) {
	GS.data->lines = realloc(GS.data->lines, sizeof(Line) * (++GS.data->numLines));
	Line *line = &(GS.data->lines[GS.data->numLines-1]);
	line->len = length;
	line->text = strndup(newtext, length);

	return;
}

void loadFile(char *filename) {
	FILE *file = fopen(filename, "r");
	if (!file) {
		fatalErr("loadFile:fopen", 1);
	}

	char *buf = NULL;
	size_t bufsize = 0;
	ssize_t linelen;

	errno = 0;

	while ((linelen = getline(&buf, &bufsize, file)) != -1) {
		//getline's return doesn't count the 0 terminating byte
		dataAppendLine(buf, linelen);
	}

	if (errno) {
		fatalErr("loadFile:getline", 1);
	}

	free(buf);
	fclose(file);

	return;
}

/*** User I/O ***/

char processInput() {
	int ch = wgetch(stdscr);
	switch (ch) {
		case KEY_MOUSE:
			break;
		case CTRL_KEY('c'):
			return 2;
			break;
		case KEY_NPAGE:
			scrollEditor((int) (GS.maxY/2));
			GS.dirtyEditor = true;
			break;
		case KEY_PPAGE:
			scrollEditor(-1 * (GS.maxY/2));
			GS.dirtyEditor = true;
			break;
		case KEY_UP:
			moveCursor(-1, 0);
			break;
		case KEY_DOWN:
			moveCursor(1, 0);
			break;
		case KEY_LEFT:
			moveCursor(0, -1);
			break;
		case KEY_RIGHT:
			moveCursor(0, 1);
			break;
	}

	return 0;
}

/*** Editor management ***/
void scrollEditor(int amt) {
	unsigned int temp = GS.lineOffset + amt;
	//Underflow
	if (amt < 0 && temp > GS.lineOffset) {
		GS.lineOffset = 0;
	} else if ((amt > 0 && temp < GS.lineOffset) || temp > GS.data->numLines) {
		//overflow or past file
		GS.lineOffset = GS.data->numLines - 1;
		GS.cursY = 1;
	} else {
		GS.lineOffset = temp;
	}
	return;
}

void panEditor(int amt) {

	return;
}

void moveCursor(int dy, int dx) {
	unsigned int newY = GS.cursY + dy;
	unsigned int newX = GS.cursX + dx;
	unsigned int currLine = GS.cursY + GS.lineOffset;
	unsigned int currCol = GS.cursX + GS.colOffset;
	unsigned int newLine = newY + GS.lineOffset;
	unsigned int newCol = newX + GS.colOffset;

	if (dy < 0 && newY > GS.cursY) { //moved cursor above screen
		newY = 0;
		scrollEditor(dy);
		GS.dirtyEditor = true;
	} else if (dy > 0 && newY > GS.maxY) {
		newY = GS.maxY;
		scrollEditor(dy);
		GS.dirtyEditor = true;
	}
	//TODO correct for when cursor is past line



	GS.cursY = newY;
	GS.cursX = newX;
	wmove(stdscr, GS.cursY, GS.cursX);

	return;
}

void moveCursorOld(int dy, int dx) {
	unsigned int newY = GS.cursY + dy;
	unsigned int newX = GS.cursX + dx;
	unsigned int currLine = GS.cursY + GS.lineOffset; //in file
	unsigned int currCol = GS.cursX + GS.colOffset; //in file

	unsigned int newLine = newY + GS.lineOffset;

	if (newLine == GS.data->numLines + 1) { //end of file
		newY = GS.cursY; //Just past last line visually
		newX = 0;
	} else if (dy < 0 && newY > GS.cursY) { //moved cursor above screen
		newY = 0;
		scrollEditor(-1);
		GS.dirtyEditor = true;
	} else if (dy > 0 && newY > GS.maxY) { //moved cursor below screen
		newY = GS.maxY;
		scrollEditor(1);
		GS.dirtyEditor = true;
	}

	currLine = GS.lineOffset + newY;
	if ((newX + GS.colOffset) >= GS.data->lines[currLine].len) {
		newX = (GS.data->lines[currLine].len - 2); //end of line visually
	}

	if (dx < 0 && newX > GS.cursX) { //moved cursor left of screen
		newX = 0;
		GS.colOffset = (!currCol)? 0 : GS.colOffset - 1;
		GS.dirtyEditor = true;
	} else if (dx > 0 && newX > GS.maxX) {
		newX = GS.maxX - 1;
	}

	GS.cursY = newY;
	GS.cursX = newX;
	wmove(stdscr, GS.cursY, GS.cursX);

	return;
}

void updateEditor() {
	getmaxyx(stdscr, GS.maxY, GS.maxX);
	wmove(stdscr, 0, 0);
	wclear(stdscr);

	for (unsigned int i = 0; i < GS.maxY; i++) {
		unsigned int currLine = i + GS.lineOffset;
		move(i, 0); //move to start of line
		if (currLine >= GS.data->numLines) {
			waddch(stdscr, '~'); //past EOF
		} else {
			unsigned int cellLen = 0;
			for (unsigned int j = 0; j < (GS.data->lines[currLine].len - 1); j++) {
				char ch = GS.data->lines[currLine].text[j];
				cellLen += ((ch == '\t') ? TAB_SIZE : 1);
			}
			if (GS.colOffset < cellLen) {
				Line *line = &(GS.data->lines[currLine]);
				unsigned int cellConsumed = 0;
				unsigned int idx = 0;
				bool midTab = false;
				while (cellConsumed < GS.colOffset) {
					if (line->text[idx] == '\t') {
						cellConsumed++;
						if (midTab) {
							idx++;
							midTab = false;
						} else {
							midTab = true;
						}
					} else {
						cellConsumed++;
						idx++;
					}
				}

				unsigned int cell = 0;
				for (; idx < line->len-1; idx++) {
					char ch = line->text[idx];
					if (ch == '\t' && midTab) {
						mvwaddch(stdscr, i, cell++, ' ');
						midTab = false;
					} else if (ch == '\t') {
						mvwaddch(stdscr, i, cell++, ' ');
						mvwaddch(stdscr, i, cell++, ' ');
					} else {
						mvwaddch(stdscr, i, cell++, ch);
					}
				}

			}
		}
	}


	refresh();
	wmove(stdscr, GS.cursY, GS.cursX);

	return;
}

int main(int argc, char **argv) {
	init();

	if (argc == 2) {
		loadFile(argv[1]);
	} else {
		fatalErr("No file given", 23);
	}

	//Need to print to screen atleast once
	updateEditor();

	while (true) {
		int result = processInput();
		if (GS.dirtyEditor) { //only update if needed
			updateEditor();
			GS.dirtyEditor = false;
		} else if (result == 2) {
			break;
		}
	}

	return 0;
}
