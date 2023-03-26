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
#ifndef SIMULATOR_ASSERT_H_
#define SIMULATOR_ASSERT_H_

/**
 * Assert functions.
 */


#include "simulator_types.h"

#define assert(conditionMustBeTrue) assert_withFileAndPosition(conditionMustBeTrue, __FILE__,__LINE__);
#define assertErrno(conditionMustBeTrue) assertErrno_withFileAndPosition(conditionMustBeTrue, __FILE__,__LINE__);
/// Assert function that checks condition and signals failure in case the condition is false.
/// Signalling failure is platform dependent:
///  * On PC simulation stops execution of program and logs the current stack trace.
void assert_withFileAndPosition(bool conditionMustBeTrue, const char * fileName, int line);

/// Assert function that checks condition and signals failure in case the condition is false.
/// Signalling of failure is same as in case of assert()
/// Also logs the errno provided by the operating system in case of PC
void assertErrno_withFileAndPosition(bool mustBeTrue, const char * fileName, int line);


#endif /* SIM_PC_HAL_ASSERT_H_ */

