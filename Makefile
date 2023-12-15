runServer:
	gcc src/server.c -o server && ./server

runClient:
	gcc src/client.c -o client && ./client
