#include <stdint.h>


#define CLIENT_ID_DEFAULT_SIZE 256

struct ClientID {
    size_t size;
    size_t capacity;
    uint8_t *data;
};

struct RequestHeader_v2 {
    int16_t request_api_key;
    int16_t request_api_version;
    int32_t correlation_id;
    struct ClientID client_id;
};

struct Request_v2 {
    int32_t message_size;
    struct RequestHeader_v2 header;
};


void destroy_request_v2(struct Request_v2 *req)
{
    free(req->header.client_id.data);
}

int expand_client_id(struct ClientID *client_id)
{
    const size_t new_capacity = client_id->capacity + CLIENT_ID_DEFAULT_SIZE;
    void *tmp = realloc(client_id->data, new_capacity);
    if (!tmp) return -1;
    client_id->data = (uint8_t *)tmp;
    client_id->capacity = new_capacity;
    return 0;
}

int append_to_client_id(struct ClientID *client_id, const uint8_t byte)
{
    if (client_id->size == client_id->capacity) {
        if (expand_client_id(client_id) < 0) return -1;
    }
    client_id->data[client_id->size++] = byte;
    return 0;
}

void print_client_id_bytes(struct ClientID *client_id)
{
    size_t idx = 0;
    while (idx < client_id->size) {
        printf("%02X", client_id->data[idx++]);
        if (idx < client_id->size) printf(" ");
    }
}

struct RequestHeader_v2 deser_request_header_v2(const uint8_t *buf, size_t buf_len)
{
    struct RequestHeader_v2 req = { 0 };
    if (buf_len < 10) return req; // Protects until after deser client_id len

    // Deserialise request api key
    int i = 0;
    for (i = 0; i < 2; i++) req.request_api_key |= (int16_t)*buf++ << ((1-i)*8);
    // Deserialise request api version
    for (i = 0; i < 2; i++) req.request_api_version |= (int16_t)*buf++ << ((1-i)*8);
    // Deserialise correlation id
    for (i = 0; i < 4; i++) req.correlation_id |= (int32_t)*buf++ << ((3-i)*8);
    // Copy over client id (message_size does not include the message_size field itself)
    // First two bytes of client_id are the length of the client_id (-1 means null value
    // and terminates the client_id byte sequence)
    uint16_t client_id_len = 0;
    for (i = 2; i--; ) {
        client_id_len |= (uint16_t)(*buf) << ((i)*8);
        append_to_client_id(&req.client_id, *buf++);
    }
    // From C23 unsigned to signed int casting is well-defined
    if ((int16_t)(client_id_len) < 0) return req;
    if (buf_len < 10 + client_id_len) {
        free(req.client_id.data);
        return (struct RequestHeader_v2){ 0 };
    }

    for (i = client_id_len; i--; ) append_to_client_id(&req.client_id, *buf++);

    return req;
}

struct Request_v2 deser_request_v2(const uint8_t *buf, size_t buf_len)
{
    if (buf_len < 4) return (struct Request_v2){ 0 };
    // Deserialise message size
    int32_t message_size = 0;
    for (int i = 0; i < 4; i++) message_size |= (int32_t)*buf++ << ((3-i)*8);
    // Deserialise header
    struct RequestHeader_v2 header = deser_request_header_v2(buf, buf_len-4);
    struct Request_v2 req = { .message_size = message_size, .header = header };

    return req;
}

void print_request_header_v2(struct RequestHeader_v2* header)
{
    printf("REQUEST HEADER v2\n");
    printf("  .request_api_key:     %d [0x%04X]\n", header->request_api_key, (uint16_t)header->request_api_key);
    printf("  .request_api_version: %d [0x%04X]\n", header->request_api_version, (uint16_t)header->request_api_version);
    printf("  .correlation_id:      %d [0x%08X]\n", header->correlation_id, (uint32_t)header->correlation_id);
    printf("  .client_id:           "); print_client_id_bytes(&header->client_id);
    printf("\n\n");
}
