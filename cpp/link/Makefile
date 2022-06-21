default:
	@echo try below commands
	@echo - make static-fail
	@echo - make static-okay
	@echo - make dynamic

# static link needs all libs (direct + indirect)
static-fail: main.o lib.a liblib.a
	gcc -o test main.o lib.a

static-okay: main.o lib.a liblib.a
	gcc -o test main.o lib.a liblib.a

# dynamic link only need direct lib
# add -rpath to specify where lib.so is
dynamic: main.o lib.so liblib.so
	gcc -o test -Wl,-rpath=. main.o lib.so

###############################################################################

# static lib can have unresolved symbols (func in lib.a calls func in liblib.a)
lib.a: lib.o
	ar -cr lib.a lib.o

liblib.a: liblib.o
	ar -cr liblib.a liblib.o

# all symbols must be resolved to link dynamic lib (link fail w/o liblib.so)
# add -rpath to specify where liblib.so is
lib.so: lib.o liblib.so
	gcc -shared -o lib.so -fPIC -Wl,-rpath=. lib.o liblib.so

liblib.so: liblib.o
	gcc -shared -o liblib.so -fPIC liblib.o

clean:
	rm -f *.o *.a *.so test
