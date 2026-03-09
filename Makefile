CC = gcc
CFLAGS = -g -Wall $(shell pkg-config --cflags gtk4)
LIBS = $(shell pkg-config --libs gtk4) \
       $(shell pkg-config --libs libavformat libavcodec libavutil) \
       -lpthread -lm

# Source files
SRC = src/main.c \
      src/audio/player.c \
      src/audio/playlist.c \
      src/audio/meta_reader.c \
      src/ui/ui.c

# Resource handling
RESOURCES_XML = src/resources/resources.gresource.xml
RESOURCES_C = src/resources/resources.c
RESOURCES_OBJ = src/resources/resources.o

# Object files
OBJ = $(SRC:.c=.o) $(RESOURCES_OBJ)
TARGET = PirateBop

# Default target
all: $(TARGET)

# Link final binary
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LIBS)

# Rule for normal .c files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Rule for generating and compiling resources
# The --sourcedir flag tells the tool to look inside src/resources for your files
$(RESOURCES_C): $(RESOURCES_XML) src/resources/styles.css src/resources/pirate.png
	glib-compile-resources $(RESOURCES_XML) \
		--target=$@ \
		--sourcedir=src/resources \
		--generate-source

# Rule for compiling the generated resource C file
$(RESOURCES_OBJ): $(RESOURCES_C)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ) $(RESOURCES_C)

.PHONY: all clean
