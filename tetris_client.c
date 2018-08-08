#include<stdio.h>
#include<unistd.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<sys/time.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>
#include<fcntl.h>
#include "./keyboard/keyboard.h"
#define BC 7 //背景颜色 白
#define BW 10
#define BH 20
#define SLEEP_TIME 3000
int WAIT_TIME = SLEEP_TIME*3;
#define DEBUG 0 
#define DEBUG_RECEIVE_KEY 0
#define DEBUG_NEXT_SHAPE 0
#define DEBUG_RECEIVE_MESSAGE 0 
#define DEBUG_NEXT_SHAPE_TEST 0
pthread_mutex_t mutex;
pthread_cond_t cond;
int left_right = 0; //等于1表示要此操作方式，默认为0
int up_down = 0;

int SendNext = 0; //此标志为0表示自己不向服务器发送“next_shape”请求,为1表示自己应该向服务器发送该信息
int receive_up = 0;
int receive_down = 0;
int receive_left = 0; //此处定义是否收到服务器发来的控制键
int receive_right = 0;	//默认为0，表示没收到
int receive_shape_count = 0;
struct shape{
	int s[5][5];
};
int FC;
int _FC;
struct shape nextshape;

struct data{
	int x;
	int y;
	int score;
};

struct shape shape_arr[7] = {
	{ 0,0,0,0,0, 0,0,1,0,0, 0,1,1,1,0, 0,0,0,0,0, 0,0,0,0,0}, //T
	{ 0,0,0,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,0,1,0,0}, //I
	{ 0,0,0,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,0,1,1,0, 0,0,0,0,0}, //L
	{ 0,0,0,0,0, 0,0,1,0,0, 0,0,1,0,0, 0,1,1,0,0, 0,0,0,0,0}, //L的镜像
	{ 0,0,0,0,0, 0,1,1,0,0, 0,1,1,0,0, 0,0,0,0,0, 0,0,0,0,0}, //田
	{ 0,0,0,0,0, 0,1,1,0,0, 0,0,1,1,0, 0,0,0,0,0, 0,0,0,0,0}, //Z
	{ 0,0,0,0,0, 0,0,1,1,0, 0,1,1,0,0, 0,0,0,0,0, 0,0,0,0,0}  //S
};

int background[BH][BW] = {0};
int background_colour[BH][BW]; 
struct map_start{
	int x;
	int y;
};

struct shape_start{
	int x;
	int y;
};
struct start_pos{
	struct map_start map;
	struct shape_start shape;
};

struct start_pos pos;
struct shape shape;
struct data p;
int sock = 0; //此处的sock是对服务器的标识，全局唯一

void send_next_message()
{
	char buf[1024] = {0};
	memset(buf, 0x00, sizeof(buf));
	pthread_mutex_lock(&mutex);
	#if DEBUG_NEXT_SHAPE_TEST
	printf("。。。。。。。。。。。。。。。。。。。拿到锁。。。。。。。。。。。。。。。。。。\n");
	fflush(stdout);
	#endif
	sprintf(buf, "next_shape");
	write(sock, buf, strlen(buf));
	#if DEBUG_NEXT_SHAPE_TEST
	printf("已向服务器发送“next_shape”请求\n");
	fflush(stdout);
	#endif
	#if DEBUG_NEXT_SHAPE_TEST
	printf("等待接收next_shape.....\n");
	fflush(stdout);
	#endif
	pthread_cond_wait(&cond, &mutex);
	#if DEBUG_NEXT_SHAPE_TEST
	printf("已收到接收next_shape.....\n");
	fflush(stdout);
	#endif
	pthread_mutex_unlock(&mutex);
	#if DEBUG_NEXT_SHAPE_TEST
	printf("。。。。。。。。。。。。。。。。。。。。。释放锁。。。。。。。。。。。。。。。。。。。。。。。\n");
	fflush(stdout);
	#endif
	usleep(WAIT_TIME);
	pthread_mutex_lock(&mutex);
	#if DEBUG_NEXT_SHAPE_TEST
	printf("。。。。。。。。。。。。。。。。。。。拿到锁。。。。。。。。。。。。。。。。。。\n");
	fflush(stdout);
	#endif

	memset(buf, 0x00, sizeof(buf));
	sprintf(buf, "已收到next_shape\0");
	write(sock, buf, strlen(buf)+1);
	#if DEBUG_NEXT_SHAPE_TEST
	printf("已回复，等待对方的回复。。。\n");
	fflush(stdout);
	#endif
	pthread_cond_wait(&cond, &mutex);
	#if DEBUG_NEXT_SHAPE_TEST
	printf("对方已回复..\n");
	fflush(stdout);
	#endif
	#if DEBUG_NEXT_SHAPE_TEST
	printf("双方都收到了next_shape的信息\n");
	fflush(stdout);
	#endif
//	receive_shape_count = 0;
	pthread_mutex_unlock(&mutex);
	#if DEBUG_NEXT_SHAPE_TEST
	printf("。。。。。。。。。。。。。。。。。。。。。释放锁。。。。。。。。。。。。。。。。。。。。。。。\n");
	fflush(stdout);
	#endif

}
void init_game(struct start_pos *pos, struct data *p)
{
	int i = 0;
	pos->map.x = 20;
	pos->map.y = 10;
	pos->shape.x =  BW/2-3;
	pos->shape.y =  0;

	p->x = pos->shape.x;
	p->y = pos->shape.y;
	p->score = 0;

	for(i = 0; i<pos->map.y*2; i++)
		printf("\n");
	int j;
	for(i = 0; i<BH; i++)
	{
		for(j = 0; j<BW; j++)
			background_colour[i][j] = BC;
	}

	int ctl_mod[2] = {left_right, up_down};
	pthread_mutex_lock(&mutex);
	left_right = 0;
	up_down = 0;
	pthread_mutex_unlock(&mutex);

	send_next_message();

	pthread_mutex_lock(&mutex);
	left_right = ctl_mod[0];
	up_down = ctl_mod[1];
	pthread_mutex_unlock(&mutex);

}

void draw_element(int x, int y, int c)
{
	x *= 2;
	x++;
	y++;
	printf("\033[%d;%dH", y, x);
	printf("\033[4%dm\033[3%dm", c, c);
	printf("[]");
	printf("\033[?25l"); //隐藏光标
	printf("\033[0m");
	fflush(stdout); 
}

int check_canmove(int x, int y, struct shape p)
{
	int i = 0;
	int j = 0;
	for(i = 0; i<5; i++)
	{
		for(j = 0; j<5; j++)
		{
			if(p.s[i][j] == 0)
				continue;
			if(y+i >= BH)
				return 0;
			if(x+j < 0)
				return 0;
			if(x+j >= BW )
				return 0;
			if(background[y+i][x+j] != 0)
				return 0;
		}
	}
	return 1;
}

void draw_shape(struct data *p, struct shape shape, int c, struct start_pos *pos)
{
	int i = 0;
	int j = 0;
	for(i = 0; i<5; i++)
		for(j = 0; j<5; j++)
			if(shape.s[i][j] != 0)
				draw_element(p->x+j+pos->map.x, p->y+i+pos->map.y, c);
}

void draw_bk(struct start_pos *pos, struct data *p)
{
	int i = 0;
	int j = 0;
	for(i = 0; i<BH; i++)
	{
		for(j = 0; j<BW; j++)
		{
			if(background[i][j] == 0)
				draw_element(j+pos->map.x, i+pos->map.y, BC);
			else
				draw_element(j+pos->map.x, i+pos->map.y, background_colour[i][j]);

		}
	}

	printf("\033[%d;%dH\033[31m当前得分：%d", pos->map.x/2, (pos->map.y+BW/2*3)*2, p->score);
	if(up_down)
	{
		printf("\033[%d;%dH↑：改变形状", pos->map.x/3*2, (pos->map.y+BW+1)*3);
		printf("\033[%d;%dH↓：加速下降", pos->map.x/3*2+1, (pos->map.y+BW+1)*3);
	}
	if(left_right)
	{
		printf("\033[%d;%dH←：向左移动", pos->map.x/3*2+2, (pos->map.y+BW+1)*3);
		printf("\033[%d;%dH→：向右移动", pos->map.x/3*2+3, (pos->map.y+BW+1)*3);
	}
	printf("\033[%d;%dHCtrl + c: 结束游戏", pos->map.x/3*2+4, (pos->map.y+BW+1)*3);

	struct data tmp;
	tmp.x = pos->map.x/3 + 4;
	tmp.y = (pos->map.y - 2);
	int k = 0;
	for(k = 6; k<12; k++)
	{
		printf("\033[%d;%dH\033[K", pos->map.x/3*2 + k, (pos->map.y+BW+1)*3);
	}
	draw_shape(&tmp, nextshape, FC, pos);
}

void set_bk(struct data *p, struct shape shape)
{
	int i,j;
	for(i = 0; i<5; i++)
	{
		for(j = 0; j<5; j++)
		{
			if(shape.s[i][j] != 0)
			{ //有东西，就把背景的位置置1, 把颜色置为前景色
				background[p->y + i][p->x + j] = 1;
				background_colour[p->y + i][p->x + j] = _FC;
			}
		}
	}
}

void clean_line(struct data *p)
{
	int i = 0;
	int j = 0;
	int total;
	int level = 0;
	for(i; i<BH; i++)
	{
		total = 0;
		for(j = 0; j<BW; j++)
		{
			if(background[i][j] != 0)
				total++;
		}
		if(total == BW)
		{
			int k = 0;
			level++;
			p->score = p->score + level*2;
			for(k = i; k>0; k--)
			{
				memcpy(background[k], background[k-1], sizeof(background[0]));
				memcpy(background_colour[k], background_colour[k-1], sizeof(background_colour[0]));
			}
			memset(background[0], 0, sizeof(background[0]));
			int z;
			for(z = 0; z<BW; z++)
			{
				background_colour[0][z] = BC;
			}
		}
	}
}
int read_count = 0;
void next_shape_test(struct shape *shape)
{
	int ctl_mod[2] = {left_right, up_down};
	char buf[1024] = {0};
	#if DEBUG_NEXT_SHAPE
	read_count ++;
	printf("第%d次在next_shape处申请锁\n", read_count);
	fflush(stdout);
	#endif
	pthread_mutex_lock(&mutex);
	#if DEBUG_NEXT_SHAPE
	printf("在屏蔽按键时拿到锁\n");
	fflush(stdout);
	#endif
	left_right = 0;
	up_down = 0;
	*shape = nextshape;
	_FC = FC;
	pthread_mutex_unlock(&mutex);
	#if DEBUG_NEXT_SHAPE
	printf("在屏蔽按键后释放锁\n");
	fflush(stdout);
	#endif

	send_next_message();

	pthread_mutex_lock(&mutex);
	#if DEBUG_NEXT_SHAPE_TEST
	printf("。。。。。。。。。。。。。。。。。。。next_shape_test恢复按键时拿到锁。。。。。。。。。。。。。。。。。。\n");
	fflush(stdout);
	#endif
	left_right = ctl_mod[0];
	up_down = ctl_mod[1];
	pthread_mutex_unlock(&mutex);
	#if DEBUG_NEXT_SHAPE_TEST
	printf("。。。。。。。。。。。。。。。。。。。next_shape_test恢复按键后释放锁。。。。。。。。。。。。。。。。。。\n");
	fflush(stdout);
	#endif
}
void show_game_over(struct data *p)
{
	
	printf("\033[10A\033[2J");
	printf("你们的分数为%d\n", p->score);
	printf("按回车键键继续\n");
	fflush(stdout);
	getchar();	
	printf("\033[?25h\033[0m");
}
void check_gameover(struct start_pos *pos, struct data * p)
{
	int i = 0;
	for(i = 0; i<BW; i++)
	{
		if(background[3][i] == 1)
		{
			printf("\033[%d;%dH", pos->map.x+BW*2, pos->map.y+BH*2);
			printf("\033[0m");
			printf("\033[?25h\033[2C你输了\n");
			recover_keyboard();
			fflush(stdout); 
			sleep(1);
			show_game_over(p);
			exit(0);
		}
	}

}


void tetris_timer(struct data *p, struct shape *shape, struct start_pos *pos)
{
	draw_shape(p, *shape, BC, pos);
	if(check_canmove(p->x, p->y+1, *shape))
	{
		p->y++;
		draw_shape(p, *shape, _FC, pos);
	}
	else
	{
		set_bk(p, *shape);
		draw_shape(p, *shape, _FC, pos);
		clean_line(p);
		next_shape_test(shape);
		draw_bk(pos, p);
		check_gameover(pos, p);
		p->y = pos->shape.y;
		p->x = pos->shape.x;
	}
//	printf("time=%d\n", time(NULL));
}
static void swap(int *a, int *b)
{
	int c = *a;
	*a = *b;
	*b = c;
}
struct shape turn_clockwise(struct shape shape)
{
	int i = 0;
	int j = 0;
	struct shape tmp;
	for(i = 0; i<5; i++)
	{
		for(j = 0; j<5; j++)
		{
			tmp.s[j][4-i] = shape.s[i][j];
		}
	}
	return tmp;
}

struct shape turn_unclockwise(struct shape shape)
{
	int i = 0;
	int j = 0;
	struct shape tmp;
	for(i = 0; i<5; i++)
	{
		for(j = 0; j<5; j++)
		{
			tmp.s[4-j][i] = shape.s[i][j];
		}
	}
	return tmp;
}

void move_by_player(struct data *p, struct shape *shape, struct start_pos *pos)
{
	if(left_right != 1 && up_down != 1)
		return;

	int n = get_key();
	//在控制方式中，1代表上，2代表下，3代表左，4代表右
	//每当键盘检测到该按键时，在处理自己按键动作的同时，还向服务器发送对应的数字
	//若是服务器发来的键，就只响应相应的动作
	if(is_left(n) && left_right == 1 || receive_left == 1)
	{
		#if DEBUG
		if(receive_left == 1)
			printf("receive_left == 1");
		if(is_left(n))
			printf("is_left");
		fflush(stdout);
		#endif
		if(check_canmove(p->x-1, p->y, *shape))
		{
			draw_shape(p, *shape, BC, pos);
			p->x--;
			draw_shape(p, *shape, _FC, pos);
			#if DEBUG
			printf("键盘left申请互斥锁\n");
			fflush(stdout);
			#endif
			pthread_mutex_lock(&mutex);
			#if DEBUG
			printf("键盘left申请到了互斥锁\n");
			fflush(stdout);
			#endif
			if(receive_left == 0)  //等于0就意味着这个键是本客户按键产生的
			{
				char buf[2];
				buf[0] = '3';
				buf[1] = 0;
				write(sock, buf, strlen(buf));
			}
			pthread_mutex_unlock(&mutex);

			#if DEBUG
			printf("键盘left释放了互斥锁\n");
			fflush(stdout);
			#endif

		}
		receive_left = 0;
		usleep(SLEEP_TIME<<2);
	}
	else if(is_right(n) && left_right == 1 || receive_right == 1)
	{
		#if DEBUG
		if(receive_right == 1)
			printf("receive_right == 1");
		if(is_right(n)) 
			printf("is_right");
		fflush(stdout);
		#endif
		if(check_canmove(p->x+1, p->y, *shape))
		{
			draw_shape(p, *shape, BC, pos);
			p->x++;
			draw_shape(p, *shape, _FC, pos);

			#if DEBUG
			printf("键盘right申请互斥锁\n");
			fflush(stdout);
			#endif
			pthread_mutex_lock(&mutex);
			#if DEBUG
			printf("键盘right申请到了互斥锁\n");
			fflush(stdout);
			#endif
			if(receive_right == 0)
			{
				char buf[2];
				buf[0] = '4';
				buf[1] = 0;
				write(sock, buf, strlen(buf));
			}
			pthread_mutex_unlock(&mutex);

			#if DEBUG
			printf("键盘left释放了互斥锁\n");
			fflush(stdout);
			#endif
		}
		receive_right = 0;
		usleep(SLEEP_TIME<<2);
	}
	else if(is_down(n) && up_down == 1 || receive_down == 1)
	{
		#if DEBUG
		if(receive_down == 1)
			printf("receive_down == 1");
		if(is_down(n)) 
			printf("is_down");
		fflush(stdout);
		#endif
		if(check_canmove(p->x, p->y+1, *shape))
		{
			draw_shape(p, *shape, BC, pos);
			p->y++;
			draw_shape(p, *shape, _FC, pos);

			#if DEBUG
			printf("键盘down申请互斥锁\n");
			fflush(stdout);
			#endif
			pthread_mutex_lock(&mutex);
			#if DEBUG
			printf("键盘down申请到了互斥锁\n");
			fflush(stdout);
			#endif
			if(receive_down == 0)
			{
				char buf[2];
				buf[0] = '2';
				buf[1] = 0;
				write(sock, buf, strlen(buf));
			}
			pthread_mutex_unlock(&mutex);
			#if DEBUG
			printf("键盘down释放了互斥锁\n");
			fflush(stdout);
			#endif
		}
		receive_down = 0;
		usleep(SLEEP_TIME<<2);
	}
	else if(is_up(n) && up_down == 1 || receive_up == 1)
	{
		#if DEBUG
		if(receive_up == 1)
			printf("receive_up == 1");
		if(is_up(n))
			printf("is_up");
		fflush(stdout);
		#endif
		draw_shape(p, *shape, BC, pos);
		*shape = turn_clockwise(*shape);
		if(check_canmove(p->x, p->y, *shape) == 0) //不能旋转，则转回
		{
			*shape = turn_unclockwise(*shape);
			draw_shape(p, *shape, _FC, pos);	
		}
		else //能旋转则转
		{ 
			draw_shape(p, *shape, _FC, pos);	
			
			#if DEBUG
			printf("键盘up申请互斥锁\n");
			fflush(stdout);
			#endif
			pthread_mutex_lock(&mutex);
			#if DEBUG
			printf("键盘up申请到了互斥锁\n");
			fflush(stdout);
			#endif
			if(receive_up == 0)
			{
				char buf[2];
				buf[0] = '1';
				buf[1] = 0;
				write(sock, buf, strlen(buf));
			}
			usleep(SLEEP_TIME<<2);
			pthread_mutex_unlock(&mutex);
			#if DEBUG
			printf("键盘up释放了互斥锁\n");
			fflush(stdout);
			#endif
		}
		receive_up = 0;
	}
}


void handler(int s)
{
	tetris_timer(&p, &shape, &pos);
}

void handler_int(int s)
{
	printf("\033[%d;%dH", BW*5, BH*5);		
	printf("\033[0m");
	printf("\033[?25h\033[2C\n");
	recover_keyboard();
	fflush(stdout); 
	exit(0);
}

//在控制方式中，1代表上，2代表下，3代表左，4代表右
void receive_message(int sock)
{
	int count = 0;
	char buf[1024];
	ssize_t s = 0;
	while(1)
	{
		memset(buf, 0x00, sizeof(buf));
		#if DEBUG_RECEIVE_MESSAGE 
		printf("接信息时申请锁\n");
		#endif
		pthread_mutex_lock(&mutex);
		#if DEBUG_RECEIVE_MESSAGE 
		printf("接信息时拿到锁\n");
		fflush(stdout);
		#endif

		fcntl(sock, F_SETFL,fcntl(sock, F_GETFL) | O_NONBLOCK); //将套接字设为非阻塞模式
		#if DEBUG_RECEIVE_MESSAGE
		printf("开始读。。。\n");
		#endif
		s = 0;
		s = read(sock, buf, sizeof(buf));
		fcntl(sock, F_SETFL, 0); //还原为阻模式
		buf[s] = 0;
		#if DEBUG_RECEIVE_MESSAGE
		printf("读到”%s“。。。s=%d\n", buf, s);
		#endif
		if(strlen(buf) == 2)
		{
			nextshape = shape_arr[buf[0] - '0'];
			FC = buf[1] - '0';
			#if DEBUG_NEXT_SHAPE_TEST
			printf("已设置形状和颜色，buf为%s\n", buf);
			fflush(stdout);
			#endif
		}
		else if(strcmp(buf, "1") == 0)
		{
			receive_up = 1;
			#if DEBUG_RECEIVE_KEY
			printf("接收到up\n");
			fflush(stdout);
			#endif
		}
		else if(strcmp(buf, "2") == 0)
		{
			receive_down = 1;
			#if DEBUG_RECEIVE_KEY
			printf("接收到down\n");
			fflush(stdout);
			#endif
		}
		else if(strcmp(buf, "3") == 0)
		{
			receive_left = 1;
			#if DEBUG_RECEIVE_KEY
			printf("接收到left\n");
			fflush(stdout);
			#endif
		}
		else if(strcmp(buf, "4") == 0)
		{
			receive_right = 1;
			#if DEBUG_RECEIVE_KEY
			printf("接收到right\n");
			fflush(stdout);
			#endif
		}

		if(strlen(buf) == 2)
		{
			pthread_cond_signal(&cond);
		}
		else if(strcmp(buf, "对方已收到next_shape") == 0)
		{
			pthread_cond_signal(&cond);
			#if DEBUG_NEXT_SHAPE_TEST
			printf("接收到“对方已收到next_shape”\n");
			fflush(stdout);
			#endif
		}
		pthread_mutex_unlock(&mutex);
		#if DEBUG_RECEIVE_MESSAGE
		printf("接信息时释放锁\n");
		fflush(stdout);
		#endif
		usleep(SLEEP_TIME);
	}
}

typedef struct Arg{
	int sock;
}Arg;
void * ReceiveKey(void *ptr)
{
	Arg *arg = (Arg * )ptr;
//	receive_key(arg->sock);
	receive_message(arg->sock);
	free(arg);
	return NULL;
}
void start_game()
{
	srand(time(NULL));
	pthread_t tid = 0;
	Arg * arg = (Arg *)malloc(sizeof(Arg));
	arg->sock = sock;
	pthread_create(&tid, NULL, ReceiveKey, (void *)arg);
	pthread_detach(tid);	

	init_keyboard();
	init_game(&pos, &p);
	next_shape_test(&shape);
	draw_bk(&pos, &p);

	struct sigaction act;
	act.sa_handler = handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGALRM, &act, NULL);

	signal(SIGINT, handler_int);
	signal(SIGQUIT, SIG_IGN);

	struct itimerval it;
	it.it_value.tv_sec = 0;
	it.it_value.tv_usec = 1;
	it.it_interval.tv_sec = 1;
	it.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL, &it, NULL);

	while(1)
	{
		move_by_player(&p, &shape, &pos);
	;
	}
	recover_keyboard();
}
void set_control_mode()
{
	int c = 0;
	char buf[1025];
	while(1)
	{
		memset(buf, 0x00, sizeof(buf));
		ssize_t s = read(sock, buf, sizeof(buf));
		buf[s] = 0;
		printf("%s", buf);
		fflush(stdout);
		if(strcmp(buf, "请选择操作方式:\n") == 0)
		{
			printf("选择控制上下请输入1\n");
			printf("选择控制左右请输入2\n");
			fflush(stdout);
			SendNext = 1;
			char c_buf = '\0';
			scanf("%c", &c_buf);
			while( (c = getchar()) != '\n' && c != EOF); //清除键盘缓冲区
			while(c_buf != '1' && c_buf != '2')
			{
				printf("选择有误，请重新选择\n");
				fflush(stdout);
				scanf("%c", &c_buf);
				while( (c = getchar()) != '\n' && c != EOF); //清除键盘缓冲区
			}

			write(sock, &c_buf, sizeof(c_buf));
			break;
		}
		else if(strcmp(buf, "与服务器连接成功，等待对方选择操作方式，请稍后。。。\n") == 0)
		{
			SendNext = 0;
			break;
		}
	}
	memset(buf, 0x00, sizeof(buf));
	ssize_t sr = read(sock, buf, sizeof(buf));
	buf[sr] = 0;
	if(strcmp(buf, "up_down") == 0)
	{
		up_down = 1;
		left_right = 0;
		printf("已设置控制方式为：控制上下\n");
	}
	else if(strcmp(buf, "left_right") == 0)
	{
		up_down = 0;
		left_right = 1;
		printf("已设置控制方式为：控制左右\n");
	}
	else
	{
		printf("设置控制方式失败\n");
	}
	fflush(stdout);
	sleep(2);

}

int main(int argc, char *argv[]) 
{
	if(argc !=3 )
	{
		printf("Usage: %s [ip] [port]\n", argv[0]);
		return 1;	//参数错误，退出码为1
	}
	sock = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(argv[2]));
	server.sin_addr.s_addr = inet_addr(argv[1]);

	if(connect(sock, (struct sockaddr *)(&server), sizeof(server)) < 0)
	{
		perror("connect");
	}

	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
	set_control_mode();

	start_game();

	close(sock);
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
	return 0;
}
