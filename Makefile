all:
	x86_64-w64-mingw32-gcc -o who who.c -lwtsapi32

clean:
	rm who.exe
