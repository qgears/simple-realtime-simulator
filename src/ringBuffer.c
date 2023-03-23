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

#include "ringBuffer.h"
#include <string.h>

void ringBuffer_create(ringBuffer_t * ringBuffer, uint32_t bufferSize, uint8_t * buffer)
{
	ringBuffer->ptrRead=0;
	ringBuffer->ptrWrite=0;
	ringBuffer->bufferSize=bufferSize;
	ringBuffer->buffer=buffer;
}

bool ringBuffer_write(ringBuffer_t * ringBuffer, uint32_t nBytes, uint8_t * data)
{
	if(ringBuffer->buffer!=NULL && ringBuffer_availableWrite(ringBuffer)>=nBytes)
	{
		uint32_t at=ringBuffer->ptrWrite;
		uint32_t newPtr=at+nBytes;
		if(newPtr>ringBuffer->bufferSize)
		{
			uint32_t firstSize=ringBuffer->bufferSize-at;
			uint32_t secondSize=nBytes-firstSize;
			memcpy(&(ringBuffer->buffer[at]), data, firstSize);
			at=0;
			memcpy(&(ringBuffer->buffer[at]), &(data[firstSize]), secondSize);
			at=secondSize;
			ringBuffer->ptrWrite=at;
		}else
		{
			memcpy(&(ringBuffer->buffer[at]), data, nBytes);
			at+=nBytes;
			if(at==ringBuffer->bufferSize)
			{
				at=0;
			}
			ringBuffer->ptrWrite=at;
		}
		return true;
	}else
	{
		return false;
	}
}
bool ringBuffer_read(ringBuffer_t * ringBuffer, uint32_t nBytes, uint8_t * data)
{
	if(ringBuffer_availableRead(ringBuffer)>=nBytes)
	{
		uint32_t at=ringBuffer->ptrRead;
		uint32_t newPtr=at+nBytes;
		if(newPtr>ringBuffer->bufferSize)
		{
			uint32_t firstSize=ringBuffer->bufferSize-at;
			uint32_t secondSize=nBytes-firstSize;
			if(data!=NULL)
			{
			  memcpy(data, &(ringBuffer->buffer[at]), firstSize);
			}
			at=0;
      if(data!=NULL)
      {
        memcpy(&(data[firstSize]), &(ringBuffer->buffer[at]), secondSize);
      }
			at=secondSize;
			ringBuffer->ptrRead=at;
		}else
		{
      if(data!=NULL)
      {
        memcpy(data, &(ringBuffer->buffer[at]), nBytes);
      }
			at+=nBytes;
			if(at==ringBuffer->bufferSize)
			{
				at=0;
			}
			ringBuffer->ptrRead=at;
		}
		return true;
	}else
	{
		return false;
	}
}
bool ringBuffer_peek(ringBuffer_t * ringBuffer, uint32_t nBytes, uint8_t * data)
{
	if(ringBuffer_availableRead(ringBuffer)>=nBytes)
	{
		uint32_t at=ringBuffer->ptrRead;
		uint32_t newPtr=at+nBytes;
		if(newPtr>ringBuffer->bufferSize)
		{
			uint32_t firstSize=ringBuffer->bufferSize-at;
			uint32_t secondSize=nBytes-firstSize;
			memcpy(data, &(ringBuffer->buffer[at]), firstSize);
			at=0;
			memcpy(&(data[firstSize]), &(ringBuffer->buffer[at]), secondSize);
			at=secondSize;
		}else
		{
			memcpy(data, &(ringBuffer->buffer[at]), nBytes);
			at+=nBytes;
			if(at==ringBuffer->ptrWrite)
			{
				at=0;
			}
		}
		return true;
	}else
	{
		return false;
	}
}


uint32_t ringBuffer_availableWrite(ringBuffer_t * ringBuffer)
{
	uint32_t fill=ringBuffer_availableRead(ringBuffer);
	return ringBuffer->bufferSize-1-fill;
}

uint32_t ringBuffer_availableRead(ringBuffer_t * ringBuffer)
{
	uint32_t ret=ringBuffer->ptrWrite+ringBuffer->bufferSize-ringBuffer->ptrRead;
	if(ret>=ringBuffer->bufferSize)
	{
		ret-=ringBuffer->bufferSize;
	}
	return ret;
}

void ringBuffer_clear(ringBuffer_t * ringBuffer)
{
  ringBuffer->buffer=NULL;
  ringBuffer->bufferSize=0;
  ringBuffer->ptrRead=0;
  ringBuffer->ptrWrite=0;
}
bool ringBuffer_isCreated(ringBuffer_t * ringBuffer)
{
  return ringBuffer->buffer!=NULL;
}

