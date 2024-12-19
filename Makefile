# Compilador e flags
CC = g++

# Alvos
all: player gs

player: player.cpp
	$(CC) -o player player.cpp common.cpp

gs: gs.cpp
	$(CC) -o GS gs.cpp common.cpp
 
clean:
	rm -f player GS
