
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

#define PORT 51230
#define BUFLEN 512
#define N 512
#define LEN 20
#define MAX_CLIENT 4
#define ERR -1
#define OK 1
#define STAT 20

//メッセージタイプtype
#define DISCOVER 1
#define REQUEST 3
#define RELEASE 5

//クライアントの状態


//stat文字列

char nl[STAT] = "NULL";    //stat == 0
char in[STAT] = "INIT";    //stat == 1
char wr[STAT] = "WAIT_REQ";//stat == 2
char iu[STAT] = "IN_USE";  //stat == 3
char wra[STAT] = "WAIT_REQ_AGAIN"; //stat == 4;


struct ipmask {
	struct ipmask *fp;
	struct ipmask *bp;
	char ip[LEN];
	char netmask[LEN];
};

struct client {
	struct client *fp;
	struct client *bp;
	int stat; //clientの状態
	int ttlcounter; //below; networork byte order
	struct in_addr id; //クライアントの識別子(IPアドレス)
	struct in_addr addr; //クライアントに割り当てたIPアドレス
	struct in_addr netmask; //クライアントに割り当てたnetmask
	uint16_t ttl; //IPアドレスの使用期限
	struct timeval start_time; //クライアントごとに設定されるREQUESTメッセージを送信したときの時刻
	struct sockaddr_in again; //再送用の宛先が格納されている
	int s; //再送用ソケット
	
};


struct dhcph {
	uint8_t type;
	uint8_t code;
	uint16_t ttl;
	in_addr_t address;
	in_addr_t netmask;
};

 


struct ipmask ipmask_list_head;
struct ipmask ipmask_use_list_head;
struct client client_list_head;

uint16_t use_ip_time;

void insert_ipmask_tail(struct ipmask *h, struct ipmask *p);
void insert_ipmask_use_tail(struct ipmask *h, struct ipmask *p);
void insert_client_tail(struct client *h, struct client *p);
void remove_ipmask_list(struct ipmask *p);
void remove_client_list(struct client *p);
void ipmask_list_print();
void client_list_print();
void ipmask_use_list_print();
void init(char *file);
int create_client(struct sockaddr_in *skt, struct ipmask *p);
void create_use_list(struct ipmask *p);
char * get_stat(struct client *c);
void ttl_decrement();

void ttl_count_flag();
void setitimer_ttl();
void setitimer_timeout();

void remove_ipmask_use_list(in_addr_t address);
void release_client(in_addr_t address);
void ttl_timout_event(struct client *c);

void wait_event();

struct client * search_client(struct sockaddr_in *skt);
struct ipmask * search_ipmask();
struct in_addr ipaddr;
int ttl_flag = 0;
struct itimerval timer;
struct itimerval val;

struct timeval current_time; //現在時刻の取得

//第１引数./mydhcpd 第２引数config_file

int main(int argc, char *argv[]) {
	int s;
	int myport = PORT;
	int i, val, n;
	fd_set rdfds;
	int count;
	int ok;
	struct sockaddr_in myskt;
	struct sockaddr_in skt; //clientの情報 source port & source IP
	struct timeval timeout;

	char buf[BUFLEN];

	socklen_t sktlen;
	struct dhcph dhcph_packet;
	struct ipmask *p, dammy_p;
	struct client *c;
	char *stat;
	char pre_stat[STAT];
	struct sigaction act;

	
	ipmask_list_head.fp = ipmask_list_head.bp = &ipmask_list_head;
	ipmask_use_list_head.fp = ipmask_use_list_head.bp = &ipmask_use_list_head;
	client_list_head.fp = client_list_head.bp = &client_list_head;


	//configファイルから使用期限, IPアドレス, ネットマスクを取得する
	if (argc != 2) {
		fprintf(stderr, "Usage: ./mydhcpd config_file\n");
		exit(1);
	}
 	
	init(argv[1]);
	printf("\n***********************************\n");
	printf("INIT: read config_file\n");
	printf("\n***********************************\n");

	printf("config_file contents");

	printf("\n--------------------------------------\n");
	
	printf("use_ip_time;: %d\n", use_ip_time);
	ipmask_list_print();
	
	printf("\n--------------------------------------\n");

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}


	memset(&myskt, 0, sizeof(myskt));
	myskt.sin_family = AF_INET;
	myskt.sin_port = htons(myport);
	myskt.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(s, (struct sockaddr *)&myskt, sizeof(myskt)) < 0) {
		perror("bind");
		exit(1);
	}

	//memset(&act, 0, sizeof(0));
	/*act.sa_handler = ttl_count_flag;
	act.sa_flags = SA_RESTART;
	sigaction(SIGALRM, &act, NULL);
	*/
	signal(SIGALRM, ttl_count_flag);
	

	printf("## wait for event ##\n");
	while (1) {
   	
		if (ttl_flag == 1) {
			ttl_decrement();
			ttl_flag = 0;
		}
				
		sktlen = sizeof skt;
		FD_ZERO(&rdfds);
		FD_SET(0, &rdfds);
		FD_SET(s, &rdfds);

		//タイムアウト設定
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;

		wait_event();

		
		do {
			if ((n = select(s+1, &rdfds, NULL, NULL, &timeout)) < 0) {
				if (errno != EINTR) {
					perror("select");
					exit(1);
				}
			}
		}while (n < 0);


		if (FD_ISSET(s, &rdfds)) {
			//クライアントからパケット受信
	    
			
			if ((count = recvfrom(s, &dhcph_packet, sizeof(dhcph_packet), 0,
								(struct sockaddr *)&skt, &sktlen)) < 0) {
				perror("recvfrom");
				exit(1);
			}

			c = search_client(&skt); //クライアント識別子により、そのクライアントの状態を探す

			if (c == &client_list_head) { //クライアントが存在しないため新しくクライアントを作成 //INIT状態
				//DISCOVERを受信したときの処理
				if (dhcph_packet.type == DISCOVER) { 

					printf("<messge recived from %s : %d>\n", inet_ntoa(skt.sin_addr), ntohs(skt.sin_port));
					printf("recv: DISCOVER message\n");
					
					ipaddr.s_addr = dhcph_packet.address;
					printf("type: %d(DISCOVER), code %d, ttl %d, IP %s ",
						   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
					ipaddr.s_addr = dhcph_packet.netmask;
					printf("netmask %s\n", inet_ntoa(ipaddr));
					
					p = search_ipmask(); // 空いてる組みを検索
					
					if (p == &ipmask_list_head) {
						//空いてる組みがない場合
						dhcph_packet.code = 1;
						dhcph_packet.type = 2;
						if ((count = sendto(s, &dhcph_packet,sizeof(dhcph_packet), 0,
											(struct sockaddr *)&skt, sizeof skt)) < 0) {
							perror("sendto");
							exit(1);
						}
						
						printf("\nOFFER(FAILURE) message send to %s\n", inet_ntoa(skt.sin_addr));
						
						ipaddr.s_addr = dhcph_packet.address;
						printf("type: %d(OFFER), code %d, ttl %d, IP %s ",
							   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
						ipaddr.s_addr = dhcph_packet.netmask;
						printf("netmask %s\n", inet_ntoa(ipaddr));
					
						printf("## stat changed (ID %s): NULL->terminate\n", inet_ntoa(skt.sin_addr)); 					
						printf("\n--------------------------------------\n");				   	   
					}
					else {
						//空いている組みを発見
						strcpy(dammy_p.ip, p->ip);
						strcpy(dammy_p.netmask, p->netmask);						
						
						create_use_list(p);
						remove_ipmask_list(p); //ipmaskリストから削除

					
						printf("\n");
						create_client(&skt, &dammy_p); //クライアントを作成
						c = search_client(&skt);
						printf("--new  client %s --\n", inet_ntoa(c->id));
						printf("**clinet list**\n");					
						client_list_print();
						
						printf("## stat changed (ID %s): NULL->%s\n", inet_ntoa(c->id), get_stat(c)); 					
						
						printf("Address alocate:ip-> %s netmask-> %s\n", dammy_p.ip, dammy_p.netmask);
						dhcph_packet.code = 0; //空いてる組みあり
						
						dhcph_packet.ttl = htons(use_ip_time); // ネットオーダーに変換
						
						dhcph_packet.type = 2;
						dhcph_packet.address = inet_addr(dammy_p.ip); //ip提示 ネットオーダーに変換
						dhcph_packet.netmask = inet_addr(dammy_p.netmask); //netma_sk提示 ネットオーダーに変換
						
						printf("\n**address pool**\n");
						ipmask_list_print();
						
						printf("\n**address use list**\n");
						ipmask_use_list_print();
						
						strcpy(pre_stat, get_stat(c));
						c->stat = 2; //wait_req
						printf("\nOFFER(SUCCESS) message sent to %s\n", inet_ntoa(skt.sin_addr));
						
						ipaddr.s_addr = dhcph_packet.address;
						printf("type: %d(OFFER), code %d, ttl %d, IP %s ",
							   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
						ipaddr.s_addr = dhcph_packet.netmask;
						printf("netmask %s\n", inet_ntoa(ipaddr));
						
						printf("## stat changed (ID %s): %s->%s\n", inet_ntoa(c->id), pre_stat, get_stat(c)); 					
						printf("\n--------------------------------------\n");				   	   
						
						memset(pre_stat, 0, sizeof(pre_stat));
						if ((count = sendto(s, &dhcph_packet,sizeof(dhcph_packet), 0,
											(struct sockaddr *)&skt, sizeof skt)) < 0) {
							perror("sendto");
							exit(1);
						}

						//OFFERメッセージを送信しWAIT_REQUSET状態に遷移したため10秒タイムアウトを開始する
						if (gettimeofday(&c->start_time, NULL) < 0) {
							perror("gettimeofday");
							exit(1);
						}

						
						//再送するために、sktのコピーをクライアントの再送用構造体に格納する
						//まずはコピー先のagainを初期化
						memset(&c->again, 0, sizeof(c->again));
						memcpy(&c->again, &skt, sizeof(skt)); //sktをagainにコピー
						//再送するためソケットをコピー
						c->s = s;

						
						printf("WAIT_REQ timeout(10sec) count start : ");
						printf("%s\n", ctime((time_t*)&c->start_time.tv_sec));
						printf("## wait for event ##\n");
						
					}
				}
				else { //クライアントが存在していないのにも関わらずDESICOVERメッセージ以外を受け取った場合
					//正直書く必要はあまりないがプロトコルに整合しないためwait_eventに戻るというメッセージを表示
					printf("\n<messge recived from %s : %d>\n", inet_ntoa(skt.sin_addr), ntohs(skt.sin_port));
					printf("**recv: inconsistency protocol message**!\n");
					printf("**This client not exisit!**\n");
					printf("## wait for event ##\n");

				}												
			}

			else {//クライアント存在する・・・すなわち少なくとWAIT_REQ状態以降
				//ここからはクライアントの状態を確かめ、それに応じたイベント処理が行われる
				
				switch (c->stat) { //まずクライアントの状態で分岐
				case 2: //WAIT_REQ状態のとき
				case 4: //WAIT_REQ_AGAIN状態のとき
					if (dhcph_packet.type == REQUEST) {
						//REQUESTメッセージ受信
					
						/*クライアントからのREQUESTメッセージがOFFER
						  メッセージで提示されたものと違う場合code=4
						  同じの場合はcode=0をクライアントに返す*/
						
				   		if (dhcph_packet.code == 2) { //割り当て要求の場合
							//割り当てOKの場合
							
							strcpy(pre_stat, get_stat(c));
							
							printf("<messge recived from %s : %d>\n", inet_ntoa(skt.sin_addr), ntohs(skt.sin_port));
							printf("recv: REQUEST message\n");			   				
							ipaddr.s_addr = dhcph_packet.address; 
							printf("type: %d(REQUEST), code %d, ttl %d, IP %s ",
								   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
							ipaddr.s_addr = dhcph_packet.netmask;
							printf("netmask %s\n", inet_ntoa(ipaddr));//ホストオーダーに変換
							
							
							if (dhcph_packet.address == c->addr.s_addr && dhcph_packet.netmask == c->netmask.s_addr
								&& dhcph_packet.ttl <= c->ttl){
								c->stat = 3; //IN_USEの状態に遷移
								c->ttlcounter = ntohs(dhcph_packet.ttl);
								c->ttl = dhcph_packet.ttl;
								dhcph_packet.code = 0;
								dhcph_packet.type = 4; //ACK
								printf("REQUEST(OK): %s\n", inet_ntoa(c->id));
								printf("ACK(SUCCESS) message send to %s\n", inet_ntoa(skt.sin_addr));
								
								ipaddr.s_addr = dhcph_packet.address;
								printf("type: %d(ACK), code %d, ttl %d, IP %s ",
									   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
								ipaddr.s_addr = dhcph_packet.netmask;
								printf("netmask %s\n", inet_ntoa(ipaddr));
								
								
								printf("## stat changed (ID %s): %s->%s\n", inet_ntoa(c->id), pre_stat, get_stat(c));
								
								printf("\n--------------------------------------\n");
								memset(pre_stat, 0, sizeof(pre_stat));
								if ((count = sendto(s, &dhcph_packet,sizeof(dhcph_packet), 0,
													(struct sockaddr *)&skt, sizeof skt)) < 0) {
									perror("sendto");
									exit(1);
								}
								
								setitimer_ttl(); //ttl_countススタート（使用期限)
								
								
								
								
								
							}
							
							else {
								dhcph_packet.code = 4; //エラー
								dhcph_packet.type = 4; //ACK
								printf("recv: REQUEST(NO): %s\n", inet_ntoa(c->id));
								
								if (dhcph_packet.address != c->addr.s_addr)
									printf("**offer IP not equal request IP**\n");
								if (dhcph_packet.netmask != c->netmask.s_addr)
									printf("**offer NETMASK not equal request NETMASK**\n");
								if (dhcph_packet.ttl != c->ttl)
									printf("**offer TTL not equal request TTL**\n");
								
								printf("ACK(FAILURE) message send to %s\n", inet_ntoa(skt.sin_addr));
								
								ipaddr.s_addr = dhcph_packet.address;
								printf("type: %d(ACK), code %d, ttl %d, IP %s ",
									   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
								ipaddr.s_addr = dhcph_packet.netmask;
								printf("netmask %s\n", inet_ntoa(ipaddr));
																																
								printf("\n");
								
								if ((count = sendto(s, &dhcph_packet,sizeof(dhcph_packet), 0,
													(struct sockaddr *)&skt, sizeof skt)) < 0) {
									perror("sendto");
									exit(1);
								}
								
								printf("<--recall IP: %s-->\n", inet_ntoa(c->addr));
								remove_ipmask_use_list(c->addr.s_addr); //使用リストから削除し、未使用リストの最後尾に追加
								printf("## stat changed (ID %s): %s->terminate\n", inet_ntoa(c->id), pre_stat);
								release_client(c->addr.s_addr); //クライアントリストから削除
								printf("\n**address pool**\n");
								ipmask_list_print();
								
								printf("\n**address use list**\n");
								ipmask_use_list_print();
								
								printf("## wait for event ##\n");								
								
							}
							
							
						}
					}
					else {
						//WAIT_REQ状態にも関わらず、REQUESTメッセージ以外が送られてきたときの処理
						printf("\n<messge recived from %s : %d>\n", inet_ntoa(skt.sin_addr), ntohs(skt.sin_port));
						printf("**recv: inconsistency protocol message!!**\n");

						printf("## wait for event ##\n");

						
					}
					break;
					
					
					
				case 3: //IN_USE状態のとき
					if (dhcph_packet.type == REQUEST) { //REQUESTメッセージ受信
						if (dhcph_packet.code == 3) { //試用期間延長要求の場合
							
							strcpy(pre_stat, get_stat(c));
							
							printf("<messge recived from %s : %d>\n", inet_ntoa(skt.sin_addr), ntohs(skt.sin_port));
							printf("recv: REQUEST message\n");			   				
							ipaddr.s_addr = dhcph_packet.address;
							printf("type: %d(REQUEST), code %d, ttl %d, IP %s ",
								   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
							ipaddr.s_addr = dhcph_packet.netmask;
							printf("netmask %s\n", inet_ntoa(ipaddr));
							
							printf("IP use time: Extension!!\n");
							
							//期間延長要求によりttlcounterリセット							
							
							if (dhcph_packet.address == c->addr.s_addr && dhcph_packet.netmask == c->netmask.s_addr
								&& dhcph_packet.ttl <= c->ttl){
								c->ttlcounter = (int)use_ip_time;
								c->stat = 3;
								c->ttlcounter = ntohs(dhcph_packet.ttl); //ttlリセット
								//c->ttl = dhcph_packet.ttl; //一応(いらいないと思うが..)
								dhcph_packet.code = 0;
								dhcph_packet.type = 4; //ACK
								printf("REQUEST(EXTENSION): %s\n", inet_ntoa(c->id));
								printf("ACK(SUCCESS) message send to %s\n", inet_ntoa(skt.sin_addr));
								
								printf("## stat changed (ID %s): %s->%s\n", inet_ntoa(c->id), pre_stat, get_stat(c));
								
								printf("\n--------------------------------------\n");
								memset(pre_stat, 0, sizeof(pre_stat));
								if ((count = sendto(s, &dhcph_packet,sizeof(dhcph_packet), 0,
													(struct sockaddr *)&skt, sizeof skt)) < 0) {
									perror("sendto");
									exit(1);
								}
							}
							
						}else {
							//IN_USE状態にも関わらずcodeが延長要求以外で送られてきた場合
							printf("\n<messge recived from %s : %d>\n", inet_ntoa(skt.sin_addr), ntohs(skt.sin_port));
							printf("**recv: inconsistency protocol message!!**\n");

							printf("## wait for event ##\n");
							
						}

						
					}

					else if (dhcph_packet.type == RELEASE){ //RELEASE メッセージを受信したためIPアドレス解放
						
						strcpy(pre_stat, get_stat(c));
						
						printf("<messge recived from %s : %d>\n", inet_ntoa(skt.sin_addr), ntohs(skt.sin_port));
						printf("recv: RELEASE message\n");			   				
						ipaddr.s_addr = dhcph_packet.address;
						printf("type: %d(RELEASE), code %d, ttl %d, IP %s ",
							   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
						ipaddr.s_addr = dhcph_packet.netmask;
						printf("netmask %s\n", inet_ntoa(ipaddr));
						remove_ipmask_use_list(dhcph_packet.address); //使用リストから削除し、未使用リストの最後尾に追加
						printf("## stat changed (ID %s): %s->terminate\n", inet_ntoa(c->id), pre_stat);
						release_client(dhcph_packet.address); //クライアントリストから削除

						printf("\n**address pool**\n");
						ipmask_list_print();
						
						printf("\n**address use list**\n");
						ipmask_use_list_print();
						
						printf("## wait for event ##\n");

						
					}
					else {

						printf("\n<messge recived from %s : %d>\n", inet_ntoa(skt.sin_addr), ntohs(skt.sin_port));
						printf("**recv: inconsistency protocol message!!**\n");

						printf("## wait for event ##\n");

					}
			   
					break;			   					
				}
				
			}



		}
		
    }
	
	return 0;
}



void init(char *file)
{
	FILE *fp;
	char ipnet[N], tmp_ip[LEN], tmp_netmask[LEN], *tkn;
	int state = 0;
	int len, i;
	struct ipmask *p;
	
	if ((fp = fopen(file, "r")) == NULL) {
		fprintf(stderr, "can't open file!: %s\n", file);
		exit(1);
	}
	
	while (fgets(ipnet, N, fp) != NULL) {
		len = strlen(ipnet);
		ipnet[len - 1] = '\0';
		if (state == 0) {
			use_ip_time = (uint16_t)atoi(ipnet);
			state = 1;
		}
		else {
			p = (struct ipmask *)malloc(sizeof(struct ipmask));
			if (p == NULL) {
				fprintf(stderr, "Can't allocate memory\n");
				exit(1);
			}		
			
			tkn = strtok(ipnet, " ");
			for (i = 0; tkn[i] != '\0'; i++) {
				p->ip[i] = tkn[i];
			}
			p->ip[i] = '\0';
			//		strcpy(p->ip, tkn);
			while (tkn != NULL) {
				tkn = strtok(NULL, " ");
				if (tkn != NULL) {
					// 	strcpy(p->netmask, tkn);
					for (i = 0; tkn[i] != '\0'; i++) {
						p->netmask[i] = tkn[i];
					}
					p->netmask[i] = '\0';
				}
			}
			insert_ipmask_tail(&ipmask_list_head, p);
		}
		
	}

	close(fp);
}


int create_client(struct sockaddr_in *skt, struct ipmask *p)
{
	struct client *c;

	c = (struct client *)malloc(sizeof(struct client));
	if (c == NULL) {
		fprintf(stderr, "Can't allocate memory\n");
		exit(1);
	}

	c->id.s_addr = skt->sin_addr.s_addr; //ID設定

	if (inet_aton(p->ip, &c->addr) == 0) {//バイナリーオーダー変換
		perror("inet_aton");
		exit(1);
	}

	if (inet_aton(p->netmask, &c->netmask) == 0) {//バイナリーオーダー変換
		perror("inet_aton");
		exit(1);
	}

	c->ttlcounter = (int)use_ip_time;
	c->ttl = htons(use_ip_time);        //書き換え
	c->stat = 1;
	/*	inet_aton("0.0.0.0", &c->addr);
	inet_aton("0.0.0.0", &c->netmask);
	*/
	
	
	insert_client_tail(&client_list_head, c);

	printf("new client create: client ID %s\n", inet_ntoa(c->id));
	
	return OK;
			
}

void create_use_list(struct ipmask *p)
{
	struct ipmask *u;

	u = (struct ipmask *)malloc(sizeof(struct ipmask));
	if (u == NULL) {
		fprintf(stderr, "Can't allocate memory\n");
		exit(1);
	}

	strcpy(u->ip, p->ip);
	strcpy(u->netmask, p->netmask);

	insert_ipmask_use_tail(&ipmask_use_list_head, u);
	
}


struct client * search_client(struct sockaddr_in *skt)
{
	struct client *h, *c;
	h = &client_list_head;
	for (c = h->fp; c != h; c = c->fp) {
		if (skt->sin_addr.s_addr == c->id.s_addr)
			break;
	}

	return c;
}

void release_client(in_addr_t address)
{
	struct client *h, *c;
	h = &client_list_head;
	for (c = h->fp; c != h; c = c->fp) {
		if (c->addr.s_addr == address) {
			remove_client_list(c);
			free(c);			
			break;			
		}
	}		
}

void insert_ipmask_tail(struct ipmask *h, struct ipmask *p)
{
	h->bp->fp = p;
	p->bp = h->bp;
	p->fp = h;
	h->bp = p;
}

void insert_ipmask_use_tail(struct ipmask *h, struct ipmask *p)
{
	h->bp->fp = p;
	p->bp = h->bp;
	p->fp = h;
	h->bp = p;
}


void insert_client_tail(struct client *h, struct client *p)
{
	h->bp->fp = p;
	p->bp = h->bp;
	p->fp = h;
	h->bp = p;
	
}

void ipmask_list_print()
{
	struct ipmask *h, *p;
	h = &ipmask_list_head;
	for (p = h->fp; p != h; p = p->fp)
		printf("IP: %s NETMASK: %s\n", p->ip, p->netmask);
	
}


void client_list_print()
{
	struct client *h, *p;
	h = &client_list_head;
	for (p = h->fp; p != h; p = p->fp) {
		printf("ID: %s ", inet_ntoa(p->id));
		printf("IP: %s\n", inet_ntoa(p->addr));
	}
	
}


void ipmask_use_list_print()
{
	struct ipmask *h, *p;
	h = &ipmask_use_list_head;
	for (p = h->fp; p != h; p = p->fp)
		printf("IP: %s NETMASK: %s\n", p->ip, p->netmask);
	
}


/*
使用リストからipmaskを探し配列にコピー
そして使用リストから解放
この配列を利用し、新しく未使用リストにipmaskを登録
 */
void remove_ipmask_use_list(in_addr_t address)
{
	struct ipmask *h, *p, *l;
	char tmp_ip[LEN], tmp_netmask[LEN];

	ipaddr.s_addr = address;
	strcpy(tmp_ip, inet_ntoa(ipaddr));
	
	h = &ipmask_use_list_head;
	for (p = h->fp; p != h; p = p->fp) {
		if (strcmp(tmp_ip, p->ip) == 0) {
			strcpy(tmp_netmask, p->netmask);			
			remove_ipmask_list(p);
			free(p);

			l = (struct ipmask *)malloc(sizeof(struct ipmask));
			if (l == NULL) {
				fprintf(stderr, "Can't allocate memory\n");
				exit(1);
			}

			strcpy(l->ip, tmp_ip);
			strcpy(l->netmask, tmp_netmask);
			insert_ipmask_tail(&ipmask_list_head, l);
			
			break;
		}
	}
	
}

void remove_ipmask_list(struct ipmask *p)
{
	p->bp->fp = p->fp;
	p->fp->bp = p->bp;
	p->fp = p->bp = NULL;
}

void remove_client_list(struct client *p)
{
	p->bp->fp = p->fp;
	p->fp->bp = p->bp;
	p->fp = p->bp = NULL;
	
}

struct ipmask * search_ipmask()
{
	struct ipmask *h, *p;
	h = &ipmask_list_head;
	for (p = h->fp; p != h; p = p->fp) {
		return p;
	}
}

char * get_stat(struct client *c)
{
	if (c->stat == 0) {
		return nl;
	}
	else if (c->stat == 1) {
		return in;
	}
	else if (c->stat == 2) {
		return wr;
	}
	else if (c->stat == 3) {
		return iu;
	}
	else if (c->stat == 4) {
		return wra;
	}
		
}

void ttl_count_flag()
{
	ttl_flag = 1;
}

void ttl_decrement()
{
	struct client *h, *c;
	h = &client_list_head;
	for (c = h->fp; c != h; c = c->fp) {
		if (c->stat == 3) { //状態がIN_USEのクライアントだけ
			c->ttlcounter = c->ttlcounter - 1;
			if (c->ttlcounter < 0) { //タイムアウト処理:使用期限
				ttl_timout_event(c);
				break;
			}
			else {
				printf("--client ip->%s, TTL->%2d--\n", inet_ntoa(c->addr), c->ttlcounter);
			}
		}
	}
	
}

void setitimer_ttl()
{
	memset(&timer, 0, sizeof(timer));	
	timer.it_interval.tv_sec = 1;
	timer.it_interval.tv_usec = 0;		
	timer.it_value = timer.it_interval;
	
	if (setitimer(ITIMER_REAL, &timer, NULL) < 0) {
		perror("setitimer");
		exit(1);
	}

}

void setitimer_timeout()
{
	memset(&val, 0, sizeof(val));	
	val.it_interval.tv_sec = 10;
	val.it_interval.tv_usec = 0;		
	val.it_value = val.it_interval;
	
	if (setitimer(ITIMER_REAL, &val, NULL) < 0) {
		perror("setitimer");
		exit(1);
	}

}

void ttl_timout_event(struct client *c)
{
	char pre_stat[STAT];
	strcpy(pre_stat, get_stat(c));
	printf("** TTL timout IP:%s", inet_ntoa(c->addr));
	printf("(client ID:%s) **\n", inet_ntoa(c->id));

	printf("<--recall IP: %s-->\n", inet_ntoa(c->addr));
	remove_ipmask_use_list(c->addr.s_addr); //使用リストから削除し、未使用リストの最後尾に追加
	printf("## stat changed (ID %s): %s->terminate\n", inet_ntoa(c->id), pre_stat);
	release_client(c->addr.s_addr); //クライアントリストから削除
	printf("\n**address pool**\n");
	ipmask_list_print();
	
	printf("\n**address use list**\n");
	ipmask_use_list_print();

	printf("## wait for event ##\n");	
	
}


void wait_event()
{
	int count;
	struct dhcph dhcph_packet_again;
	struct in_addr ipaddr_again;
	long t;  
	struct client *h, *c;
	h = &client_list_head;
	for (c = h->fp; c != h; c = c->fp) {
		if (c->stat == 2) { //状態がWAIT_REQのクライアントだけ
			if (gettimeofday(&current_time, NULL) < 0) {
				perror("gettimeofday");
				exit(1);
			}
			t = current_time.tv_sec - c->start_time.tv_sec;
			if (t >= 10) {
				c->stat = 4; //WAIT_REQ_AGAIN状態に遷移
				

				printf("\n**WAIT_REQ timeout! (ID %s)**\n", inet_ntoa(c->id));


				//再送する
				printf("\nOFFER(AGAIN) message sent to %s\n", inet_ntoa(c->again.sin_addr));

				
				dhcph_packet_again.code = 0; //空いてる組みあり
				dhcph_packet_again.ttl = htons(use_ip_time);  //ネットーワークオーダーに変換
				dhcph_packet_again.type = 2;
				dhcph_packet_again.address = c->addr.s_addr; //ip提示
				dhcph_packet_again.netmask = c->netmask.s_addr; //netmask提示
	
				ipaddr_again.s_addr = dhcph_packet_again.address;
				printf("type: %d(OFFER), code %d, ttl %d, IP %s ",
					   dhcph_packet_again.type, dhcph_packet_again.code, ntohs(dhcph_packet_again.ttl), inet_ntoa(ipaddr_again));
				ipaddr_again.s_addr = dhcph_packet_again.netmask;
				printf("netmask %s\n", inet_ntoa(ipaddr_again));
				
				printf("## stat changed (ID %s): WAIT_REQ->%s\n", inet_ntoa(c->id), get_stat(c)); 					
				printf("\n--------------------------------------\n");
				if ((count = sendto(c->s, &dhcph_packet_again, sizeof(dhcph_packet_again), 0,
									(struct sockaddr *)&c->again, sizeof c->again)) < 0) {
					perror("sendto");
					exit(1);
				}
				if (gettimeofday(&c->start_time, NULL) < 0) {
					perror("gettimeofday");
					exit(1);
				}
			
				printf("WAIT_REQ_AGAIN timeout(10sec) count start : ");
				printf("%s\n", ctime((time_t*)&c->start_time.tv_sec));
				printf("## wait for event ##\n");
				

				
			}
			
		}
		else if (c->stat == 4) {
			if (gettimeofday(&current_time, NULL) < 0) {
				perror("gettimeofday");
				exit(1);
			}
			t = current_time.tv_sec - c->start_time.tv_sec;
			if (t >= 10) {
				c->stat = 5;
				printf("**WAIT_REQ_AGAIN timeout (ID %s)**\n", inet_ntoa(c->id));
				//確保したIPを解放&クライアントを削除し、terminateへ遷移
				printf("<--recall IP: %s-->\n", inet_ntoa(c->addr));
				remove_ipmask_use_list(c->addr.s_addr); //使用リストから削除し、未使用リストの最後尾に追加
				printf("## stat changed (ID %s): WAIT_REQ_AGAIN->terminate\n", inet_ntoa(c->id));
				release_client(c->addr.s_addr); //クライアントリストから削除
				printf("\n**address pool**\n");
				ipmask_list_print();
				
				printf("\n**address use list**\n");
				ipmask_use_list_print();
				
				printf("## wait for event ##\n");
				break;

			}
			
		}
	}
	
}
