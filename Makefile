all:
	gcc client.cpp -o client
	gcc server.cpp -o server

clean:
	rm client server output.txt