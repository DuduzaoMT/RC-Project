# Compilador e flags
CC = g++

# Alvos
all: player gameserver

player: player.cpp
	$(CC) -o player player.cpp common.cpp

gameserver: gameserver.cpp
	$(CC) -o gameserver gameserver.cpp common.cpp
 
clean:
	rm -f player gameserver
