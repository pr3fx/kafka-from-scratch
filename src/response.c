#include <stdint.h>


struct ResponseHeader_v0 {
    int32_t message_size;
    int32_t correlation_id;
};


struct ResponseHeader_v0 construct_response(const int32_t correlation_id)
{
    struct ResponseHeader_v0 resp = { 
        .message_size = (int32_t)4,
        .correlation_id = correlation_id 
    };
    return resp;
}


/*
 *  Assumes buf is big enough
 */
void serialise_response(struct ResponseHeader_v0* resp, uint8_t* buf)
{
    // Serialise message size
    for (int i = 0; i < 4; i++) buf[i] = (uint8_t)((resp->message_size & (0xFF000000 >> (i*8))) >> ((3-i)*8));
    // Serialise correlation id
    for (int i = 0; i < 4; i++) buf[i+4] = (uint8_t)((resp->correlation_id & (0xFF000000 >> (i*8))) >> ((3-i)*8));
}


void print_response_header_v0(struct ResponseHeader_v0* resp)
{
    printf("RESPONSE HEADER v0:\n");
    printf("  .message_size    %08X\n", resp->message_size);
    printf("  .correlation_id  %08X\n", resp->correlation_id);
    printf("\n");
}
