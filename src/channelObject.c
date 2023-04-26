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

#include "localClock.h"
#include "channelObject.h"
#include "assert.h"
#include <sys/time.h>
#include <stdio.h>
#include <inttypes.h>
#include <time.h>
#include "simulator_types.h"


#define channelObject_datagramSize(co) ((co)->messageSize+8)

/// State of simulator busy wait cycles

/// Current target wait time measured in global clock ticks.
static uint64_t currentTarget;
/// Timestamp (of operating system real time) when waiting for other simulators started.
static uint64_t startWaitAtMillis;
/// Store whether the current wait cycle reached the timeout to be logged to stderr.
static bool wasLogged=false;


/// Millisecond timestamp getter (CPU time) used to log too long waiting periods of the simulator.
static uint64_t current_millis();
/// Called by the simulator from the inside of busy loops waiting for other processes.
/// Detects too long wait periods and emits log messages to stderr in these cases.
static void busyWaitIterate(uint64_t availableTimestamp, uint64_t targetTimestamp, const char * debugName);
/// Must be called after the busyWaitIterate cycles to signal that execution goes on.
static void busyWaitDone(uint64_t availableTimestamp, uint64_t targetTimestamp);


void channelObject_create(channelObject_t * co, localClock_t * clock, uint32_t messageSize)
{
  assert(clock!=NULL);
	co->messageSize=messageSize;
	co->nSink=0;
	co->simulatedUntil=0;
	co->minimalLatency=1;
	co->clock=clock;
}

void channelObject_setMinimalLatency(channelObject_t * co, uint64_t minimalLatency)
{
  assert(minimalLatency>0);
  co->minimalLatency=minimalLatency;
}

channelObjectSink_t * channelObject_allocateSink(channelObject_t * co, uint32_t bufferSize, uint8_t * buffer)
{
	uint32_t index=co->nSink;
	assert(index<MAX_CHANNEL_SINK);
	channelObjectSink_t * sink=&(co->sinks[index]);
	ringBuffer_create(&(sink->buffer), bufferSize, buffer);
	sink->host=co;
	co->nSink++;
	return sink;
}
void channelObject_waitSimulatedUntil(channelObject_t * co, uint64_t timestamp)
{
  if(co->simulatedUntil<timestamp)
  {
    while(co->simulatedUntil<timestamp)
    {
      busyWaitIterate(co->simulatedUntil, timestamp, co->debugName);
    }
    busyWaitDone(co->simulatedUntil, timestamp);
  }
}

void channelObject_processEventsUntil(channelObjectSink_t * sink, uint64_t timestamp)
{
	channelObject_t * co=sink->host;
	uint8_t * buffer=sink->readBuffer;
	while(co->simulatedUntil<timestamp)
	{
	  localClock_checkExit(sink->host->clock);
		busyWaitIterate(co->simulatedUntil, timestamp, co->debugName);
	}
  busyWaitDone(co->simulatedUntil, timestamp);
  uint32_t availablebytes=ringBuffer_availableRead(&(sink->buffer));
	while(availablebytes>=channelObject_datagramSize(co))
	{
		ringBuffer_peek(&(sink->buffer), 8, buffer);
		uint64_t t=*((uint64_t *)buffer);
		if(t>timestamp)
		{
			// All events processed until the timestamp
			return;
		}
		ringBuffer_read(&(sink->buffer), channelObject_datagramSize(co), buffer);
		uint64_t timestamp=*((uint64_t *) buffer);
		channelObjectEventCallback_t eventCallback=sink->callback;
		if(eventCallback!=NULL)
		{
			eventCallback(sink->parameter, timestamp, sink, buffer+8, co->messageSize);
		}
		availablebytes=ringBuffer_availableRead(&(sink->buffer));
	}
}
uint64_t channelObjectSink_getNextEventTimeStamp(channelObjectSink_t * sink)
{
  uint32_t availablebytes=ringBuffer_availableRead(&(sink->buffer));
  uint64_t ret=UINT64_MAX;
  if(availablebytes>=CHANNEL_OBJECT_HEADER_SIZE)
  {
    ringBuffer_peek(&(sink->buffer), 8, (uint8_t *)&ret);
  }
  return ret;
}
void channelObject_processEventsUntilNoWait(channelObjectSink_t * sink, uint64_t timestamp)
{
  channelObject_t * co=sink->host;
  uint8_t * buffer=sink->readBuffer;
/*  while(co->simulatedUntil<timestamp)
  {
    busyWaitIterate(co->simulatedUntil, timestamp);
  }
  busyWaitDone(co->simulatedUntil, timestamp);
  */
  uint32_t availablebytes=ringBuffer_availableRead(&(sink->buffer));
  while(availablebytes>=channelObject_datagramSize(co))
  {
    ringBuffer_peek(&(sink->buffer), 8, buffer);
    uint64_t t=*((uint64_t *)buffer);
    if(t>timestamp)
    {
      // All events processed until the timestamp
      return;
    }
    ringBuffer_read(&(sink->buffer), channelObject_datagramSize(co), buffer);
    uint64_t timestamp=*((uint64_t *) buffer);
    channelObjectEventCallback_t eventCallback=sink->callback;
    if(eventCallback!=NULL)
    {
      eventCallback(sink->parameter, timestamp, sink, buffer+8, co->messageSize);
    }
    availablebytes=ringBuffer_availableRead(&(sink->buffer));
  }
}
uint64_t channelObject_insertEvent(channelObject_t * co, uint64_t timestamp, uint8_t * data)
{
	assert(co!=NULL);
	if(timestamp<=co->simulatedUntil)
	{
		// TODO should it be an assert? It is not allowed to add an event to the current end of simulation timestamp
		timestamp=co->simulatedUntil+1;
	}
	for(uint32_t i=0;i<co->nSink;++i)
	{
		channelObjectSink_t * sink=&(co->sinks[i]);
		if(sink->enabled)
		{
			/// Block while there is space in the ringbuffer. Dropping packages is not an option
			/// deadlock is easily detectable if it causes one.
		  if(ringBuffer_availableWrite(&(sink->buffer))< channelObject_datagramSize(co))
		  {
        while(ringBuffer_availableWrite(&(sink->buffer))< channelObject_datagramSize(co))
        {
          busyWaitIterate(timestamp, timestamp, "write ringbuffer");
        }
        busyWaitDone(timestamp, timestamp);
		  }
			ringBuffer_write(&(sink->buffer), 8, (uint8_t *)&timestamp);
			ringBuffer_write(&(sink->buffer), co->messageSize, data);
		}
	}
	co->simulatedUntil=timestamp;
	return timestamp;
}

void channelObject_updateTime(channelObject_t * co, uint64_t timestamp)
{
  uint64_t t=timestamp+co->minimalLatency;
	if(t<co->simulatedUntil)
	{
	}else
	{
	  co->simulatedUntil=t;
	}
}

void channelObjectSink_setEnabled(channelObjectSink_t * sink, bool enabled, channelObjectEventCallback_t callback, void * parameter, uint32_t bufferSize, uint8_t * buffer)
{
	sink->parameter=parameter;
	sink->callback=callback;
	sink->enabled=enabled;
	if(sink->callback!=NULL)
	{
	  assert(bufferSize>=sink->host->messageSize+CHANNEL_OBJECT_HEADER_SIZE);
	  assert(buffer!=NULL);
	  sink->readBuffer=buffer;
	}
}

static uint64_t current_millis() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    uint64_t milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    return milliseconds;
}

static void busyWaitIterate(uint64_t availableTimestamp, uint64_t targetTimestamp, const char * debugName)
{
  if(wasLogged)
  {
    // In case we already waited 10 milliseconds then we guess the other processes are stopped by debugging.
    // In this case proper sleep uses less CPU and performance is not important.
    struct timespec t;
    t.tv_sec = 0u;
    t.tv_nsec = 1000000u;
    nanosleep(&t, NULL);
  }
  if(currentTarget!=targetTimestamp)
  {
    currentTarget=targetTimestamp;
    startWaitAtMillis=current_millis();
  }else
  {
    uint64_t t=current_millis();
    if(!wasLogged && t-startWaitAtMillis > 10)
    {
      fprintf(stderr, "Busy wait for simulation of timestamp spent 10 millis. Name: %s Available global timestamp: %" PRIu64 " required: %" PRIu64 "...", debugName, availableTimestamp, targetTimestamp);
      fflush(stderr);
      wasLogged=true;
    }
  }
}
static void busyWaitDone(uint64_t availableTimestamp, uint64_t targetTimestamp)
{
  if(wasLogged)
  {
    wasLogged=false;
      fprintf(stderr, "DONE\n");
      fflush(stderr);
  }
}

