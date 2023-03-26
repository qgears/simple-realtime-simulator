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

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

/* Obtain a backtrace and print it to stdout. https://www.gnu.org/software/libc/manual/html_node/Backtraces.html */
static void print_trace (void)
{
  void *array[10];
  char **strings;
  int size, i;

  size = backtrace (array, 10);
  strings = backtrace_symbols (array, size);
  if (strings != NULL)
  {
	fprintf( stderr, "Stack frames:\n");
    for (i = 0; i < size; i++)
      fprintf (stderr, "%s\n", strings[i]);
  }

  free (strings);
}

void assert_withFileAndPosition(bool mustBeTrue, const char * fileName, int line)
{
	if(!mustBeTrue)
	{
		fprintf( stderr, "Assert fail %s %d\n", fileName, line);
		print_trace();
		exit(1);
	}
}
void assertErrno_withFileAndPosition(bool mustBeTrue, const char * fileName, int line)
{
	if(!mustBeTrue)
	{
    fprintf( stderr, "Assert fail %s %d\n", fileName, line);
    perror("Assert fail Errno");
		print_trace();
		exit(1);
	}
}
