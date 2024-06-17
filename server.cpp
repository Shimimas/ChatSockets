#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

#define NUM 5
#define MAX_CLIENTS 5

int server_sock;
int all_connections[MAX_CLIENTS + 1];

int create_a_socket(int *sock) {
	struct sockaddr_in server;

	if ( (*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) { 
		perror("socket creation failed"); 
		return 1; 
	}

	server.sin_family = AF_INET;
	server.sin_port = 0;
	server.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(*sock, (struct sockaddr *) &server, sizeof(server)) == -1) {
		perror("cannot bind socket"); 
		return 1;
	}

	socklen_t len = sizeof(server);
	getsockname(*sock, (sockaddr *)&server, &len);

	printf("Server port: %d\n", ntohs(server.sin_port));

	return 0;
}

void sig_handler(int signo) {
	if (signo == SIGINT) {
		for (int i = 0; i < MAX_CLIENTS + 1; i++) {
			if (all_connections[i] > 0) {
				close(all_connections[i]);
			}
		}

		exit(1);
	}
}

int main(int argc, char *argv[]) {
	int client_sock;
	int client_len;
	int maxfd;
	int ret;
	int ret_msg;
	char buffer[1024];
	struct sockaddr_in client;
	fd_set fds;

	if (create_a_socket(&server_sock) == 1) {
		return 1;
	}

    if ((listen(server_sock, MAX_CLIENTS)) != 0) { 
        perror("listen failed"); 
        return 1; 
    }

	all_connections[0] = server_sock;

	for (int i = 1; i < MAX_CLIENTS + 1; i++) {
		all_connections[i] = -1;
	}

	client_len = sizeof(client);

	signal(SIGINT, sig_handler);

	while (1) {
		FD_ZERO(&fds);
		//maxfd = server_sock + 1;
		FD_SET(all_connections[0], &fds);

		for (int i = 1; i < MAX_CLIENTS + 1; i++) {
			if (all_connections[i] >= 0) {
				FD_SET(all_connections[i], &fds);
			}
		}

		ret = select(FD_SETSIZE, &fds, NULL, NULL, NULL);

		if (ret >= 0) {
			if (FD_ISSET(all_connections[0], &fds)) { 
				client_sock = accept(all_connections[0], (struct sockaddr*)&client, (socklen_t*)&client_len);

				if (client_sock >= 0) {
					printf("Accepted a new connection with fd: %d\n", client_sock);
					for (int i = 1; i < MAX_CLIENTS + 1; i++) {
						if (all_connections[i] < 0) {
							all_connections[i] = client_sock; 
							break;
						}
					}
				}

				ret--;

				if (!ret) {
					continue;
				}
			} 

			for (int i = 1; i < MAX_CLIENTS + 1; i++) {
				if ((all_connections[i] > 0) && FD_ISSET(all_connections[i], &fds)) {
					bzero(&buffer, sizeof(buffer));
					ret_msg = recv(all_connections[i], &buffer, sizeof(buffer), 0);	

					if (ret_msg == 0) {
						printf("Closing connection for fd:%d\n", all_connections[i]);
						close(all_connections[i]);
						all_connections[i] = -1; /* Connection is now closed */
					}

					if (ret_msg > 0) { 
						printf("Received data (len %d bytes, fd: %d): %s\n", ret_msg, all_connections[i], buffer);
						buffer[0] = 'D';
						buffer[1] = '\0';
						send(all_connections[i], &buffer, sizeof(buffer), 0);
					}

					if (ret_msg == -1) {
						perror("recv() failed");
						break;
					}
				}
			}
		}
	}

	for (int i = 0; i < MAX_CLIENTS + 1; i++) {
		if (all_connections[i] > 0) {
			close(all_connections[i]);
		}
	}

	return 0;
}
