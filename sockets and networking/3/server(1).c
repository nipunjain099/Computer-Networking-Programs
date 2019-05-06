#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
struct client_info {
	int sockno;
	char ip[INET_ADDRSTRLEN];
};
int ol=0;
int clients[100];
int n = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void sendtoallingrp(char *msg,int curr,char grpcode)
{
	int i;
	pthread_mutex_lock(&mutex);
	for(i = 0; i < n; i++) {
		if(clients[i] != curr&&online[i][4]==grpcode) {
			if(send(clients[i],msg,strlen(msg),0) < 0) {
				perror("sending failure");
				continue;
			}
		}
	}
	pthread_mutex_unlock(&mutex);
}
void sendtopersonal(char *msg,int curr)
{
	char tosend[5];
	for(int i=0;i<4;i++)
	{
		tosend[i]=msg[i];
	}
	tosend[4]='\0';
	int i;
	pthread_mutex_lock(&mutex);
	for(i = 0; i < n; i++) {
		if(clients[i] != curr&&online[i]==tosend) {
			if(send(clients[i],msg,strlen(msg),0) < 0) {
				perror("sending failure");
				continue;
			}
		}
	}
	pthread_mutex_unlock(&mutex);
}
void *recvmg(void *sock,char *online,int ol)
{
	struct client_info cl = *((struct client_info *)sock);
	char *final_msg=online[ol];
	char msg[500];
	int len;
	int i;
	int j;
	while((len = recv(cl.sockno,msg,500,0)) > 0) {
		msg[len] = '\0';
		strcat(final_msg,msg);
		if(msg[0]=='#')
		{
			//strcat(final_msg,msg);
			sendtoallingrp(final_msg,cl.sockno,msg[3]);
		}
		else
		{
			sendtopersonal(final_msg,cl.sockno)
		}
		//sendtoall(msg,cl.sockno);
		memset(msg,'\0',sizeof(msg));
	}
	pthread_mutex_lock(&mutex);
	printf("%s disconnected\n",cl.ip);
	for(i = 0; i < n; i++) {
		if(clients[i] == cl.sockno) {
			j = i;
			while(j < n-1) {
				clients[j] = clients[j+1];
				j++;
			}
		}
	}
	n--;
	pthread_mutex_unlock(&mutex);
}
int main(int argc,char *argv[])
{
	struct sockaddr_in my_addr,their_addr;
	int my_sock;
	int their_sock;
	socklen_t their_addr_size;
	int portno;
	pthread_t sendt,recvt;
	char msg[500];
	char online[100][5];
	int len;
	struct client_info cl;
	char ip[INET_ADDRSTRLEN];;
	;
	if(argc > 2) {
		printf("too many arguments");
		exit(1);
	}
	portno = atoi(argv[1]);
	my_sock = socket(AF_INET,SOCK_STREAM,0);
	memset(my_addr.sin_zero,'\0',sizeof(my_addr.sin_zero));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(portno);
	my_addr.sin_addr.s_addr = inet_addr("192.168.70.59");
	their_addr_size = sizeof(their_addr);

	if(bind(my_sock,(struct sockaddr *)&my_addr,sizeof(my_addr)) != 0) {
		perror("binding unsuccessful");
		exit(1);
	}

	if(listen(my_sock,5) != 0) {
		perror("listening unsuccessful");
		exit(1);
	}

	while(1) {
		if((their_sock = accept(my_sock,(struct sockaddr *)&their_addr,&their_addr_size)) < 0) {
			perror("accept unsuccessful");
			exit(1);
		}
		pthread_mutex_lock(&mutex);
		inet_ntop(AF_INET, (struct sockaddr *)&their_addr, ip, INET_ADDRSTRLEN);
		printf("%s connected\n",ip);
		cl.sockno = their_sock;
		strcpy(cl.ip,ip);
		clients[n] = their_sock;
		n++;
		if((len = recv(cl.sockno,msg,500,0)) > 0) {	
		msg[len] = '\0';
		strcpy(online[ol],msg);
		sendtoall(strcat(msg+" is online "),cl.sockno);
		memset(msg,'\0',sizeof(msg));
		}
		pthread_create(&recvt,NULL,recvmg,&cl,online,ol);
		ol++;
		pthread_mutex_unlock(&mutex);
	}
	return 0;
}