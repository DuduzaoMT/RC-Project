# Compilador e flags
CC = gcc

# Alvos
all: player gs

player: player.c
	$(CC) -o player player.c

gs: gs.c
	$(CC) -o gs gs.c

clean:
	rm -f player gs
