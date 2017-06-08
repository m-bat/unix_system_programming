

#include "buf.h"

struct command_table {
	char *cmd;
	void (*func)(int, char *[]);
}cmd_tbl[] = {
	{"buf",  buf_proc},
	{"hash", hash_proc},
	{"quit", exit_proc},
	{"init", init_proc},
	{"free", free_print},
	{"set", set_proc},
	{"reset", reset_proc},
	{"getblk", getblk_proc},
	{"brelse", brelse_proc},
	{"help", help_proc},
	{NULL, NULL}
};

int scenario4 = 4;

int main()
{	
	struct buf_header *p;
	struct command_table *t;
	int ac;
	char *av[16];
	char str[BUFSIZE];
	int i, exit = 1, flag = 0, j = 0, k = 0, l;

	init_proc(1, av);
	
	while (1) {
		j = 0;
		k = 0;
		flag = 0;
		char buf[BUFSIZE];
		printf("$ ");
		if (fgets(buf, BUFSIZE, stdin) == NULL) {
			fprintf(stderr, "input error\n");
			return -1;
		}
		
		if (buf[0] == '\n')
			continue;
		for (i = 0; buf[i] != '\0'; i++) {
			if (flag == 1) {
				if (buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\t') {
					str[k++] = '\0';
					if ((av[j] = (char *)malloc(sizeof(char) * (k + 1))) == NULL) {
						fprintf(stderr, "Can't allocate memory!\n");
						return -1;
					}
					strcpy(av[j++], str);
					flag = 0;
					k = 0;
					continue;
				}
			}
			if (buf[i] != ' ' &&  buf[i] != '\n'){
				str[k++] = buf[i];
				flag = 1;
			}
		}
		ac = j;
		
		for (t = cmd_tbl; t->cmd; t++) {
			if (strcmp(av[0], t->cmd) == 0) {
				(*t->func)(ac, av);
				break;
			}
		}
		
		if (t->cmd == NULL) {
			fprintf(stderr, "command not found\n");
		
		}
		for (i = 0; i < ac; i++)
			free(av[i]);
    
	}
	
	return 0;
	
}



