CC = cc
CFLAGS = -g -shared -fPIC
LIBS = -lrary -L./lib/ 
LIBOUT = ./lib
OUT = ./bin

bin:
	mkdir -p bin
	mkdir -p lib

library: bin
	$(CC) -c $(CFLAGS) stackScan.c -o $(OUT)/stackScan.o 
	$(CC) -c $(CFLAGS) library.c -o $(OUT)/library.o 
	$(CC) $(CFLAGS) $(OUT)/library.o $(OUT)/stackScan.o -o $(LIBOUT)/library.so -lc -lexecinfo
	rm $(OUT)/library.o

test: library 
	$(CC) -c -g test.c -o $(OUT)/test.o
	$(CC) -pie $(OUT)/test.o -o $(OUT)/test -Wl,-rpath,$(LIBOUT) $(LIBS) -lc -lexecinfo -lssl -lcrypto


