#include <stdint.h>


struct response {
    int32_t message_size;
    int32_t correlation_id;
};


/*
 *  Hardcoded message size for now
 */
struct response construct_response(const int32_t correlation_id)
{
    struct response resp = { .message_size = (int32_t)8, .correlation_id = (int32_t)7 };
    return resp;
}


/*
 *  Assumes ser_buf is big enough. Hardcode field serialisation for now
 */
void serialise_response(struct response* response, uint8_t* ser_buf)
{
    // Serialise message size
    for (int i = 0; i < 4; i++) ser_buf[i] = (uint8_t)((response->message_size & (0xFF000000 >> (i*8))) >> ((3-i)*8));
    // Serialise correlation id
    for (int i = 0; i < 4; i++) ser_buf[i+4] = (uint8_t)((response->correlation_id & (0xFF000000 >> (i*8))) >> ((3-i)*8));
}
