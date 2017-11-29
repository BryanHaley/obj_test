CC?=gcc
DEBUG?=-g -Wall
OPTIONS?=
LIBS?=-lglfw -framework OpenGL
INCLUDES?=
EXE?=objtest
EXTENSION?=.nix
SOURCES?=objtest.c file_loaders.c

all: release
debug:
	$(CC) $(OPTIONS) $(DEBUG) $(LIBS) $(SOURCES) -o $(EXE)$(EXTENSION)
release:
	$(CC) $(OPTIONS) $(LIBS) $(SOURCES) -o $(EXE)$(EXTENSION)