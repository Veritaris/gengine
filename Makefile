compileServer:
	gcc src/server.c -o server.a

compileClient:
	gcc src/client.c -o client.a

runServer:
	make compileServer && ./server.a inet

runClient:
	make compileClient && ./client.a inet

build:
	make compileServer
	make compileClient

build-valgrind:
	gcc -O0 -std=17 -Wall -ggdb3 src/server.c -o server.valgrind.a
	gcc -O0 -std=17 -Wall -ggdb3 src/client.c -o client.valgrind.a

run-valgrind-server:
	valgrind --leak-check=full \
	--show-leak-kinds=all \
	--track-origins=yes \
	--verbose \
	--log-file=valgrind-server-out.txt \
	./server.valgrind.a inet 0.0.0.0 10312

run-valgrind-client:
	valgrind --leak-check=full \
	--show-leak-kinds=all \
	--track-origins=yes \
	--verbose \
	--log-file=valgrind-client-out.txt \
	./client.valgrind.a inet 127.0.0.1 10312