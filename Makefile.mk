# Compiler and flags
CC = gcc
CFLAGS = -g -Wall -Isrc
LDFLAGS = -L.
LIBS = -lpandora -lpthread
RM = rm -f

# Files
TARGETS = server client
LIB = libpandora.a
OBJ = pandora.o
HEADER = src/pandora.h
SOURCES_LIB = src/pandora.c
SOURCES_SERVER = server.c
SOURCES_CLIENT = client.c

# Phony targets prevent conflicts with files of the same name
.PHONY: all clean install

# Default target: build the server and client
all: $(TARGETS)

# Executable targets
server: $(SOURCES_SERVER) $(LIB)
	$(CC) $(CFLAGS) $(SOURCES_SERVER) -o server $(LDFLAGS) $(LIBS)

client: $(SOURCES_CLIENT) $(LIB)
	$(CC) $(CFLAGS) $(SOURCES_CLIENT) -o client $(LDFLAGS) $(LIBS)

# Library target
$(LIB): $(OBJ)
	ar rcs $(LIB) $(OBJ)

# Object file target
$(OBJ): $(SOURCES_LIB) $(HEADER)
	$(CC) $(CFLAGS) -c $(SOURCES_LIB) -o $(OBJ)

# Housekeeping targets
install:
	sudo cp $(HEADER) /usr/include/pandora.h
	sudo cp $(LIB) /usr/lib/libpandora.a

clean:
	$(RM) $(TARGETS) $(LIB) $(OBJ)
