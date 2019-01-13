#include <safe32/safe32.h>
#include "safe32_version.h"

// #define KSLogger_LocalLevel TRACE
#include "kslogger.h"

#define DECODED_BYTES_PER_GROUP 5
#define ENCODED_CHARS_PER_GROUP 8
#define ENCODED_BITS_PER_BYTE   5
#define DECODED_BITS_PER_BYTE   8

#define CHARACTER_CODE_ERROR      0x7f
#define CHARACTER_CODE_WHITESPACE 0x7e

static const uint8_t g_encode_char_to_decode_value[] =
{
#define ERRR CHARACTER_CODE_ERROR
#define WHTE CHARACTER_CODE_WHITESPACE
            ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
/* wh sp */ ERRR, WHTE, WHTE, ERRR, ERRR, WHTE, ERRR, ERRR,
            ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
            ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
/* space */ WHTE, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
/* -     */ ERRR, ERRR, ERRR, ERRR, ERRR, WHTE, ERRR, ERRR,
/* 0-7   */ 0x00, ERRR, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
/* 8-9   */ 0x07, 0x08, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
/* A-G   */ ERRR, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
/* H-O   */ 0x10, ERRR, 0x11, 0x12, ERRR, 0x13, 0x14, 0x00,
/* P-W   */ 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c,
/* X-Z   */ 0x1d, 0x1e, 0x1f, ERRR, ERRR, ERRR, ERRR, ERRR,
/* a-g   */ ERRR, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
/* h-o   */ 0x10, ERRR, 0x11, 0x12, ERRR, 0x13, 0x14, 0x00,
/* p-w   */ 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c,
/* x-z   */ 0x1d, 0x1e, 0x1f, ERRR, ERRR, ERRR, ERRR, ERRR,
            ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
            ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
            ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
            ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
            ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
            ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
            ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
            ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
            ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
            ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
            ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
            ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
            ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
            ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
            ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
            ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR, ERRR,
#undef WHTE
#undef ERRR
};

static const uint8_t g_decode_value_to_encode_char[] =
{
    '0', '2', '3', '4', '5', '6', '7', '8',
    '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
    'h', 'j', 'k', 'm', 'n', 'p', 'q', 'r',
    's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
};

static const int g_encode_to_decode_size[]       = { 0, 0, 1, 1, 2, 3, 3, 4, 5 };

static const int g_encoded_size_to_bit_padding[] = { 0, 3, 2, 1, 4, 1, 2, 3, 0 };

static const int g_decode_to_encode_size[]       = { 0, 2, 4, 5, 7, 8 };

static const int g_decoded_size_to_bit_padding[] = { 0, 2, 4, 1, 3, 0 };


// ===========================================================================
// Code below this point is the same in all safeXX codecs (with a different
// function name prefix).
// After changing anything below this point, please copy the changes to all
// other codecs.
// ===========================================================================

static inline int calculate_length_chunk_count(int64_t length)
{
    const int bits_per_chunk = ENCODED_BITS_PER_BYTE - 1;

    int chunk_count = 0;
    for(uint64_t i = length; i; i >>= bits_per_chunk, chunk_count++)
    {
    }

    if(chunk_count == 0)
    {
        chunk_count = 1;
    }

    return chunk_count;
}

const char* safe32_version()
{
    return SAFE32_VERSION;
}

int64_t safe32_get_decoded_length(const int64_t encoded_length)
{
    if(encoded_length < 0)
    {
        return SAFE32_ERROR_INVALID_LENGTH;
    }
    int64_t groups = encoded_length / ENCODED_CHARS_PER_GROUP;
    int remainder = g_encode_to_decode_size[encoded_length % ENCODED_CHARS_PER_GROUP];
    KSLOG_DEBUG("Encoded Length %d, groups %d, mod %d, remainder %d, result %d",
                encoded_length, groups, encoded_length % ENCODED_CHARS_PER_GROUP,
                remainder, groups * DECODED_BYTES_PER_GROUP + remainder);
    return groups * DECODED_BYTES_PER_GROUP + remainder;
}

safe32_status safe32_decode_feed(const uint8_t** const enc_buffer_ptr,
                                 const int64_t enc_length,
                                 uint8_t** const dec_buffer_ptr,
                                 const int64_t dec_length,
                                 const safe32_stream_state stream_state)
{
    if(enc_length < 0 || dec_length < 0)
    {
        return SAFE32_ERROR_INVALID_LENGTH;
    }
    const uint8_t* enc = *enc_buffer_ptr;
    uint8_t* dec = *dec_buffer_ptr;

    const uint8_t* const enc_end = enc + enc_length;
    const uint8_t* const dec_end = dec + dec_length;
    const int enc_chars_per_group = ENCODED_CHARS_PER_GROUP;
    const int enc_bits_per_char = ENCODED_BITS_PER_BYTE;
    const int dec_bits_per_byte = DECODED_BITS_PER_BYTE;
    const int dec_mask = 0xff;

    KSLOG_DEBUG("Decode %d chars into %d bytes, stream state %d",
                enc_end - enc, dec_end - dec, stream_state);

    #define WRITE_BYTES(ENC_CHAR_COUNT) \
    { \
        const int bytes_to_write = g_encode_to_decode_size[ENC_CHAR_COUNT]; \
        KSLOG_DEBUG("Writing %d chars as %d decoded bytes", ENC_CHAR_COUNT, bytes_to_write); \
        for(int shift_amount = bytes_to_write - 1; shift_amount >= 0; shift_amount--) \
        { \
            *dec++ = (accumulator >> (dec_bits_per_byte * shift_amount)) & dec_mask; \
            KSLOG_DEBUG("Write: Extract pos %d: %02x", \
                (dec_bits_per_byte * shift_amount), \
                (accumulator >> (dec_bits_per_byte * shift_amount)) & dec_mask); \
        } \
    }

    const uint8_t* last_enc = enc;
    int enc_group_char_count = 0;
    int64_t accumulator = 0;

    while(enc < enc_end)
    {
        const uint8_t encoded_char = *enc++;
        const int decoded_value = g_encode_char_to_decode_value[encoded_char];
        if(decoded_value == CHARACTER_CODE_WHITESPACE)
        {
            KSLOG_TRACE("Whitespace");
            continue;
        }
        if(decoded_value == CHARACTER_CODE_ERROR)
        {
            KSLOG_DEBUG("Error: Invalid source data: %02x: [%c]", encoded_char, encoded_char);
            *enc_buffer_ptr = enc - 1;
            *dec_buffer_ptr = dec;
            return SAFE32_ERROR_INVALID_SOURCE_DATA;
        }
        accumulator = (accumulator << enc_bits_per_char) | decoded_value;
        enc_group_char_count++;
        KSLOG_DEBUG("Accumulate enc group %d [%c] (%02x). Value = %x",
                    enc_group_char_count, encoded_char, decoded_value, accumulator);
        if(dec + g_encode_to_decode_size[enc_group_char_count] >= dec_end)
        {
            break;
        }
        if(enc_group_char_count == enc_chars_per_group)
        {
            WRITE_BYTES(enc_group_char_count);
            enc_group_char_count = 0;
            accumulator = 0;
            last_enc = enc;
        }
    }

    bool enc_is_at_end = (stream_state & SAFE32_SRC_IS_AT_END_OF_STREAM) && enc >= enc_end;
    bool dec_is_at_end = (stream_state & SAFE32_DST_IS_AT_END_OF_STREAM) && dec + g_encode_to_decode_size[enc_group_char_count] >= dec_end;

    if(enc_group_char_count > 0 && (enc_is_at_end || dec_is_at_end))
    {
        const int phantom_bits = g_encoded_size_to_bit_padding[enc_group_char_count];
        KSLOG_DEBUG("E Phantom bits: %d", phantom_bits);
        accumulator >>= phantom_bits;
        WRITE_BYTES(enc_group_char_count);
        last_enc = enc;
    }

    // Skip over any trailing whitespace
    for(; enc < enc_end; enc++)
    {
        if(g_encode_char_to_decode_value[*enc] != CHARACTER_CODE_WHITESPACE)
        {
            last_enc = enc;
            break;
        }
    }

    *enc_buffer_ptr = last_enc;
    *dec_buffer_ptr = dec;

    enc_is_at_end = (stream_state & SAFE32_SRC_IS_AT_END_OF_STREAM) && enc >= enc_end;
    dec_is_at_end = (stream_state & SAFE32_DST_IS_AT_END_OF_STREAM) && dec + g_encode_to_decode_size[enc_group_char_count] >= dec_end;

    KSLOG_DEBUG("Stream state = %d, enc %d, dec %d", stream_state, enc >= enc_end, dec >= dec_end);
    KSLOG_TRACE("enc %p %p, dec %p %p", enc, enc_end, dec, dec_end);
    KSLOG_DEBUG("Enc at end %d, dec at end %d", enc_is_at_end, dec_is_at_end);
    if(enc_is_at_end || dec_is_at_end)
    {
        if(stream_state & SAFE32_EXPECT_DST_STREAM_TO_END)
        {
            if(dec_is_at_end)
            {
                KSLOG_DEBUG("OK: Dest is at end of stream & controls end of stream");
                return SAFE32_STATUS_OK;
            }
            else
            {
                KSLOG_DEBUG("Error: Dest controls end of stream, but source is at end");
                return SAFE32_ERROR_TRUNCATED_DATA;
            }
        }
        else
        {
            if(enc_is_at_end)
            {
                KSLOG_DEBUG("OK: Source is at end of stream & controls end of stream");
                return SAFE32_STATUS_OK;
            }
            else
            {
                KSLOG_DEBUG("Error: Source controls end of stream, but dest is at end");
                return SAFE32_ERROR_NOT_ENOUGH_ROOM;
            }
        }
    }

    KSLOG_DEBUG("Decode partially complete");
    return SAFE32_STATUS_PARTIALLY_COMPLETE;

    #undef WRITE_BYTES
}

int64_t safe32_read_length_field(const uint8_t* const buffer,
                                 const int64_t buffer_length,
                                 uint64_t* const length)
{
    if(buffer_length < 0)
    {
        return SAFE32_ERROR_INVALID_LENGTH;
    }
    const int bits_per_chunk = ENCODED_BITS_PER_BYTE - 1;
    const int continuation_bit = 1 << bits_per_chunk;
    const int chunk_mask = continuation_bit - 1;
    KSLOG_DEBUG("bits %d, continue %02x, mask %02x",
                bits_per_chunk, continuation_bit, chunk_mask);

    const uint8_t* buffer_end = buffer + buffer_length;
    uint64_t value = 0;
    int decoded = 0;

    const uint8_t* enc = buffer;
    while(enc < buffer_end)
    {
        decoded = g_encode_char_to_decode_value[(int)*enc];
        value = (value << bits_per_chunk) | (decoded & chunk_mask);
        KSLOG_DEBUG("Chunk %d: '%c' (%d), continue %d, value portion = %d",
                    enc - buffer, *enc, decoded, decoded & continuation_bit,
                    (decoded & chunk_mask));
        enc++;
        if(!(decoded & continuation_bit))
        {
            break;
        }
    }
    if(decoded & continuation_bit)
    {
        KSLOG_DEBUG("Error: Unterminated length field");
        return SAFE32_ERROR_UNTERMINATED_LENGTH_FIELD;
    }
    *length = value;
    KSLOG_DEBUG("Length = %d, chunks = %d", value, enc - buffer);
    return enc - buffer;
}

int64_t safe32_decode(const uint8_t* const enc_buffer,
                      const int64_t enc_length,
                      uint8_t* const dec_buffer,
                      const int64_t dec_length)
{
    if(enc_length < 0 || dec_length < 0)
    {
        return SAFE32_ERROR_INVALID_LENGTH;
    }
    const uint8_t* enc = enc_buffer;
    uint8_t* dec = dec_buffer;
    const safe32_status status = safe32_decode_feed(
                                    &enc,
                                    enc_length,
                                    &dec,
                                    dec_length,
                                    SAFE32_SRC_IS_AT_END_OF_STREAM | SAFE32_DST_IS_AT_END_OF_STREAM);
    if(status != SAFE32_STATUS_OK)
    {
        if(status == SAFE32_STATUS_PARTIALLY_COMPLETE)
        {
            return SAFE32_ERROR_NOT_ENOUGH_ROOM;
        }
        return status;
    }
    return dec - dec_buffer;
}

int64_t safe32l_decode(const uint8_t* const enc_buffer,
                       const int64_t enc_length,
                       uint8_t* const dec_buffer,
                       const int64_t dec_length)
{
    if(enc_length < 0 || dec_length < 0)
    {
        return SAFE32_ERROR_INVALID_LENGTH;
    }
    uint64_t length = 0;
    const int64_t bytes_used = safe32_read_length_field(enc_buffer, enc_length, &length);
    if(bytes_used < 0)
    {
        return bytes_used;
    }
    if(length > (uint64_t)(enc_length - bytes_used))
    {
        KSLOG_DEBUG("Require %d bytes but only %d available", length, enc_length - bytes_used);
        return SAFE32_ERROR_TRUNCATED_DATA;
    }
    KSLOG_DEBUG("Used %d bytes", bytes_used);
    const int64_t read_length = enc_length - bytes_used;
    const uint8_t* enc = enc_buffer + bytes_used;
    uint8_t* dec = dec_buffer;
    const safe32_status status = safe32_decode_feed(
                                    &enc,
                                    read_length,
                                    &dec,
                                    dec_length,
                                    SAFE32_SRC_IS_AT_END_OF_STREAM | SAFE32_DST_IS_AT_END_OF_STREAM);
    if(status != SAFE32_STATUS_OK)
    {
        if(status == SAFE32_STATUS_PARTIALLY_COMPLETE)
        {
            return SAFE32_ERROR_NOT_ENOUGH_ROOM;
        }
        return status;
    }
    return dec - dec_buffer;
}

int64_t safe32_get_encoded_length(const int64_t decoded_length,
                                  const bool include_length_field)
{
    if(decoded_length < 0)
    {
        return SAFE32_ERROR_INVALID_LENGTH;
    }
    const int64_t groups = decoded_length / DECODED_BYTES_PER_GROUP;
    const int remainder = g_decode_to_encode_size[decoded_length % DECODED_BYTES_PER_GROUP];
    int length_length = 0;
    if(include_length_field)
    {
        length_length = calculate_length_chunk_count(decoded_length);
    }
    KSLOG_DEBUG("Decoded Length %d, groups %d, mod %d, remainder %d, length_length %d, result %d",
                decoded_length, groups, decoded_length % DECODED_BYTES_PER_GROUP, remainder,
                length_length, groups * DECODED_BYTES_PER_GROUP + remainder + length_length);
    return groups * ENCODED_CHARS_PER_GROUP + remainder + length_length;
}

safe32_status safe32_encode_feed(const uint8_t** const dec_buffer_ptr,
                                 const int64_t dec_length,
                                 uint8_t** const enc_buffer_ptr,
                                 const int64_t enc_length,
                                 const bool is_end_of_data)
{
    if(dec_length < 0 || enc_length < 0)
    {
        return SAFE32_ERROR_INVALID_LENGTH;
    }
    const uint8_t* dec = *dec_buffer_ptr;
    uint8_t* enc = *enc_buffer_ptr;

    const uint8_t* const dec_end = dec + dec_length;
    const uint8_t* const enc_end = enc + enc_length;
    const int dec_chars_per_group = DECODED_BYTES_PER_GROUP;
    const int dec_bits_per_byte = DECODED_BITS_PER_BYTE;
    const int enc_bits_per_byte = ENCODED_BITS_PER_BYTE;
    const int enc_mask = (1 << ENCODED_BITS_PER_BYTE) - 1;

    KSLOG_DEBUG("Encode %d bytes into %d encoded chars, ending %d",
                dec_end - dec, enc_end - enc, is_end_of_data);

    #define WRITE_BYTES(SRC_BYTE_COUNT) \
    { \
        int bytes_to_write = g_decode_to_encode_size[SRC_BYTE_COUNT]; \
        KSLOG_DEBUG("Writing %d bytes as %d encoded chars", SRC_BYTE_COUNT, bytes_to_write); \
        if(enc + bytes_to_write > enc_end) \
        { \
            KSLOG_DEBUG("Error: Need %d chars but only %d available", bytes_to_write, enc_end - enc); \
            *dec_buffer_ptr = last_dec; \
            *enc_buffer_ptr = enc; \
            return SAFE32_STATUS_PARTIALLY_COMPLETE; \
        } \
        for(int shift_amount = bytes_to_write - 1; shift_amount >= 0; shift_amount--) \
        { \
            *enc++ = g_decode_value_to_encode_char[(accumulator >> (enc_bits_per_byte * shift_amount)) & enc_mask]; \
            KSLOG_DEBUG("Write: Extract pos %02d: %02x: %c", \
                (enc_bits_per_byte * shift_amount), \
                (accumulator >> (enc_bits_per_byte * shift_amount)) & enc_mask, \
                g_decode_value_to_encode_char[(accumulator >> (enc_bits_per_byte * shift_amount)) & enc_mask]); \
        } \
    }

    const uint8_t* last_dec = dec;
    int dec_group_byte_count = 0;
    int64_t accumulator = 0;

    while(dec < dec_end)
    {
        const uint8_t decoded_byte = *dec++;
        accumulator = (accumulator << dec_bits_per_byte) | decoded_byte;
        dec_group_byte_count++;
        KSLOG_DEBUG("Accumulate #%d (%02x). Value = %x", dec_group_byte_count, decoded_byte, accumulator);
        if(dec_group_byte_count == dec_chars_per_group)
        {
            WRITE_BYTES(dec_group_byte_count);
            dec_group_byte_count = 0;
            accumulator = 0;
            last_dec = dec;
        }
    }

    if(dec_group_byte_count > 0)
    {
        if(is_end_of_data)
        {
            const int phantom_bits = g_decoded_size_to_bit_padding[dec_group_byte_count];
            KSLOG_DEBUG("E Phantom bits: %d", phantom_bits);
            accumulator <<= phantom_bits;
            WRITE_BYTES(dec_group_byte_count);
        }
        else
        {
            dec -= dec_group_byte_count;
        }
        last_dec = dec;
    }

    *dec_buffer_ptr = last_dec;
    *enc_buffer_ptr = enc;

    return SAFE32_STATUS_OK;
#undef WRITE_BYTES
}

int64_t safe32_write_length_field(const uint64_t length,
                                  uint8_t* const enc_buffer,
                                  const int64_t enc_buffer_length)
{
    if(enc_buffer_length < 0)
    {
        return SAFE32_ERROR_INVALID_LENGTH;
    }
    const int bits_per_chunk = ENCODED_BITS_PER_BYTE - 1;
    const int continuation_bit = 1 << bits_per_chunk;
    const int chunk_mask = continuation_bit - 1;
    KSLOG_DEBUG("bits %d, continue %02x, mask %02x", bits_per_chunk, continuation_bit, chunk_mask);

    int chunk_count = 0;
    for(uint64_t i = length; i; i >>= bits_per_chunk, chunk_count++)
    {
    }
    if(chunk_count == 0)
    {
        chunk_count = 1;
    }
    KSLOG_DEBUG("Value: %lu, chunk count %d", length, chunk_count);

    if(chunk_count > enc_buffer_length)
    {
        KSLOG_DEBUG("Error: Require %d bytes but only %d available", chunk_count, enc_buffer_length);
        return SAFE32_ERROR_NOT_ENOUGH_ROOM;
    }

    uint8_t* enc = enc_buffer;
    for(int shift_amount = chunk_count - 1; shift_amount >= 0; shift_amount--)
    {
        const int should_continue = (shift_amount == 0) ? 0 : continuation_bit;
        const int code = ((length>>(bits_per_chunk * shift_amount)) & chunk_mask) + should_continue;
        const uint8_t encoded_char = g_decode_value_to_encode_char[code];
        *enc++ = encoded_char;
        KSLOG_DEBUG("Chunk %d: '%c' (%d), continue %d", shift_amount, encoded_char,
                    ((length>>(bits_per_chunk * shift_amount)) & chunk_mask), should_continue);
    }
    return chunk_count;
}

int64_t safe32_encode(const uint8_t* const dec_buffer,
                      const int64_t dec_length,
                      uint8_t* const enc_buffer,
                      const int64_t enc_length)
{
    if(dec_length < 0 || enc_length < 0)
    {
        return SAFE32_ERROR_INVALID_LENGTH;
    }
    const uint8_t* dec = dec_buffer;
    uint8_t* enc = enc_buffer;
    const safe32_status status = safe32_encode_feed(&dec, dec_length, &enc, enc_length, true);
    if(status != SAFE32_STATUS_OK)
    {
        if(status == SAFE32_STATUS_PARTIALLY_COMPLETE)
        {
            return SAFE32_ERROR_NOT_ENOUGH_ROOM;
        }
        return status;
    }
    return enc - enc_buffer;
}

int64_t safe32l_encode(const uint8_t* const dec_buffer,
                       const int64_t dec_length,
                       uint8_t* const enc_buffer,
                       const int64_t enc_length)
{
    if(dec_length < 0 || enc_length < 0)
    {
        return SAFE32_ERROR_INVALID_LENGTH;
    }
    int64_t bytes_used = safe32_write_length_field(dec_length, enc_buffer, enc_length);
    if(bytes_used < 0)
    {
        return bytes_used;
    }

    const uint8_t* dec = dec_buffer;
    uint8_t* enc = enc_buffer + bytes_used;
    safe32_status status = safe32_encode_feed(&dec, dec_length, &enc, enc_length, true);
    if(status != SAFE32_STATUS_OK)
    {
        if(status == SAFE32_STATUS_PARTIALLY_COMPLETE)
        {
            return SAFE32_ERROR_NOT_ENOUGH_ROOM;
        }
        return status;
    }
    return enc - enc_buffer;
}
