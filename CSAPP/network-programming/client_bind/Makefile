all: echo_server echo_client

echo_server:
	gcc -o echo_server echo_server.c

echo_client:
	gcc -o echo_client echo_client.c

.PHONY: clean
clean:
	rm echo_server echo_client