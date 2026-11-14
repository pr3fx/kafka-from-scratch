#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <threads.h>
#include <errno.h>
#include <unistd.h>
#include "sds/sds.c"
#include "serdes.c"
#include "types.c"
#include "request.c"
#include "response.c"
#include "server.c"


static int server_fd;

void close_server(void)
{
    close(server_fd);
}

int main(int argc, char* argv[]) {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);
	
	int client_addr_len;
	struct sockaddr_in client_addr;
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		printf("Socket creation failed: %s...\n", strerror(errno));
		return 1;
	}
	
	// Since the tester restarts your program quite often, setting SO_REUSEADDR
	// ensures that we don't run into 'Address already in use' errors
	int reuse = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		printf("SO_REUSEADDR failed: %s \n", strerror(errno));
		return 1;
	}
	
	struct sockaddr_in serv_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(9092),
		.sin_addr = { htonl(INADDR_ANY) },
	};
	
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("Bind failed: %s \n", strerror(errno));
		return 1;
	}
	
	int connection_backlog = 5;
	if (listen(server_fd, connection_backlog) != 0) {
		printf("Listen failed: %s \n", strerror(errno));
		return 1;
	}

    if (atexit(close_server) != 0) {
        printf("[ERROR] Could not register server cleanup function.\n");
        return 1;
    }
	
	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);

    // You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");

    while (1) {
        int *client_fd = (int *)malloc(sizeof(int));  // Thread process to free
        if (!client_fd) {
            printf("[ERROR] Could not allocate client_fd for client connection.\n");
        }
        *client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
        printf("Client connected\n");

        thrd_t t;
        thrd_create(&t, service_request, client_fd);
        thrd_detach(t);
    }

	close(server_fd);
	return 0;
}
