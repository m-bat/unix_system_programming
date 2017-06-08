

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>

#define TKN_NORMAL    1
#define TKN_REDIR_IN  2 
#define TKN_REDIR_OUT 3
#define TKN_PIPE      4
#define TKN_BG        5
#define TKN_EOL       6
#define TKN_EOF       7
#define TKN_ERR       8

#define BUF 20
#define MAX 32
#define NUM 2

int getargs(int *ac, char *av[]);
int gettoken(char *token, int len);
void free_av(int ac, char *av[]);
void free_pdf(int num, int *pdf[]);
void free_env(char num, char *env[]);
void sigmsg();
void sigmsg_BG();


int bg_count = 1;
int shell_pid = 0;
int main(int argc, char *argv[], char *envp[])
{


	int c, fd_in, fd_out, stat[BUF], pipe_r = 0;

	int count = 0, result, i= 0, j = 0,k = 0, l = 0, token[MAX], array[10];
	int ac = 0, pid, status, pre_pipe = 0, nex_pipe = 0;
	char *av[MAX];
	int *pdf[MAX];
	char *buf[MAX];
	char *env[MAX];
	char buf_exe[256];
	int redir_out[10], redir_in[10], redir = 0, num = 0;
	int redir_pipe = 0, out = 0, bg = 0, tmp = 0;
	int fd = open("/dev/tty", O_RDWR);
	int pipe_pgid;
	int flag = 0;
	int env_num;
	char *environ[] ={NULL};

	char * path;
	char *strenv;
	

    	 

	if ((strenv = getenv("PATH")) == NULL) {
		fprintf(stderr, "Can't find PATH\n");
	}
	
	
	for (i = 0; i < MAX; i++) {
		
		if ((env[i] = (char *)malloc(sizeof(char) * 256)) == NULL) {
			fprintf(stderr, "Can't allocate memory!\n");
			return;
		}
		
	}
	
	for (i = 0; strenv[i] != '\0'; i++) {
		if (flag == 0) {
			if (strenv[i] != '/')
				continue;
			else
				flag = 1;
		}
		
		if (strenv[i] != ':') {
				env[j][k++] = strenv[i];
		}else{
			env[j][k++] = '/';
			env[j++][k] = '\0';
			k = 0;
		}
	}
	env[j][k++] = '/';
	env[j][k] = '\0';
	

	
	env_num = j;
	j = 0;
	k = 0;
	

	
	do {
		struct sigaction *sig;
		sig = (struct sigaction *)malloc(sizeof(struct sigaction));

				

		bg = 0;
		out = 0;
		redir_pipe = 0;
		l = 0;
		num = 0;
		pipe_r = 0;
		redir = 0;
		k = 0; j = 0;
		ac = 0;
		for (i = 0; i < 10; i ++) {
			redir_out[i] = 0;
			redir_in[i] = 0;
		}

		
			sig->sa_handler = SIG_IGN;
			if (sigaction(SIGINT, sig, NULL) == -1)
			perror("sigaction");

			
			sig->sa_handler = SIG_IGN;
			if (sigaction(SIGTTIN, sig, NULL) == -1)
				perror("sigaction");
			
			sig->sa_handler = SIG_IGN;
			if (sigaction(SIGTTOU, sig, NULL) == -1)
				perror("sigaction");
			
			sig->sa_handler = SIG_DFL;
			if (sigaction(SIGCHLD, sig, NULL) == -1)
				perror("sigaction");
		


		pre_pipe = 0, nex_pipe = 0;
		printf("mysh $ ");



		while ((result = getargs(&ac, av)) != TKN_EOL) {

			
			if (result == TKN_BG) {
				av[ac - 1] = NULL;
				bg = 1;
			
			}
			
			if (result == TKN_REDIR_IN) {
				redir = 1;
				out = 1;
				redir_in[k++] = ac;

				av[ac - 1] = NULL;
			}
			if (result == TKN_REDIR_OUT) {

				if (pre_pipe == 1) {
					redir_pipe = 1;
				}
					redir = 1;
					redir_out[j++] = ac;
					av[ac - 1] = NULL;
			}
			if (result == TKN_PIPE) {
				pipe_r = 1;
				av[ac - 1] = NULL;


				if (pre_pipe == 0) {
					
					if ((pdf[num] = (int *)malloc(sizeof(int) * NUM)) == NULL) {
						fprintf(stderr, "Can't allocate memory!\n");
						return;
					}
					
					pipe(pdf[num]);
					if ((pid = fork()) == 0) {

						if (setpgid(pid, 0) == -1) {
							error("setpgid");
							exit(1);
						}
						pipe_pgid = getpgrp();
						sig->sa_handler = SIG_DFL;
						if (sigaction(SIGINT, sig, NULL) == -1)
							perror("sigaction");
						
						sig->sa_handler = SIG_DFL;
						if (sigaction(SIGTTIN, sig, NULL) == -1)
							perror("sigaction");
						
						sig->sa_handler = SIG_DFL;
						if (sigaction(SIGTTOU, sig, NULL) == -1)
							perror("sigaction");
						
						sig->sa_handler = SIG_DFL;
						if (sigaction(SIGCHLD, sig, NULL) == -1)
							perror("sigaction");
		
						
						if (out == 1) {

							for (i = 0; i < 10; i++) {

								if (redir_in[i] != 0) {
									fd_in = open(av[redir_in[i]], O_RDONLY);
									close(0);
									dup(fd_in);
									close(fd_in);
								}
							}
							
						}
							
						close(1);
						dup(pdf[num][1]);
						close(pdf[num][0]);
						close(pdf[num][1]);
				
						for (i = 0; i <= env_num; i++) {
					strcpy(buf_exe, env[i]);
					strcat(buf_exe, av[0]);
					execve(buf_exe, av, envp);
					memset(buf_exe, '\0', 256);
				}
						if (i == env_num + 1) {
							fprintf(stderr, "-bash: %s: コマンドが見つかりません\n", av[0]);
							exit(1);
						}
			
					
						}
						else if (pid >= 1) {
						wait(&stat[l++]);
						pre_pipe = 1;
						free_av(ac, av);
			
						ac = 0;
						num++;
				
						continue;
					}

				}

				else if (pre_pipe == 1) {
					if ((pdf[num] = (int *)malloc(sizeof(int) * NUM)) == NULL) {
						fprintf(stderr, "Can't allocate memory!\n");
						return;
					}

					pipe(pdf[num]);
					if ((pid = fork()) == 0) {
						if (setpgid(pid, pipe_pgid) == -1) {
							error("setpgid");
							exit(1);
						}
					
						sig->sa_handler = SIG_DFL;
						if (sigaction(SIGINT, sig, NULL) == -1)
							perror("sigaction");
						
						sig->sa_handler = SIG_DFL;
						if (sigaction(SIGTTIN, sig, NULL) == -1)
							perror("sigaction");

						sig->sa_handler = SIG_DFL;
						if (sigaction(SIGTTOU, sig, NULL) == -1)
							perror("sigaction");
						
						sig->sa_handler = SIG_DFL;
						if (sigaction(SIGCHLD, sig, NULL) == -1)
							perror("sigaction");
						
						close(0);
						dup(pdf[num - 1][0]);
						close(pdf[num - 1][0]);
						close(pdf[num - 1][1]);

						close(1);
						dup(pdf[num][1]);
						close(pdf[num][0]);
						close(pdf[num][1]);

			  
						for (i = 0; i <= env_num; i++) {
					strcpy(buf_exe, env[i]);
					strcat(buf_exe, av[0]);
					execve(buf_exe, av, envp);
					memset(buf_exe, '\0', 256);

					if (i == env_num + 1) {
					fprintf(stderr, "-bash: %s: コマンドが見つかりません\n", av[0]);
					exit(1);
				}
				}
						
						
					}
					
					else if (pid >= 1) {
						close(pdf[num - 1][0]);
						close(pdf[num - 1][1]);
						
						wait(&stat[l++]);
						
						free_av(ac, av);
						ac = 0;
						num++;

						continue;
						
					}
				}				
				
			}
			
		}


		

		if (ac == 1 && result == TKN_EOL) {
			free_av(ac , av);
			ac = 0;
		
			continue;
		}
		if (strcmp(av[0], "exit") == 0) {
			exit(0);
		}
			


		if (pipe_r == 0) {
		
			av[ac-1] = NULL;

			if (strcmp(av[0], "cd") == 0) {
				if (ac == 2 ) {
					chdir(getenv("HOME"));
					continue;
				}
				
				else  if (chdir(av[1]) < 0){				  
					fprintf(stderr, "-bash: cd: %s; そのようなファイルやディレクトリはありません\n", av[1]);
					continue;
				}
				
				free_av(ac, av);
				ac = 0;
				continue;
			}


			if ((pid = fork()) == 0 ) {

				sig->sa_handler = SIG_DFL;
				if (sigaction(SIGINT, sig, NULL) == -1)
					perror("sigaction");

			   	sig->sa_handler = SIG_DFL;
				if (sigaction(SIGTTIN, sig, NULL) == -1)
					perror("sigaction");

				sig->sa_handler = SIG_DFL;
				if (sigaction(SIGTTOU, sig, NULL) == -1)
					perror("sigaction");

				sig->sa_handler = SIG_DFL;
				if (sigaction(SIGCHLD, sig, NULL) == -1)
					perror("sigaction");
			   
   
		
				

				
				if (setpgid(pid, 0) == -1) {
					error("setpgid");
					exit(1);
					}

				
				for (i = 0; i < 10; i++) {
					if (redir_out[i] != 0) {
						redir = 1;
					
					
						fd_out = open(av[redir_out[i]], O_WRONLY|O_CREAT|O_TRUNC,
									  0644);
						close(1);
						dup(fd_out);
						close(fd_out);
					}
					if (redir_in[i] != 0) {
						fd_in = open(av[redir_in[i]], O_RDONLY);
						close(0);
						dup(fd_in);
						close(fd_in);
					}
				}
				
			

				for (i = 0; i <= env_num; i++) {
					strcpy(buf_exe, env[i]);
					strcat(buf_exe, av[0]);
					execve(buf_exe, av, envp);
					memset(buf_exe, '\0', 256);

			   }

				if (i == env_num + 1) {
					fprintf(stderr, "-bash: %s: コマンドが見つかりません\n", av[0]);
					exit(1);
				}

				
			}
			else if (pid >= 1) {			
				
				if (bg == 0) {
					if (tcsetpgrp(fd, pid) == -1) {
					perror("tcsetpgrp");
					exit(1);
					}
		
					do {
					if ((tmp = wait(&status)) < 0) {
						perror("wait");
						exit(1);
					}
				
					}while(pid != tmp);

					if (tcsetpgrp(fd, getpgrp()) == -1) {
					perror("tcsetpgrp");
					exit(1);
					}
					
				
			

				}
				else {

					if (tcsetpgrp(fd, tcgetpgrp(fd)) == -1) {
						perror("tcsetpgrp");
						exit(1);
					}
					printf("pid %d\n", pid);
					bg_count++;
					if (signal(SIGCHLD, (void *)sigmsg_BG) == SIG_ERR) {
					perror("signal");
					exit(1);
					}
			
					

				}				
			}
			
			free_av(ac , av);
			ac = 0;
			pre_pipe = 0;
		}

				
		 if (pre_pipe == 1) {
			av[ac - 1] = NULL;
			if ((pid = fork()) == 0) {
			
				if (setpgid(pid, pipe_pgid) == -1) {
					error("setpgid");
					exit(1);
				}
					
				sig->sa_handler = SIG_DFL;
				if (sigaction(SIGINT, sig, NULL) == -1)
					perror("sigaction");

				sig->sa_handler = SIG_DFL;
				if (sigaction(SIGTTIN, sig, NULL) == -1)
					perror("sigaction");

				sig->sa_handler = SIG_DFL;
				if (sigaction(SIGTTOU, sig, NULL) == -1)
					perror("sigaction");

				sig->sa_handler = SIG_DFL;
				if (sigaction(SIGCHLD, sig, NULL) == -1)
					perror("sigaction");
		
				close(0);
				dup(pdf[num - 1][0]);
				close(pdf[num - 1][0]);
				close(pdf[num - 1][1]);
				if (redir_pipe == 1) {

					for (i = 0; i < 10; i++) {
					if (redir_out[i] != 0) {
						redir = 1;
					
					
						fd_out = open(av[redir_out[i]], O_WRONLY|O_CREAT|O_TRUNC,
									  0644);
						close(1);
						dup(fd_out);
						close(fd_out);
					}
				}
				
				
				
				
				for (i = 0; i <= env_num; i++) {
					strcpy(buf_exe, env[i]);
					strcat(buf_exe, av[0]);
					execve(buf_exe, av, envp);
					memset(buf_exe, '\0', 256);
				}
				if (i == env_num + 1) {
					fprintf(stderr, "-bash: %s: コマンドが見つかりません\n", av[0]);
					exit(1);
				}
					

				}
				else{
				
				for (i = 0; i <= env_num; i++) {
					strcpy(buf_exe, env[i]);
					strcat(buf_exe, av[0]);
					execve(buf_exe, av, envp);
					memset(buf_exe, '\0', 256);
				}
				if (i == env_num + 1) {
					fprintf(stderr, "-bash: %s: コマンドが見つかりません\n", av[0]);
					exit(1);
				}
					
				}

			}
				
			else if (pid >= 1) {				
			
	  			close(pdf[num - 1][0]);
				close(pdf[num - 1][1]);						 
				wait(&stat[l++]);

			
				num++;

				if (redir_pipe == 0) 
					free_av(ac, av);
				ac = 0;
		

			}
			
		 }

		
		
	}while (1);

	free_env(env_num + 1, env);
	return 0;
}

void sigmsg_BG() {
	int status;

	bg_count--;
	wait(&status);
}


void sigmsg() {

	


	printf("sigmsg %d\n", getpid());

	if (shell_pid != getpid()) {
		killpg(0, SIGKILL);
	}
	

}

int getargs(int *ac, char *av[])
{
	int i = 0;


	if ((av[*ac] = (char *)malloc(sizeof(char) * MAX)) == NULL) {
					fprintf(stderr, "Can't allocate memory!\n");
					return;
	}

	i = *ac;
	*ac = *ac + 1;
	return gettoken(av[i], MAX);
				
}


void free_av(int ac, char *av[]) {
	int i = 0;
	for (i = 0; i < ac; i++)
		free(av[i]);
			
}

void free_pdf(int num, int *pdf[]) {
	int i = 0;
	for (i = 0; i < num; i++)
		free(pdf[i]);
			
}

void free_env(char num, char *env[]) {
	int i = 0;
	for (i = 0; i <= num; i++)
		free(env);
}


int gettoken(char *token, int len) {

	int c;
	int i = 0, state = 0, flag = 0;
	while (1) {
		c = getc(stdin);
		if (i == 0) {
			if (isalnum(c) != 0 || c == '-' || c == '.') {
				flag = 1;
			}
			else if (c == ' ' || c == '\t') {
				continue;
			}
		}
		if (i == 0 && flag == 0) {
			token[i++] = c;
			token[i] = '\0';
			switch (c) {
			case '<':
				return TKN_REDIR_IN;
			case '>':
				
				return TKN_REDIR_OUT;
			case '|':
				return TKN_PIPE;
			case '&':
				return TKN_BG;
			case '\n':
				return TKN_EOL;
			}
		}

		if (i > 0) {
			if (c == '|' || c == '&' || c == '>' || c == '<' || c == '\t' || c == ' ' || c == '\n') {
				token[i] = '\0';
				ungetc(c, stdin);
				return TKN_NORMAL;
			}
		}

		token[i++] = c;
	}
}



	
	

	
			
