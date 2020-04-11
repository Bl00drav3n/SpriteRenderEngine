CC = clang++
CFLAGS = -g -Wno-writable-strings -DGAME_DEBUG -DBUILD_GAME_DLL
LFLAGS = -shared

RUNDIR = ../rundir
TARGET = game.dll

all: $(RUNDIR)/$(TARGET)

$(RUNDIR)/$(TARGET): *.cpp *.h
	$(CC) $(LFLAGS) $(CFLAGS) -o $(RUNDIR)/$(TARGET) game.cpp
