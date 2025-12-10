CC=gcc
CFLAGS=-fPIC -Iinclude -O2 

all: libs prog1 prog2

libs:
	$(CC) $(CFLAGS) -shared libs/impl1/prime.c libs/impl1/translate.c -o libs/impl1/libimpl1.so
	$(CC) $(CFLAGS) -shared libs/impl2/prime.c libs/impl2/translate.c -o libs/impl2/libimpl2.so

prog1:
	$(CC) -Iinclude prog1/prog1.c -Llibs/impl1 -limpl1 -o prog1/prog1

prog2:
	$(CC) -Iinclude prog2/prog2.c -ldl -o prog2/prog2

clean:
	rm -f libs/impl1/*.so libs/impl2/*.so prog1/prog1 prog2/prog2
