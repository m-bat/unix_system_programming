#include "buf.h"
void help_proc(int ac, char *av[])
{
	if (ac > 1) {
		fprintf(stderr, "Usage: help [no option]\n");
	}
	else {
		printf("---------------------------------------------------\n");
		printf("\x1b[1m");
		printf("help\n\n");
		printf("\x1b[0m");
		printf("To display the syntax and function of each command\n");
		printf("---------------------------------------------------\n");
		printf("\x1b[1m");
		printf("init\n\n");
		printf("\x1b[0m");
		printf("To initialize the hash list and a free list, and set to the state of Figure 2.15\n");
		printf("---------------------------------------------------\n");
		printf("\x1b[1m");
		printf("buf [buffer number...]\n\n");
		printf("\x1b[0m");
		printf("If the argument is not specified, to display the status of all of the buffer.");
		printf("If an argument is specified, to display the status of the specified buffer number.\n");
		printf("---------------------------------------------------\n");
		printf("\x1b[1m");
		printf("hash [logical block number...]\n\n");
		printf("\x1b[0m");
		printf("If the argument is not specified, to display the status of all of the hash list.");
		printf("If an argument is specified, to display the status of the specified hash list of hash value.\n");
		printf("---------------------------------------------------\n");
		printf("\x1b[1m");
		printf("free\n\n");
		printf("\x1b[0m");
		printf("To display free list.\n");
		printf("---------------------------------------------------\n");
		printf("\x1b[1m");
		printf("getblk n\n\n");
		printf("\x1b[0m");
		printf("It specifies the logical block number as an argument, to execute tgetblk(n).\n");
		printf("---------------------------------------------------\n");
		printf("\x1b[1m");
		printf("brelse n\n\n");
		printf("\x1b[0m");
		printf("It specifies the logical block number as an argument. Using a pointer bp of the buffer header of the number, to excute brelse(bp).\n");
		printf("---------------------------------------------------\n");
		printf("\x1b[1m");
		printf("set n stat[stat...]\n\n");
		printf("\x1b[0m");
		printf("To set the buffer of the logical block number n to state stat.\n");
		printf("---------------------------------------------------\n");
		printf("\x1b[1m");
		printf("reset n stat[stat...]\n\n");
		printf("\x1b[0m");
		printf("To reset the buffer of the logical block number n to state stat.\n");
		printf("---------------------------------------------------\n");
		printf("\x1b[1m");
		printf("quit\n\n");
		printf("\x1b[0m");
		printf("To exit this software\n");	
	}
}

void exit_proc(int ac, char *av[])
{
	exit(0);
}

void buf_proc(int ac, char *av[]) {
	int j = 0, i = 0, count = 0, k;
	char str[NSTAT + 1];
	char *endptr;
	long buf[BUFSIZE];
	int boolean = 0;
	struct buf_header *p, *h;
	if (ac == 1) {
		for (i = 0; i < NHASH; i++) {
			h = &hash_head[i];
			for (p = h->hash_fp; p != h; p = p->hash_fp) {
				printf("[%2d: %2d %s]\n", j++, p->blkno, stat(p, str));
				
			}
		}
	}
	else {
		for (i = 1; i < ac; i++, j++) {
			count = 0;
			boolean = 0;
			buf[j] = strtol(av[i], &endptr, 10);
			if (*endptr !='\0') {
				fprintf(stderr, "Usage: buf [buffer number...]\n");
			}
			else {
				for (k = 0; k < NHASH; k++) {
					if (boolean == 0) {
						h = &hash_head[k];
						for (p = h->hash_fp; p != h; p = p->hash_fp, count++) {
							if (count == buf[j]) {
								printf("[%2d: %2d %s]\n", count, p->blkno, stat(p, str));
								boolean = 1;
								break;
							}
						}
					}
				}
				if (boolean == 0) {
					fprintf(stderr, "buf number %ld is not exsit\n", buf[j]);
			
				}
			}
		}
	}
}


void hash_proc(int ac, char *av[]) {
	int i = 0, j = 0;
	long hash[16];
	char *endptr;
	
	if (ac == 1) {
		list_print();
	}
	else {
		for (i = 1; i < ac; i++, j++) {		
			hash[j] = strtol(av[i], &endptr, 10);
			if (*endptr !='\0') {
				fprintf(stderr, "Usage: hash [hash value...]\n");
			}
			else {
				list(hash[j]);
				
			}
			
		}
	}
}

void init_proc(int ac, char *av[])
{
	struct buf_header *p;
	int array[ARRAY_MAX] = {28, 4, 64, 17, 5, 97, 98, 50, 10, 3, 35, 99};
	int free[6] = {3, 5, 4, 28, 97, 10};
	int i, mod;

	if (ac > 1) {
		fprintf(stderr, "Usage: init [no option]\n");
	}
	else {

	hash_head_init();
	for (i = 0; i < ARRAY_MAX; i++) {
		p = (struct buf_header *)malloc(sizeof(struct buf_header));
		if (p == NULL) {
			fprintf(stderr, "Can't allocate memory\n");
			exit(1);
		}
		
		mod = hash_mod(array[i]);
		p->blkno = array[i];
		p->stat = STAT_VALID;

		if (array[i] == 64 || array[i] == 17 || array[i] == 98||
			array[i] == 50 || array[i] == 35 || array[i] == 99) {
			 p->stat |= STAT_LOCKED;	 
		}
		
		insert_tail(&hash_head[mod], p);
	}
	for (i = 0; i < 6; i++) {
		mod = hash_mod(free[i]);
		for (p = hash_head[mod].hash_fp; p != &hash_head[mod]; p = p->hash_fp) {
			if (p->blkno == free[i])
				insert_free_tail(&free_list_header, p);
		}
	}

	printf("execute init\n");
	}
}

//フリーリスト表示
void free_print(int ac, char *av[]) {
	int j = 0, i = 0, count = 0, boolean = 0;
	char str[NSTAT + 1];
	struct buf_header *p, *t;

	if (ac > 1) {
		fprintf(stderr, "Usage: free [no option]\n");
	}
	else {
		for (p = free_list_header.free_fp; p != &free_list_header; p = p->free_fp) {
			count = 0;
			boolean = 0;
			for (i = 0; i < NHASH; i++) {
				for (t = hash_head[i].hash_fp; t != &hash_head[i]; t = t->hash_fp, count++) {
					if (p == t) {
						boolean = 1;
						break;
					}
				}
				if (boolean == 1) {
					break;
				}
			}
			printf("[%2d: %2d %s] ", count, p->blkno, stat(p, str));
		}
		printf("\n");
	}
}


void set_proc(int ac, char *av[])
{
	int blkno, i;
	char stat[NSTAT];
	char *endptr;
	int flag = 0, state = 0;
	if (ac < 3) {
		fprintf(stderr, "Usage: set n stat [stat...]\n");
	}
	else {
		blkno = strtol(av[1], &endptr, 10);
		if (*endptr !='\0') {
			fprintf(stderr, "Usage: set n stat [stat...]\n");
		}
		else {
			for (i = 2; i < ac; i++) {
				if (strlen(av[i]) >= 2) {
					fprintf(stderr, "can't %s do not exist\n", av[i]);
					continue;
				}else {
					if (state == 0) { 
						strcpy(stat, av[i]);
						state = 1;
						flag = 1;
					}
					else {
						strcat(stat, av[i]);
						state = 1;
					}
				}				
			}
			if (flag == 1) {
				set(blkno, stat);
			}
		}
	}
}

void reset_proc(int ac, char *av[])
{
	int blkno, i;
	char stat[NSTAT];
	char *endptr;
	int state = 0, flag = 0;
	if (ac < 3) {
		fprintf(stderr, "Usage: reset n stat [stat...]\n");
	}
	else {
		blkno = strtol(av[1], &endptr, 10);
		if (*endptr !='\0') {
			fprintf(stderr, "Usage: reset n stat [stat...]\n");
		}
		else {
			for (i = 2; i < ac; i++) {
				if (strlen(av[i]) >= 2) {
					fprintf(stderr, "can't reset %s\n", av[i]);
					continue;
				}else {
					if (state == 0) { 
						strcpy(stat, av[i]);
						state = 1;
						flag = 1;
					}
					else {
						strcat(stat, av[i]);
						state = 1;
					}
				}				
			}
			if (flag == 1) {
				reset(blkno, stat);
			}
		}
	}
	
}

void getblk_proc(int ac, char *av[])
{
	int blkno;
	char *endptr;
	if (ac != 2) {
		fprintf(stderr, "Usage: getblk n\n");
	}
	else {
		blkno = strtol(av[1], &endptr, 10);
		if (*endptr !='\0') {
			fprintf(stderr, "Usage: getblk n\n");
		}
		else {
			getblk(blkno);
		}
	}	
}

void brelse_proc(int ac, char *av[])
{
	int blkno;
	char *endptr;
	if (ac != 2) {
		fprintf(stderr, "Usage: brelse n\n");
	}
	else {
		blkno = strtol(av[1], &endptr, 10);
		if (*endptr !='\0') {
			fprintf(stderr, "Usage: brelse n\n");
		}
		else {
			command_brelse(blkno);
		}
	}
}

//command操作終了

//list操作関数群
void insert_head(struct buf_header *h, struct buf_header *p) {
	p->hash_bp = h;
	p->hash_fp = h->hash_fp;
	h->hash_fp->hash_bp = p;
	h->hash_fp = p;
}

void insert_tail(struct buf_header *h, struct buf_header *p) {	
	h->hash_bp->hash_fp = p;
	p->hash_bp = h->hash_bp;
	p->hash_fp = h;
	h->hash_bp = p;	
}

void insert_free_head(struct buf_header *h, struct buf_header *p) {	
	p->free_bp = h;
	p->free_fp = h->free_fp;
	h->free_fp->free_bp = p;
	h->free_fp = p;
}

void insert_free_tail(struct buf_header *h, struct buf_header *p) {	
	h->free_bp->free_fp = p;
	p->free_bp = h->free_bp;
	p->free_fp = h;
	h->free_bp = p;	
}

void remove_from_hash(struct buf_header *p) {
	p->hash_bp->hash_fp = p->hash_fp;
    p->hash_fp->hash_bp = p->hash_bp;
	p->hash_fp = p->hash_bp = NULL;
}

void remove_free_from_hash(struct buf_header *p) {
	p->free_bp->free_fp = p->free_fp;
    p->free_fp->free_bp = p->free_bp;
	p->free_fp = p->free_bp = NULL;
}

struct buf_header * search_hash(int blkno) {
	int h;
	struct buf_header *p;

	h = hash_mod(blkno);
	for (p = hash_head[h].hash_fp; p != &hash_head[h]; p = p->hash_fp) {
		if (p->blkno == blkno)
			return p;
	}

	return NULL;
}


void hash_head_init()
{
	int i;

	for (i = 0; i < NHASH; i++) {
		hash_head[i].hash_fp=&hash_head[i];
		hash_head[i].hash_bp=&hash_head[i];
	}
	free_list_header.free_fp = &free_list_header;
	free_list_header.free_bp = &free_list_header;
}

char * stat(struct buf_header *p, char *str)
{
	int i;
	for (i = 0; i < NSTAT; i++)
		str[i] = '-';

	str[i] = '\0';
	
	if (p->stat & STAT_LOCKED)
		str[5] = 'L';
	if (p->stat & STAT_VALID)
		str[4] = 'V';
	if (p->stat & STAT_DWR)
		str[3] = 'D';
	if (p->stat & STAT_KRDWR)
		str[2] = 'K';
	if (p->stat & STAT_WAITED)
		str[1] = 'W';
	if (p->stat & STAT_OLD)
		str[0] = 'O';

	return str;
}

int hash_mod(int num)
{
	int mod;
	return mod = num % NHASH;
}

//list形式で全データ表示
void list_print()
{
	int i, j = 0;
	struct buf_header *p, *h;
    char str[NSTAT + 1];

	for (i = 0; i < NHASH; i++) {
		h = &hash_head[i];
		printf("%d: ", i);
		for (p = h->hash_fp; p != h; p = p->hash_fp) {
			
			printf("[%2d: %2d %s] ", j++, p->blkno, stat(p, str));
			
		}
  
		printf("\n");
	}
}
 

//引数ありのhashlistを表示
 void list(long hash)
 {
	 int i = 0, k, count = 0;
	 char str[NSTAT + 1];
	 struct buf_header *p, *h;
	 if (hash == 0 || hash == 1 || hash == 2 || hash == 3) {
		 h = &hash_head[hash];
		 printf("%ld: ", hash);
		 /*for ( p = h->hash_fp; p != h; p = p->hash_fp)
			 printf("[%2d: %2d %s] ", i++, p->blkno, stat(p, str));
			 printf("\n");*/
		 for (k = 0; k < NHASH; k++) {
			 h = &hash_head[k];
			 for (p = h->hash_fp; p != h; p = p->hash_fp, count++) {
				 if (k == hash) {
					 printf("[%2d: %2d %s]", count, p->blkno, stat(p, str));
					 
				 }
			 }
		 }

		 printf("\n");
	 }
	 else
		 fprintf(stderr, "hash number %ld do not exist\n", hash);
 }
	  


//statセット

void set(int blkno, char *stat)
{
	unsigned int x;
	int i, len, j;
	struct buf_header *p, *h;
	len = strlen(stat);
	
	for (i = 0; i < NHASH; i++) {
		h = &hash_head[i];
		for (p = h->hash_fp; p != h; p = p->hash_fp) {
			if (p->blkno == blkno) {
				for (j = 0; j < len; j++) {
					if ((x = stat_decision(stat[j])) == STAT_LOCKED) {
						if (!(p->stat & STAT_LOCKED)  && p->free_fp != NULL)
							remove_free_from_hash(p);
					}
					else if (x == -1) {
						printf("can't %c do not exist\n", stat[j]);
						continue;
					}
					p->stat |= stat_decision(stat[j]);
				}
				return;
			}
		}
	}
	fprintf(stderr, "blocknumber %d is not exist\n", blkno);
}

void reset(int blkno, char *stat)
{

	unsigned int x;
	int i, len, j;
	struct buf_header *p, *h;
	len = strlen(stat);
	for (i = 0; i < NHASH; i++) {
		h = &hash_head[i];
		for (p = h->hash_fp; p != h; p = p->hash_fp) {
			if (p->blkno == blkno) {
				for (j = 0; j < len; j++) {
					if ((x = stat_decision(stat[j])) != -1)
						p->stat &= ~stat_decision(stat[j]);
					else
						printf("can't reset %c\n", stat[j]);
				}
				return;
			}
		}
	}
	fprintf(stderr, "blocknumber %d do not exist\n", blkno);
}

unsigned int stat_decision(char stat)
{
	int x;
	switch(stat) {
	case 'L':
		x = STAT_LOCKED;
		break;
	case 'V':
		x = STAT_VALID;
		break;
	case 'D':
		x = STAT_DWR;
		break;
	case 'K':
		x = STAT_KRDWR ;
		break;
	case 'W':
		x = STAT_WAITED;
		break;
	case 'O':
		x = STAT_OLD;
		break;
	default :
		x = -1;
		break;
	}

	return x;
}



void command_brelse(int blkno)
{
	int mod;
	struct buf_header *p;
	mod = hash_mod(blkno);
	for (p = hash_head[mod].hash_fp; p != &hash_head[mod]; p = p->hash_fp) {
		if (p->blkno == blkno) {
			brelse(p);
			break;
		}
	}
	
}
