# Compilador e flags
CC = g++

# Alvos
all: player gs

player: player.cpp
	$(CC) -o player player.cpp

gs: gs.cpp
	$(CC) -o gs gs.cpp

clean:
	rm -f player gs
