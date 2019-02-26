 /*
 ============================================================================
 Name        : s_epoll.c
 Author      : Greein-jy
 Version     : v1.0
 date        : 2018-10-31
 Description : I/O复用模型（epoll） 
 Function    : 非一问一答，仅客户端单端发送数据（刚好克服一问一答的定时问题）

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

#define MYPORT 8080

//远端服务器
#define SERVER_IP  "192.168.31.41"
#define PORT_S        8887
 
#define REQ_FRAME_SIZE    9      //数据请求帧长度

//传感器数据请求格式
#define HEAD    0xAA        
#define LEN     0x09
#define OPTION  0x00
#define CMD     0X01
#define DATA1   0x0f      //数据域，表示需要请求哪些数据
#define DATA2   0xff      //数据域，表示需要请求哪些数据
#define TAIL    0x55

typedef  unsigned char  uchar;


int listenfd,c_fd,epfd;
uchar req_buf[REQ_FRAME_SIZE] = {0};
struct sockaddr_in saddr, caddr;

uchar send_buf[100] = {0};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  //初始化锁，静态分配


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

int sock_init()
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
		return -1;
	}
	printf("connected server!\n");

    //socket init（As a server）
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
 
    return 0;
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

//接收一帧数据
int get_one_frame(int connfd)
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

int main( int argc , char ** argv )
{
    int i,ret;
    int connfd;
	int addr_len;

    sock_init();

    struct epoll_event ev,events[100];

    epfd = epoll_create(512);

    ev.data.fd = listenfd;       
    ev.events = EPOLLIN | EPOLLET; 
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev); 

	
    req_data_frame();
	//循环接收 客户端的连入
	while(1)
	{   
        int nfds = epoll_wait(epfd,events,50,-1);
        if(nfds > 0)
        {
            for(i=0;i<nfds;++i)
            {
                if(!(events[i].events & EPOLLIN)){
                    continue;
                }
                if(events[i].data.fd == listenfd){   //有新的连接

                    addr_len = sizeof( caddr );
                    if( (connfd=accept(listenfd,(struct sockaddr*)&caddr,&addr_len)) > 0 )
                    {
                        printf("%s:%d connect successfully!\n", inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
                    } 

                    /* set NONBLOCK */  
                    int flags = fcntl(connfd, F_GETFL, 0);  
                    fcntl(connfd, F_SETFL, flags | O_NONBLOCK); 

                    //监听读事件
                    ev.events = EPOLLIN|EPOLLET;
                    ev.data.fd = connfd;

                    //添加至监听
                    epoll_ctl(epfd,EPOLL_CTL_ADD, connfd, &ev);

                    /*
                    //发送数据请求
                    ret = send(connfd,req_buf,REQ_FRAME_SIZE,MSG_DONTWAIT);  //flags设置为MSG_DONTWAIT，以非阻塞方式发送
                    if(ret == -1){   //客户端连接关闭
                        printf("%s:%d connect closed!\n", inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));

                        //从监听中移除
				        epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                        close(events[i].data.fd);   
                        break;   
                    }
                    */

                }
                else{   //读取客户端消息 
                    printf("start to receive data...\n");
                    ret = get_one_frame(events[i].data.fd);
                    if(ret == -1 )
                        break;
                    /*
                    ret = send(events[i].data.fd,req_buf,REQ_FRAME_SIZE,MSG_DONTWAIT);
                    if(ret == -1){   //客户端连接关闭
                        printf("%s:%d connect closed!\n", inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));

                        //从监听中移除
				        epoll_ctl(epfd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
                        close(events[i].data.fd);   
                        break;   
                    }
                    */
                }    
            } 
        }
    }

    //销毁锁
    pthread_mutex_destroy(&mutex);
    close(listenfd);
    close(c_fd);
	return 0;
}











    