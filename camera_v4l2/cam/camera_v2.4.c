/*
 ============================================================================
 Name        : camera_v2.3.c
 Author      : Greein-jy
 Version     : v2.4
 date        : 2019-01-19
 Description : V4L2 capture video,add yuyv_2_yuv420p and yuyv_2_rgb

 =========================功能概述===========================================
 代码主要实现V4L2视频图像采集，采集格式为YUYV(即YUV422)，采集到的图片做了两种处理：
    1.保存在文件（xxx.yuv）中；
    2.格式转换（本代码转换了两种其它图片格式，分别是：YUV420P和RGB888），
      并且同样保存为本地文件，便于查看效果
 ============================================================================
 ==========================ChangeLog=========================================
    修改网络通信方式：由TCP改为UDP。
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <errno.h>
#include <malloc.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>
#include <linux/videodev2.h>
#include <sys/epoll.h>

//TCP
#include <arpa/inet.h>
#include <sys/socket.h>

#include <errno.h>


#define  MYPORT    8887

#define  DEV_NAME     "/dev/video5"
#define  EPOLLEVENTS  500

#define  WIDTH    320
#define  HEIGHT   240

typedef unsigned char uchar;
typedef unsigned int  uint;


struct buffer
{
    void *start;
    size_t length;
};

struct buffer *buffers;
unsigned long n_buffers;
//unsigned long file_length;

static int cam_fd;

int sfd,connfd;
struct sockaddr_in saddr,caddr;  //saddr记录本机地址信息，caddr用于记录客户端的地址信息

struct epoll_event ev,events[100];

uchar recv_buf[20] = {0};


void udp_init()
{
    sfd = socket( AF_INET, SOCK_DGRAM, 0 );  //AF_INET:IPV4;SOCK_DGRAM:UDP
    if(sfd < 0)
    {
        printf("create socket fail!\n");
        return;
    }
   
    //绑定发送端口
	memset(&saddr, 0, sizeof(saddr) );
	saddr.sin_family = AF_INET;                   //协议簇：IPV4
    saddr.sin_port = htons(MYPORT);               //端口号，需要网络序转换
	saddr.sin_addr.s_addr = htonl( INADDR_ANY );  //IP地址，需要进行网络序转换，INADDR_ANY：本地地址
 
	int ret = bind( sfd, (struct sockaddr *)&saddr, sizeof(saddr) );
    if(ret < 0){
        printf("socket bind fail!\n");
        return;
    }
    printf( "socket bind success! \n" );

}

/*获取当前时间（化为总ms数）*/
static int get_time_now()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return (now.tv_sec * 1000 + now.tv_usec / 1000);
}

//利用select制作的微妙定时器
int usTimer(long us)
{
    struct timeval timeout;
    timeout.tv_sec  = 0;
    timeout.tv_usec = us;

    return select(0,NULL,NULL,NULL,&timeout);
}

/*********************************************
*函数：void yuyv_to_rgb888(uchar* yuyv,uchar* rgb888);
*功能：YUYV格式转RGB888格式
*参数：
*        yuyv  ： yuyv图像数据起始位置
*        rgb888:  转换后rgb888数据存储起始位置
*        width :  图像宽度 
*        height:  图像高度
*返回值： 无
*********************************************/
void yuyv_to_rgb888(uchar* yuyv,uchar* rgb888,unsigned int width,unsigned int height)
{
    uint i;
    uchar* y0 = yuyv + 0;   
    uchar* u0 = yuyv + 1;
    uchar* y1 = yuyv + 2;
    uchar* v0 = yuyv + 3;

    uchar* r0 = rgb888 + 0;
    uchar* g0 = rgb888 + 1;
    uchar* b0 = rgb888 + 2;
    uchar* r1 = rgb888 + 3;
    uchar* g1 = rgb888 + 4;
    uchar* b1 = rgb888 + 5;
   
    float rt0 = 0, gt0 = 0, bt0 = 0, rt1 = 0, gt1 = 0, bt1 = 0;

    for(i = 0; i <= (width * height) / 2 ;i++)
    {
        bt0 = 1.164 * (*y0 - 16) + 2.018 * (*u0 - 128); 
        gt0 = 1.164 * (*y0 - 16) - 0.813 * (*v0 - 128) - 0.394 * (*u0 - 128); 
        rt0 = 1.164 * (*y0 - 16) + 1.596 * (*v0 - 128); 
   
    	bt1 = 1.164 * (*y1 - 16) + 2.018 * (*u0 - 128); 
        gt1 = 1.164 * (*y1 - 16) - 0.813 * (*v0 - 128) - 0.394 * (*u0 - 128); 
        rt1 = 1.164 * (*y1 - 16) + 1.596 * (*v0 - 128); 
    
      
       	if(rt0 > 250)  	rt0 = 255;
		if(rt0< 0)  rt0 = 0;	

		if(gt0 > 250) 	gt0 = 255;
		if(gt0 < 0)	gt0 = 0;	

		if(bt0 > 250)	bt0 = 255;
		if(bt0 < 0)	bt0 = 0;	

		if(rt1 > 250)	rt1 = 255;
		if(rt1 < 0)	rt1 = 0;	

		if(gt1 > 250)	gt1 = 255;
		if(gt1 < 0)	gt1 = 0;	

		if(bt1 > 250)	bt1 = 255;
		if(bt1 < 0)	bt1 = 0;	
					
		*r0 = (uchar)rt0;
		*g0 = (uchar)gt0;
		*b0 = (uchar)bt0;
	
		*r1 = (uchar)rt1;
		*g1 = (uchar)gt1;
		*b1 = (uchar)bt1;

        yuyv = yuyv + 4;
        rgb888 = rgb888 + 6;
        if(yuyv == NULL)
          break;

        y0 = yuyv;
        u0 = yuyv + 1;
        y1 = yuyv + 2;
        v0 = yuyv + 3;
  
        r0 = rgb888 + 0;
        g0 = rgb888 + 1;
        b0 = rgb888 + 2;
        r1 = rgb888 + 3;
        g1 = rgb888 + 4;
        b1 = rgb888 + 5;
    }   
}

/*********************************************
*函数：void yuyv_to_yuv420p(const uchar *yuyv, uchar *yuv420p, uint width, uint height);
*功能：YUYV格式转YUV420P格式
*参数：
*        yuyv   ： yuyv图像数据起始位置
*        yuv420p:  转换后yuv420p数据存储起始位置
*        width  :  图像宽度 
*        height :  图像高度
*返回值： 无
*********************************************/
void yuyv_to_yuv420p(const uchar *yuyv, uchar *yuv420p, uint width, uint height)
{
    
    uchar *y = yuv420p;
    uchar *u = yuv420p + width*height;
    uchar *v = yuv420p + width*height + width*height/4;

    uint i,j;
    uint base_h;
    uint is_y = 1, is_u = 1;
    uint y_index = 0, u_index = 0, v_index = 0;

    unsigned long yuyv_length = 2 * width * height;

    //序列为YU YV YU YV，一个yuv422帧的长度 width * height * 2 个字节
    //丢弃偶数行 u v

    for(i=0; i<yuyv_length; i+=2)
    {
        *(y+y_index) = *(yuyv+i);
        y_index++;
    }

    for(i=0; i<height; i+=2)
    {
        base_h = i*width*2;
        for(j=base_h+1; j<base_h+width*2; j+=2)
        {
            if(is_u){
                *(u+u_index) = *(yuyv+j);
                u_index++;
                is_u = 0;}
            else{
                *(v+v_index) = *(yuyv+j);
                v_index++;
                is_u = 1;}
        }
    }
}


static int open_cam()
{
    struct v4l2_capability cap;
    cam_fd = open(DEV_NAME, O_RDWR | O_NONBLOCK, 0);  //非阻塞方式打开摄像头
    if (cam_fd < 0)
    {
        perror("open device failed!");
        return -1;
    }

    /*获取摄像头信息*/
    if (ioctl(cam_fd, VIDIOC_QUERYCAP, &cap) < 0)
    {
        perror("get info failed!");
        return -1;
    }

    printf("Driver Name:%s\n Card Name:%s\n Bus info:%s\n version:%d\n capabilities:%x\n \n ", cap.driver, cap.card, cap.bus_info,cap.version,cap.capabilities);
             
    if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE){
        printf("Device %s: supports capture.\n",DEV_NAME);
    }
    if ((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING){
        printf("Device %s: supports streaming.\n",DEV_NAME);
    }
    return 0;
}

static int set_cap_frame()
{
    struct v4l2_format fmt;

    /*设置摄像头捕捉帧格式及分辨率*/
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = WIDTH;   
    fmt.fmt.pix.height      = HEIGHT;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;   //图像采集格式设:YUYV(YUV422)/MJPEG

    if (ioctl(cam_fd, VIDIOC_S_FMT, &fmt) < 0)
    {
        perror("set fmt failed!");
        return -1;
    }

    printf("fmt.type:%d\n",fmt.type);
    printf("pix.pixelformat:%c%c%c%c\n", \
            fmt.fmt.pix.pixelformat & 0xFF,\
            (fmt.fmt.pix.pixelformat >> 8) & 0xFF, \
            (fmt.fmt.pix.pixelformat >> 16) & 0xFF,\
            (fmt.fmt.pix.pixelformat >> 24) & 0xFF);
    printf("pix.width:%d\n",fmt.fmt.pix.width);
    printf("pix.height:%d\n",fmt.fmt.pix.height);
    printf("pix.field:%d\n",fmt.fmt.pix.field);

    return 0;
}

void get_fps()
{
    struct v4l2_streamparm *parm;
    parm = (struct v4l2_streamparm *)calloc(1, sizeof(struct v4l2_streamparm));
    memset(parm, 0, sizeof(struct v4l2_streamparm));
    parm->type = V4L2_BUF_TYPE_VIDEO_CAPTURE; 

    /*
    parm->parm.capture.timeperframe.numerator = 1;
    parm->parm.capture.timeperframe.denominator = 2;
      
    int result = ioctl(cam_fd,VIDIOC_S_PARM, parm);
    if(result == -1)
    {
        perror("Set fps failed!\n");
    }
    */

    int rel = ioctl(cam_fd,VIDIOC_G_PARM, parm);
    if(rel == 0)
    {
        printf("Frame rate:  %u/%u\n",parm->parm.capture.timeperframe.denominator,parm->parm.capture.timeperframe.numerator);
    }
    free(parm);
}

static void init_mmap()
{
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    
    /*申请4个图像缓冲区（位于内核空间）*/
    req.count  = 4;                             //缓存区数量，即缓存队列里保持多少张照片，一般不超过5个 (经测试3.0.15内核最多申请23个缓存区)
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;              //MMAP内存映射方式
    ioctl(cam_fd, VIDIOC_REQBUFS, &req);

    printf("req.count:%d\n",req.count);

    buffers = calloc(req.count, sizeof(struct buffer));
  
    /*将申请的4个缓冲区映射到用户空间*/
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers)
    {
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = n_buffers;

        ioctl(cam_fd, VIDIOC_QUERYBUF, &buf);

        buffers[n_buffers].length = buf.length;

        buffers[n_buffers].start = mmap(NULL,
                                        buf.length,
                                        PROT_READ | PROT_WRITE,
                                        MAP_SHARED,
                                        cam_fd,
                                        buf.m.offset);
    }
    
    /*缓冲区入队列*/
    int i;
    for (i = 0; i < n_buffers; ++i)
    {
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;
        ioctl(cam_fd, VIDIOC_QBUF, &buf);
    }
 
}


/*使能视频设备输出视频流*/
static void start_cap()
{   
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(cam_fd, VIDIOC_STREAMON, &type);
}

//图像数据处理
static void process_img(void* addr,int length)
{ 
    // struct tm *ptr;
    // time_t t;
    // uchar file_name[41];
    // time(&t);
    // ptr = localtime(&t);
    // strftime(file_name,sizeof(file_name),"Greein_%Y%m%d_%H%M%S",ptr);
    // strcat(file_name,".yuv");

    // FILE* fp_yuyv = fopen(file_name, "a+");  //以追加的方式打开
    // FILE* fp_yuv420 = fopen("yuv420p_1080p.yuv", "a+");
    //FILE* fp_rgb888 = fopen("rgb888_1080p.rgb888", "a+");

    // fwrite(addr,length,1,fp_yuyv);       //写入yuyv文件

    //申请格式转换后的数据存储buffer
	// uchar *yuv420_buf = (uchar *)malloc(3*WIDTH*HEIGHT/2*sizeof(uchar)); 
    //  uchar *rgb888_buf = (uchar *)malloc(WIDTH*HEIGHT*3*sizeof(uchar));  

    //YUYV转YUV420P
    // yuyv_to_yuv420p(addr,yuv420_buf,WIDTH,HEIGHT); 
    // fwrite(yuv420_buf, WIDTH*HEIGHT*3/2, 1, fp_yuv420);

    //YUYV转RGB888
    //  yuyv_to_rgb888(addr,rgb888_buf,WIDTH,HEIGHT);
    //fwrite(rgb888_buf, WIDTH*HEIGHT*3, 1, fp_rgb888);

    //发送到显示端
    // int count;
    // int send_len = length;
    // while(1){  
    //     count = send(connfd,addr,send_len,0);   //发送到客户端
    //     send_len -= count;
    //     if(!count)
    //         break;
    // }

    //free(yuv420_buf); 
    //  free(rgb888_buf);

    // fclose(fp_yuyv);
    //fclose(fp_yuv420);
    //fclose(fp_rgb); 
}

/*获取一帧图像*/
static int read_frame()
{
    struct v4l2_buffer buf;

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    ioctl(cam_fd, VIDIOC_DQBUF, &buf);  //出队，从队列中取出一个缓冲帧

        int count=0;
        int left_size = buffers[buf.index].length;
        printf("left_size = %d\n",left_size);
        //分包发送，UDP单次数据报大小不能太大，否则发送会失败！
        while(1){
            if(left_size >= 8000){

                count = sendto(sfd,buffers[buf.index].start,8000,0,(struct sockaddr*)&caddr, sizeof(caddr));
                left_size -= 8000;
                printf("send count = %d\n",count);

                if(count == -1){
                    printf("send fail!!!\n");
                    printf("errno %d\n",errno);
                    perror("send error");
                    return -1;
                }
            }
            else{
                count = sendto(sfd,buffers[buf.index].start,left_size,0,(struct sockaddr*)&caddr, sizeof(caddr));
                printf("send count = %d,send finished!\n",count);
                break;
            }
            
        }
        //发送结束标志
        sendto(sfd,"End!!!",6,0,(struct sockaddr*)&caddr, sizeof(caddr));

    
    // // 发送到显示端
    // int count;
    // int send_len = buffers[buf.index].length;
    // printf("buffers[buf.index].length = %d\n",buffers[buf.index].length);
    // while(1){  
    //     count = send(connfd,buffers[buf.index].start,send_len,0);   //发送到客户端
    //     printf("count = %d\n",count);
    //     send_len -= count;
    //     if(!count)
    //         break;
    // }

    // send(connfd,buffers[buf.index].start,buffers[buf.index].length,0);

    //process_img(buffers[buf.index].start,buffers[buf.index].length);
    
    ioctl(cam_fd, VIDIOC_QBUF, &buf); //重新入队

    return 1;

}

static void main_loop()
{
    
    // FILE* fp_yuv420 = fopen("yuv420p_1080p.yuv", "a+");  
    // FILE* fp_rgb = fopen("rgb24_1080p.rgb", "a+");    
    int epfd = epoll_create(512);

    //监听客户端图片请求
    ev.data.fd = sfd;       
    ev.events = EPOLLIN | EPOLLET;    
    epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &ev);

    int start_t = get_time_now();

    int i,count=1;
    
        while(1){
            int nfds = epoll_wait(epfd,events,EPOLLEVENTS,-1);
            if(nfds > 0)
            {
                for(i=0;i<nfds;i++){
                    
                    if( (events[i].data.fd == sfd) && (events[i].events & EPOLLIN) ){  //有客户端请求图片
                        int len = sizeof(caddr);
                        int num = recvfrom(sfd,recv_buf,sizeof(recv_buf),0,(struct sockaddr*)&caddr,&len);
                        if(num == -1){
                            printf("recieve data fail!\n");
                            return;
                        }

                        printf("[%s : %d]#  %s\n",inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port),recv_buf);
                        memset(recv_buf,0,sizeof(recv_buf));

                        //加入对camera采集事件的监听
                        ev.data.fd = cam_fd;       
                        ev.events = EPOLLIN | EPOLLET;    
                        epoll_ctl(epfd, EPOLL_CTL_ADD, cam_fd, &ev);
                    }   

                    else if( (events[i].data.fd == cam_fd) && (events[i].events & EPOLLIN) ){
                        read_frame();  //读取图片
                        printf("Capture frame: %d\n",count++);

                        //ET模式，故需要修改监听标志,否则无法再次触发
                        ev.data.fd = cam_fd;       
                        ev.events = EPOLLIN | EPOLLET;    
                        epoll_ctl(epfd, EPOLL_CTL_DEL, cam_fd, &ev);

                        //30fps,故延时33ms
                        // usTimer(33000);     
                    }               
                }
            }

        }

 
    int end_t = get_time_now();
    printf("Total time-consuming: %dms\n",end_t-start_t);

    
//    fclose(fp_yuv420);
//    fclose(fp_rgb);  
}

static void stop_cap()
{
	enum v4l2_buf_type type;
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(-1 == ioctl(cam_fd,VIDIOC_STREAMOFF,&type))
	{
		perror("Fail to ioctl 'VIDIOC_STREAMOFF'");
		exit(EXIT_FAILURE);
	}
}


void close_cam()
{
    int i;
    //解除映射
    for (i = 0; i < n_buffers; ++i)
    {
        munmap(buffers[i].start, buffers[i].length);
    }

    free(buffers);   
    close(cam_fd);
    printf("Camera Capture Done.\n");
}

int main(int argc, char **argv)
{
    open_cam();
    set_cap_frame();
    get_fps();
    init_mmap();
    udp_init();
    start_cap();
    main_loop();
    stop_cap();
    close_cam();
    return 0;
}
