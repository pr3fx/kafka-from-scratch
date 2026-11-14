#include <stdint.h>


#define CLIENT_ID_DEFAULT_SIZE 256

#define API_VERSIONS_KEY 18
#define DESCRIBE_TOPIC_PARTITIONS_KEY 75


struct DescribeTopicPartitions_RequestBody {
    uint8_t topics_len;
    sds *topics;
    int32_t response_partition_limit;
    struct TopicCursor cursor;
};

ssize_t deser_describe_topic_partitions_request(uint8_t const *buf, struct DescribeTopicPartitions_RequestBody *req)
{
    ssize_t buf_pos = 0;
    uint8_t tag_buffer;
    req->topics_len = buf[buf_pos++];
    req->topics = (sds *)malloc(sizeof(sds)*(req->topics_len-1));
    for (int i = 0; i < req->topics_len-1; i++) {
        size_t name_len = buf[buf_pos++];
        req->topics[i] = sdsnewlen(&buf[buf_pos], name_len-1); buf_pos += name_len-1;
    }
    tag_buffer = buf[buf_pos++];
    req->response_partition_limit = DESER_INT32(&buf[buf_pos]); buf_pos += 4;
    req->cursor = (struct TopicCursor){ 0 }; buf_pos += 2;
    tag_buffer = buf[buf_pos++];

    return buf_pos;
}

void destroy_describe_topic_partitions_request(struct DescribeTopicPartitions_RequestBody *req)
{
    if (req->topics) {
        for (int i = 0; i < req->topics_len-1; i++) sdsfree(req->topics[i]);
        free(req->topics);
    }
}

struct RequestHeader_v2 {
    int16_t request_api_key;
    int16_t request_api_version;
    int32_t correlation_id;
    sds client_id;
};

struct RequestBody {
    int16_t type_key;
    union {
        struct DescribeTopicPartitions_RequestBody describe_topic_partitions_body;
    } body;
};

struct Request_v2 {
    int32_t message_size;
    struct RequestHeader_v2 header;
    struct RequestBody body;
};


void destroy_request_v2(struct Request_v2 *req)
{
    sdsfree(req->header.client_id);
    switch (req->body.type_key) {
        case DESCRIBE_TOPIC_PARTITIONS_KEY:
            destroy_describe_topic_partitions_request(&req->body.body.describe_topic_partitions_body);
            break;
    }
}

void print_client_id_bytes(sds client_id)
{
    for (int i = 0; i < sdslen(client_id); i++) {
        printf("%02X", (uint8_t)client_id[i]);
        if (i < sdslen(client_id)) printf(" ");
    }
}

ssize_t deser_request_header_v2(uint8_t const *buf, struct RequestHeader_v2 *header)
{
    ssize_t buf_pos = 0;

    header->request_api_key = DESER_INT16(&buf[buf_pos]); buf_pos += 2;
    header->request_api_version = DESER_INT16(&buf[buf_pos]); buf_pos += 2;
    header->correlation_id = DESER_INT32(&buf[buf_pos]); buf_pos += 4;
    uint16_t client_id_len = DESER_INT16(&buf[buf_pos]); buf_pos += 2;
    // Copy over client id (message_size does not include the message_size field itself)
    // First two bytes of client_id are the length of the client_id (-1 means null value
    // and terminates the client_id byte sequence)
    // From C23 unsigned to signed int casting is well-defined
    if ((int16_t)(client_id_len) < 0) return buf_pos;
    header->client_id = sdsnewlen(&buf[buf_pos], client_id_len); buf_pos += client_id_len;
    uint8_t tag_buffer = buf[buf_pos++];

    return buf_pos;
}

struct Request_v2 deser_request_v2(const uint8_t *buf, size_t buf_len)
{
    ssize_t buf_pos = 0;
    if (buf_len < 4) return (struct Request_v2){ 0 };

    // Deserialise message size
    int32_t message_size = DESER_INT32(&buf[buf_pos]); buf_pos += 4;

    // Deserialise header
    struct RequestHeader_v2 header;
    buf_pos += deser_request_header_v2(&buf[buf_pos], &header);

    // Deserialise body
    struct RequestBody body = { 0 };
    switch (header.request_api_key) {
        case DESCRIBE_TOPIC_PARTITIONS_KEY:
            body.type_key = DESCRIBE_TOPIC_PARTITIONS_KEY;
            buf_pos += deser_describe_topic_partitions_request(&buf[buf_pos], &body.body.describe_topic_partitions_body);
            break;
    }

    return (struct Request_v2){
        .message_size = message_size,
        .header = header,
        .body = body
    };
}

void print_request_header_v2(struct RequestHeader_v2* header)
{
    printf("REQUEST HEADER v2\n");
    printf("  .request_api_key:     %d [0x%04X]\n", header->request_api_key, (uint16_t)header->request_api_key);
    printf("  .request_api_version: %d [0x%04X]\n", header->request_api_version, (uint16_t)header->request_api_version);
    printf("  .correlation_id:      %d [0x%08X]\n", header->correlation_id, (uint32_t)header->correlation_id);
    printf("  .client_id:           "); print_client_id_bytes(header->client_id);
    printf("\n\n");
}
