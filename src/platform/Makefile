CC = clang++
CFLAGS = -g -Wno-writable-strings -DGAME_DEBUG
LFLAGS = -llibucrt -llibvcruntime -llibcmt -lkernel32 -luser32 -lopengl32 -lgdi32 -luuid -ldsound

RUNDIR = ../../rundir
TARGET = launch.exe

all: $(RUNDIR)/$(TARGET)

$(RUNDIR)/$(TARGET): *.cpp ../game_platform.h
	$(CC) $(LFLAGS) $(CFLAGS) -I.. -o $(RUNDIR)/$(TARGET) win32.cpp
