# Compilador e flags
CC = g++

# Alvos
all: player gs

player: player.cc
	$(CC) -o player player.cc

gs: gs.cc
	$(CC) -o gs gs.cc

clean:
	rm -f player gs
