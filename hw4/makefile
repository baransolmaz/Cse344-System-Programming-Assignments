compile: main.c 
	clear
	gcc -Wall -Wextra main.c -o hw4 -lrt -lpthread

clean:
	rm -f *.o
	rm -f hw4

run_5x2:
	./hw4 -C 5 -N 2 -F InputExample/input5x2.txt

run_8x3:
	./hw4 -C 8 -N 3 -F InputExample/input8x3.txt

run_10x5:
	./hw4 -C 10 -N 5 -F InputExample/input10x5.txt

memory_leak_5x2:
	valgrind --leak-check=yes --track-origins=yes --show-reachable=yes ./hw4 -C 5 -N 2 -F InputExample/input5x2.txt

memory_leak_8x3:
	valgrind --leak-check=yes --track-origins=yes --show-reachable=yes ./hw4 -C 8 -N 3 -F InputExample/input8x3.txt

memory_leak_10x5:
	valgrind --leak-check=yes --track-origins=yes --show-reachable=yes ./hw4 -C 10 -N 5 -F InputExample/input10x5.txt

shared_mem_leak:
	ipcs
	ls /dev/shm -a