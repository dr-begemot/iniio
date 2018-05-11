.PHONY: clean 

iniio.so:	iniio.o
    ld -shared -soname $@.1 -o $@.1.0 iniio.o -lc
    ln -s ./iniio.so.1.0 iniio.so.1
    ln -s ./iniio.so.1   iniio.so

iniio.o:
    gcc ./src/iniio.c -fPIC -c $<

client:
    gcc -o client client.c -L. -lprint

clean:
    rm -f *.so* *.o client