#include <ncurses.h>
#include <panel.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>


/*	Prototypes	*/
//ncurses management
void closingTime();
void fatalErr(const char *str, int eval);
void init();

//File I/O
void dataAppendLine(char *newtext, size_t length);
void loadFile(char *filename);

//User I/O
char processInput();

//Editor management
void moveCursor(int dy, int dx);
void scrollEditor(int amt);
void updateEditor();
