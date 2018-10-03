CC = gcc
CFLAGS = -Wall -Wextra -pedantic
LINKS = -lpanel -lncurses

SRCS = src/wim.c

MAIN = wim

$(MAIN): $(SRCS)
	$(CC) $(SRCS) -o $(MAIN) $(CFLAGS) -g $(LINKS)

prod: $(SRCS)
	$(CC) $(SRCS) -o $(MAIN) $(CFLAGS) -O3 $(LINKS)
