#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<pthread.h>
#include<fcntl.h>
#define BC 7 //背景颜色 白
#define SLEEP_TIME 500
#define IP 120.79.49.217
#define PORT 8080
pthread_mutex_t mutex;
typedef struct Arg{
	int client1_sock;
	int client2_sock;
}Arg;

int connection_count = 0;
int request_count = 0;
int sock1_request_next_shape = 0;
int sock2_request_next_shape = 0;;
int sock1 = 0;
int sock2 = 0;
int next_shape(int client1_sock, int client2_sock)
{
//	if(request_count < 2)
//		return -1;
	if(!sock1_request_next_shape || !sock2_request_next_shape)
		return -2;
	char buf[3];
	int n = rand()%7;
	int c = BC;
	while(c == BC || c == 0)
	{
		c = rand()%8;
	}
	buf[0] = n + '0';//形状
	buf[1] = c + '0';//颜色
	buf[2] = '\0'; //作为字符串结尾的填充

	write(client1_sock, buf, strlen(buf)+1);
	printf("已向%d发送next_shape的信息:%s\n", client1_sock, buf);
	fflush(stdout);
	write(client2_sock, buf, strlen(buf)+1);
	printf("已向%d发送next_shape的信息:%s\n", client2_sock, buf);
	fflush(stdout);
	request_count = 0;
	sock1_request_next_shape = 0;
	sock2_request_next_shape = 0;

	return 0;
} 
void service(int src_sock, int des_sock)
{
	char buf[1025];
	while(1)
	{
		memset(buf, 0x00, sizeof(buf));
		int s = read(src_sock, buf, sizeof(buf)-1);
		printf("收到%d的“%s”请求\n", src_sock,  buf);
		fflush(stdout);
		if(s == 0)
		{
			printf("%d断开连接\n", src_sock);
			close(src_sock);
			--connection_count;
			break;
		}
		else if(s < 0)
		{
			continue;
		}
		buf[s] = 0;
		printf("%d申请互斥锁\n", src_sock);
		pthread_mutex_lock(&mutex);
		printf("%d拿到了互斥锁\n", src_sock);
		printf("正在等待%d的“%s”请求\n", src_sock, buf);
		if(strcmp(buf, "next_shape") == 0)
		{
			request_count++;
			if(src_sock == sock1)
			{
				sock1_request_next_shape = 1;
			}
			else if(src_sock == sock2)
			{
				sock2_request_next_shape = 1;
//				pthread_mutex_unlock(&mutex);
//				continue;
			}
			else
			{
				sock1_request_next_shape = 0;
				sock2_request_next_shape = 0;
			}
			int result = next_shape(src_sock, des_sock); 
			if(result < 0)
				printf("第一个nest_shape请求到来, 来自%d\n", src_sock);
			else
				printf("第二个next_shape请求到来，来自%d， 且完成回复\n", src_sock);

			pthread_mutex_unlock(&mutex);
			printf("%d释放了锁\n", src_sock);
			fflush(stdout);
		}
		//在控制方式中，1代表上，2代表下，3代表左，4代表右
		else if(strcmp(buf, "1") == 0 || strcmp(buf, "2") == 0 || strcmp(buf, "3") == 0 || strcmp(buf, "4") == 0)
		{
			write(des_sock, buf, strlen(buf));
			printf("完成从%d到%d的按键转发\n", src_sock, des_sock);
			pthread_mutex_unlock(&mutex);
			printf("%d释放了锁\n", src_sock);
			fflush(stdout);
		}
		else if(strcmp(buf, "已收到next_shape") == 0)
		{
			memset(buf, 0x00, sizeof(buf));
			sprintf(buf, "对方已收到next_shape");
			ssize_t s = write(des_sock, buf, strlen(buf)+1);
			printf("已收到%d的回复信息，并完成向%d的转发,长度为%d字节, 内容为“%s”\n", src_sock, des_sock, s, buf);
			pthread_mutex_unlock(&mutex);
		}
		else 
		{
			if(buf[0] != '\0')
			{
				printf("收到%d的未知报文:%s\n", src_sock, buf);
			}
			else printf("收到空报文\n");

			fflush(stdout);
			pthread_mutex_unlock(&mutex);
			printf("%d释放了锁\n", src_sock);
		}
		usleep(SLEEP_TIME);
	}
}

void *Service(void *ptr)
{
	Arg *arg = (Arg *)ptr;
	service(arg->client1_sock, arg->client2_sock);
	free(arg);
	return NULL;
}
int main(int argc, char *argv[])
{
	srand(time(NULL));
	if(argc != 1)
	{
		printf("Usage: %s \n", argv[0]);
		return 1;
	}
	pthread_mutex_init(&mutex, NULL);
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0)
	{
		perror("socket");
		return 2;
	}
	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.s_addr = INADDR_ANY;

	int opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if(bind(sock, (struct sockaddr *)(&server), sizeof(server)) < 0)
	{
		perror("bind");
		return 3;
	}

	if(listen(sock, 5) < 0)
	{
		perror("sock");
		return 4;
	}

	struct sockaddr_in client;
	int client_sock[2];
	while(1)
	{
		socklen_t len = sizeof(client); 
		client_sock[connection_count] = accept(sock, (struct sockaddr *)&client, &len);
		if(client_sock < 0)
		{
			continue;
		}
		printf("已有%d位玩家连接\n", connection_count + 1);
		char buf[1025];
		if(connection_count == 0)
		{
			memset(buf, 0x00, sizeof(buf));
			sprintf(buf, "与服务器连接成功\n");
			write(client_sock[0], buf, strlen(buf));

			memset(buf, 0x00, sizeof(buf));
			sprintf(buf, "等待对方连接，请稍后。。。\n");
			write(client_sock[0], buf, strlen(buf));
		}
		if(connection_count == 1)
		{
			memset(buf, 0x00, sizeof(buf));
			sprintf(buf, "与服务器连接成功，等待对方选择操作方式，请稍后。。。\n");
			write(client_sock[1], buf, strlen(buf));
			printf("已向玩家2发送：%s", buf);

			memset(buf, 0x00, sizeof(buf));
			sprintf(buf, "请选择操作方式:\n");
			write(client_sock[0], buf, strlen(buf));
			printf("已向玩家1发送:%s", buf);
			fflush(stdout);

			memset(buf, 0x00, sizeof(buf));
			ssize_t s = read(client_sock[0], buf, sizeof(buf));	
			buf[s] = 0;
			printf("玩家1选择：%s\n", buf);
			if(strcmp(buf, "1") == 0)	//玩家1选择控制上下
			{
				memset(buf, 0x00, sizeof(buf));
				sprintf(buf, "up_down");
				write(client_sock[0], buf, strlen(buf));

				memset(buf, 0x00, sizeof(buf));
				sprintf(buf, "left_right");
				write(client_sock[1], buf, strlen(buf));

				printf("向客户端设置操作方式成功\n");
			}
			else if(strcmp(buf, "2") == 0) //玩家1选择控制左右
			{
				memset(buf, 0x00, sizeof(buf));
				sprintf(buf, "left_right");
				write(client_sock[0], buf, strlen(buf));

				memset(buf, 0x00, sizeof(buf));
				sprintf(buf, "up_down");
				write(client_sock[1], buf, strlen(buf));

				printf("向客户端设置成功\n");
			}
			else
			{
				printf("向客户端设置控制方式失败\n");
			}
		}
		connection_count ++;
		if(connection_count >= 2)
			break;
	}

	sock1 = client_sock[0];
	sock2 = client_sock[1];

	Arg *arg1 = (Arg *)malloc(sizeof(Arg));
	arg1->client1_sock = client_sock[0];
	arg1->client2_sock = client_sock[1];
	pthread_t tid1 = 0;
	pthread_create(&tid1, NULL, Service, (void *)arg1);
	pthread_detach(tid1);
	printf("线程1创建成功\n");
	fflush(stdout);	

	Arg *arg2 = (Arg *)malloc(sizeof(Arg));
	arg2->client1_sock = client_sock[1];
	arg2->client2_sock = client_sock[0];
	pthread_t tid2 = 0;
	pthread_create(&tid2, NULL, Service, (void *)arg2);
	pthread_detach(tid2);
	printf("线程2创建成功\n");
	fflush(stdout);	
	
	while(connection_count)
	{
		;		
	}
	pthread_mutex_destroy(&mutex);
	close(sock);
	return 0;
}
