# Compiler
CC = cc

# Compiler flags
CFLAGS = -Wall -g -pedantic -pthread

# Header files
HDRS = macros.h

# Executable files
EXE1 = prodcons
EXE2 = socketor
EXE3 = multiserver

# Object files
OBJS1 = $(EXE1).o
OBJS2 = $(EXE2).o
OBJS3 = $(EXE3).o

# Source files
SRCS1 = $(EXE1).c
SRCS2 = $(EXE2).c
SRCS3 = $(EXE3).c


# Targets
all: $(EXE1) $(EXE2) $(EXE3)

$(EXE1): $(OBJS1)
	$(CC) $(CFLAGS) -o $(EXE1) $(OBJS1)

$(EXE2): $(OBJS2)
	$(CC) $(CFLAGS) -o $(EXE2) $(OBJS2)

$(EXE3): $(OBJS3)
	$(CC) $(CFLAGS) -o $(EXE3) $(OBJS3)

$(OBJS1): $(SRCS1) $(HDRS)
	$(CC) $(CFLAGS) -c $(SRCS1) $(HDRS)

$(OBJS2): $(SRCS2) $(HDRS)
	$(CC) $(CFLAGS) -c $(SRCS2) $(HDRS)

$(OBJS3): $(SRCS3) $(HDRS)
	$(CC) $(CFLAGS) -c $(SRCS3) $(HDRS)

clean:
	rm -f $(OBJS1) $(OBJS2) $(OBJS3) $(EXE1) $(EXE2) $(EXE3)



# End of Makefile
