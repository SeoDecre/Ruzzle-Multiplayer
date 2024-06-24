# Compiler
CC = gcc

# Compiler flags
CFLAGS = -c

# Source files
SRC = main.c include/game_matrix.c include/game_players.c include/dictionary.c include/server.c

# Object files
OBJ = main.o game_matrix.o game_players.o dictionary.o server.o

# Executable
EXEC = paroliere_srv

# Libraries
LIBS = -lpthread

# Default target
all: $(EXEC)

# Linking the executable
$(EXEC): $(OBJ)
	$(CC) $^ $(LIBS) -o $@

# Compiling source files into object files
main.o: main.c
	$(CC) $(CFLAGS) main.c -o main.o

game_matrix.o: include/game_matrix.c
	$(CC) $(CFLAGS) include/game_matrix.c -o game_matrix.o

game_players.o: include/game_players.c
	$(CC) $(CFLAGS) include/game_players.c -o game_players.o

dictionary.o: include/dictionary.c
	$(CC) $(CFLAGS) include/dictionary.c -o dictionary.o

server.o: include/server.c
	$(CC) $(CFLAGS) include/server.c -o server.o

# Clean target to remove object files and executable
clean:
	rm -f $(OBJ) $(EXEC)

.PHONY: all clean
