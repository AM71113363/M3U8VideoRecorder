# Project: AM71113363

CC   = gcc.exe -s
WINDRES = windres.exe
RES  = main.res
OBJ  = main.o connect.o $(RES)
LINKOBJ  = main.o connect.o $(RES)
LIBS =  -L"C:/Dev-Cpp/lib" -mwindows mbedtlsLib.a -lws2_32 -lcomctl32  
INCS =  -I"C:/Dev-Cpp/include" 
BIN  = M3U8VideoRecorder.exe
CFLAGS = $(INCS)  
RM = rm -f

.PHONY: all all-before all-after clean clean-custom

all: all-before M3U8VideoRecorder.exe all-after


clean: clean-custom
	${RM} $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(CC) $(LINKOBJ) -o "M3U8VideoRecorder.exe" $(LIBS)

main.o: main.c
	$(CC) -c main.c -o main.o $(CFLAGS)

connect.o: connect.c
	$(CC) -c connect.c -o connect.o $(CFLAGS)

main.res: main.rc 
	$(WINDRES) -i main.rc --input-format=rc -o main.res -O coff 
