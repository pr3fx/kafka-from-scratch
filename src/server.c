int service_request(void *arg)
{
    int *client_fd = arg;
    uint8_t read_buf[1024];

    while (1) {
        // Peek for available data, if not close connection and return
        int8_t res = recv(*client_fd, read_buf, 1, MSG_PEEK | MSG_DONTWAIT);
        if (res <= 0) {
            break;
        }
        // Receive request
        size_t read_size = read(*client_fd, read_buf, sizeof(read_buf)-1);
        printf("Received %zu bytes from client:\n", read_size);
        struct Request_v2 req = deser_request_v2(read_buf, read_size);
        print_request_header_v2(&req.header);

        // Construct response
        struct Response_v0 resp = response_from_request(&req);
        print_response_v0(&resp);
        uint8_t ser_buf[256];
        ssize_t message_size = serialise_response(&resp, ser_buf);
        if (message_size < 0) {
            destroy_response(&resp);
            destroy_request_v2(&req);
            close(*client_fd);
        }

        // Send response
        size_t bytes_to_write = message_size+4;
        write(*client_fd, ser_buf, bytes_to_write);
        printf("Sent %zu bytes to client\n", bytes_to_write);
        destroy_response(&resp);
        destroy_request_v2(&req);
    }

    close(*client_fd);

    return 0;
}
