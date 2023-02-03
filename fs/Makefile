default: testfat32

%.o : %.c
	g++ -g -o $@ -c $<

%.o : %.cpp
	g++ -g -o $@ -c $<

fat32.o: fat32.c fat32.h
testfat32.o: testfat32.c fat32.h

testfat32: fat32.o testfat32.o
	g++ -g -o $@ $^

clean:
		rm  testfat32  *.o
