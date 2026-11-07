#include <stdint.h>


struct ResponseHeader_v0 {
    int32_t correlation_id;
};

struct ResponseBody {
    int16_t error_code;
};

struct Response_v0 {
    int32_t message_size;
    struct ResponseHeader_v0 header;
    struct ResponseBody body;
};


#define API_VERSIONS_API 18

#define ERROR_UNKNOWN_SERVER_ERROR -1
#define ERROR_NONE 0
#define ERROR_UNSUPPORTED_VERSION 35

struct Response_v0 response_from_request(struct Request_v2 *req)
{
    struct ResponseHeader_v0 header = { .correlation_id = req->header.correlation_id };
    int16_t error_code = 0;
    switch (req->header.request_api_key) {
        case API_VERSIONS_API:
            if (req->header.request_api_version >= 0 && req->header.request_api_version < 5) {
                error_code = ERROR_NONE;
            } else { error_code = ERROR_UNSUPPORTED_VERSION; }
            break;
        default:
            error_code = ERROR_UNKNOWN_SERVER_ERROR;
            break;
    }
    struct ResponseBody body = { .error_code = error_code };
    struct Response_v0 resp = { .message_size = 6, .header = header, .body = body };

    return resp;
}


/*
 *  Assumes buf is big enough
 */
void serialise_response(struct Response_v0* resp, uint8_t* buf)
{
    // Serialise message size
    for (int i = 0; i < 4; i++) *buf++ = (uint8_t)((resp->message_size & (0xFF000000 >> (i*8))) >> ((3-i)*8));
    // Serialise header
    for (int i = 0; i < 4; i++) *buf++ = (uint8_t)((resp->header.correlation_id & (0xFF000000 >> (i*8))) >> ((3-i)*8));
    // Seriales body
    for (int i = 2; i--; ) *buf++ = (uint8_t)((resp->body.error_code & (0xFF00 >> ((1-i)*8))) >> ((i)*8));
}


void print_response_header_v0(struct ResponseHeader_v0* resp)
{
    printf("RESPONSE HEADER v0:\n");
    printf("  .correlation_id  %08X\n", resp->correlation_id);
    printf("\n");
}
