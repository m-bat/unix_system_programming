


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
#define STAT 30

//状態
#define OFFER 2
#define ACK 4


char nl[STAT] = "NULL"; //stat == 0
char in[STAT] = "INIT"; //stat == 1
char wo[STAT] = "WAIT_OFFER"; //stat == 2
char wa[STAT] = "WAIT_ACK";   //stat == 3
char iu[STAT] = "IN_USE";     //stat == 4
char wea[STAT] = "WAIT_EXIT_ACK"; //stat == 5
char ex[STAT] = "exiting";    //stat == 6
char weaa[STAT] = "WAIT_EXIT_ACK_AGAIN"; //stat == 7
char woa[STAT] = "WAIT_OFFER_AGAIN"; //stat == 8
char waa[STAT] = "WAIT_AKC_AGAIN"; //stat === 9

struct dhcph {
	uint8_t type;
	uint8_t code;
	uint16_t ttl;
	in_addr_t address;
	in_addr_t netmask;
};

int stat = 1;
uint16_t use_ip_time = 0;

struct itimerval timer;
void setitimer_ttl(int ttlcounter);
void ttl_count_flag();
void ip_extension_msg();
char * get_stat(int st);
void release_flag_set();
void release();
void wait_event();


int ttl_flag = 0;
int release_flag = 0;

int s;
char pre_stat[STAT];
struct sockaddr_in myskt, skt;
struct dhcph dhcph_packet;
struct in_addr ipaddr;

struct timeval current_time; //現在時刻の取得
struct timeval start_time;


int main(int argc, char *argv[]) {
	fd_set rdfds;
	int count;
	int n;
	in_port_t myport = PORT;
	//	char ip[BUFLEN] = "131.113.108.54";
	socklen_t sktlen;
	struct in_addr dammy_ip, dammy_netmask;
	struct timeval timeout;


	if (argc != 2) {
		fprintf(stderr, "Usage: ./mydhcpc server-IP-address\n");
		exit(1);
	}

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}	   

	memset(&myskt, 0, sizeof(myskt));
	myskt.sin_family = AF_INET;
	myskt.sin_port = htons(myport);
	if (inet_aton(argv[1], &myskt.sin_addr) < 0) {
		perror("inet_aton");
		exit(1);
	}



	printf("INIT\n");
	printf("--server IP: %s:%d\n", argv[1], myport);
	
	// typeフィールド1にしてそれ以外は０を設定
	dhcph_packet.type = 1;
	dhcph_packet.code = 0;
	dhcph_packet.ttl = htons(0);
	dhcph_packet.address = inet_addr("0.0.0.0");
	dhcph_packet.netmask = inet_addr("0.0.0.0");
	
	
	
	if ((count = sendto(s, &dhcph_packet, sizeof(dhcph_packet), 0,
						(struct sockaddr *)&myskt, sizeof(myskt))) < 0) {
		perror("sendto");
		exit(1);
	}
	printf("** DISCOVER send **\n");

	stat = 2; //WAIT_OFFER遷移

	//DISCOVERを送信しWAIT_OFFERに遷移したため10秒タイムアウトを開始する
	
	if (gettimeofday(&start_time, NULL) < 0) {
		perror("gettimeofday");
		exit(1);
	}

	ipaddr.s_addr = dhcph_packet.address;
	printf("type: %d(DISCOVER), code %d, ttl %d, IP %s ",
		   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
	ipaddr.s_addr = dhcph_packet.netmask;
	printf("netmask %s\n", inet_ntoa(ipaddr));
	
	printf("## stat changed: INIT->%s ##", get_stat(stat));
		
	printf("\n-------------------------------------------\n");
	
	
	//		memset(&dhcph_packet, 0, sizeof(dhcph_packet));
	
	


		
	while (1) {

		wait_event();
		//1/2の時間が経過したときに行われる
		if (ttl_flag == 1) {
			if (stat == 4) //IN_USEのときREQUEST(extension)msg送信
				ip_extension_msg();	
			ttl_flag = 0;
		}

		
		//SIGHUPを受け取ったときの処理
		if (release_flag == 1) {
			if (stat == 4) //IN_USEのときSIGHUPを受け取ったらRELEASEmsg送信
				release();
			exit(0);
		}

		
		sktlen = sizeof skt;
		FD_ZERO(&rdfds);
		FD_SET(0, &rdfds);
		FD_SET(s, &rdfds);
		
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		
		do {
			if ((n = select(s+1, &rdfds, NULL, NULL, &timeout)) < 0) {
				if (errno != EINTR) {
					perror("select");
					exit(1);
				}
			}
		}while (n < 0);
		
		
		if (FD_ISSET(s, &rdfds)) {
			if ((count = recvfrom(s, &dhcph_packet, sizeof(dhcph_packet), 0,
								  (struct sockaddr *)&skt, &sktlen)) < 0) {
				perror("recvform");
				exit(1);
			}


			//状態遷移に従い書き直す
			/*stat==4(IN_USE)のときはrecv処理ではなくシグナルのためここのswitch文ない
			  whileループの先頭に記述してある.
			 */

			switch (stat) {
			case 2: //WAIT_OFFRE状態
			case 8: //WAIT_OFFER_AGAIN状態

				strcpy(pre_stat, get_stat(stat));
				if (dhcph_packet.type == OFFER) { //OFFER受信
					
					if (dhcph_packet.code == 0) { //recv OFFER OK
						
						
						printf("## OFFER(OK) recived ##\n");
						
						ipaddr.s_addr = dhcph_packet.address;
						printf("type: %d(OFFER), code %d, ttl %d, IP %s ",
							   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
						ipaddr.s_addr = dhcph_packet.netmask;
						printf("netmask %s\n", inet_ntoa(ipaddr));			


						/*サーバーのWAIT_REQUSETのタイムアウト処理を確認できる
						  DEBUGのために記述する場所：
						 */

						//DEBUG終了
						
					
						//割り当て要求sent
						dhcph_packet.type = 3;
						dhcph_packet.code = 2; //割り当て要求
						dhcph_packet.ttl = dhcph_packet.ttl; //TTL設定 DEBUGで書き換えてもOK
						use_ip_time = ntohs(dhcph_packet.ttl); //ホストオーダーに変換
						if ((count = sendto(s, &dhcph_packet, sizeof(dhcph_packet), 0,
											(struct sockaddr *)&myskt, sizeof(myskt))) < 0) {
							perror("sendto");
							exit(1);
						}
						
						stat = 3; //WAIT_ACKに遷移
						
						//REQUESTを送信しWAIT_ACKに遷移したため10秒タイムアウトを開始する
						
						if (gettimeofday(&start_time, NULL) < 0) {
							perror("gettimeofday");
							exit(1);
						}
						
						
						printf("## REQUEST (alloc) sent ##\n");
						ipaddr.s_addr = dhcph_packet.address;
						printf("type: %d(DISCOVER), code %d, ttl %d, IP %s ",
							   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
						ipaddr.s_addr = dhcph_packet.netmask;
						printf("netmask %s\n", inet_ntoa(ipaddr));
					
						printf("## stat changed: %s->%s ##\n", pre_stat, get_stat(stat));
						
						
						printf("\n-------------------------------------------\n");
					}
					else if (dhcph_packet.code == 1) { //recv OFFER NG 割り当てるIPがなかった
						//NG処理を記述
						
						printf("## OFFER(NG) recived ##\n");
						
						stat = 6;
						ipaddr.s_addr = dhcph_packet.address;
						printf("type: %d(OFFER), code %d, ttl %d, IP %s ",
							   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
						ipaddr.s_addr = dhcph_packet.netmask;
						printf("netmask %s\n", inet_ntoa(ipaddr));
						
						printf("No IP Address! Sorry..\n");
						printf("## stat changed: %s->%s ##\n", pre_stat, get_stat(stat));
						
						exit(0);
					}
					
				}
				else {  //WAIT_OFFER || WAIT_OFFER_AGAIN状態にも関わらずOFFERメッセージ以外を受け取った時

					
					printf("## inconsistency protocol message recived ##\n");
					stat = 6;
					ipaddr.s_addr = dhcph_packet.address;
					printf("type: %d, code %d, ttl %d, IP %s ",
						   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
					ipaddr.s_addr = dhcph_packet.netmask;
					printf("netmask %s\n", inet_ntoa(ipaddr));
					printf("## stat changed: %s->%s ##\n", pre_stat, get_stat(stat));

				}
												
				break;
					
			case 3: //WAIT_ACK状態
			case 9: //WAIT_ACK_AGAIN状態

				strcpy(pre_stat, get_stat(stat));
				if (dhcph_packet.type == ACK) {
				
					setitimer_ttl((int)use_ip_time / 2);
					
					
					if (dhcph_packet.code == 0) {  //ACK OK
						
						printf("## ACK (OK) received ##\n");
						signal(SIGHUP, release_flag_set); //SIGHUPを登録
						signal(SIGALRM, ttl_count_flag); //SIGALRMを登録
						
						stat = 4; //IN_USEの状態に遷移
						ipaddr.s_addr = dhcph_packet.address;
						printf("type: %d(ACK), code %d(SUCCESS), ttl %d, IP %s ",
							   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
						ipaddr.s_addr = dhcph_packet.netmask;
						printf("netmask %s\n", inet_ntoa(ipaddr));					
						printf("## stat changed: %s->%s ##\n", pre_stat, get_stat(stat));
						
						printf("\n-------------------------------------------\n");
						printf("%s", ctime((time_t*)&current_time.tv_sec));
						
					}
					
					else if (dhcph_packet.code == 4) { //ACK NG
							
						printf("## ACK (NG) received ##\n");
						
						ipaddr.s_addr = dhcph_packet.address;
						printf("type: %d(ACK), code %d(SUCCESS), ttl %d, IP %s ",
							   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
						ipaddr.s_addr = dhcph_packet.netmask;
						printf("netmask %s\n", inet_ntoa(ipaddr));					
						printf("## stat changed: %s->exiting ##\n", pre_stat);
						exit(0);
						
					}										
					
					}
				else { //WAIT_ACK || WAIT_ACK_AGAIN状態にも関わらず、ACKメッセージ以外を受信したば場合
					printf("## inconsistency protocol message recived ##\n");
					stat = 6;
					ipaddr.s_addr = dhcph_packet.address;
					printf("type: %d, code %d, ttl %d, IP %s ",
						   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
					ipaddr.s_addr = dhcph_packet.netmask;
					printf("netmask %s\n", inet_ntoa(ipaddr));
					printf("## stat changed: %s->%s ##\n", pre_stat, get_stat(stat));
					
				}
				
				
				break;
			case 5: //WAIT_EXT_ACK状態
			case 7: //WAIT_EXT_ACK_AGAIN状態

				if (dhcph_packet.type == ACK) {
					strcpy(pre_stat, get_stat(stat));
					setitimer_ttl((int)use_ip_time / 2);
					
					
					if (dhcph_packet.code == 0) {  //ACK OK
							
						printf("## ACK (OK) received ##\n");
						
						stat = 4; //IN_USEの状態に遷移
						ipaddr.s_addr = dhcph_packet.address;
						printf("type: %d(ACK), code %d(SUCCESS), ttl %d, IP %s ",
							   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
						ipaddr.s_addr = dhcph_packet.netmask;
						printf("netmask %s\n", inet_ntoa(ipaddr));					
						printf("## stat changed: %s->%s ##\n", pre_stat, get_stat(stat));
						printf("\n-------------------------------------------\n");
						printf("%s", ctime((time_t*)&current_time.tv_sec));
					}
					
					else if (dhcph_packet.code == 4) { //ACK NG
						
						printf("## ACK (NG) received ##\n");
						
						ipaddr.s_addr = dhcph_packet.address;
						printf("type: %d(ACK), code %d(SUCCESS), ttl %d, IP %s ",
							   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
						ipaddr.s_addr = dhcph_packet.netmask;
						printf("netmask %s\n", inet_ntoa(ipaddr));					
						printf("## stat changed: %s->exiting ##\n", pre_stat);
						exit(0);
						
					}										
					
				}

				else { //WAIT_EXT_ACK || WAIT_EXT_ACK_AGAIN状態にも関わらず、ACKメッセージ以外を受信したば場合
					printf("## inconsistency protocol message recived ##\n");
					stat = 6;
					ipaddr.s_addr = dhcph_packet.address;
					printf("type: %d, code %d, ttl %d, IP %s ",
						   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
					ipaddr.s_addr = dhcph_packet.netmask;
					printf("netmask %s\n", inet_ntoa(ipaddr));
					printf("## stat changed: %s->%s ##\n", pre_stat, get_stat(stat));
					
				}
				
				
				
				break;
			}
			
			//完了
			
			
		}
	}
	
	
	if (close(s) < 0) {
		perror("close");
		exit(1);
	}
		  
	
	return 0;
}


char * get_stat(int st)
{
	if (st == 0) {
		return nl;
	}
	else if (st == 1) {
		return in;
	}

	else if (st == 2) {
		return wo;
	}
	
	else if (st == 3) {
		return wa;
	}
	else if (st == 4) {
		return iu;
	}
	else if (st == 5) {
		return wea;
	}
	else if (st == 6) {
		return ex;
	}

	else if (st == 7) {
		return weaa;
	}

	else if (st == 8) {
		return woa;
	}
	else if (st == 9) {
		return waa;
	}

}

void setitimer_ttl(int ttlcounter)
{
	memset(&timer, 0, sizeof(timer));
	timer.it_interval.tv_sec = ttlcounter;
	timer.it_interval.tv_usec = 0;
	timer.it_value = timer.it_interval;

	if (setitimer(ITIMER_REAL, &timer, NULL) < 0) {
		perror("setitimer");
		exit(1);
	}

}

void ttl_count_flag()
{
	ttl_flag = 1;
}

void ip_extension_msg()
{
	int count;
	dhcph_packet.code = 3; //延長コード
	dhcph_packet.type = 3;

	strcpy(pre_stat, get_stat(stat));

	if ((count = sendto(s, &dhcph_packet, sizeof(dhcph_packet), 0,
						(struct sockaddr *)&myskt, sizeof(myskt))) < 0) {
		perror("sendto");
		exit(1);
	}


	//REQUEST(extension)を送信しWAIT_EXT_ACKに遷移したため10秒タイムアウトを開始する

	if (gettimeofday(&start_time, NULL) < 0) {
		perror("gettimeofday");
		exit(1);
	}

	
	

	stat = 5; //WAIT_EXT_ACKに遷移
	
	printf("\n** REQUEST (extension) sent **\n");
	
	ipaddr.s_addr = dhcph_packet.address;
	printf("type: %d(ACK), code %d(SUCCESS), ttl %d, IP %s ",
		   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
	ipaddr.s_addr = dhcph_packet.netmask;
	printf("netmask %s\n", inet_ntoa(ipaddr));					
	printf("## stat changed: %s->%s ##\n", pre_stat, get_stat(stat));
	
	printf("\n-------------------------------------------\n");

	


}

void release_flag_set()
{
	if (stat == 4) {
		release_flag = 1;
	}
}

//HUPを受けっとたとき解放するIPアドレスを指定しサーバに送信
void release()
{
	int count;
	
	printf("SIGHUP received\n");
	printf("## Releasing IP address ##\n");
	printf("** RELEASE sent **\n");
	
	dhcph_packet.type = 5;
	dhcph_packet.code = 0;
	dhcph_packet.ttl = htons(0); //ネットワークオーダーに変換
	dhcph_packet.netmask = 0;

	if ((count = sendto(s, &dhcph_packet, sizeof(dhcph_packet), 0,
						(struct sockaddr *)&myskt, sizeof(myskt))) < 0) {
		perror("sendto");
		exit(1);
	}
	ipaddr.s_addr = dhcph_packet.address;
	printf("type: %d(RELEASE), code %d(SUCCESS), ttl %d, IP %s ",
		   dhcph_packet.type, dhcph_packet.code, ntohs(dhcph_packet.ttl), inet_ntoa(ipaddr));
	ipaddr.s_addr = dhcph_packet.netmask;
	printf("netmask %s\n", inet_ntoa(ipaddr));

	printf("## stat changed: %s->exiting ##\n", get_stat(stat));

}

void wait_event()
{
	int count;
	long t;

	if (gettimeofday(&current_time, NULL) < 0) {
		perror("gettimeofday");
		exit(1);		 
	}

	t = current_time.tv_sec - start_time.tv_sec;
	
	switch(stat) {
	case 2: //WAIT_OFFER

		if (t >= 10) {//timeout10sec
			printf("\n**WAIT_OFFER timeout!**\n");
			printf("\n**DISCOVER(AGAIN) message sent**\n");
			stat = 8; // WAIT_OFFER_AGAINに遷移
			printf("## stat changed: WAIT_OFFER->%s ##\n", get_stat(stat));
			printf("%s", ctime((time_t*)&current_time.tv_sec));

			//DSICOVER(AGAIN)msg再送
			if ((count = sendto(s, &dhcph_packet, sizeof(dhcph_packet), 0,
								(struct sockaddr *)&myskt, sizeof(myskt))) < 0) {
				perror("sendto");
				exit(1);
			}
			//DSICOVER(AGAIN)を送信しWAIT_OFFER_AGAINに遷移したため10秒タイムアウトを開始する			
			if (gettimeofday(&start_time, NULL) < 0) {
				perror("gettimeofday");
				exit(1);
			}
		   
		}
		
		
		break;
		
	case 3: //WAIT_ACK


		if (t >= 10) {//timeout10sec
			printf("\n**WAIT_ACK timeout!**\n");
			printf("\n**REQUEST(alloc:AGAIN) message sent**\n");
			stat = 9; // WAIT_ACK_AGAINに遷移
			printf("## stat changed: WAIT_ACK->%s ##\n", get_stat(stat));
			printf("%s", ctime((time_t*)&current_time.tv_sec));

			//REQUEST(alloc:AGAIN)msg再送
			if ((count = sendto(s, &dhcph_packet, sizeof(dhcph_packet), 0,
								(struct sockaddr *)&myskt, sizeof(myskt))) < 0) {
				perror("sendto");
				exit(1);
			}
			//REQUEST(alloc:AGAIN)を送信しWAIT_ACK_AGAINに遷移したため10秒タイムアウトを開始する			
			if (gettimeofday(&start_time, NULL) < 0) {
				perror("gettimeofday");
				exit(1);
			}
		   
		}


		
		break;
		
	case 5: //WAIT_EXT_ACK

		if (t >= 10) {//timeout10sec
			printf("\n**WAIT_EXT_ACK timeout!**\n");
			printf("\n**REQUEST(extention: AGAIN) message sent**\n");
			stat = 7; // WAIT_EXT_ACK_AGAINに遷移
			printf("## stat changed: WAIT_EXT_ACK->%s ##\n", get_stat(stat));
			printf("%s", ctime((time_t*)&current_time.tv_sec));

			//REQUEST(extension)msg再送
			if ((count = sendto(s, &dhcph_packet, sizeof(dhcph_packet), 0,
								(struct sockaddr *)&myskt, sizeof(myskt))) < 0) {
				perror("sendto");
				exit(1);
			}
			//REQUEST(extension:AGAIN)を送信しWAIT_EXT_ACK_AGAINに遷移したため10秒タイムアウトを開始する			
			if (gettimeofday(&start_time, NULL) < 0) {
				perror("gettimeofday");
				exit(1);
			}
		   
		}
		
		break;

	case 7: //WAIT_EXT_ACK_AGAIN

		if (t >= 10) {//timeout10sec
			printf("\n**WAIT_EXT_ACK_AGAIN timeout!**\n");
			stat = 6; // extingに遷移
			printf("## stat changed: WAIT_EXT_ACK_AGAIN->%s ##\n", get_stat(stat));
			printf("%s", ctime((time_t*)&current_time.tv_sec));
			exit(0);
		}

		
		break;

	case 8: //WAIT_OFFER_AGAIN

		if (t >= 10) {//timeout10sec
			printf("\n**WAIT_OFFER_AGAIN timeout!**\n");
			stat = 6; // extingに遷移
			printf("## stat changed: WAIT_OFFER_AGAIN->%s ##\n", get_stat(stat));
			printf("%s", ctime((time_t*)&current_time.tv_sec));
			exit(0);
		}


		break;

	case 9: //WAIT_ACK_AGAIN
		if (t >= 10) {//timeout10sec
			printf("\n**WAIT_ACK_AGAIN timeout!**\n");
			stat = 6; // extingに遷移
			printf("## stat changed: WAIT_ACK_AGAIN->%s ##\n", get_stat(stat));
			printf("%s", ctime((time_t*)&current_time.tv_sec));
			exit(0);
		}


		break;
		
	}
}
