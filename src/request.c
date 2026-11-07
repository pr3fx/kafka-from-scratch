#include <stdint.h>


struct RequestHeader_v2 {
    int32_t message_size;
    int16_t request_api_key;
    int16_t request_api_version;
    int32_t correlation_id;
    char* client_id;
};


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
    // Copy over client id
    req.client_id = (char*)malloc(buf_len-buf_pos);
    size_t idx = 0;
    while (buf_pos < req.message_size) req.client_id[idx++] = buf[buf_pos++];

    return req;
}


void print_request_header_v2(struct RequestHeader_v2* header)
{
    printf("REQUEST HEADER v2\n");
    printf("  .message_size:        %08X\n", (uint32_t)header->message_size);
    printf("  .request_api_key:     %04X\n", (uint16_t)header->request_api_key);
    printf("  .request_api_version: %04X\n", (uint16_t)header->request_api_version);
    printf("  .correlation_id:      %08X\n", (uint32_t)header->correlation_id);
    printf("\n");
}
