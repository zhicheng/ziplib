CFLAGS = -g
LDFLAGS = -lz
CC = gcc
all: zip

zip: zip.o
	$(CC) $(LDFLAGS) -o $@ $< 

zip.o: zip.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf *.o zip 
