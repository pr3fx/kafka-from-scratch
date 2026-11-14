struct TopicCursor {
    sds topic_name;
    int32_t partition_index;
};

ssize_t serialise_topic_cursor(struct TopicCursor *cursor, uint8_t *buf)
{
    ssize_t size = 0;
    if (sdslen(cursor->topic_name)) {
        buf[size++] = (uint8_t)sdslen(cursor->topic_name);
        for (int i = 0; i < sdslen(cursor->topic_name); i++) {
            buf[size++] = (uint8_t)cursor->topic_name[i];
        }
        SERIALISE_INT32(&buf[size], cursor->partition_index); size += 4;
    } else {
        buf[size++] = 0xFFU;
    }
    buf[size++] = 0;  // TAG_BUFFER
    return size;
}

void destroy_topic_cursor(struct TopicCursor *cursor)
{
    sdsfree(cursor->topic_name);
}

struct Partition {
    int16_t error_code;
    int32_t partition_index;
    int32_t leader_id;
    int32_t leader_epoch;
    int32_t replica_nodes;
    int32_t isr_nodes;
    int32_t eligible_leader_replicas;
    int32_t last_known_elr;
    int32_t offline_replicas;
};

struct Topic {
    int16_t error_code;
    sds name;
    uint8_t topic_id[16];
    bool is_internal;
    uint8_t partitions_len;
    struct Partition *partitions;
    int32_t topic_authorized_operations;
};

struct Topic construct_topic(int16_t error_code, sds name)
{
    struct Topic topic = (struct Topic){ 0 };
    topic.error_code = error_code;
    topic.name = sdsdup(name);
    topic.is_internal = false;
    topic.partitions_len = 1;
    topic.partitions = NULL;
    topic.topic_authorized_operations = 0;
    return topic;
}

ssize_t serialise_topic(struct Topic *topic, uint8_t *buf)
{
    ssize_t size = 0;
    SERIALISE_INT16(&buf[size], topic->error_code); size += 2;
    buf[size++] = (uint8_t)sdslen(topic->name);
    for (int i = 0; i < sdslen(topic->name); i++) buf[size++] = (uint8_t)topic->name[i];
    for (int i = 0; i < 16; i++) buf[size++] = topic->topic_id[i];
    buf[size++] = (uint8_t)topic->is_internal;
    buf[size++] = topic->partitions_len;
    // TODO: add Partition serialisation
    SERIALISE_INT32(&buf[size], topic->topic_authorized_operations); size += 4;
    buf[size++] = 0;  // TAG_BUFFER
    return size;
}

void destroy_topic(struct Topic *topic)
{
    sdsfree(topic->name);
    if (topic->partitions) free(topic->partitions);
}
