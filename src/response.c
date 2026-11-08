#include <stdint.h>


#define API_VERSIONS_KEY 18

#define ERROR_UNKNOWN_SERVER_ERROR -1
#define ERROR_NONE 0
#define ERROR_UNSUPPORTED_VERSION 35


struct APIKeysElement {
    int16_t api_key;
    int16_t min_version;
    int16_t max_version;
    uint8_t tag_buffer;
};

struct APIVersions_ResponseBody {
    int16_t error_code;
    uint8_t api_keys_arr_len;
    struct APIKeysElement *api_keys;
    int32_t throttle_time_ms;
    uint8_t tag_buffer;
};

struct ResponseHeader_v0 {
    int32_t correlation_id;
};

struct ResponseBody {
    int16_t type_key;
    union {
        struct APIVersions_ResponseBody api_versions_body;
    } body;
};

struct Response_v0 {
    struct ResponseHeader_v0 header;
    struct ResponseBody body;
};


struct APIVersions_ResponseBody construct_api_versions_body(int16_t version)
{
    struct APIVersions_ResponseBody body = { 0 };
    if (version < 0 || version > 4) {
        body.error_code = ERROR_UNSUPPORTED_VERSION;
        return body;
    }

    body.api_keys = (struct APIKeysElement *)malloc(sizeof(struct APIKeysElement));
    if (!body.api_keys) {
        body.error_code = ERROR_UNKNOWN_SERVER_ERROR;
        return body;
    }
    body.api_keys_arr_len = 2;
    body.api_keys->api_key = 18;
    body.api_keys->min_version = 0;
    body.api_keys->max_version = 4;
    body.api_keys->tag_buffer = 0;


    return body;
}

void destroy_api_versions_body(struct APIVersions_ResponseBody *body)
{
    if (body->api_keys) free(body->api_keys);
}

ssize_t serialise_api_versions_body(struct APIVersions_ResponseBody *body, uint8_t *buf)
{
    ssize_t size = 0;
    for (int i = 2; i--; ) buf[size++] = (uint8_t)((body->error_code & (0xFF00 >> ((1-i)*8))) >> ((i)*8));
    buf[size++] = body->api_keys_arr_len;
    for (int j = 0; j < body->api_keys_arr_len - 1; j++) {
        for (int i = 2; i--; ) buf[size++] = (uint8_t)((body->api_keys[j].api_key & (0xFF00 >> ((1-i)*8))) >> ((i)*8));
        for (int i = 2; i--; ) buf[size++] = (uint8_t)((body->api_keys[j].min_version & (0xFF00 >> ((1-i)*8))) >> ((i)*8));
        for (int i = 2; i--; ) buf[size++] = (uint8_t)((body->api_keys[j].max_version & (0xFF00 >> ((1-i)*8))) >> ((i)*8));
        buf[size++] = body->api_keys[j].tag_buffer;
    }
    for (int i = 0; i < 4; i++) buf[size++] = (uint8_t)((body->throttle_time_ms & (0xFF000000 >> (i*8))) >> ((3-i)*8));
    buf[size++] = body->tag_buffer;

    return size;
}

void print_api_versions_body(struct APIVersions_ResponseBody *body)
{
    printf("  .error_code        %04X\n", body->error_code);
    printf("  .api_keys_arr_len  %02X\n", body->api_keys_arr_len);
    printf("  .api_keys          ");
    for (int i = 0; i < body->api_keys_arr_len - 1; i++) {
        printf("%04X ", body->api_keys[i].api_key);
        printf("%04X ", body->api_keys[i].min_version);
        printf("%04X ", body->api_keys[i].max_version);
        printf("%02X", body->api_keys[i].tag_buffer);
        if (i < body->api_keys_arr_len - 2) printf(" ");
        else printf("\n");
    }
    printf("  .throttle_time_ms  %08X\n", body->throttle_time_ms);
    printf("  .tag_buffer        %02X\n", body->tag_buffer);
}


struct Response_v0 response_from_request(struct Request_v2 *req)
{
    // Header
    struct ResponseHeader_v0 header = { .correlation_id = req->header.correlation_id };

    // Body
    struct ResponseBody body;
    switch (req->header.request_api_key) {
        case API_VERSIONS_KEY:
            body = (struct ResponseBody){
                .type_key = req->header.request_api_key,
                .body = construct_api_versions_body(req->header.request_api_version)
            };
            break;
        default:
            body = (struct ResponseBody){ 0 };
            break;
    }

    return (struct Response_v0){
        .header = header,
        .body = body
    };
}


/*
 *  Assumes buf is big enough
 */
ssize_t serialise_response(struct Response_v0* resp, uint8_t* buf)
{
    ssize_t size = -1;
    // Serialise body
    switch (resp->body.type_key) {
        case API_VERSIONS_KEY:
            size = serialise_api_versions_body((struct APIVersions_ResponseBody*)&resp->body.body, buf+8);
    }
    if (size < 0) return -1;
    size += 4;
    // Serialise message size
    for (int i = 0; i < 4; i++) *buf++ = (uint8_t)((size & (0xFF000000 >> (i*8))) >> ((3-i)*8));
    // Serialise header
    for (int i = 0; i < 4; i++) *buf++ = (uint8_t)((resp->header.correlation_id & (0xFF000000 >> (i*8))) >> ((3-i)*8));

    return size;
}

void destroy_response(struct Response_v0* resp)
{
    switch (resp->body.type_key) {
        case API_VERSIONS_KEY:
            destroy_api_versions_body((struct APIVersions_ResponseBody*)&resp->body.body);
            break;
    }
}


void print_response_header_v0(struct ResponseHeader_v0 *resp)
{
    printf("RESPONSE HEADER v0:\n");
    printf("  .correlation_id  %08X\n", resp->correlation_id);
}

void print_response_body(struct ResponseBody *body)
{
    printf("RESPONSE BODY:\n");
    switch (body->type_key) {
        case API_VERSIONS_KEY:
            print_api_versions_body((struct APIVersions_ResponseBody*)&body->body);
            break;
    }
}

void print_response_v0(struct Response_v0 *resp)
{
    printf("RESPONSE v0 ------------------------------\n");
    print_response_header_v0(&resp->header);
    print_response_body(&resp->body);
    printf("------------------------------------------\n");
}
