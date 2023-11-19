all: empty part1


empty:
	gcc -o empty empty.c

part1:
	gcc -o part1 part1.c

clean:
	rm part1 empty 

run: empty part1
	strace -o empty.trace ./empty
	strace -o part1.trace ./part1