compile: main.c 
	clear
	gcc -Wall -Wextra main.c -o hw5 -lrt -lpthread -lm

clean:
	rm -f *.o
	rm -f hw5

test_medium_size: medium1 medium2 medium3
test_small_size: small1 small2 small3
test_large_size: large1 large2 large3
test_all: test_small_size test_medium_size test_large_size

medium1: 
	./hw5 -i InputExample/data1 -j InputExample/data2 -o OutputExample/output6x2.csv -n 6 -m 2
medium2: 
	./hw5 -i InputExample/data1 -j InputExample/data2 -o OutputExample/output6x4.csv -n 6 -m 4
medium3: 
	./hw5 -i InputExample/data1 -j InputExample/data2 -o OutputExample/output6x8.csv -n 6 -m 8

small1:
	./hw5 -i InputExample/data1 -j InputExample/data2 -o OutputExample/output4x2.csv -n 4 -m 2
small2:
	./hw5 -i InputExample/data1 -j InputExample/data2 -o OutputExample/output4x4.csv -n 4 -m 4
small3:
	./hw5 -i InputExample/data1 -j InputExample/data2 -o OutputExample/output4x8.csv -n 4 -m 8

large1:
	./hw5 -i InputExample/data1 -j InputExample/data2 -o OutputExample/output8x2.csv -n 8 -m 2
large2:
	./hw5 -i InputExample/data1 -j InputExample/data2 -o OutputExample/output8x4.csv -n 8 -m 4
large3:
	./hw5 -i InputExample/data1 -j InputExample/data2 -o OutputExample/output8x8.csv -n 8 -m 8

memory_leak4x2:
	valgrind --leak-check=yes --track-origins=yes --show-reachable=yes ./hw5 -i InputExample/input1.txt -j InputExample/input2.txt -o OutputExample/output4x2.csv -n 4 -m 2

memory_leak8x8:
	valgrind --leak-check=yes --track-origins=yes --show-reachable=yes ./hw5 -i InputExample/data1 -j InputExample/data2 -o OutputExample/output8x8.csv -n 8 -m 8