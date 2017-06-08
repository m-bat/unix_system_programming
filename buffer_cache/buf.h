#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define STAT_LOCKED   0x00000001
#define STAT_VALID    0x00000002
#define STAT_DWR      0x00000004
#define STAT_KRDWR    0x00000008
#define STAT_WAITED   0x00000010
#define STAT_OLD      0x00000020
#define ARRAY_MAX 12
#define BUFSIZE 256
#define NHASH 4
#define NSTAT 6

struct buf_header {
	int blkno;
	struct buf_header *hash_fp;
	struct buf_header *hash_bp;
	unsigned int stat;
	struct buf_header *free_fp;
	struct buf_header *free_bp;
	char *cache_data;
};

void insert_head(struct buf_header *h, struct buf_header *p);
void insert_tail(struct buf_header *h, struct buf_header *p);
void insert_free_tail(struct buf_header *h, struct buf_header *p);
void remove_from_hash(struct buf_header *p);
struct buf_header * search_hash(int blkno);
void list_print();
int hash_mod(int num);
void hash_head_init();
char * stat(struct buf_header *p, char *str);
void set(int blkno, char *stat);
void reset(int blkno, char *stat);

unsigned int stat_decision(char stat);
void free_print();
struct buf_header *getblk(int blkno);
void brelse(struct buf_header *buffer);
void command_brelse(int blkno);
void list(long blkno);

struct buf_header hash_head[NHASH];
struct buf_header free_list_header;


void hash_proc(int, char *[]);
void exit_proc(int, char *[]);
void init_proc(int, char *[]);
void free_print(int, char *[]);
void buf_proc(int, char *[]);
void set_proc(int, char *[]);
void reset_proc(int, char *[]);
void getblk_proc(int, char *[]);
void brelse_proc(int, char *[]);
void help_proc(int, char *[]);
