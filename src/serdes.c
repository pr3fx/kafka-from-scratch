#define SERIALISE_INT32(buf, val) do { \
    uint32_t tmp = (uint32_t)(val); \
    (buf)[0] = (uint8_t)((tmp >> 24) & 0xFFU); \
    (buf)[1] = (uint8_t)((tmp >> 16) & 0xFFU); \
    (buf)[2] = (uint8_t)((tmp >> 8)  & 0xFFU); \
    (buf)[3] = (uint8_t)(tmp & 0xFFU); \
} while (0)

#define DESER_INT32(buf) ((int32_t)( \
    ((uint32_t)(buf)[0] << 24) |       \
    ((uint32_t)(buf)[1] << 16) |       \
    ((uint32_t)(buf)[2] << 8)  |       \
    ((uint32_t)(buf)[3])               \
))

#define SERIALISE_INT16(buf, val) do { \
    uint16_t tmp = (uint32_t)(val); \
    (buf)[0] = (uint8_t)((tmp >> 8)  & 0xFFU); \
    (buf)[1] = (uint8_t)(tmp & 0xFFU); \
} while (0)

#define DESER_INT16(buf) ((int16_t)( \
    ((uint16_t)(buf)[0] << 8)  |       \
    ((uint16_t)(buf)[1])               \
))
