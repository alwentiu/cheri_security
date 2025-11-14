CC = cc
CFLAGS = -g -shared -fPIC
LIBS = -lrary -L./lib/ -lrary2 -L./lib/
LIBOUT = ./lib
OUT = ./bin

bin:
	mkdir -p bin
	mkdir -p lib

library: bin
	$(CC) -c $(CFLAGS) library.c -o $(OUT)/library.o 
	$(CC) $(CFLAGS) $(OUT)/library.o -o $(LIBOUT)/library.so -lc -lexecinfo
	rm $(OUT)/library.o

library2: bin
	$(CC) -c $(CFLAGS) library2.c -o $(OUT)/library2.o 
	$(CC) $(CFLAGS) $(OUT)/library2.o -o $(LIBOUT)/library2.so -lc -lexecinfo
	rm $(OUT)/library2.o


test: library library2
	$(CC) -c -g test.c -o $(OUT)/test.o
	$(CC) -pie $(OUT)/test.o -o $(OUT)/test -Wl,-rpath,$(LIBOUT) $(LIBS) -lc -lexecinfo


