


#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>


#define PORT 10
#define BUFSIZE 128
#define HOSTLEN 256
#define DATASIZE 1024
#define PKTSIZE 1028
#define TMPSIZE 128

//type定義
#define QUIT 0x01
#define PWD 0x02
#define CWD 0x03
#define LIST 0x04
#define RETR 0x05
#define STOR 0x06
#define OK 0x10
#define CWD_ERR 0x11
#define FILE_ERR 0x012
#define UNKWN_ERR 0x13
#define DATA 0x20


void quit_proc(int, char *[], int);
void pwd_proc(int, char *[], int);
void cd_proc(int, char *[], int);
void dir_proc(int, char *[], int);
void lpwd_proc(int, char *[], int);
void lcd_proc(int, char *[], int);
void ldir_proc(int, char *[], int);
void get_proc(int, char *[], int);
void put_proc(int, char *[], int);
void help_proc(int, char *[], int);


struct command_table {
	char *cmd;
	void (*func)(int, char *[], int);
}cmd_tbl[] = {
	{"quit", quit_proc},
	{"pwd", pwd_proc},
	{"cd", cd_proc},
	{"dir", dir_proc},
	{"lpwd", lpwd_proc},
	{"lcd", lcd_proc},
	{"ldir", ldir_proc},
	{"get", get_proc},
	{"put", put_proc},
	{"help", help_proc},
	{NULL, NULL}
};
/*

struct myftph_data {
	uint8_t type;
	uint8_t code;
	uint16_t length; 
	char data[DATASIZE];
};


struct myftph {
	uint8_t type;
	uint8_t code;
	uint16_t length;
};

struct myftph_data ftph_d;
struct myftph ftph;
*/

struct myftph {
	uint8_t type;
	uint8_t code;
	uint16_t length;
};

char pkt_data[sizeof(struct myftph) + DATASIZE];
char pkt[sizeof(struct myftph)];

int sig_flag;

int main(int argc, char *argv[]) {

	struct addrinfo hints;
	struct addrinfo *res;
	struct command_table *t;
	   
	int err, sd;
	char node[HOSTLEN], serv[PORT] = "50021";
	struct sockaddr_in skt;
	int count;
	char str[BUFSIZE], *av[16];
	int ac, i, j = 0, k = 0, flag = 0, m;

	char *dammy[1];


	if (argc != 2) {
		fprintf(stderr, "Usage: ./myftpd hostname\n");
		exit(1);
	}


	strcpy(node, argv[1]);
	
	memset(&hints, 0, sizeof hints);
	//hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((err = getaddrinfo(node, serv, &hints, &res)) < 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		exit(1);
	}

	if ((sd = socket(res->ai_family, res->ai_socktype,
					 res->ai_protocol)) < 0) {
		perror("socket");
		exit(1);				  
	}

	if (connect(sd, res->ai_addr, res->ai_addrlen) < 0) {
		perror("connect");
		exit(1);
	}

	freeaddrinfo(res);
	
	printf("success connect!\n");
	while (1) {
				
		j = 0;
		k = 0;
		flag = 0;
		char buf[BUFSIZE];
		//入力解析
		printf("myftp$: ");
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
			if (buf[i] != ' ' && buf[i] != '\n') {
				str[k++] = buf[i];
				flag = 1;		
			}
		}
		ac = j;

		for (t = cmd_tbl; t->cmd; t++) {
			if (strcmp(av[0], t->cmd) == 0) {
				(*t->func)(ac, av, sd);
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





void quit_proc(int ac, char *av[], int sd)
{
	int count, len;
	struct myftph *ftph = (struct myftph *)pkt;

	ftph->type = QUIT;
	ftph->length = htons(0x00);

	if ((count = send(sd, &pkt, sizeof pkt, 0)) < 0) {
		perror("send");
		exit(1);
	}

	if ((count = recv(sd, &pkt, sizeof pkt, 0)) < 0) {
		perror("recv");
		exit(1);		
	}

	ftph = (struct myftph *)pkt;
	if (ftph->type == OK) {
		printf("type %d(OK)\n", ftph->type);
		exit(0);
	}

	
}

void pwd_proc(int ac, char *av[], int sd)
{
	
	int count, len;
	struct myftph *ftph = (struct myftph *)pkt;
	char *data;
 
	ftph->type = PWD;
	ftph->length = htons(0x00);
			
	if ((count = send(sd, &pkt, sizeof pkt, 0)) < 0) {
		perror("send");
		exit(1);
	}
	
	if ((count = recv(sd, &pkt_data, sizeof pkt_data, 0)) < 0) {
		perror("recv");
		exit(1);
	}
	
	ftph = (struct myftph *)pkt_data;
	data = pkt_data + sizeof(struct myftph);

		
	if (ftph->type == OK) {
		data[ntohs(ftph->length)] = '\0';

		printf("Server: %s\n", data);
	}
	else {
		printf("error recv: type->0x%02x code->0x%02x\n", ftph->type, ftph->code);
	}
}

void cd_proc(int ac, char *av[], int sd)
{
 
	int count, len;

	
	if (ac == 1) { //ホームディレクとに移動したいことをサーバーに送信 dataは空
		struct myftph *ftph = (struct myftph *)pkt;
		ftph->type = CWD; //dataは空よりstructはヘッダだけ
		len = 0;
		ftph->length = htons(len); //ネットバイトオーダーに変換
		if ((count = send(sd, &pkt, sizeof pkt, 0)) < 0) {
			perror("send");
			exit(1);
		}
		
		if ((count = recv(sd, &pkt, sizeof pkt, 0)) < 0) {
			perror("recv");
			exit(1);
		}

		ftph = (struct myftph *)pkt;

		if (ftph->type == OK && ftph->code == 0x00) {
			printf("change directry!\n");
		}
		else if (ftph->type == CWD_ERR) {
			printf("error recv: type->0x%02x code->0x%02x\n", ftph->type, ftph->code);
		}
	}

	else if (ac == 2) {
		struct myftph *ftph = (struct myftph *)pkt_data;
		char *data = pkt_data + sizeof(struct myftph);


		//引数が指定されているためdataありのstruct dhcph_data
		
		ftph->type = CWD;
		//av[1]に移動したいディレクトリが文字列で格納されている
		len = strlen(av[1]);
		ftph->length = htons(len); //ネットバイトオーダーに変換
		strcpy(data, av[1]);

		//DEBUG
		//printf("D->data: %s\n", data);
		//printf("D->length: %d\n", len);
		//DEBUG終了
		
		if ((count = send(sd, &pkt_data, len + sizeof(pkt), 0)) < 0) {
			perror("send");
			exit(1);
		}

		ftph = (struct myftph *)pkt;
		if ((count = recv(sd, &pkt, sizeof pkt, 0)) < 0) {
			perror("recv");
			exit(1);
		}

		if (ftph->type == OK && ftph->code == 0x00) {
			printf("change directry!\n");
		}
		else {
			printf("error recv: type->0x%02x code->0x%02x\n", ftph->type, ftph->code);
		}
	}

	else { //引数が正しくないためサーバーには何も送信しない
		fprintf(stderr, "Usage: cmd directry\n");
	}
	
	
}

void dir_proc(int ac, char *av[], int sd)
{
	int count, len;
	char *data;
	struct myftph *ftph;
	char buf[DATASIZE];

	if (ac < 3) {
		if (ac == 1) {
			struct myftph *ftph = (struct myftph *)pkt;
			ftph->length = htons(0);
			ftph->type = LIST;
			if ((count = send(sd, &pkt, sizeof(pkt), 0)) < 0) {
				perror("send");
				exit(1);
			}
			
		}
		
		else if (ac == 2) {
			
			ftph = (struct myftph *)pkt_data;
			data = pkt_data + sizeof(struct myftph);
			
			//引数が指定されているためdataありのstruct dhcph_data
			
			ftph->type = LIST;
			//av[1]に取得したいディレクトリが文字列で格納されている
			len = strlen(av[1]);
			ftph->length = htons(len); //ネットバイトオーダーに変換
			strcpy(data, av[1]);
			
			//DEBUG
			
			//printf("D->data: %s\n", data);
			//printf("D->length: %d\n", len);
			//DEBUG終了
			if ((count = send(sd, &pkt_data, len + sizeof(pkt), 0)) < 0) {
				perror("send");
				exit(1);
			}
		}


		ftph = (struct myftph *)pkt;
		if ((count = recv(sd, &pkt, sizeof pkt, 0)) < 0) {
			perror("recv");
			exit(1);
		}

		if (ftph->type == OK && ftph->code == 0x01) {

			while(1) {
				
				//char *data = pkt_data + sizeof(struct myftph);
				//len = ntohs(ftph->length);
				if ((count = recv(sd, &pkt, sizeof pkt, 0)) < 0) {
					perror("recv");
					exit(1);
				}
								
				if (ftph->type == DATA) {

					len = ntohs(ftph->length);
					if ((count = recv(sd, buf, len, 0)) < 0) {
						perror("recv");
						exit(1);
					}					

					buf[len] = '\0';					
					printf("%s", buf);
					if (ftph->code == 0x00) {
						break;
					}
					
				}
				else {
					printf("error recv: type->0x%02x code->0x%02x\n", ftph->type, ftph->code);
					break;
				}

				memset(buf, '\0', sizeof(buf));
			}
			
		}else {
			printf("error recv: type->0x%02x code->0x%02x\n", ftph->type, ftph->code);
		}
	}
	
	else { //引数が正しくないためサーバーには何も送信しない
			fprintf(stderr, "Usage: cmd directry\n");
	}
	printf("\n");

	return ;	
}

void lpwd_proc(int ac, char *av[], int sd)
{
	char str[DATASIZE];
	FILE *fp;
	fp = popen("pwd", "r");
	while (fgets(str, DATASIZE, fp) != NULL);
	pclose(fp);

	printf("Client: %s", str);
		
}

void lcd_proc(int ac, char *av[], int sd)
{

	if (ac == 1) { //ホームディレクトリに移動
		chdir(getenv("HOME"));
	}
	else if (ac == 2) { //引数で指定されたディレクトリに移動
		if (chdir(av[1]) < 0) {
			fprintf(stderr, "Can't found this directory!\n");
		}
		else {
			printf("change dirctory!\n");
		}
	}
	else {
		fprintf(stderr, "Usage: lcd change_directy\n");
	}
}

void ldir_proc(int ac, char *av[], int sd)
{

	char pathname[TMPSIZE], file[TMPSIZE], tmp[TMPSIZE];
	FILE *fp;
	struct dirent *dp;
	struct stat st;
	char ls[DATASIZE];
	int flag = 0;
	DIR *dir;
	int len;
	
	if (ac == 1) {
		//カレントディレクトリの情報取得
		pathname[0] = '.';
		pathname[1] = '\0';
	}
	else if (ac == 2) {
		//argv[2]で指定されたディレクトリの情報を抜き出す
		
	}
	//DEBUG
	//printf("pathname: %s\n", pathname);
	//DEBUG終了

	if (stat(pathname, &st) < 0) {
		perror("stat");
		return ;
	}

	if (S_ISDIR(st.st_mode)) { //ディレクトリの場合
		//そのディレクトリをオープン
		if ((dir = opendir(pathname)) == NULL) {
			perror("opendir");
			exit(1);
		}
		
		
		for (dp=readdir(dir); dp != NULL; dp=readdir(dir)) {
			
			strcpy(file, pathname);
			strcat(file, "/");
			
			strcat(file, dp->d_name);
			
			//DEBUG用表示
			//printf("file: %s\n", file);
			//DEBUG終了
			
			if (stat(file, &st) < 0) {
				perror("stat2");
				
			}
			/*
			if (S_ISDIR(st.st_mode)) {
				continue;
			}
			*/
			else {
				sprintf(tmp, "%ld", st.st_size);
		
				strcpy(ls, tmp);
				strcat(ls , "\t");
				strcat(ls, asctime(localtime(&st.st_mtime)));
				len = strlen(ls);
				ls[len - 1] = '\0';
				strcat(ls, "\t");
				strcat(ls, dp->d_name);
				strcat(ls, "\n");
				len = strlen(ls);
				ls[len] = '\0';
				printf("%s", ls);
			}
		}
		
		
	}
		

}


void get_proc(int ac, char *av[], int sd)
{
	//サーバーからクライアント側にファイルを転送する

	int count, len, code;
	struct myftph *ftph = (struct myftph *)pkt_data;

	char *data = pkt_data + sizeof(struct myftph);
	char file_name[TMPSIZE];
	FILE *fp;
	long fsize;
	int ret;

	if (ac >= 2 && ac <= 3) { //正しい引数
		//まずはサーバーに欲しいファイルを知らせる
		len = strlen(av[1]);		
		ftph->type = RETR;
		ftph->length = htons(len);
		strcpy(data, av[1]);

		if ((count = send(sd, &pkt_data, len + sizeof(pkt), 0)) < 0) {
			perror("send");
			exit(1);
		}

		//そのファイルがあるか、あった場合はアクセス可能だったのかを
		//確認するための応答メッセージを受信する

		ftph = (struct myftph *)pkt;
		if ((count = recv(sd, &pkt, sizeof pkt, 0)) < 0) {
			perror("recv");
			exit(1);
		}

		if (ftph->type == FILE_ERR) { //エラーが帰った来た
			printf("error recv: type->0x%02x code->0x%02x\n", ftph->type, ftph->code);
		}

		else if (ftph->type == UNKWN_ERR) {
			printf("error recv: type->0x%02x code->0x%02x\n", ftph->type, ftph->code);
		}
		else if (ftph->type == OK && ftph->code == 0x01){ //エラーがなかった場合
			//まずは保存するファイルをクライアント側に作成する

			//DEBUG 
			//printf("request OK\n");
			//DEBUG終了
			
			if (ac == 2) {
				fp = fopen(av[1], "wb");
				strcpy(file_name, av[1]);				
			}
			else {
				fp = fopen(av[2], "wb");
				strcpy(file_name, av[2]);
			}

			if (fp == NULL) {
				fprintf(stderr, "can't create file\n");
				return ;
			}
			

			do {
				ftph = (struct myftph *)pkt;
				if ((count = recv(sd, &pkt, sizeof pkt, 0)) < 0){
					
					perror("recv");
					exit(1);
				}
				
				if (ftph->type == DATA) {
					//データが送られてきたときの処理
					code = ftph->code;
					len = ntohs(ftph->length);
					
					//DEBUG
					//printf("recv data size %d\n", len);
					//DEBUGm終了

					ftph = (struct myftph *)pkt_data;
					data = pkt_data + sizeof(struct myftph);

					if ((count = recv(sd, data, len, 0)) < 0) {
						perror("recv");
					}

					if (code == 0x00) {
						ret = fwrite(data, sizeof(char), len, fp);
						fclose(fp);
						break;
					}

					else if (code == 0x01) {
						ret = fwrite(data, sizeof(char), len, fp);
						fclose(fp);
						fp = fopen(file_name, "ab");
					}										
				}
				
			}while(1);
			printf("Success recv data!\n");
			
		}
		
	}

	else { //引数に謝りがある
		fprintf(stderr, "Usage: get <path1> <path2>\n");
		
	}
	
	
}

void put_proc(int ac, char *av[], int sd)
{
	//クライアントからサーバーにデータを転送する
	int count, len;
	struct myftph *ftph = (struct myftph *)pkt_data;
	char *data = pkt_data + sizeof(struct myftph);
	FILE *fp;
	long fsize;
	int ret;
	
	if (ac >=2 && ac <= 3) { //正しい引数

		//argv[1]で示したファイルをオープンして
		//カレントディレクトリにあるか確かめる

		//まずファイルをバイナリーモードでオープンする
		fp = fopen(av[1], "rb"); 

		if (fp == NULL) {
			fprintf(stderr, "Can't open file %s\n", av[1]);
			return;
		}

		fseek( fp, 0, SEEK_END );
		fsize = ftell( fp ); //ファイルサイズ取得
		fclose(fp);

		
		if (ac == 2) {
			strcpy(data, av[1]); //サーバーに作られるファイル名
		}
		else if (ac == 3) {
			strcpy(data, av[2]); //引数で指定した名前で作成する
			//DEBUG
			//printf("av[2]: %s\n", data);
			//DEBUG終了
		}
		
		ftph->type = STOR;
		len = strlen(data);
		ftph->length = htons(len); //ネットバイトオーダーに変換

		//まずサーバーに作るファイルの名前を送り
		//ファイルが作成できるかどうかのリプライメッセーzジを受諾
		if ((count = send(sd, &pkt_data, len + sizeof(pkt), 0)) < 0) {
			perror("send");
			exit(1);
		}
		
		if ((count = recv(sd, &pkt, sizeof pkt, 0)) < 0) {
			perror("recv");
			exit(1);
		}
		ftph = (struct myftph *)pkt;  

		//リプライメーセージでファイルアクセス権限確認
		if (ftph->type == FILE_ERR) { //エラーリプライ受諾
			if (ftph->code == 0x01) { //ファイルのアクセス権限がない
				printf("error recv: type->0x%02x code->0x%02x\n", ftph->type, ftph->code);
			}
		}
		else if (ftph->type == UNKWN_ERR) {
			printf("error recv: type->0x%02x code->0x%02x\n", ftph->type, ftph->code);
		}
		else if (ftph->type == OK && ftph->code == 0x02) { //OKリプライ受諾

			//DEBUG
			//			printf("reply recv type %d code %d\n", ftph->type, ftph->code);
			ftph = (struct myftph *)pkt_data;
			data = pkt_data + sizeof(struct myftph);

			//関数の先頭で開いたファイルをfsize分バイナリーモードで
			//dataに格納する

			//１バイトブロックごとに計fsize分読み取る

			fp = fopen(av[1], "rb"); 
			
			if (fp == NULL) {
				fprintf(stderr, "Can't open file %s\n", av[1]);
				return;
			}
			
			printf("fsize: %d sent!\n", fsize);


			if (fsize <= DATASIZE) { //データサイズが1024以下の場合は一回のsendでオッケー
				ftph->code = 0x00;
				
				ret = fread(data, sizeof(char), fsize, fp);
				printf("strlen %d\n", strlen(data));
														

				if (ret != fsize) {
					fprintf(stderr, "read error\n");
					return ;
				}
				ftph->type = DATA;
				ftph->length = htons(fsize);

				//DEBUG
				//printf("send data: \n%s\n", data);
				//DEBUG終了
				
				//データを送る
				if ((count = send(sd, &pkt_data, fsize + sizeof(pkt), 0)) < 0) {
					perror("send");
					exit(1);
				}

				//DEBUG
				//printf("send data!\n");
				//DEBUG終了
				
				fclose(fp);
			}
			else { //データサイズが1024より大きい場合はsendを分割
				int n_size, code;
				while (fsize > 0) {
					if (DATASIZE < fsize) {

						ftph->code = 0x01;
						n_size = DATASIZE;
					}
					else  {
						ftph->code = 0x00;
						n_size = fsize;
					}

					//DEGUB
					//printf("n_size %d\n", n_size);
					ret = fread(data, sizeof(char), n_size, fp);
					//printf("strlen %d\n", strlen(data));

					//DEBUG終了
				
					if (ret != n_size) {
						fprintf(stderr, "read error\n");
						return ;
					}
					ftph->type = DATA;
					ftph->length = htons(n_size);

					if ((count = send(sd, &pkt_data, n_size + sizeof(pkt), 0)) < 0) {
						perror("send");
						exit(1);
					}
					
					//printf("send data!\n");
					fsize -= n_size;
					
				}
				fclose(fp);								
			}
			printf("Success send data!\n");
		}
				

		
	}
	else {

		fprintf(stderr, "Usage: put <path1> <path2>\n");
		
	}
	
}

void help_proc(int ac, char *av[], int sd)
{
	if (ac > 1) {
		fprintf(stderr, "Usage: help [no option]\n");		
	}
	else {
		printf("-----------------------------------------------\n");
		printf("\x1b[1m");
		printf("help\n\n");
		printf("\x1b[0m");
		printf("To display ths syntax and function of each command\n");
		printf("-----------------------------------------------\n");
		printf("\x1b[1m");
		printf("quit\n\n");
		printf("\x1b[0m");
		printf("To exit this software\n");
		printf("-----------------------------------------------\n");
		printf("\x1b[1m");
		printf("pwd\n\n");
		printf("\x1b[0m");
		printf("To display current directory on remote server\n");
		printf("-----------------------------------------------\n");
		printf("\x1b[1m");
		printf("cd [directory]\n\n");
		printf("\x1b[0m");
		printf("To change directory on remote server\n");
		printf("-----------------------------------------------\n");
		printf("\x1b[1m");
		printf("dir [directory or file]\n\n");
		printf("\x1b[0m");
		printf("To display detail of file on remote server\n");
		printf("-----------------------------------------------\n");
		printf("\x1b[1m");
		printf("lpwd\n\n");
		printf("\x1b[0m");
		printf("To display current directory on local client\n");
		printf("-----------------------------------------------\n");
		printf("\x1b[1m");
		printf("lcd [directory]\n\n");
		printf("\x1b[0m");
		printf("To change directory on remote server\n");
		printf("-----------------------------------------------\n");
		printf("\x1b[1m");
		printf("ldir [directory or file]\n\n");
		printf("\x1b[0m");
		printf("To display detail of file on local client\n");
		printf("-----------------------------------------------\n");
		printf("\x1b[1m");
		printf("get filename on server[rename file]\n\n");
		printf("\x1b[0m");
		printf("To copy file from remote server to local client\n");
		printf("-----------------------------------------------\n");
		printf("\x1b[1m");
		printf("put filename on client[rename file]\n\n");
		printf("\x1b[0m");
		printf("To copy file from local client tp remote server\n");
	}
}
