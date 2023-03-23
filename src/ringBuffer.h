/*
MIT License

Copyright (c) 2023 Q-Gears Kft., Hungary

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */
#ifndef SIMULATOR_RINGBUFFER_H
#define SIMULATOR_RINGBUFFER_ H

/// ringbuffer implementation used by the simulator to store channel events

#include "simulator_types.h"

/// Structure to store fields of a ringbuffer object.
typedef struct
{
	//! ptrRead==ptrWrite means empty
  volatile uint32_t ptrRead;
	volatile uint32_t ptrWrite;
	uint32_t bufferSize;
	uint8_t * buffer;
} ringBuffer_t;

/// Initialize the given structure with initial values: empty ringbuffer
/// @param ringBuffer user provided static storage of the ringbuffer object
/// @param bufferSize size of the buffer used to store data (bufferSize-1 bytes can be used due to implementation details)
/// @param buffer static allocated buffer to store actual data
void ringBuffer_create(ringBuffer_t * ringBuffer, uint32_t bufferSize, uint8_t * buffer);

/// Set buffer pointer to NULL - ringbuffer is in not usabe state - this is not standard feature of ringbuffer implementations
void ringBuffer_clear(ringBuffer_t * ringBuffer);
/// Is this ringbuffer in created state? - this is not standard of ringbuffer implementations
bool ringBuffer_isCreated(ringBuffer_t * ringBuffer);

/// Write data into the ringbuffer
/// @return false means there was not enough space in the ringbuffer and nothing was written. true means the data was fully written into the
/// buffer
bool ringBuffer_write(ringBuffer_t * ringBuffer, uint32_t nBytes, uint8_t * data);
/// Read data from ringbuffer and update the read pointer.
/// @param data pointer of target buffer to write data to. NULL is allowed and means data is not read just skipped but read pointer is updated
/// @return true means there was enough data in the buffer and read was done and read pointer was increased. false means there was not enough data and the state of the buffer was not changed.
bool ringBuffer_read(ringBuffer_t * ringBuffer, uint32_t nBytes, uint8_t * data);
/// Read data from ringbuffer and leave the read pointer unchanged.
/// @return true means there was enough data in the buffer. false means there was not enough data and the data buffer is unchanged.
bool ringBuffer_peek(ringBuffer_t * ringBuffer, uint32_t nBytes, uint8_t * data);
/// Get the number of available bytes to write
uint32_t ringBuffer_availableWrite(ringBuffer_t * ringBuffer);
/// Get the number of available bytes to read
uint32_t ringBuffer_availableRead(ringBuffer_t * ringBuffer);

#endif

