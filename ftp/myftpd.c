


#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>


#define PORT 10
#define BUFLEN 1500
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
*/

struct myftph {
	uint8_t type;
	uint8_t code;
	uint16_t length;	
};

int proc(int s2);

int main(int argc, char *argv[])
{
	int s, s2, count;
	struct sockaddr_in myskt;
	struct sockaddr_in skt;
	//	socklen_t sktlen;
	char str[PORT] = "50021";
	struct addrinfo hints, *res;
	struct sockaddr_storage sin;
	char *serv;
	int sd, sd1, err, sktlen;
	int status;
	


	if (argc == 2) {
		if (chdir(argv[1]) < 0) {
			perror("chdir");
			exit(1);
		}
	}

	printf("connect wait...\n");
	
	memset(&hints, 0, sizeof hints);

	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;

	if ((err = getaddrinfo(NULL, str, &hints, &res)) < 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		exit(1);
	}

	//res->ai_family
	if ((s = socket(res->ai_family, res->ai_socktype,
					 res->ai_protocol)) < 0) {
		perror("socket");
		exit(1);
	}


	if (bind(s, res->ai_addr, res->ai_addrlen) < 0) {

		perror("bind");
		exit(1);
	}
	
	freeaddrinfo(res);



	//	freeaddrinfo(res);		 		  

	/*	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}

	memset(&myskt, 0, sizeof(myskt));
	myskt.sin_family = AF_INET;
	myskt.sin_port = htons(myport);
	myskt.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(s, (struct sockaddr *)&myskt, sizeof myskt) < 0) {
		perror("bind");
	  	exit(1);
	}
	*/

	if (listen(s, 5) < 0) {
		perror("listen");
		exit(1);
	}
	
	while (1) {

		int pid;
		sktlen = sizeof (struct sockaddr_storage);
		if ((s2 = accept(s, (struct sockaddr *)&sin, &sktlen)) < 0) {
			perror ("accept");
			exit(1);
		}
		
		printf("connetct request\n");
				


		if ((pid == fork()) == 0) { //子プロセスの生成


			while (1) {
				if (proc(s2) == QUIT)
					break;
			}
		}

		else {
			wait(&status);
			close(s2);			
		}

		
	}

	if (close(s) < 0) {
		perror("close");
		exit(1);
	}

	return 0;
}
   

int proc(int s2)
{
	int i, len, len_t;
	/*	struct myftph_data ftph_d;
	struct myftph ftph;
	*/
	//struct myftph_data *p;

	int count;
	int flag = 0, flag_t = 0;
	char buf[DATASIZE], *pp;
	FILE *fp;

	char pkt_data[sizeof(struct myftph) + DATASIZE];
	char pkt[sizeof(struct myftph)];
	struct myftph *ftph;
	char *data;


	char tmp[TMPSIZE], file[TMPSIZE];
	struct stat st;
	char pathname[TMPSIZE];
	DIR *dir;
	struct dirent *dp, *tdp;
	int ret;
	long fsize;
	int n_size;
	int code;
	char file_name[TMPSIZE];
	
	/*	struct myftph *ftph = (struct myftph *)pkt_data;
		char *data = pkt_data + sizeof(struct myftph);n*/
	
 
	//p = (struct myftph_data *)buf; //bufをヘッダデータ型にキャスト
	
	if ((count = recv(s2, pkt_data, sizeof(struct myftph), 0)) < 0) { //まずヘッダ部分の4byteだけ取り出す
		perror("recv");
		exit(1);
	}
	
	ftph = (struct myftph *)pkt_data;
	printf("recv head: type->0x%02x code->0x%02x\n", ftph->type, ftph->code);

	switch(ftph->type) {

	case QUIT:

		ftph = (struct myftph *)pkt;
		ftph->type = OK;
		ftph->code = 0x00;
		ftph->length = htons(0);
		if ((count = send(s2, pkt, sizeof pkt, 0)) < 0) {
			perror("sendto");
			exit(1);
		}

		printf("client exit\n");
		return QUIT;
		
	case PWD:
		ftph = (struct myftph *)pkt_data;
		data = pkt_data + sizeof(struct myftph);
		fp = popen("pwd", "r");
		while (fgets(data, DATASIZE, fp) != NULL);
		pclose(fp);


		ftph->length = (uint16_t)(strlen(data) - 1); //最後の改行文字をカウントしない　ネットバイトオーダーに変換
		for (i = 0; data[i] != '\0'; i++) {
			if (data[i] == '\n') {
				data[i] = '\0';
				break;
			}
		}
		
		ftph->length = htons(ftph->length);
		ftph->type = OK;
		ftph->code = 0x00;
		if ((count = send(s2, &pkt_data, sizeof pkt_data, 0)) < 0) {
			perror("sendto");
			exit(1);
		}
		printf("send: type->0x%02x code->0x%02x length->%d\n", ftph->type, ftph->code, ntohs(ftph->length));
		break;
		
	case CWD:
		len = ntohs(ftph->length);
		if (len == 0) {			
			//ホームディレクトリに移動
			ftph = (struct myftph *)pkt;
			chdir(getenv("HOME"));
			ftph->type = OK;
			ftph->code = 0x00;
			ftph->length = htons(0);
			
			if ((count = send(s2, pkt, sizeof pkt, 0)) < 0) {
				perror("sendto");
				exit(1);
			}
		printf("send: type->0x%02x code->0x%02x length->%d\n", ftph->type, ftph->code, ntohs(ftph->length));
		}

		else {
			//引数で指定したディレクトリに移動
			data = pkt_data + sizeof(struct myftph);
			
			if ((count = recv(s2, data, len, 0)) < 0) {
				perror("recv");
				exit(1);
			}
	
			data[len] = '\0';
			printf("data: %s\n", data);
			ftph = (struct myftph *)pkt;
			
			if (chdir(data) < 0) {
				//チェンジデェレクトリの際にエラーを検出
				perror("cwd");
				if (errno == ENOENT) {
					ftph->type = FILE_ERR;
					ftph->code = 0x00;					
				}
				else if (errno == EACCES) {
					ftph->type = FILE_ERR;
					ftph->code = 0x01;
				}
				else {
					ftph->type = UNKWN_ERR;
					ftph->code = 0x05;
				}
				ftph->length = htons(0);
				
				if ((count = send(s2, &pkt, sizeof pkt, 0)) < 0) {
					perror("sendto");
					exit(1);
				}
				printf("send: type->0x%02x code->0x%02x length->%d\n", ftph->type, ftph->code, ntohs(ftph->length));
			}
			else {
				ftph->type = OK;
				ftph->code = 0x00;
				ftph->length = htons(0);

				if ((count = send(s2, &pkt, sizeof pkt, 0)) < 0) {
					perror("sendto");
					exit(1);
				}
				printf("send: type->0x%02x code->0x%02x length->%d\n", ftph->type, ftph->code, ntohs(ftph->length));
						
			}
		}
		
		break;		

	case LIST:
		ftph = (struct myftph *)pkt_data;
		data = pkt_data + sizeof(struct myftph);		

		len = ntohs(ftph->length);

		if (len > 0) {
			if ((count = recv(s2, data, len, 0)) < 0) {
				perror("recv");
				exit(1);
			}
			data[len] = '\0';
			strcpy(pathname, data);

			printf("pathname: %s\n", pathname);
		}
		else {
			pathname[0] = '.';
			pathname[1] = '\0';
		}
		
		if (stat(pathname, &st) < 0) {
			//ファイルがない場合 エラーメッセージを送信する
			perror("stat1");
			ftph = (struct myftph *)pkt;
			ftph->type = FILE_ERR;
			ftph->code = 0x00;
			ftph->length = htons(0);

			if ((count = send(s2, &pkt, sizeof(pkt), 0)) < 0) {
				perror("send");
				exit(1);
			}

			printf("send: type->0x%02x code->0x%02x length->%d\n", ftph->type, ftph->code, ntohs(ftph->length));
			
			return ;
		}
		
		if (S_ISDIR(st.st_mode)) { //ディレクトリの場合
			//そのディレクトリをオープン
			if ((dir = opendir(pathname)) == NULL) {
				perror("opendir");
				//パーミッションでディレクトリを開けないときの処理
				ftph = (struct myftph *)pkt;
				ftph->type = FILE_ERR;
				ftph->code = 0x01;
				ftph->length = htons(0);

				if ((count = send(s2, &pkt, sizeof(pkt), 0)) < 0) {
					perror("send");
					exit(1);
				}

				printf("send: type->0x%02x code->0x%02x length->%d\n", ftph->type, ftph->code, ntohs(ftph->length));
				return ;						
			
			}
			else {
				ftph = (struct myftph *)pkt;
				ftph->type = OK;
				ftph->code = 0x01;
				ftph->length = htons(0);
				
				if ((count = send(s2, &pkt, sizeof(pkt), 0)) < 0) {
					perror("send");
					exit(1);
				}
				
				printf("send: type->0x%02x code->0x%02x length->%d\n", ftph->type, ftph->code, ntohs(ftph->length));
			
			}

			len_t = 0;
			dp=readdir(dir);
			ftph = (struct myftph *)pkt_data;
			data = pkt_data + sizeof(struct myftph);
			
			while (dp != NULL) {

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
				if (S_ISDIR(st.st_mode) && !S_ISREG(st.st_mode)) {
					dp=readdir(dir);
					continue;
				}
				*/
				else {
					sprintf(tmp, "%ld", st.st_size);
					
					//if (flag == 0) {
					strcpy(buf, tmp);
					//flag++;
					//}
					//else {
					//strcat(buf, tmp);
					//}
					
					strcat(buf , "\t");
					strcat(buf, asctime(localtime(&st.st_mtime)));
					len = strlen(buf);
					buf[len - 1] = '\0';
					strcat(buf, "\t");
					strcat(buf, dp->d_name);
					strcat(buf, "\n");
					len = strlen(buf);
					
					buf[len] = '\0';
			   
					//len = len - 1;
					len_t += len;

					if (len_t <= DATASIZE) {
						
						if (flag_t == 0) {
							strcpy(data, buf);
							flag_t++;
						}
						else {
							strcat(data, buf);
						}
						dp=readdir(dir);
					}
					else {
						len_t -= len;
						//とりあえず一回データを送信する
						ftph->length = htons(len_t);
						ftph->type = DATA;
						ftph->code = 0x01;
						if ((count = send(s2, &pkt_data, len_t + sizeof(pkt), 0)) < 0) {
							perror("sendto");
							exit(1);
						}

						printf("send: type->0x%02x code->0x%02x length->%d\n", ftph->type, ftph->code, ntohs(ftph->length));
						//DEBUG
						//printf("%s\n", data);
						//DEBUG
						//whileの先頭に戻り再度読み込む
						flag = 0;
						len = 0;
						len_t = 0;
						flag_t = 0;
						
					}
				}
			   
			}

			/*
			data[len] = '\0';
			printf("total len %d\n", len);
			*/
		   
		}
		else { //ディレクトリじゃない場合　
			//ファイルの場合
			sprintf(tmp, "%ld", st.st_size);
			strcpy(buf, tmp);
			strcat(buf , "\t");
			strcat(buf, asctime(localtime(&st.st_mtime)));
			len = strlen(buf);
			buf[len - 1] = '\0';
			strcat(buf, "\t");
			strcat(buf, pathname);
			len = strlen(buf);
			buf[len] = '\0';
			len = len - 1;

		}


		len_t--;
		ftph->length = htons(len_t);
		ftph->type = DATA;
		ftph->code = 0x00;
		if ((count = send(s2, &pkt_data, len_t + sizeof(pkt), 0)) < 0) {
			perror("sendto");
			exit(1);
		}				

		printf("send: type->0x%02x code->0x%02x length->%d\n", ftph->type, ftph->code, ntohs(ftph->length));
		//printf("%s\n", data);
		
		break;
		
	case STOR:
		len = ntohs(ftph->length);
		ftph = (struct myftph *)pkt_data;
		data = pkt_data + sizeof(struct myftph);
		
		if ((count = recv(s2, data, len, 0)) < 0) {
			perror("recv");
			exit(1);
		}

		data[len] = '\0';
		strcpy(file_name, data);
		printf("data: %s\n", data);
	
		//クライアント側からputしていいかを受諾
		//まずdataに入ってるファイルを作成することができるかどうか
		//を確認する
		fp = fopen(data, "wb");

		if (fp == NULL) { //ファイルを作成できない場合
			perror("fopen\n");
			//エラーがなんなのかを確認する
			
			ftph = (struct myftph *)pkt;
			ftph->type = FILE_ERR;
			
			//putコマンドにおいてはアクセス権だけ確認する
			if (errno == EACCES) { //Permision deneied				
				ftph->code = 0x01;
			}
			else {
				ftph->type = UNKWN_ERR;
				ftph->code = 0x05;
			}
		   

			//エラーメッセージを送信する
			if((count = send(s2, &pkt, sizeof(struct myftph), 0)) < 0) {
				perror("send");
				exit(1);				
			}
			printf("send: type->0x%02x code->0x%02x length->%d\n", ftph->type, ftph->code, ntohs(ftph->length));
		}
		else {
			ftph = (struct myftph *)pkt;
			ftph->type = OK;
			ftph->code = 0x02;
			ftph->length = htons(0);

			//OKメッセージをクライアントに送信する
			if((count = send(s2, &pkt,sizeof(struct myftph), 0)) < 0) {
				perror("send");
				exit(1);				
			}
			printf("send: type->0x%02x code->0x%02x length->%d\n", ftph->type, ftph->code, ntohs(ftph->length));
										
		}
			  

		break;

	case RETR:

		len = ntohs(ftph->length);
		
		ftph = (struct myftph *)pkt_data;
		data = pkt_data + sizeof(struct myftph);

		if ((count = recv(s2, data, len, 0)) < 0) {
			perror("recv");
		}

		data[len] = '\0';
		strcpy(file_name, data);
		fp = fopen(data, "rb");


		if (fp == NULL) { //file open エラー
			perror("fopen");

			ftph = (struct myftph *)pkt;
			ftph->type = FILE_ERR;
			ftph->length = htons(0);

			//なんのエラーなのかを判断する
			if (errno == ENOENT) { //No such file or directory
				ftph->code = 0x00;				
			}

			else if (errno == EACCES) { //Permision denied
				ftph->code = 0x01;
			}

			else {
				ftph->type = UNKWN_ERR;
				ftph->code = 0x05;
			}
			
			if ((count = send(s2, &pkt, sizeof(pkt), 0)) < 0) {
				perror("send");
				exit(1);
			}
			
			return ;
		}
		else { //ファイルが無事開けときの処理
			//まずはOKメッセージをsend
			ftph = (struct myftph *)pkt;
			ftph->type = OK;
			ftph->code = 0x01;

			if ((count = send(s2, &pkt, sizeof(pkt), 0)) < 0) {
				perror("send");
				exit(1);
			}

			//次にファイルデータを送信する
			fseek(fp, 0, SEEK_END);
			fsize = ftell(fp); //ファイルサイズ取得
			fclose(fp);
			printf("file size %d\n", fsize);

			ftph = (struct myftph *)pkt_data;
			data = pkt_data + sizeof(struct myftph);

			fp = fopen(data, "rb");

			if (fsize <= DATASIZE) {
				//データサイズが1024以下の場合
				ret = fread(data, sizeof(char), fsize, fp);
				printf("strlen %d\n", strlen(data));

				if (ret != fsize) {
					fprintf(stderr, "read error\n");
					return ;
				}

				ftph->type = DATA;
				ftph->length = htons(fsize);

				if ((count = send(s2, &pkt_data, fsize+sizeof(pkt),
								  0)) < 0) {
					perror("send");
					exit(1);
				}

				printf("send data!\n");
				fclose(fp);									
			}
			else { //データサイズが1024より大きい場合sendを分割
				n_size = 0;
				code = 0;
				while (fsize > 0) {
					if (DATASIZE < fsize) {
						ftph->code = 0x01;
						n_size = DATASIZE;
						
					}
					else {
						ftph->code = 0x00;
						n_size = fsize;
					}

					printf("n_size %d\n", n_size);
					ret = fread(data, sizeof(char), n_size, fp);

					if (ret != n_size) {
						fprintf(stderr, "read error\n");
						return ;
					}

					ftph->type = DATA;
					ftph->length = htons(n_size);

					if ((count = send(s2, &pkt_data, n_size + sizeof
									  (pkt), 0)) < 0) {
						perror("send");
						exit(1);
					}

					fsize -= n_size;
				}
				printf("Success send data\n");
				fclose(fp);
				
			}
			
		  
		}
		
		break;

	case DATA:
		//データが送られてくるので
		//先ほど作ったファイルに格納する
		
		code = ftph->code;
		len = ntohs(ftph->length);
		
		//DEGUG
		printf("recv data size %d\n", len);
		
		//DEBUG終了
		
		ftph = (struct myftph *)pkt_data;
		data = pkt_data + sizeof(struct myftph);
		
		
		if ((count = recv(s2, data, len, 0)) < 0) {
			perror("recv");
		}
		

		if (code == 0x00) {
			ret = fwrite(data, sizeof(char), len, fp);
			fclose(fp);
		}
		else if (code == 0x01) {
			ret = fwrite(data, sizeof(char), len, fp);
			fclose(fp);
			fp = fopen(file_name, "ab");			
			
		}						
		
		break;

	}
	
	return 0;

}

