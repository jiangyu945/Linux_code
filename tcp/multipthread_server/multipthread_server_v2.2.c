/*
 ============================================================================
 Name        : multipthread_server.c
 Author      : Greein-jy
 Version     : v2.2
 date        : 2018-11-18
 Description : 接收格式由十六进制改为字符串格式
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
#include <sys/select.h>
#include <time.h>

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

unsigned short CRC_Compute(uchar*,uchar);
void req_data_frame(void);
int get_one_frame(int);
void* pthread_handler(void*);

int s_fd,c_fd,new_sock;
uchar req_buf[REQ_FRAME_SIZE] = {0};
struct sockaddr_in saddr, caddr;
uchar ipbuf[50];

uchar send_buf[100] = {0};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  //初始化锁，静态分配


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
	s_fd = socket( AF_INET, SOCK_STREAM, 0 );
   
	memset(&saddr, 0, sizeof(saddr) );
	memset( ipbuf, 0, sizeof(ipbuf) );
	saddr.sin_family = AF_INET;
    saddr.sin_port = htons(MYPORT); 
	saddr.sin_addr.s_addr = htonl( INADDR_ANY ); //any address
 
	bind( s_fd, (struct sockaddr *)&saddr, 16 );
	listen( s_fd, 30 );

    //设置fd可重用  
    int opt=1;  
    if(setsockopt(s_fd,SOL_SOCKET ,SO_REUSEADDR,(const char*)&opt,sizeof(opt)) != 0)  
    {  
        printf("setsockopt error in reuseaddr[%d]\n", s_fd);  
        return ;  
    }  

	printf( "Accepting client connections ... \n" );
 
    return 0;
}

int main( int argc , char ** argv )
{
	int addr_len;

    sock_init();

	addr_len = sizeof( caddr );
	//循环接收 客户端的连入
	while(1)
	{   
        if((new_sock = accept( s_fd, (struct sockaddr*)&caddr, &addr_len ))>0)
		{
			printf("%s:%d connect successfully!\n", inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
		} 
		
		//创建一个线程来接收新客户端的信息
        pthread_t id;
		pthread_create(&id,NULL,pthread_handler,(void*)&new_sock);
    
        pthread_detach(id);	//将状态改为unjoinable状态,unjoinable状态的线程，资源在线程函数退出时或pthread_exit时会被自动释放。线程默认的状态是joinable,需要在之后适时调用pthread_join.
	}

    //销毁锁
    pthread_mutex_destroy(&mutex);
    close(s_fd);
    close(c_fd);
	return 0;
}

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


//线程处理函数
void* pthread_handler(void* sock)
{
    int newsock = *((int*)sock);
    int nByte;
    int ret;

	/* set NONBLOCK */  
    int flags = fcntl(newsock, F_GETFL, 0);  
    fcntl(newsock, F_SETFL, flags | O_NONBLOCK); 
	
    req_data_frame();
    while(1)                        
    { 
        fd_set fds; //定义文件描述符集
        struct timeval timeout;
        timeout.tv_sec = 10;  //设置超时时间为10s
        timeout.tv_usec = 0;
        FD_ZERO(&fds);//清除文件描述符集
        FD_SET(newsock, &fds);//将文件描述符加入文件描述符集中
    	  
        //发送数据请求    
        ret = send(newsock,req_buf,REQ_FRAME_SIZE,0);
        if(ret == -1){  //连接已断开
            close(newsock);   
            return NULL;   
        }
        printf("send req success!\n");   

        //监听socket，超时时间为10s
        if(select(newsock+1,&fds,NULL,NULL,&timeout) > 0 )  
        {  
            printf("start to receive data...\n");

            //接收数据  
            ret = get_one_frame(newsock);
            if(ret == -1 )
                break;
            sleep(10);   //延时10s，每10s请求一次数据
        }    
    }
    close(newsock);   
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

                     //加（互斥）锁
                    pthread_mutex_lock(&mutex);

                    //2.填充数据到send_buf
                        /*填充时间*/
                    memcpy(send_buf,strtime,19);
                        /*填充数据*/
                    memcpy(&send_buf[19],str_buf,len*2);
                
                    //3.发送数据到远程服务端
                    send(c_fd,send_buf,len*2+19,MSG_DONTWAIT);
                    memset(send_buf,0,sizeof(send_buf));

                     //解锁
                    pthread_mutex_unlock(&mutex);

                    memset(str_buf,0,sizeof(str_buf));
                    memset(hex_buf,0,sizeof(hex_buf));
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

