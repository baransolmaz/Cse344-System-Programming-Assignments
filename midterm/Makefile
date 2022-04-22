target:test

test: 1801042601.c serverY.c become_daemon.c serverZ.c
	clear
	gcc -Wall 1801042601.c -o client
	gcc -Wall serverY.c become_daemon.c -o serverY
	gcc -Wall serverZ.c become_daemon.c -o serverZ -lrt -lpthread
