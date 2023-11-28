# Makefile for mp3 encoder demo frontend
# Requires GCC version 2.95.2 or better

CC = gcc

.c.o:
	$(CC) -I../library -c $<

mp3encdemo: main.o inputfile.o ../library/libmp3Encode.a 
	$(CC) -o $@ $^ 

clean:
	@@$(RM) mp3encdemo inputfile.o main.o
