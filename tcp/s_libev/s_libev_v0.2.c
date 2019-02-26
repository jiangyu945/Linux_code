/*
 ============================================================================
 Name        : s_libev.c
 Author      : Greein-jy
 Version     : v1.0
 date        : 2018-11-19
 Description : 接收格式由十六进制改为字符型，并且调整请求发送与接收触发无关
 Question    : 客户端断开，再次重连后，定时器定时出误差
 ============================================================================
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#include <signal.h>
#include <time.h>

#include <sys/epoll.h>
#include <errno.h>

#include "ev.h"

 //本机端口
 #define MYPORT 8080

//远端服务器IP 端口
#define SERVER_IP  "192.168.31.41"
#define PORT_S        8887
 
#define REQ_FRAME_SIZE    9      //数据请求帧长度

//传感器数据请求格式
#define HEAD    0xAA        
#define LEN     0x09
#define OPTION  0x00
#define CMD     0X01
#define DATA1   0x0f      //数据域
#define DATA2   0xff      //数据域
#define TAIL    0x55

typedef  unsigned char  uchar;

int c_fd,listenfd;
uchar req_buf[REQ_FRAME_SIZE] = {0};

typedef struct mytimer{
    int fd;
}my_timer;

//CRC校验函数
unsigned short  CRC_Compute(uchar* snd,uchar len)
{
	auto int i, j;
	auto unsigned short  c,crc=0xFFFF;
	for(i = 0;i < len; i ++){
		c = snd[i] & 0x00FF;
		crc ^= c;
		for(j = 0;j < 8; j ++){
			if (crc & 0x0001){
				crc>>=1;
				crc^=0xA001;
			}else crc>>=1;
		}
	}
	return (crc);
}

/*********************************************************************************
C prototype : void StrToHex(unsigned char *pbDest, unsigned char *pbSrc, int nLen)
parameter(s): [OUT] pbDest - 输出缓冲区
[IN] pbSrc - 字符串
[IN] nLen - 16进制数的字节数(字符串的长度/2)
return value: 
remarks : 将字符串转化为16进制数
**********************************************************************************/
void StrToHex(unsigned char *pbDest, unsigned char *pbSrc, int nLen)
{
    char h1,h2;
    unsigned char s1,s2;
    int i;

    for(i=0; i<nLen; i++)
    {
        h1 = pbSrc[2*i];
        h2 = pbSrc[2*i+1];

        s1 = h1 - 0x30;
        if (s1 > 9) 
        s1 -= 7;

        s2 = h2 - 0x30;
        if (s2 > 9) 
        s2 -= 7;

        pbDest[i] = s1*16 + s2;
    }
}

void sock_init()
{
    //socket init（As a client）
    c_fd = socket( AF_INET, SOCK_STREAM, 0 );
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
 
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT_S);
	servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if(connect(c_fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("connect fialed");
		return;
	}
	printf("connected server!\n");


    //socket init（As a server）
    struct sockaddr_in saddr;

	listenfd = socket( AF_INET, SOCK_STREAM, 0 );
   
	memset(&saddr, 0, sizeof(saddr) );
	saddr.sin_family = AF_INET;
    saddr.sin_port = htons(MYPORT); 
	saddr.sin_addr.s_addr = htonl( INADDR_ANY ); //any address
 
	bind( listenfd, (struct sockaddr *)&saddr, 16 );
	listen( listenfd, 30 );

    //设置fd可重用  
    int opt=1;  
    if(setsockopt(listenfd,SOL_SOCKET ,SO_REUSEADDR,(const char*)&opt,sizeof(opt)) != 0)  
    {  
        printf("setsockopt error in reuseaddr[%d]\n", listenfd);  
        return ;  
    }  
	printf( "Accepting client connections ... \n" );
}


//请求帧构造函数
void req_data_frame()
{	
    req_buf[0]=HEAD;
	req_buf[1]=LEN;
	req_buf[2]=OPTION;
	req_buf[3]=CMD;
	req_buf[4]=DATA1;
	req_buf[5]=DATA2;

	unsigned short crc16 = CRC_Compute(req_buf,(uchar)6);
	req_buf[6] = (uchar)crc16;
    req_buf[7] = (uchar)(crc16>>8);
	req_buf[8]=TAIL;
}


int handle_data(int connfd)
{
    uchar recv_tmp[256] = {0};
    uchar str_buf[256] = {0};
    uchar hex_buf[256] = {0};
    uchar send_buf[256] = {0};
	int nByte,get_complete_flag=0;
	int i,len,count=0;

    while(1)
    {
        /***********************************接收数据*******************************************************************/  
        nByte = recv( connfd, recv_tmp, sizeof(recv_tmp), MSG_DONTWAIT); 
        //printf("nByte = %d\n",nByte);
        if(nByte>0)           
        {
            for(i=0;i<nByte;i++)
            { 
                if(recv_tmp[i] == '\0'){
                    get_complete_flag = 1;
                    len = count/2;
                    printf("len = %d\n",len);
                    break;
                }
                else{
                    strncpy(&str_buf[count],&recv_tmp[i],1);
                    count++;
                }
            }

            if(get_complete_flag == 1)                         												         	  
            {   
                /*打印字符串格式数据*/
                printf("str_buf = %s\n",str_buf);

                /*字符串转十六进制*/
                StrToHex(hex_buf,str_buf,len);

                unsigned short result = CRC_Compute(hex_buf,(uchar)(len-3));
                uchar result_1 = (uchar)result;
                uchar result_2 = (uchar)(result>>8);
            
                if( (hex_buf[0] == HEAD) && (hex_buf[1] == len) && (hex_buf[len-3] == result_1) && (hex_buf[len-2] == result_2) && (hex_buf[len-1] == TAIL) )
                {	
                /*************************************校验通过，数据处理**************************************************************/																				    		
                    /*打印十六进制数据*/
                    int k;
                    printf("hex_buf = ");
                    for(k=0;k<len;k++){
                        printf("%02x", hex_buf[k]);
                    }
                    printf("\n");

                    //1.添加时间戳
                    struct tm *ptr;
                    time_t t;
                    uchar strtime[20];
                    time(&t);
                    ptr = localtime(&t);
                    strftime(strtime,sizeof(strtime),"%Y-%m-%d %H:%M:%S",ptr);

                    //2.填充数据到send_buf
                        /*填充时间*/
                    memcpy(send_buf,strtime,19);
                        /*填充数据*/
                    memcpy(&send_buf[19],str_buf,len*2);
                
                    //3.发送数据到远程服务端
                    send(c_fd,send_buf,len*2+19,MSG_DONTWAIT);
                    memset(str_buf,0,sizeof(str_buf));
                    memset(hex_buf,0,sizeof(hex_buf));
                    memset(send_buf,0,sizeof(send_buf));
                    return 0;
                }
                else
                { 
                    //校验失败，丢弃本帧数据
                    printf("Check failed!\n");
                    memset(send_buf,0,sizeof(send_buf));
                    return 0;
                }
                
            }
            else{
                memset(send_buf,0,sizeof(send_buf));
                printf("The data length is failed!\n");
                return -2;
            }    
        } 
        else if(nByte == 0)    //连接已断开
        {	
            printf("Connect close!!!\n");												
            close(connfd);
            break;
        }
        else{  //-1,出错
            //errno=11(EAGAIN),表示在非阻塞模式下调用了阻塞操作！
            if(!(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)){
                close(connfd);
                return -3;
            }
        }
    }
    return -1;
}

void signal_cb(struct ev_loop *loop,ev_signal *signal_w,int e)
{
    puts("Process Exit！");
    ev_signal_stop(loop,signal_w);
    ev_break(loop,EVBREAK_ALL);
}

void timer_cb(struct ev_loop *loop, ev_timer *time_w,int revents)
{
    int tmp_sd = *(int*)time_w->data;
    puts("Date req ... ");
    
    int ret = send(tmp_sd,req_buf,REQ_FRAME_SIZE,MSG_DONTWAIT);
    if(ret < 0){
        if(ret == ENOTCONN || ret == EPIPE){ //连接已关闭
            printf("This connection has closed!\n");
            //释放资源
            close(tmp_sd); 
            ev_timer_stop(loop, time_w); 
            free(time_w);
        }
        else if( !(errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) ){ //排除非阻塞正常返回值
            //释放资源
            close(tmp_sd); 
            ev_timer_stop(loop, time_w); 
            free(time_w);
        }
    } 
}

void read_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)  
{    
    if(EV_ERROR & revents)  
    {  
        printf("error event in read\n");  
        return ;  
    }  

    //接收数据  
    printf("Date receiving...\n");
    handle_data(watcher->fd);  
}  


static void sock_cb(struct ev_loop *loop, ev_io *w,int revents)    
{
    struct sockaddr_in caddr;
    int addr_len = sizeof( caddr );
      
    if(EV_ERROR & revents)  
    {  
        printf("error event in accept\n");  
        return ;  
    }  
      
    //获取与客户端相连的fd 
    int connfd=accept(w->fd, (struct sockaddr*)&caddr, &addr_len); 
    if(connfd > 0)  
    {  
        printf("%s:%d connect successfully!\n", inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));   
    }  
 
    //设置fd可重用  
    int opt=1;  
    if(setsockopt(connfd,SOL_SOCKET ,SO_REUSEADDR,(const char*)&opt,sizeof(opt)) != 0)  
    {  
        printf("setsockopt error in reuseaddr[%d]\n", connfd);  
        return ;  
    } 
     
    /* set NONBLOCK */  
    int flags = fcntl(connfd, F_GETFL, 0);  
    fcntl(connfd, F_SETFL, flags | O_NONBLOCK); 
    
    
    //创建客户端的io watcher  
    struct ev_io *w_client = (struct ev_io*)malloc(sizeof(struct ev_io));    
    if(w_client == NULL)  
    {  
        printf("malloc error in accept_cb\n");  
        return ;  
    }   

    //监听新的fd  
    ev_io_init(w_client, read_cb, connfd, EV_READ);  
    ev_io_start(loop, w_client); 
    
    
    //设置超时定时器watcher,定时发送数据请求
    ev_timer *t_client = (struct ev_timer*)malloc(sizeof(struct ev_timer)); 
    my_timer *t = (struct mytimer*)malloc(sizeof(struct mytimer));
    t->fd = connfd;
    t_client->data = &(t->fd);

    ev_init(t_client,timer_cb);  
    ev_timer_set(t_client,0,10.0);       //重复10秒超时触发
    ev_timer_start(loop, t_client);    //开启定时器watcher
}

int main(int argc,char **argv)
{
    sock_init();
    req_data_frame();

    struct ev_loop *poller = EV_DEFAULT;  //使用默认的事件循环
    //struct ev_loop * epoller = ev_loop_new(EVBACKEND_EPOLL | EVFLAG_NOENV);   //动态分配，且只使用epoll
    ev_io sock_w;//定义一个watcher
    ev_signal signal_w;

    ev_init(&sock_w,sock_cb);
    ev_io_set(&sock_w,listenfd,EV_READ);
    ev_io_start(poller,&sock_w);

    ev_init(&signal_w,signal_cb);
    ev_signal_set(&signal_w,SIGINT);
    ev_signal_start(poller,&signal_w);

    //设置轮询的时间,默认为0，系统以最小时间调度轮询，此值越大延迟越大，但能提高效率。通常设为0.1即能满足时效和提高效率（当然游戏除外）
    ev_set_io_collect_interval(poller,0.1);

    //开启循环
    ev_run(poller,0);

    close(listenfd);
    close(c_fd);
	return 0;
}
