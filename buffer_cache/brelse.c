#include "buf.h"

extern scenario4;
void brelse(struct buf_header *buffer)
{
	if (scenario4 == 1) {
		printf("Wakeup processes waiting for any buffer\n");
		scenario4 = 0;
	}

	if (buffer->stat & STAT_WAITED) {
		printf("Wakeup processes waiting for buffer of blkno %d\n", buffer->blkno);
		reset(buffer->blkno, "W");
	}
	//raise_cpu_level();
	if (!(buffer->stat & STAT_OLD)) {
		insert_free_tail(&free_list_header, buffer);
		printf("enqueue buffer at end of free list\n");
	}
	else {
		insert_free_head(&free_list_header, buffer);
		reset(buffer->blkno, "O");
		printf("enqueue buffer at beginning of free list\n");
	}

	//lower_cpu_level();

	buffer->stat &= ~STAT_LOCKED;
}
