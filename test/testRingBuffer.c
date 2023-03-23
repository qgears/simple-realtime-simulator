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
#include "assert.h"
#include "ringBuffer.h"

#define SIZE 27

void testRingBuffer()
{
  ringBuffer_t rb;
  uint8_t b[SIZE];
  uint8_t data[SIZE];
  ringBuffer_create(&rb, SIZE, b);
  assert(rb.buffer==b);
  assert(rb.bufferSize==SIZE);
  assert(rb.ptrRead==0);
  assert(rb.ptrWrite==0);
  assert(ringBuffer_availableRead(&rb)==0);
  assert(ringBuffer_availableWrite(&rb)==SIZE-1);
  ringBuffer_write(&rb, 25, data);
  assert(ringBuffer_availableRead(&rb)==25);
  assert(ringBuffer_availableWrite(&rb)==1);
  ringBuffer_write(&rb, 1, data);
  assert(ringBuffer_availableRead(&rb)==26);
  assert(ringBuffer_availableWrite(&rb)==0);
  assert(rb.ptrWrite==26);
  assert(rb.ptrRead==0);
  assert(!ringBuffer_write(&rb, 1, data));
  assert(ringBuffer_read(&rb, 1, data));
  assert(ringBuffer_write(&rb, 1, data));
  assert(rb.ptrWrite==0);
}
