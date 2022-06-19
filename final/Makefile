compile: src/*.c libs/*.h
	clear
	gcc -Wall src/client.c libs/helper.h -o client -lrt -lpthread
	gcc -Wall src/server.c src/queue.c libs/helper.h libs/queue.h -o server -lrt -lpthread
	gcc -Wall src/servant.c src/avl_*.c libs/*.h -o servant -lrt -lpthread

clean:
	rm -f *.o
	rm -f client servant server