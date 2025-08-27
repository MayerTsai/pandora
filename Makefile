hellomake: 
	mkdir -p include lib tmp
	cp src/pandora.h include/pandora.h
	gcc -c src/pandora.c -lpthread -o tmp/libpandora.o
	ar rcs lib/libpandora.a tmp/libpandora.o
	rm -rf tmp

install:
	sudo mv include/pandora.h /usr/include/pandora.h
	sudo mv lib/libpandora.a /usr/lib/libpandora.a
	rm -rf include lib
