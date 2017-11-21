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
#define TABSIZE 2

#define ERRORLOGPATH "/tmp/wim.err"
FILE *errorlog;

typedef unsigned int uint;

/*** Data ***/

/*
 *	Represents one line in the open file
 */
typedef struct Line {
	uint len;
	char *text;
} Line;

/*
 *	Represents the open file, line by line
 */
typedef struct FileData {
	uint numLines;
	Line *lines;
} FileData;

/*
 *	Holds general global data like number of open windows
 */
typedef struct GlobalState {
	FileData *data;
	uint maxX;
	uint maxY;
	uint lineOffset;
	uint colOffset;
	uint cursX;
	uint cursY;
	bool dirtyEditor;
	enum modes {editor, command} mode;
	char *filename;
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
 *	Used to write to error, depracated
 */
void fatalErr(const char *str, int eval) {
	fwrite(str, strlen(str), 1, errorlog);
	exit(eval);

	return;
}

/*
 *	Sets up the initial environment and allocating starting space
 */
void init(char **argv) {
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

	errorlog = fopen(ERRORLOGPATH, "a+");
	if (!(GS.data = malloc(sizeof(FileData)))) {
		fprintf(errorlog, "Could not get needed space to store file data\n");
		exit(1);
	}


	GS.data->numLines = 0;
	GS.maxX = 0;
	GS.maxY = 0;
	GS.lineOffset = 0;
	GS.cursX = 0;
	GS.cursY = 0;
	GS.colOffset = 0;
	GS.dirtyEditor = false;
	GS.mode = editor;
	GS.filename = strdup(argv[1]);
	return;
}

/*** File I/O ***/

/*
 *	Allocates space for new line, copies over the text, not null-terminated
 */
void dataAppendLine(char *newtext, size_t length) {
	GS.data->lines = realloc(GS.data->lines, sizeof(Line) * (++GS.data->numLines));
	Line *line = &(GS.data->lines[GS.data->numLines-1]);
	line->len = length;
	line->text = strndup(newtext, length);

	return;
}

/*
 *	Reads in the file, loading line by line into the data structure
 */
void loadFile(char *filename) {
	FILE *file = fopen(filename, "r");
	if (!file) {
		fprintf(errorlog, "Could not open specified file: %s\n", filename);
		exit(1);
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
		fprintf(errorlog, "Could not read a line from: %s\n", filename);
		exit(1);
	}

	free(buf);
	fclose(file);

	return;
}

void writeToFile() {
	int fd = open(GS.filename, O_CREAT|O_TRUNC|O_WRONLY);

	for (uint i = 0; i < GS.data->numLines; i++) {
		Line *currLine = GS.data->lines + i;
		int amt = 0;
		while ((amt = write(fd, currLine->text + amt, currLine->len - amt)) != 0) {
			if (amt == -1) {
				fprintf(errorlog, "Could not write to file: %s\n", GS.filename);
				exit(1);
			}
		}
	}

	return;
}

/*** User I/O ***/

/*
 *	Read in a single character from keyboard, non blocking, and handle it
 */
char processInput() {
	int ch = wgetch(stdscr);
	switch (ch) {
		//Ignored keys
		case KEY_NPAGE:
		case KEY_PPAGE:
		case KEY_MOUSE:
			break;
		case CTRL_KEY('c'):
			return 2;
			break;
		case CTRL_KEY('s'):
			writeToFile();
			break;
		case KEY_UP:
			moveCursorUp();
			break;
		case KEY_DOWN:
			moveCursorDown();
			break;
		case KEY_LEFT:
			moveCursorLeft();
			break;
		case KEY_RIGHT:
			moveCursorRight();
			break;
		case 13:
			handleEnterKey();
			break;
		default:
			insert(ch);
	}

	return 0;
}


/*
 *	Takes the char ch and inserts it into the current place in line
 */
void insert(char ch) {
	//Ignore anything that is not one of these
	if (!(ch == isgraph(ch) || ch == ' ' || ch == '\t')) {
		return;
	}



	return;
}

/*
 *	Either splits the current line in two, divided by the cursor. Or it will
 *	execute the built up command
 *	TODO command execution
 */
void handleEnterKey() {
	if (GS.mode == editor) {
		insertNewLine();
	} else if (GS.mode == command) {
		commandExecution();
	}

	return;
}

/*
 *	Executes the built up command
 */
void commandExecution() {
	return;
}

/*
 *	Splits the current line in two, dividing by the cursor
 * 	TODO handle end of line edge cases
 */
void insertNewLine() {

	uint currLine = (GS.lineOffset + GS.cursY);

	//We must now split the line, inserting a new Line into the data
	GS.data->lines = realloc(GS.data->lines, sizeof(Line) * (GS.data->numLines + 1));

	//If we are at the last line, we have nothing to move over
	if (currLine != GS.data->numLines - 1) {
		Line *src = (GS.data->lines + currLine + 1);
		Line *dest = src + 1;
		uint numBytes = (GS.data->numLines - 1 - currLine) * sizeof(Line *);

		//move the remaining lines over by (sizeOf(Line)) bytes
		memmove(dest, src, numBytes);
	}

	GS.data->numLines++;

	uint idx = getCurrentIndexInLine(); //current index in current line->text

	Line *tempLine = (GS.data->lines + currLine);

	Line *newLine = tempLine + 1;
	newLine->len = tempLine->len - idx;
	newLine->text = malloc(newLine->len);
	memcpy(newLine->text, tempLine->text+idx, newLine->len);
	char *newText = malloc(idx+1);
	memcpy(newText, tempLine->text, idx);
	newText[idx] = '\n';
	free(tempLine->text);
	tempLine->text = newText;
	tempLine->len = idx + 1;

	//move the cursor down, beginning of the line
	moveCursorDown();
	GS.cursX = 0;
	GS.colOffset = 0;

	GS.dirtyEditor = true;

	return;
}

/*** Helper Functions ***/

/*
 *	Returns the index in the current line that the cursor is currently in
 */
unsigned int getCurrentIndexInLine() {
	uint currCell = GS.colOffset + GS.cursX;
	uint currLine = GS.lineOffset + GS.cursY;
	Line *line = (GS.data->lines + currLine);

	uint idx;
	uint tempCell = 0;
	//last line might have no \n char, so we must alter range
	uint lineLen = (line->text[line->len-1] != '\n') ? line->len : line->len - 1;

	for (uint i = 0; i < lineLen; i++) {
		char ch = line->text[i];

		if (tempCell == currCell) {
			idx = i;
			break;
		}

		if (ch == '\t') {
			tempCell = nextTabCol(tempCell);
		} else {
			tempCell++;
		}

		if (tempCell == currCell) {
			idx = i + 1;
			break;
		}
	}

	return idx;
}

/*
 *	Just gives next tab collumn
 */
unsigned int nextTabCol(unsigned int currCol) {
	return ( (uint)(currCol / TABSIZE) + 1) * TABSIZE;
}

/*** Editor management ***/

/*
 *	Move editor up/down by amount
 *	might be replaced soon
 */
void scrollEditor(int amt) {
	uint temp = GS.lineOffset + amt;
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

/*
 *	Moves the cursor up by one line, scrolling, and moving cursor appropriately
 *	in the new line
 */
void moveCursorUp() {
	uint X = GS.cursX;
	uint Y = GS.cursY;
	uint currLine = Y + GS.lineOffset;
	Line *line = (GS.data->lines + currLine);

	if (currLine == 0) { //top of the file, nothing to do
		return;
	}

	if (Y == 0) {
		GS.lineOffset--;
		GS.dirtyEditor = true;
	} else {
		Y--;
	}

	currLine = Y + GS.lineOffset;
	line = (GS.data->lines + currLine);
	uint currCell = X + GS.colOffset;

	//validate X position in file
	//find closes valid position, that is <= X
	uint cellLen = 0;
	uint prevCellLen = 0;
	uint jumpCell = 0;
	//Want to get up to last char
	uint lineLen = (line->text[line->len-1] != '\n') ? line->len : line->len - 1;

	for (uint i = 0; i < lineLen; i++) {
		char ch = line->text[i];

		prevCellLen = cellLen;
		if (ch == '\t') {
			cellLen = nextTabCol(cellLen);
		} else {
			cellLen++;
		}

		if (prevCellLen <= currCell && currCell < cellLen) {
			jumpCell = prevCellLen;
		} else if (currCell == cellLen) {
			jumpCell = cellLen;
		}
	}

	if (cellLen < currCell) {
		jumpCell = cellLen;
	}

	if (jumpCell < GS.colOffset) {
		GS.colOffset = jumpCell;
		GS.dirtyEditor = true;
	}

	X = jumpCell;
	GS.cursX = X;
	GS.cursY = Y;
	wmove(stdscr, Y, X);

	return;
}


/*
 *	Moves the cursor down by one line, scrolling, and moving cursor
 *	appropriately in new line
 */
void moveCursorDown() {
	uint X = GS.cursX;
	uint Y = GS.cursY;
	uint currLine = Y + GS.lineOffset;
	Line *line = (GS.data->lines + currLine);


	if (currLine == (GS.data->numLines - 1)) { //one past last line
		return;
	}

	if (Y == GS.maxY - 1) {
		GS.lineOffset++;
		GS.dirtyEditor = true;
	} else {
		Y++;
	}

	currLine = Y + GS.lineOffset;
	line = (GS.data->lines + currLine);
	uint currCell = X + GS.colOffset;

	//validate X position in file
	uint cellLen = 0;
	uint prevCellLen = 0;
	uint jumpCell = 0;
	//last line might have no \n char, so we must alter range
	uint lineLen = (line->text[line->len-1] != '\n') ? line->len : line->len - 1;
	for (uint i = 0; i < lineLen; i++) {
		char ch = line->text[i];

		prevCellLen = cellLen;
		if (ch == '\t') {
			cellLen = nextTabCol(cellLen);
		} else {
			cellLen++;
		}

		if (prevCellLen <= currCell && currCell < cellLen) {
			jumpCell = prevCellLen;
		} else if (currCell == cellLen) {
			jumpCell = cellLen;
		}
	}

	if (cellLen < currCell) {
		jumpCell = cellLen;
	}

	if (jumpCell < GS.colOffset) {
		GS.colOffset = jumpCell;
		GS.dirtyEditor = true;
	}

	X = jumpCell;
	GS.cursX = X;
	GS.cursY = Y;
	wmove(stdscr, Y, X);

	return;
}

/*
 *	Moves cursor left by one character in line, panning editor left if
 *	necessary
 */
void moveCursorLeft() {
	uint X = GS.cursX;
	uint Y = GS.cursY;
	uint currLine = Y + GS.lineOffset;
	Line *line = (GS.data->lines + currLine);

	uint cellLen = 0;
	uint jumpCell; //cell we are going to jump left to
	uint prevCellLen = 0;
	//last line might have no \n char, so we must alter range
	uint lineLen = (line->text[line->len-1] != '\n') ? line->len : line->len - 1;
	for (uint i = 0; i < lineLen; i++) {
		if (cellLen == (GS.colOffset + X)) {
			jumpCell = prevCellLen;
			break;
		}

		prevCellLen = cellLen;
		char ch = line->text[i];
		if (ch == '\t') {
			cellLen = nextTabCol(cellLen);
		} else {
			cellLen++;
		}

		if (cellLen == (GS.colOffset + X)) {
			jumpCell = prevCellLen;
			break;
		}
	}

	if (jumpCell < GS.colOffset) {
		//pan left necessary amount, set X=0
		X = 0;
		GS.colOffset = jumpCell;
		GS.dirtyEditor = true;
	} else {
		X = jumpCell - GS.colOffset;
	}

	GS.cursX = X;
	wmove(stdscr, Y, X);

	return;
}

/*
 *	Moves cursor right by one character in line, panning editor if necessary
 */
void moveCursorRight() {
	uint X = GS.cursX;
	uint Y = GS.cursY;
	uint currLine = Y + GS.lineOffset;
	Line *line = (GS.data->lines + currLine);

	uint cellLen = 0;
	uint jumpCell; //cell we are going to jump right to
	uint prevCellLen = 0;
	bool endOfLine = false; //cursor already at end of line
	//last line might have no \n char, so we must alter range
	uint lineLen = (line->text[line->len-1] != '\n') ? line->len : line->len - 1;
	for (uint i = 0; i < lineLen; i++) {
		prevCellLen = cellLen;

		char ch = line->text[i];
		if (ch == '\t') {
			cellLen = nextTabCol(cellLen);
		} else {
			cellLen++;
		}

		if (prevCellLen == (GS.colOffset + X)) { //we were previously at current char
			jumpCell = cellLen;
			break;
		}

		endOfLine = ((i + 1) == lineLen);
	}

	if (endOfLine) {
		GS.cursX = X;
		wmove(stdscr, Y, X);
		return;
	}

	if ((jumpCell - GS.colOffset) >= GS.maxX) { //next char is past screen
		GS.colOffset += jumpCell - (GS.colOffset + X);
		X += jumpCell - (GS.colOffset + X); //a little ugly visually, but it does work
		GS.dirtyEditor = true;
	} else {
		X = jumpCell - GS.colOffset;
	}

	GS.cursX = X;
	wmove(stdscr, Y, X);

	return;
}



/*
 *	Prints text file to the screen, using offsets to fit correct info in screen
 */
void updateEditor() {
	getmaxyx(stdscr, GS.maxY, GS.maxX);
	wmove(stdscr, 0, 0);
	wclear(stdscr);

	for (uint i = 0; i < GS.maxY; i++) {
		uint currLine = i + GS.lineOffset;
		wmove(stdscr, i, 0);
		if (currLine >= GS.data->numLines) {
			waddch(stdscr, '~');
		} else {
			Line *line = (GS.data->lines + currLine);
			uint cellConsumed = 0;
			uint idx = 0;

			while (cellConsumed < GS.colOffset && idx < (line->len - 1)) {
				if (line->text[idx] == '\t') {
					cellConsumed = nextTabCol(cellConsumed);
					idx++;
				} else {
					idx++;
					cellConsumed++;
				}
			}

			uint currCol = 0;
			//Only happens when last character hit was a tab
			if (cellConsumed > GS.colOffset) {
				currCol = cellConsumed - GS.colOffset;
				wmove(stdscr, i, currCol);
			}

			//Print the rest of the line
			for (; idx < (line->len); idx++) {
				char ch = line->text[idx];
				if (ch == '\t') {
					currCol = nextTabCol(currCol);
				} else if (ch != '\n') {
					mvwaddch(stdscr, i, currCol++, ch);
				}
			}
		}
	}

	refresh();
	wmove(stdscr, GS.cursY, GS.cursX);

	return;
}

int main(int argc, char **argv) {
	init(argv);

	if (argc == 2) {
		loadFile(argv[1]);
	} else {
		fprintf(errorlog, "No file given");
		exit(23);
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
