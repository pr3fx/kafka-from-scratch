#include <stdint.h>


#define CLIENT_ID_DEFAULT_SIZE 256

struct ClientID {
    size_t size;
    size_t capacity;
    uint8_t *data;
};

struct RequestHeader_v2 {
    int32_t message_size;
    int16_t request_api_key;
    int16_t request_api_version;
    int32_t correlation_id;
    struct ClientID client_id;
};


void destroy_request_header_v2(struct RequestHeader_v2 *header)
{
    free(header->client_id.data);
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

struct RequestHeader_v2 deser_request_header_v2(uint8_t* buf, size_t buf_len)
{
    struct RequestHeader_v2 req = { 0 };
    size_t buf_pos = 0;
    // Deserialise message size
    for (int i = 0; i < 4; i++) req.message_size |= (int32_t)buf[buf_pos++] << ((3-i)*8);
    // Deserialise request api key
    for (int i = 0; i < 2; i++) req.request_api_key |= (int16_t)buf[buf_pos++] << ((1-i)*8);
    // Deserialise request api version
    for (int i = 0; i < 2; i++) req.request_api_version |= (int16_t)buf[buf_pos++] << ((1-i)*8);
    // Deserialise correlation id
    for (int i = 0; i < 4; i++) req.correlation_id |= (int32_t)buf[buf_pos++] << ((3-i)*8);
    // Copy over client id (message_size does not include the message_size field itself)
    while (buf_pos < req.message_size + 4) append_to_client_id(&req.client_id, buf[buf_pos++]);

    return req;
}


void print_request_header_v2(struct RequestHeader_v2* header)
{
    printf("REQUEST HEADER v2\n");
    printf("  .message_size:        %08X\n", (uint32_t)header->message_size);
    printf("  .request_api_key:     %04X\n", (uint16_t)header->request_api_key);
    printf("  .request_api_version: %04X\n", (uint16_t)header->request_api_version);
    printf("  .correlation_id:      %08X\n", (uint32_t)header->correlation_id);
    printf("  .client_id:           "); print_client_id_bytes(&header->client_id);
    printf("\n\n");
}
