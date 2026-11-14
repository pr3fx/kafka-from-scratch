#include <stdint.h>


#define ERROR_UNKNOWN_SERVER_ERROR -1
#define ERROR_NONE 0
#define ERROR_UNKNOWN_TOPIC_OR_PARTITION 3
#define ERROR_UNSUPPORTED_VERSION 35


/*
 *   RESPONSE BODY DEFINITIONS AND METHODS (constructor/destructor)
 */
// DescribeTopicPartitions ----------------------------------------------------
struct DescribeTopicPartitions_ResponseBody {
    int32_t throttle_time_ms;
    uint8_t topics_len;
    struct Topic *topics;
    struct TopicCursor next_cursor;
};

void print_describe_topic_partitions_body(struct DescribeTopicPartitions_ResponseBody *body)
{
    printf("  .throttle_time_ms  %04X\n", body->throttle_time_ms);
    printf("  .topics_len        %01X\n", body->topics_len);
}


struct DescribeTopicPartitions_ResponseBody construct_describe_topic_partitions_body(struct DescribeTopicPartitions_RequestBody *req)
{
    struct DescribeTopicPartitions_ResponseBody body = { 0 };
    body.topics = (struct Topic *)malloc(sizeof(struct Topic)*req->topics_len);
    for (int i = 0; i < req->topics_len-1; i++) {
        body.topics[i] = construct_topic(ERROR_UNKNOWN_TOPIC_OR_PARTITION, req->topics[i]);
    }
    body.topics_len = req->topics_len;
    body.next_cursor = (struct TopicCursor){ .topic_name=sdsempty(), .partition_index=0 };

    return body;
}

void destroy_describe_topic_partitions_body(struct DescribeTopicPartitions_ResponseBody* body)
{
    if (body->topics) {
        for (int i = 1; i < body->topics_len; i++) destroy_topic(&body->topics[i]);
        free(body->topics);
    }
    destroy_topic_cursor(&body->next_cursor);
}

ssize_t serialise_describe_topic_partitions_body(struct DescribeTopicPartitions_ResponseBody *body, uint8_t *buf)
{
    ssize_t size = 0;
    SERIALISE_INT32(&buf[size], body->throttle_time_ms); size += 4;
    buf[size++] = body->topics_len;
    for (int i = 0; i < body->topics_len-1; i++) {
        size += serialise_topic(&body->topics[i], &buf[size]);
    }
    size += serialise_topic_cursor(&body->next_cursor, &buf[size]);

    return size;
}


// APIVersions ----------------------------------------------------------------
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

struct APIVersions_ResponseBody construct_api_versions_body(int16_t version)
{
    static const struct APIKeysElement supported_apis[] = {
        (struct APIKeysElement){ .api_key=API_VERSIONS_KEY, .min_version=0, .max_version=4, .tag_buffer=0 },  // APIVersions
        (struct APIKeysElement){ .api_key=DESCRIBE_TOPIC_PARTITIONS_KEY, .min_version=0, .max_version=0, .tag_buffer=0 }   // DescribeTopicPartitions
    };
    struct APIVersions_ResponseBody body = { 0 };
    if (version < 0 || version > 4) {
        body.error_code = ERROR_UNSUPPORTED_VERSION;
        return body;
    }

    body.api_keys_arr_len = 1 + sizeof(supported_apis) / sizeof(supported_apis[0]);

    body.api_keys = (struct APIKeysElement *)malloc(sizeof(supported_apis));
    if (!body.api_keys) {
        body.error_code = ERROR_UNKNOWN_SERVER_ERROR;
        return body;
    }
    for (int i = 0; i < body.api_keys_arr_len; i++) {
        body.api_keys[i] = supported_apis[i];
    }

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

/*
 *  FULL RESPONSE DEFINITIONS AND METHODS
 */
struct ResponseHeader {
    int32_t correlation_id;
};

struct ResponseBody {
    int16_t type_key;
    union {
        struct APIVersions_ResponseBody api_versions_body;
        struct DescribeTopicPartitions_ResponseBody describe_topic_partitions_body;
    } body;
};

struct Response {
    struct ResponseHeader header;
    struct ResponseBody body;
};


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


struct Response response_from_request(struct Request_v2 *req)
{
    // Header
    struct ResponseHeader header = { .correlation_id = req->header.correlation_id };

    // Body
    struct ResponseBody body;
    switch (req->header.request_api_key) {
        case API_VERSIONS_KEY:
            body = (struct ResponseBody){
                .type_key = API_VERSIONS_KEY,
                .body.api_versions_body = construct_api_versions_body(req->header.request_api_version)
            };
            break;
        case DESCRIBE_TOPIC_PARTITIONS_KEY:
            body = (struct ResponseBody){
                .type_key = DESCRIBE_TOPIC_PARTITIONS_KEY,
                .body.describe_topic_partitions_body = construct_describe_topic_partitions_body((struct DescribeTopicPartitions_RequestBody *)&req->body.body)
            };
            break;
        default:
            body = (struct ResponseBody){ 0 };
            break;
    }

    return (struct Response){
        .header = header,
        .body = body
    };
}


/*
 *  Assumes buf is big enough
 */
ssize_t serialise_response(struct Response* resp, uint8_t* buf)
{
    ssize_t size = -1;
    // Serialise body
    switch (resp->body.type_key) {
        case API_VERSIONS_KEY:
            size = serialise_api_versions_body((struct APIVersions_ResponseBody*)&resp->body.body, buf+9);
            break;
        case DESCRIBE_TOPIC_PARTITIONS_KEY:
            size = serialise_describe_topic_partitions_body((struct DescribeTopicPartitions_ResponseBody *)&resp->body.body, buf+9);
            break;
    }
    if (size < 0) return -1;
    // Serialise header
    SERIALISE_INT32(&buf[4], resp->header.correlation_id); size += 4;
    buf[8] = 0; size++; // TAG_BUFFER
    // Serialise message size
    SERIALISE_INT32(buf, size); size += 4;

    return size;
}

void destroy_response(struct Response* resp)
{
    switch (resp->body.type_key) {
        case API_VERSIONS_KEY:
            destroy_api_versions_body((struct APIVersions_ResponseBody*)&resp->body.body);
            break;
        case DESCRIBE_TOPIC_PARTITIONS_KEY:
            destroy_describe_topic_partitions_body((struct DescribeTopicPartitions_ResponseBody *)&resp->body.body);
            break;
    }
}


void print_response_header(struct ResponseHeader *resp)
{
    printf("RESPONSE HEADER:\n");
    printf("  .correlation_id  %08X\n", resp->correlation_id);
    printf("  .tag_buffer      00\n");
}

void print_response_body(struct ResponseBody *body)
{
    printf("RESPONSE BODY:\n");
    switch (body->type_key) {
        case API_VERSIONS_KEY:
            print_api_versions_body((struct APIVersions_ResponseBody*)&body->body);
            break;
        case DESCRIBE_TOPIC_PARTITIONS_KEY:
            print_describe_topic_partitions_body(&body->body.describe_topic_partitions_body);
            break;
    }
}

void print_response(struct Response *resp)
{
    printf("RESPONSE ---------------------------------\n");
    print_response_header(&resp->header);
    print_response_body(&resp->body);
    printf("------------------------------------------\n");
}
