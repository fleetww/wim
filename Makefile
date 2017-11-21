CC = gcc
CFLAGS = -Wall -Wextra -pedantic
LINKS = -lpanel -lncurses

SRCS = wim.c

MAIN = bin/wim

$(MAIN): $(SRCS)
	$(CC) $(SRCS) -o $(MAIN) $(CFLAGS) -g $(LINKS)

prod: $(SRCS)
	$(CC) $(SRCS) -o $(MAIN) $(CFLAGS) -O3 $(LINKS)
