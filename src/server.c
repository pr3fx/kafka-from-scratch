int service_request(void *arg)
{
    int client_fd;
    if (arg) {
        client_fd = *(int*)arg;
        free(arg);
    } else {
        printf("Could not read client_fd\n");
        return -1;
    }
    uint8_t read_buf[1024];  // TODO: change to dynamically allocate based on
                             // received header

    while (1) {
        // Peek for available data, if not close connection and return
        int8_t res = recv(client_fd, read_buf, 1, MSG_PEEK);
        if (res <= 0) {
            break;
        }
        // Receive request
        size_t read_size = read(client_fd, read_buf, sizeof(read_buf)-1);
        printf("Received %zu bytes from client:\n", read_size);
        struct Request_v2 req = deser_request_v2(read_buf, read_size);
        print_request_header_v2(&req.header);

        // Construct response
        struct Response resp = response_from_request(&req);
        print_response(&resp);
        uint8_t ser_buf[256];
        ssize_t message_size = serialise_response(&resp, ser_buf);
        if (message_size < 0) {
            destroy_response(&resp);
            destroy_request_v2(&req);
            close(client_fd);
        }

        // Send response
        write(client_fd, ser_buf, message_size);
        printf("Sent %zu bytes to client\n", message_size);
        destroy_response(&resp);
        destroy_request_v2(&req);
    }

    close(client_fd);

    return 0;
}
