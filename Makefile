CC=gcc
CFLAGS=-Wall -Ofast
# CFLAGS=-Wall -g
# DEFS=
LDFLAGS=-ljpeg -lm
RM=rm -f

# Uncomment under Win32 (CYGWIN/MinGW):
# EXE=.exe

JPEGENT=jpegent$(EXE)

all: $(JPEGENT)
	strip $(JPEGENT)

$(JPEGENT): jpegent.o main.o
	$(CC) $(LDFLAGS) -o $(JPEGENT) jpegent.o main.o

.c.o:
	$(CC) ${CFLAGS} ${DEFS} -c $*.c

clean:
	$(RM) $(JPEGENT) *.o jpegent.js jpegent.wasm core

emscripten:
	emcc -O3 -s USE_LIBJPEG=1 -s MODULARIZE -s EXPORTED_RUNTIME_METHODS=ccall -s EXPORTED_FUNCTIONS='["_jpeg_entropy","_malloc"]' -o jpegent.js jpegent.c
