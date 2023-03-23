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

#include "sharedMemory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "assert.h"

#define TIMEOUT_MILLIS 10000

void * sharedMemory_open(const char * name, uint32_t sizeBytes, bool master)
{
    /* shared memory file descriptor */
    int shm_fd=-1;

    /* pointer to shared memory object */
    void* ptr;

    if(master)
    {
		/* create the shared memory object */
		shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);

    assertErrno(shm_fd>=0);

		/* configure the size of the shared memory object */
		assertErrno(ftruncate(shm_fd, sizeBytes)==0);
    }else
    {
    	int ctr=0;
        /* open the shared memory object */
    	while(shm_fd==-1)
    	{
    		shm_fd = shm_open(name, O_RDWR, 0666);
    		if(shm_fd==-1)
    		{
    			ctr++;
    			if(ctr>TIMEOUT_MILLIS)
    			{
    				assert(false);
    			}
    			usleep(1000);
    		}
    	}

        assertErrno(shm_fd>=0);
    }
    /* memory map the shared memory object to the same address in each process: */
    void * sharedPtr=(void *)0x10000;
    ptr = mmap(sharedPtr, sizeBytes, PROT_WRITE, MAP_SHARED|MAP_FIXED, shm_fd, 0);
    assertErrno(ptr!=MAP_FAILED);
    return ptr;
}
