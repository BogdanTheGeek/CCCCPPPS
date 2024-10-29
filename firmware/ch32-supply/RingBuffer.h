//------------------------------------------------------------------------------
//       Filename: RingBuffer.h
//------------------------------------------------------------------------------
//       Homely Energy (c) 2024
//------------------------------------------------------------------------------
//       Purpose : Defines the Ring Buffer API
//------------------------------------------------------------------------------
//       Notes : None
//------------------------------------------------------------------------------
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// Module includes
//------------------------------------------------------------------------------
#include <stddef.h>
#include <stdint.h>

//------------------------------------------------------------------------------
// Module exported defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Module exported type definitions
//------------------------------------------------------------------------------
typedef void (*RingBufferMutexOpperation_t)(void *lock);

typedef struct
{
    void *lock;
    RingBufferMutexOpperation_t take;
    RingBufferMutexOpperation_t release;
} RingBufferMutexInterface_t;

typedef struct RingBuffer
{
    uint8_t *data;
    size_t size;
    size_t head;
    size_t tail;

    RingBufferMutexInterface_t mutex;
} RingBuffer_t;

typedef enum
{
    eRING_BUFFER_STATUS_UNKNOWN = 0,
    eRING_BUFFER_STATUS_SUCCESS,
    eRING_BUFFER_STATUS_ERROR,
    eRING_BUFFER_STATUS_INVALID_PARAM,
    eRING_BUFFER_STATUS_OVERFLOW,
} RingBufferStatus_e;

//------------------------------------------------------------------------------
// Module exported functions
//------------------------------------------------------------------------------
RingBufferStatus_e RingBuffer_Init(RingBuffer_t *ring, uint8_t *data, size_t size);

RingBufferStatus_e RingBuffer_InitThreadSafe(RingBuffer_t *ring, uint8_t *data, size_t size, const RingBufferMutexInterface_t mutex);

RingBufferStatus_e RingBuffer_Put(RingBuffer_t *ring, const uint8_t *data, size_t size);
RingBufferStatus_e RingBuffer_Get(RingBuffer_t *ring, uint8_t *data, size_t size, size_t *bytesRead);
size_t RingBuffer_Peek(const RingBuffer_t *ring);

RingBufferStatus_e RingBuffer_IndexOf(RingBuffer_t *ring, const uint8_t value, size_t *index);
RingBufferStatus_e RingBuffer_Find(RingBuffer_t *ring, const uint8_t *value, size_t valueSize, size_t *index);

//------------------------------------------------------------------------------
// Module exported variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif
