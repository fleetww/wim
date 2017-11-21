#include <ncurses.h>
#include <panel.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>


/*	Prototypes	*/
//ncurses management
void closingTime();
void fatalErr(const char *str, int eval);
void init();

//File I/O
void dataAppendLine(char *newtext, size_t length);
void loadFile(char *filename);
void writeToFile();

//User I/O
char processInput();
void handleEnterKey();
void insert(char ch);
void insertNewLine();
void commandExecution();

//Helper functions
unsigned int getCurrentIndexInLine();

//Editor management
unsigned int nextTabCol(unsigned int currCol);
void moveCursorUp();
void moveCursorDown();
void moveCursorLeft();
void moveCursorRight();
void scrollEditor(int amt);
void updateEditor();
