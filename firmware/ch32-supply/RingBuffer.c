//------------------------------------------------------------------------------
//       Filename: RingBuffer.c
//------------------------------------------------------------------------------
//       Homely Energy (c) 2024
//------------------------------------------------------------------------------
//       Purpose : Implements the Ring Buffer API
//------------------------------------------------------------------------------
//       Notes : None
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Module includes
//------------------------------------------------------------------------------
#include "RingBuffer.h"

//------------------------------------------------------------------------------
// Module constant defines
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// External variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// External functions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Module type definitions
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Module static function prototypes
//------------------------------------------------------------------------------
static void RingLock(const RingBuffer_t *ring);
static void RingUnlock(const RingBuffer_t *ring);
static size_t Peek(const RingBuffer_t *ring);

//------------------------------------------------------------------------------
// Module static variables
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Module externally exported functions
//------------------------------------------------------------------------------

/**
 * @brief  Initialise the ring buffer
 * @param[in]  ring - pointer to ring buffer
 * @param[in]  data - pointer to data buffer
 * @param[in]  size - size of data buffer
 * @return None
 */
RingBufferStatus_e RingBuffer_Init(RingBuffer_t *ring, uint8_t *data, size_t size)
{
    RingBufferStatus_e status = eRING_BUFFER_STATUS_INVALID_PARAM;

    if (ring && data && size)
    {
        ring->data = data;
        ring->size = size;
        ring->head = 0;
        ring->tail = 0;
        status = eRING_BUFFER_STATUS_SUCCESS;
    }

    return status;
}

/**
 * @brief  Initialise a thread safe ring buffer
 * @param[in]  ring - pointer to ring buffer
 * @param[in]  data - pointer to data buffer
 * @param[in]  size - size of data buffer
 * @param[in]  mutex - mutex interface
 * @return eRING_BUFFER_STATUS_SUCCESS if successful, otherwise an error code
 */
RingBufferStatus_e RingBuffer_InitThreadSafe(RingBuffer_t *ring, uint8_t *data, size_t size, const RingBufferMutexInterface_t mutex)
{
    RingBufferStatus_e status = RingBuffer_Init(ring, data, size);

    if (eRING_BUFFER_STATUS_SUCCESS == status)
    {
        ring->mutex = mutex;
    }

    return status;
}

/**
 * @brief  Put data into the ring buffer
 * @param[in]  ring - pointer to ring buffer
 * @param[in]  data - pointer to data buffer
 * @param[in]  size - size of data buffer
 * @return eRING_BUFFER_STATUS_SUCCESS if successful, otherwise an error code
 */
RingBufferStatus_e RingBuffer_Put(RingBuffer_t *ring, const uint8_t *data, size_t size)
{
    RingBufferStatus_e status = eRING_BUFFER_STATUS_UNKNOWN;

    if (!ring || !data || !size)
    {
        return eRING_BUFFER_STATUS_INVALID_PARAM;
    }

    RingLock(ring);

    const size_t free = ring->size - Peek(ring);

    if (size > free)
    {
        status = eRING_BUFFER_STATUS_OVERFLOW;
    }
    else
    {
        while (size--)
        {
            ring->data[ring->head++] = *data++;

            if (ring->head == ring->size)
            {
                ring->head = 0;
            }
        }

        status = eRING_BUFFER_STATUS_SUCCESS;
    }

    RingUnlock(ring);

    return status;
}

/**
 * @brief  Get data from the ring buffer
 * @param[in]  ring - pointer to ring buffer
 * @param[out]  data - pointer to data buffer
 * @param[in]  size - size of data buffer
 * @param[out]  bytesRead - number of bytes read
 * @return eRING_BUFFER_STATUS_SUCCESS if successful, otherwise an error code
 */
RingBufferStatus_e RingBuffer_Get(RingBuffer_t *ring, uint8_t *data, size_t size, size_t *bytesRead)
{
    RingBufferStatus_e status = eRING_BUFFER_STATUS_UNKNOWN;

    if (!ring || !data || !size)
    {
        return eRING_BUFFER_STATUS_INVALID_PARAM;
    }

    RingLock(ring);

    const size_t available = Peek(ring);
    size_t bytesToRead = (size < available) ? size : available;
    *bytesRead = bytesToRead;

    while (bytesToRead--)
    {
        *data++ = ring->data[ring->tail++];

        if (ring->tail == ring->size)
        {
            ring->tail = 0;
        }
    }

    RingUnlock(ring);

    status = eRING_BUFFER_STATUS_SUCCESS;

    return status;
}

/**
 * @brief  Get the number of bytes in the ring buffer
 * @param[in]  ring - pointer to ring buffer
 * @return number of bytes in the ring buffer
 */
size_t RingBuffer_Peek(const RingBuffer_t *ring)
{
    size_t size = 0;

    RingLock(ring);
    size = Peek(ring);
    RingUnlock(ring);

    return size;
}

/**
 * @brief  Get the number of bytes until the specified value. If the value is not found, eRING_BUFFER_STATUS_ERROR is returned
 * @param[in]  ring - pointer to ring buffer
 * @param[in]  value - value to search for
 * @param[out]  index - index of value
 * @return eRING_BUFFER_STATUS_SUCCESS if successful, otherwise an error code
 */
RingBufferStatus_e RingBuffer_IndexOf(RingBuffer_t *ring, const uint8_t value, size_t *index)
{
    RingBufferStatus_e status = eRING_BUFFER_STATUS_ERROR;

    if (!ring || !index)
    {
        return eRING_BUFFER_STATUS_INVALID_PARAM;
    }

    RingLock(ring);

    size_t pos = ring->tail;
    size_t count = 0;

    while (pos != ring->head)
    {
        if (ring->data[pos] == value)
        {
            *index = count;
            status = eRING_BUFFER_STATUS_SUCCESS;
            break;
        }

        if (++pos == ring->size)
        {
            pos = 0;
        }

        count++;
    }

    RingUnlock(ring);

    return status;
}

/**
 * @brief  Find the specified array of values in the ring buffer
 * @param[in]  ring - pointer to ring buffer
 * @param[in]  value - value to search for
 * @param[in]  valueSize - size of value
 * @param[out]  index - index of value
 * @return eRING_BUFFER_STATUS_SUCCESS if successful, otherwise an error code
 */
RingBufferStatus_e RingBuffer_Find(RingBuffer_t *ring, const uint8_t *value, size_t valueSize, size_t *index)
{
    RingBufferStatus_e status = eRING_BUFFER_STATUS_ERROR;

    if (!ring || !value || !valueSize || !index)
    {
        return eRING_BUFFER_STATUS_INVALID_PARAM;
    }

    RingLock(ring);

    size_t pos = ring->tail;
    size_t count = 0;

    while (pos != ring->head)
    {
        if (ring->data[pos] == value[0])
        {
            size_t matchedValues = 1;

            while (matchedValues < valueSize)
            {
                if (ring->data[(pos + matchedValues) % ring->size] != value[matchedValues])
                {
                    break;
                }

                matchedValues++;
            }

            if (matchedValues == valueSize)
            {
                *index = count;
                status = eRING_BUFFER_STATUS_SUCCESS;
                break;
            }
        }

        if (++pos == ring->size)
        {
            pos = 0;
        }

        count++;
    }

    RingUnlock(ring);

    return status;
}

//------------------------------------------------------------------------------
// Module static functions
//------------------------------------------------------------------------------

/**
 * @brief  Lock the ring buffer
 * @param[in]  ring - pointer to ring buffer
 * @return None
 */
static void RingLock(const RingBuffer_t *ring)
{
    void *const lock = ring->mutex.lock;
    const RingBufferMutexOpperation_t take = ring->mutex.take;

    if (lock && take)
    {
        take(lock);
    }
}

/**
 * @brief  Unlock the ring buffer
 * @param[in]  ring - pointer to ring buffer
 * @return None
 */
static void RingUnlock(const RingBuffer_t *ring)
{
    void *const lock = ring->mutex.lock;
    const RingBufferMutexOpperation_t release = ring->mutex.release;

    if (lock && release)
    {
        release(lock);
    }
}

/**
 * @brief  Get the number of bytes in the ring buffer
 * @param[in]  ring - pointer to ring buffer
 * @return number of bytes in the ring buffer
 */
static size_t Peek(const RingBuffer_t *ring)
{
    size_t size = 0;

    if (!ring || ring->head == ring->tail)
    {
        size = 0;
    }
    else if (ring->head >= ring->tail)
    {
        size = ring->head - ring->tail;
    }
    else
    {
        size = ring->size - (ring->tail - ring->head);
    }

    return size;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
