#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "memchunk.h"

#define RET_MAPERR 1
#define RET_ACCERR 2

static sigjmp_buf jmpbuf;

static int get_page_access(unsigned long address);
static void setup_signal_handler();
static void segfault_sigaction(int signal, siginfo_t *si, void *arg);

int get_mem_layout(struct memchunk *chunk_list, int size)
{
	struct memchunk chunk;
	int chunk_num = 0;
	unsigned long mem_position = 0;
	int page_size = 0;
	int block_accessible;

	page_size = getpagesize();
	setup_signal_handler();

	/* Initialize the first chunk */
	chunk.start = (void *) 0;
	chunk.length = 0;
 	chunk.RW = get_page_access(0);
	mem_position += page_size;

	while (mem_position <= 0xFFFFFFFF)
	{
		block_accessible = get_page_access(mem_position);

		if (block_accessible != chunk.RW) {
			/* Accessibility status changed. Save previous chunk. */
			chunk.length = mem_position -
				(unsigned long) chunk.start;
			if (chunk_num < size) { /* If array is big enough... */
				memcpy(&chunk_list[chunk_num],
				       &chunk, sizeof(chunk));
			}
			chunk_num++;
			/* Prepare next chunk */
			chunk.length = 0;
			chunk.RW = block_accessible;
			chunk.start = (void *) mem_position;
		}
		/* Break if we overflow. */
		if (mem_position + page_size < mem_position) break;
		mem_position += page_size;
	}
	chunk.length = 0xFFFFFFFF - (unsigned long) chunk.start;
	if (chunk_num < size) { /* If array is big enough... */
		memcpy(&chunk_list[chunk_num], &chunk, sizeof(chunk));
	}

	return chunk_num + 1;
}

/* Check whether the program can access the page of memory at an address. */
static int get_page_access(unsigned long address)
{
	int segv_return = 0;
	int block_accessed = 0;
	int block_accessible = 0;
	char data = 0;

	/* This is true iff a segfault is hit later and the code jumps back */
	if ((segv_return = sigsetjmp(jmpbuf, 1))) {
		if (segv_return == RET_ACCERR) {
			block_accessible = 0;
		} else if (segv_return == RET_MAPERR) {
			block_accessible = -1;
		}
		block_accessed = 1;
	}

	if (!block_accessed) {
		/* Attempt to read a block, then write its data back.
		 * If we just write random data back, we will corrupt the
		 * program's memory.
		 */
		data = ((char *) address)[0];
		((char *) address)[0] = data;
	}
	if (!block_accessed) {
		/* No segfault was hit, so we have R/W access to this page */
		block_accessible = 1;
	}
	return block_accessible;
}

/* Enable a signal handler to catch SIGSEGV events. */
static void setup_signal_handler()
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = segfault_sigaction;
	sa.sa_flags = SA_SIGINFO;
	sigaction(SIGSEGV, &sa, NULL);
}

/* Handle SIGSEGV events.
 *
 * SIGSEGV has two possible signal codes associated with it:
 *     SEGV_MAPERR: Address is not mapped to the program. This means that
 *                  it is neither readable nor writable.
 *     SEGV_ACCERR: Address is mapped to the program but cannot be accessed.
 *                  This only happens when it is readable but not writable.
 *
 * The signal handler will use siglongjmp to pass the particular code back
 * to the program.
 */
static void segfault_sigaction(int signal, siginfo_t *si, void *arg)
{
	if (si->si_code == SEGV_MAPERR) {
		siglongjmp(jmpbuf, RET_MAPERR);
	} else if (si->si_code == SEGV_ACCERR) {
		siglongjmp(jmpbuf, RET_ACCERR);
	} else {
		// Unexpected segfault
		exit(1);
	}
}
