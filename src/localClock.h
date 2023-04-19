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

#ifndef SIMULATOR_LOCALCLOCK_H
#define SIMULATOR_LOCALCLOCK_H

/// Local clock implementation (abstraction of quartz or internal oscillator that runs an MCU).
/// TODO speed up/down compared to the global clock is not implemented yet.

#include "simulator_types.h"
#include "channelObject.h"

/// Maximum number of channels (source) associated with a clock. If has to be increased it only increases RAM usage
#define CLOCK_MAX_CHANNELS 8
/// Maximum number of timers associated with a clock. If has to be increased it only increases RAM usage
#define CLOCK_N_TIMERS 8

/// When converting to/from global/local clock this is a divisor used.
/// This is 2^32 so division by it is implemneted as a shift operation.
#define BASE_MULTIPLIER ((uint64_t)4294967296ull)

/// Prototype of callback function of a timer
/// @param parameter user defined parameter object
typedef void (*localClock_timerCallback_t) (void * parameter);

/// A timer connected to the local clock
/// Both timeoutAt and period are measured in global timestamps!
typedef struct
{
	volatile bool enabled;
	volatile uint64_t timeoutAtGlobal;
	volatile uint64_t period;
	localClock_timerCallback_t callback;
	void * parameter;
  bool allocated;
} localClock_timer_t;

/// A local clock domain
/// Typically the clock of an MCU
typedef struct
{
	/// The current simulated global time tick
	uint64_t globalTime;
	uint64_t multiplierToLocal;
	uint64_t multiplierTo_us;
	/// When convertng global time to local time add this value
  int64_t addGlobalToLocalTicks;
	uint64_t multiplier_us_to_ticks;
	uint32_t nChannelOut;
	/// All output channels sourced by this clock domain. When time is advanced all output is marked to be simulated until this time
	channelObject_t * channelsOut[CLOCK_MAX_CHANNELS];
	uint32_t nChannelInSimulate;
	/// TODO implement - All input channels that are capable of triggering interrupt (pin change interrupt, UART interrupt, etc.)
	/// Time is advancement is blocked until the simulation of these input channels is ready and these are all scanned for events when advancing time.
 	channelObjectSink_t * channelsInSimulate[CLOCK_MAX_CHANNELS];
  uint32_t nChannelInFlush;
  /// All input channels that are to be flushed so that the ringbuffer does not block the writer side.
  /// Blocking wait for time advancement is not necessary.
  channelObjectSink_t * channelsInFlush[CLOCK_MAX_CHANNELS];
 	localClock_timer_t timers[CLOCK_N_TIMERS];
 	/// Require exit of this simulator thread
 	volatile bool exit;
} localClock_t;


/// Initialize the clock structure
/// @param initialGlobalTime the global timestamp when this objects connects the simulation - may be different than 0
/// @param multiplierToLocal speed of clock compared to the global clock - global time is multiplied with this and >>32 to get local time (1.0 is encoded as 2^32).
/// @param multiplierTo_us speed of clock compared to microseconds - global time is multiplied with this and >>32 to get local time (1.0 is encoded as 2^32).
void localClock_create(localClock_t * lc, uint64_t initialGlobalTime, uint64_t multiplierToLocal,
		uint64_t multiplierTo_us, uint64_t multiplier_us_to_ticks, int64_t addGlobalToLocalTicks);

/// Wait until the local time reaches the given time. This means that the simulated outputs are updated to this time.
void localClock_waitUntilGlobal(localClock_t * lc, uint64_t globalTime);

uint64_t localClock_currentLocal(localClock_t * lc);

uint64_t localClock_currentGlobal(localClock_t * lc);

uint64_t localClock_toLocal(localClock_t * lc, uint64_t globalTime);

uint64_t localClock_get_ms(localClock_t * lc);

uint64_t localClock_get_us(localClock_t * lc);
uint64_t localClock_us_to_ticks(localClock_t * lc, uint64_t us);
uint64_t localClock_ticks_to_us(localClock_t * lc, uint64_t ticks);
/// Setup timer
/// @param timerIndex identify timer to be used
/// @param enabled enable/disable timer
/// @param timeoutAt measured in local time
/// @param period measured in local time ticks
/// @param callback callback function that is called when the timer has elapsed
/// @param param parameter passed to the callback function - not accessed by the timer itself and may be NULL
void localClock_setTimer(localClock_t * lc, uint32_t timerIndex, bool enabled, uint64_t timeoutAt, uint64_t period, localClock_timerCallback_t callback, void * param);
/// Allocate one of the timers
uint32_t localClock_allocateTimer(localClock_t * lc);
/// Allocate one of the timers
void localClock_releaseTimer(localClock_t * lc, uint32_t timerIndex);
/// Register an output channel with the local clock object. The simulation of the channel is marked to advance in time whenever the clock time is advancing.
void localClock_registerChannel(localClock_t * lc, channelObject_t * channel);
/// Register an input channel to be flushed when time is advancing so that the ringbuffer will not overflow and block the writing thread.
void localClock_registerSinkToFlush(localClock_t * lc, channelObjectSink_t * sink);
/// Register an input channel to be listened when time is advancing so that time may not be advanced until the source reaches simulation time.
void localClock_registerSinkToSimulate(localClock_t * lc, channelObjectSink_t * sink);



#endif
