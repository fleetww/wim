CC = gcc
CFLAGS = -Wall -Wextra -pedantic
LINKS = -lpanel -lncurses

SRCS = wim.c

MAIN = wim

$(MAIN): $(SRCS)
	$(CC) $(SRCS) -o bin/$(MAIN) $(CFLAGS) -g $(LINKS)

prod: $(SRCS)
	$(CC) $(SRCS) -o bin/$(MAIN) $(CFLAGS) -O3 $(LINKS)

clean:
	rm $(MAIN)
