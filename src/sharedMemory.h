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

#ifndef SIM_PC_SIMULATOR_SHAREDMEMORY_H_
#define SIM_PC_SIMULATOR_SHAREDMEMORY_H_

/// Open/create shared memory used by the simulator.

#include "simulator_types.h"

/// Open the shared memory instance of the simulation instance.
/// File name of the shared memory object is default or got from environment variable.
/// The shared memory is mapped to the same pointer in each processes. The shared memory is intended to hold the simulator related objects (channels)
/// And communication between the MCUs (processes or threads) is done using this shared memory.
/// @param master true means this is the master process and should create the shared memory. False means this is not master and should wait for shm to exist.
void * sharedMemory_open(const char * name, uint32_t sizeBytes, bool master);

#endif /* SIM_PC_SIMULATOR_SHAREDMEMORY_H_ */
