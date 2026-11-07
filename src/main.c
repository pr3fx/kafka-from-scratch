#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "request.c"
#include "response.c"


int main(int argc, char* argv[]) {
	// Disable output buffering
	setbuf(stdout, NULL);
 	setbuf(stderr, NULL);
	
	int server_fd, client_addr_len;
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
	
	printf("Waiting for a client to connect...\n");
	client_addr_len = sizeof(client_addr);

    // You can use print statements as follows for debugging, they'll be visible when running tests.
	printf("Logs from your program will appear here!\n");

	int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, &client_addr_len);
	printf("Client connected\n");
    uint8_t read_buf[1024];
    size_t read_size = read(client_fd, read_buf, sizeof(read_buf)-1);

    printf("Received %zu bytes from client:\n", read_size);
    struct Request_v2 req = deser_request_v2(read_buf, read_size);
    print_request_header_v2(&req.header);

    struct ResponseHeader_v0 resp = construct_response(req.correlation_id);
    print_response_header_v0(&resp);
    uint8_t ser_buf[8];
    serialise_response(&resp, ser_buf);
    printf("ser_buf = [");
    for (int i = 0; i < 8; i++) {
        if (i != 0) printf(" ");
        printf("%d", ser_buf[i]);
    }
    printf("]\n");
    printf("Writing back response with correlation_id: %08X\n", (uint32_t)resp.correlation_id);

    write(client_fd, ser_buf, resp.message_size+4);
    printf("Sent bytes to client\n");
	close(client_fd);
	
	close(server_fd);

    destroy_request_v2(&req);
	return 0;
}
