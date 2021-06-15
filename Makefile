CC=gcc
CFLAGS=-Wall -Ofast
# CFLAGS=-Wall -g
# DEFS=
LDFLAGS=-ljpeg
RM=rm -f

# Uncomment under Win32 (CYGWIN/MinGW):
# EXE=.exe

JPEGENT=jpegent$(EXE)

all: $(JPEGENT)
	strip $(JPEGENT)

$(JPEGENT): jpegent.o
	$(CC) $(LDFLAGS) -o $(JPEGENT) jpegent.o

.c.o:
	$(CC) ${CFLAGS} ${DEFS} -c $*.c

clean:
	$(RM) $(JPEGENT) *.o core
