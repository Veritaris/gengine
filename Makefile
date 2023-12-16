compileServer:
	gcc src/server.c -o server.a

compileClient:
	gcc src/client.c -o client.a

runServer:
	make compileServer && ./server.a inet

runClient:
	make compileClient && ./client.a

build:
	make compileServer
	make compileClient