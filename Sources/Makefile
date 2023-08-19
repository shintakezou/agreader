# Makefile for AGReader v1.0
# (c) T.Pierron 2000

CC=gcc
EXE=agr
CFLAGS=-std=c89 -pedantic -Wall -Wextra -D_DEFAULT_SOURCE #-DNO_STR_HELP
INCDIR=
LIBS=
OBJECTS=main.o IO_tty.o AG_lib.o AGObj.o Input.o Navigate.o Text.o

all: final

%.o: %.c AGReader.h AGNodes.h
	$(CC) $< -c $(CFLAGS) $(INCDIR) -O2

.%.obj: %.c AGReader.h AGNodes.h
	$(CC) $< -c $(CFLAGS) $(INCDIR) -DDEBUG_MEM -g
	@touch $@

$(EXE): $(patsubst %.o,.%.obj,$(OBJECTS))
	$(CC) $(OBJECTS) -o $(EXE) $(LIBS) -g

final: $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXE) $(LIBS)

install:
	cp $(EXE) $(HOME)/bin -f

.Input.obj: Help.h
AG_lib.o: Version.h

clean:
	rm -f *.o .*.obj $(EXE)

