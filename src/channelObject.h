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
#ifndef SIMULATOR_CHANNEL_OBJECT_H
#define SIMULATOR_CHANNEL_OBJECT_H
#include "ringBuffer.h"

/// Maximum number of sinks of a single channel. If needs to be increased it only has RAM usage effect.
#define MAX_CHANNEL_SINK 4
/// When events are put into a causal effect channel then this is the size of the additional header that is put into the ringbuffer.
#define CHANNEL_OBJECT_HEADER_SIZE 8
/// Name of channel bytes limit
#define MAX_CHANNEL_NAME_LENGTH 255

struct channelObject_str;
struct channelObjectSink_str;

/// Callback type that handles events of a channel on the sink (receiver) side
/// @param parameter user provided parameter pointer - which was set by channelObjectSink_setEnabled(...) call
/// @param globalTimestamp the time stamp of the event on the global clock when it has to be processed on the receiver side
/// @param co pointer to the channel sink that this event was read from
/// @param data data of the event. Type or structure is defined by the implementor of the channel
/// @param size size of data in bytes.
typedef void (*channelObjectEventCallback_t) (void * parameter, uint64_t globalTimestamp, struct channelObjectSink_str * co, uint8_t * data, uint32_t size);

/// The channel sink object. Each receiver of the channel has one sink object that holds a ringbuffer with the channel events.
/// (Receivers need a separate sink object because the ringBuffer structure can only have one reader not more. It could be possible to implement the same behaviour with a single multi-reader ringbuffer. That could spare some RAM.
typedef struct channelObjectSink_str
{
	/// This stores event timestamps and event data pairs
	ringBuffer_t buffer;
	/// The channel that is the source of this sink
	struct channelObject_str * host;
	/// Enabled for write. When reading is not running then must be disabled to avoid blocking the write thread.
	volatile bool enabled;
	/// This callback is called when an event is processed from the channel sink.
	volatile channelObjectEventCallback_t callback;
	/// User defined parameter that is passed to the callback. Not handled by the library.
	void * parameter;
	/// Temporary buffer used to store the events read from the sink. The creator of the object allocates this buffer statically
	uint8_t * readBuffer;
} channelObjectSink_t;

/// The channel object. The event source writes the events into this object.
typedef struct channelObject_str
{
	/// The simulation of this channel is ready until this timestamp. Readers of the channel
	/// can advance their simulation until this timestamp without waiting.
	uint64_t simulatedUntil;
	/// Latency of causal effect propagation measured in global ticks. Must be at least 1.
	/// When an event is added to the channel this is added to the event timestamp.
	/// In case the source simulation is executed until timestamp T then this channel can be marked to simulatedUntil T+minimalLatency. Values more than 1 are useful because such channels can be simulated more efficient. Value 1 means source and sink can signal each other within 1 simulated tick - very small latency.
	uint64_t minimalLatency;
	/// Set a name of the channel - Useful because it is visible in debugger or can be written into log files.
	char debugName[MAX_CHANNEL_NAME_LENGTH+1];
	/// Size of the messages in this channel. Current implementation only allows same size messages within a channel.
	uint32_t messageSize;
	/// Number of event sinks registered
	uint32_t nSink;
	/// Storage for sinks registered with this channel. Unregistered sinks are unconfigured.
	channelObjectSink_t sinks[MAX_CHANNEL_SINK];
} channelObject_t;


/// Initialize the channel structure
/// @param channel uninitialized static storage channel structure
/// @param messageSize size of a single message in bytes. A 64 bit timestamp is also stored with each message.
void channelObject_create(channelObject_t * channel, uint32_t messageSize);
/// In case minimal latency is not 1 this can be set to a higher value using this method.
/// Higher value improve the performance of the simulator but means higher event propagation time in the simulated domain.
void channelObject_setMinimalLatency(channelObject_t * co, uint64_t minimalLatency);
/// Allocate a channel sink connected to the channel source
/// @param bufferSize size of the buffer in bytes that will be used as ringbuffer
/// @param buffer buffer used as ringbuffer to store events that will be read by this channel sink
channelObjectSink_t * channelObject_allocateSink(channelObject_t * channel, uint32_t bufferSize, uint8_t * buffer);
/// Wait until the timestamp is reached by the simulation of the channel.
/// Process events stored in the sink until the given timestamp.
/// Events are processed in order of their timestamps. Events processed are removed from the ringbuffer. the registered callback is executed for each event processed.
/// @param timestamp largest valid value is the current global time of the time domain. Earlier events are processed. Later events stay in the ringbuffer. Larger values may cause deadlock in case of circular dependency
void channelObject_processEventsUntil(channelObjectSink_t * co, uint64_t timestamp);
/// Process events stored in the sink until the given timestamp. Does not wait for the simulation of the channel!
/// (Useful in cases when the reader of the sink wants to empty the ringbuffer to avoid deadlock by overflow but does not need to react in real time to the channel so it omits waiting for the channel for performance reasons.)
/// Events are processed in order of their timestamps. Events processed are removed from the ringbuffer. the registered callback is executed for each event processed.
/// @param timestamp largest valid value is the current global time of the time domain. Earlier events are processed. Later events stay in the ringbuffer. Larger values may cause deadlock in case of circular dependency
void channelObject_processEventsUntilNoWait(channelObjectSink_t * co, uint64_t timestamp);
/// Add an event to the channel
/// @param timestamp the required global timestamp
/// @return the timestamp at which the event was finally enqued at. It is  more than the required timestamp if the minimalLatency requires it to be increased. Or when there is already an event in the queue then the timestamp is increased to be at least one more than the last enqueued event.
uint64_t channelObject_insertEvent(channelObject_t * co, uint64_t timestamp, uint8_t * data);
/// Update the current time of the source - means that this object is simulated until the timestamp.
/// This means that after the last event until this timestamp there is no event on the channel.
/// All listeners of the channel can be simulated until this timestamp.
void channelObject_updateTime(channelObject_t * co, uint64_t timestamp);
/// Wait until the channel is simulated until the given time. Busy wait polling the simulatedUntil timestamp.
void channelObject_waitSimulatedUntil(channelObject_t * co, uint64_t timestamp);
/// Enable/disable event propagation through the channel sink. Also sets up the callback object and the temporary buffer used
/// to store the events currently being read.
/// @param bufferSize size of buffer in bytes. Has to be at least messageSize+CHANNEL_OBJECT_HEADER_SIZE
/// @param buffer statically allocated buffer that is used by the sink object (in processEventsUntil and processEventsUntilNoWait) while the sink is active
void channelObjectSink_setEnabled(channelObjectSink_t * sink, bool enabled, channelObjectEventCallback_t callback, void * parameter, uint32_t bufferSize, uint8_t * buffer);
/// Peek into the sink ringbuffer and read the next unprocessed timestamp in the event queue of the channel sink.
/// @return In case there is no event in the ringBuffer then UINT64_MAX is returned
uint64_t channelObjectSink_getNextEventTimeStamp(channelObjectSink_t * sink);
#endif

