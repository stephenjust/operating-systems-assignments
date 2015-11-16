/* Test program for memchunk assignment.
 *
 * Stephen Just
 *
 * This program will execute get_mem_layout() and print out the results
 * to a terminal. If the number of chunks exceeds the size of the allocated
 * list, then a warning will be printed.
 */

#include <stdio.h>
#include "memchunk.h"

#define LIST_SIZE 15

int main(int argc, char *argv[])
{
	struct memchunk list[LIST_SIZE];
	int num_chunks = 0;
	int i;

	num_chunks = get_mem_layout(list, LIST_SIZE);

 	if (num_chunks > LIST_SIZE)
	{
		printf("Exceeded maximum list size of %d. Found %d chunks.\n",
		       LIST_SIZE, num_chunks);
		num_chunks = LIST_SIZE;
	}

	for (i = 0; i < num_chunks; i++)
	{
		printf("Chunk %d:\n"
		       "\tStart: %lX\n"
		       "\tEnd: %lX\n"
		       "\tLength: %lX\n"
		       "\tRW: %d\n",
		       i, (unsigned long) list[i].start,
		       (unsigned long) list[i].start + list[i].length,
		       list[i].length, list[i].RW);
	}
	return 0;
}
