CXX = g++
RM  = rm -f
BIN = eriload

# Additional LD flags, if any.
# (e.g. If you want to use libiconv, you can add "-liconv".)
LD_ADD = 

all: $(BIN)

install: $(BIN)
	gimptool-2.0 --install-bin-strip eriload

install-admin: $(BIN)
	gimptool-2.0 --install-bin-admin-strip eriload

clean:
	$(RM) $(BIN) *.o *.i *.exe *.dll *.a *.base *.exp

eriload: eriload.o extrueri.o
	$(CXX) -o $@ `gimptool-2.0 --libs` -lstdc++ $(LD_ADD) $^

eriload.o: eriload.cc
	$(CXX) `gimptool-2.0 --cflags` -DLOCALEDIR=\"`gimptool-2.0 --datadir`/locale\" -c -o $@ $<

extrueri.o: extrueri.cpp
	$(CXX) `pkg-config glib-2.0 --cflags` -c -o $@ $<
