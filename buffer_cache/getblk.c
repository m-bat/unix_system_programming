#include "buf.h"

extern scenario4;
struct buf_header *getblk(int blkno)
{
	int mod;
	int boolean;
	mod = hash_mod(blkno);
	struct buf_header *p, *f, *h;
	h = &hash_head[mod];
	p = &hash_head[mod];
	p = p->hash_fp;
	while (1) {
		boolean = 0;
		while (p != &hash_head[mod]) {
			if (p->blkno == blkno){
				boolean = 1;
				break;
			}
			p = p->hash_fp;
		}
		if (boolean) {
			if (p->stat & STAT_LOCKED ) {
				/* scenario 5*/
				//sleep();
				printf("Process goes to scenario 5\n");
				printf("Process goes to sleep\n");
				set(blkno, "W");
				return NULL;
			}

			/*scenario 1*/
			p->stat |= STAT_LOCKED;
			remove_free_from_hash(p);
			printf("Process goes to scenario 1\n");
			return p;
		}else{
			if (free_list_header.free_fp == &free_list_header) {
				/*scenario 4*/
				printf("Process goes to scenario 4\n");
				scenario4 = 1;
				printf("Process goes to sleep\n");
				return NULL;
			}

			f = free_list_header.free_fp;
			remove_free_from_hash(f);
			if (f->stat & STAT_DWR) {
				reset(f->blkno, "D");
				set(f->blkno, "KOL");
				/*scenarion 3*/		
				printf("Process goes to scenario 3\n");
				continue;;
			}

			/*scenario 2*/
			set(f->blkno, "L");
			reset(f->blkno, "V");
			remove_from_hash(f);
			f->blkno = blkno;
			insert_tail(h, f);
			
			printf("Process goes to scenario 2\n");
			return p;
		}
	}
}
