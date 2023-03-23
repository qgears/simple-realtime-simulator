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
#include "assert.h"

// TODO implement slow down or speed up clock properly


void localClock_create(localClock_t * lc, uint64_t initialGlobalTime, uint64_t multiplierToLocal,
		uint64_t multiplierTo_us, uint64_t multiplier_us_to_ticks, int64_t addGlobalToLocalTicks)
{
	lc->globalTime=initialGlobalTime;
	lc->multiplierToLocal=multiplierToLocal;
	lc->multiplierTo_us=multiplierTo_us;
	lc->multiplier_us_to_ticks=multiplier_us_to_ticks;
	lc->nChannelInFlush=0;
  lc->nChannelInSimulate=0;
	lc->nChannelOut=0;
	lc->addGlobalToLocalTicks=addGlobalToLocalTicks;
	for(uint32_t i=0;i<CLOCK_N_TIMERS;++i)
	{
		lc->timers[i].enabled=false;
	}
}

void localClock_registerChannel(localClock_t * lc, channelObject_t * channel)
{
	assert(lc->nChannelOut<CLOCK_MAX_CHANNELS);
	lc->channelsOut[lc->nChannelOut]=channel;
	lc->nChannelOut++;
}

void localClock_registerSinkToFlush(localClock_t * lc, channelObjectSink_t * sink)
{
  assert(lc->nChannelInFlush<CLOCK_MAX_CHANNELS);
  lc->channelsInFlush[lc->nChannelInFlush]=sink;
  lc->nChannelInFlush++;
}
void localClock_registerSinkToSimulate(localClock_t * lc, channelObjectSink_t * sink)
{
  assert(lc->nChannelInSimulate<CLOCK_MAX_CHANNELS);
  lc->channelsInSimulate[lc->nChannelInSimulate]=sink;
  lc->nChannelInSimulate++;
}



uint64_t localClock_currentLocal(localClock_t * lc)
{
	uint128_t value=lc->globalTime;
	value*=lc->multiplierToLocal;
	value>>=32;
	return value+lc->addGlobalToLocalTicks;
}
uint64_t localClock_currentGlobal(localClock_t * lc)
{
	return lc->globalTime;
}

uint64_t localClock_toLocal(localClock_t * lc, uint64_t globalTime)
{
	uint128_t value=globalTime;
	value*=lc->multiplierToLocal;
	value>>=32;
	return value+lc->addGlobalToLocalTicks;
}
/*uint64_t localClock_toGlobal(localClock_t * lc, uint64_t localTime)
{
	uint128_t value=localTime;
//	value*=lc->multiplierToGlobal;
//	value>>=32;
	return value;
}*/

uint64_t localClock_get_ms(localClock_t * lc)
{
	uint128_t value=localClock_currentLocal(lc);
	value*=lc->multiplierToLocal;
	value>>=32;
	return value/1000/1000;
}

uint64_t localClock_get_us(localClock_t * lc)
{
	uint128_t value=localClock_currentLocal(lc);
	value*=lc->multiplierToLocal;
	value>>=32;
	return value/1000;
}
static uint64_t localClock_tryAdvanceTimeGlobal(localClock_t * lc, uint64_t targetGlobalTime)
{
  uint64_t ret=UINT64_MAX;
//  int32_t channelIndex=-1;
//  int32_t timerIndex=-1;
  bool retry=false;
  bool waited=false;
  uint64_t now=lc->globalTime;
    for(int32_t i=0;i<lc->nChannelInSimulate;++i)
    {
      channelObjectSink_t * channelIn=lc->channelsInSimulate[i];
      uint64_t t=channelIn->host->simulatedUntil;
      if(t<=now)
      {
        channelObject_waitSimulatedUntil(channelIn->host, now+1);
      }
      t=channelIn->host->simulatedUntil;
      if(t<ret)
      {
        ret=t;
//        channelIndex=i;
      }
      t=channelObjectSink_getNextEventTimeStamp(channelIn);
      if(t<ret)
      {
        ret=t;
//        channelIndex=i;
      }
    }
    for(int32_t i=0;i<CLOCK_N_TIMERS;++i)
    {
      if(lc->timers[i].enabled)
      {
        if(lc->timers[i].timeoutAtGlobal<ret)
        {
          ret=lc->timers[i].timeoutAtGlobal;
//          timerIndex=i;
//          channelIndex=-1;
        }
      }
    }
  if(ret>targetGlobalTime)
  {
    ret=targetGlobalTime;
  }
  if(ret>lc->globalTime)
  {
    lc->globalTime=ret;
  }
  for(int32_t i=0;i<CLOCK_N_TIMERS;++i)
  {
    if(lc->timers[i].enabled)
    {
      if(lc->timers[i].timeoutAtGlobal<=ret)
      {
        if(lc->timers[i].period>0)
        {
          lc->timers[i].timeoutAtGlobal+=lc->timers[i].period;
        }else
        {
          lc->timers[i].enabled=false;
        }
        lc->timers[i].callback(lc->timers[i].parameter);
      }
    }
  }
  for(uint32_t i=0;i<lc->nChannelOut;++i)
  {
    channelObject_t * channelOut=lc->channelsOut[i];
    channelObject_updateTime(channelOut, ret);
  }
  for(uint32_t i=0;i<lc->nChannelInFlush;++i)
  {
    channelObjectSink_t * channelIn=lc->channelsInFlush[i];
    channelObject_processEventsUntilNoWait(channelIn, ret);
  }
  for(uint32_t i=0;i<lc->nChannelInSimulate;++i)
  {
    channelObjectSink_t * channelIn=lc->channelsInSimulate[i];
    channelObject_processEventsUntil(channelIn, ret);
  }
  return ret;
}
/// Advance the global time simulated by this object
/// Mark all outputs simulated until marked time
/// Set the global time value
static void localClock_advanceTimeGlobal(localClock_t * lc, uint64_t targetGlobalTime)
{
  uint64_t ret=lc->globalTime;
  while(ret<targetGlobalTime)
  {
    ret=localClock_tryAdvanceTimeGlobal(lc, targetGlobalTime);
  }
}

/*
static bool localClock_executeNextTimerTimeout(localClock_t * lc, uint64_t targetGlobalTime)
{
	if(index>=0 && ret<=targetGlobalTime)
	{
		if(lc->timers[index].period>0)
		{
			lc->timers[index].timeoutAtGlobal+=lc->timers[index].period;
		}else
		{
			lc->timers[index].enabled=false;
		}
		localClock_advanceTimeGlobal(lc, ret);
		lc->timers[index].callback(lc->timers[index].parameter);
		return true;
	}
	return false;
}
*/

/// Wait until the local time reaches the given time. This means that the simulated outputs are updated to this time.
void localClock_waitUntilGlobal(localClock_t * lc, uint64_t targetGlobalTime)
{
//	while(localClock_executeNextTimerTimeout(lc, targetGlobalTime));
	localClock_advanceTimeGlobal(lc, targetGlobalTime);
}
void localClock_setTimer(localClock_t * lc, uint32_t timerIndex, bool enabled, uint64_t timeoutAt, uint64_t period, localClock_timerCallback_t callback, void * param)
{
	assert(timerIndex<CLOCK_N_TIMERS);
	lc->timers[timerIndex].enabled=enabled;
	lc->timers[timerIndex].timeoutAtGlobal=timeoutAt;
	lc->timers[timerIndex].period=period;
	lc->timers[timerIndex].callback=callback;
	lc->timers[timerIndex].parameter=param;
}
uint64_t localClock_us_to_ticks(localClock_t * lc, uint64_t us)
{
	uint128_t v=us;
	return (uint64_t)((v*lc->multiplier_us_to_ticks) >> 32);
}
uint64_t localClock_ticks_to_us(localClock_t * lc, uint64_t ticks)
{
  uint128_t v=ticks;
  v<<=32;
  return (uint64_t)(v/lc->multiplier_us_to_ticks);
}


