/*
 ============================================================================
 Name        : camera.c
 Author      : Greein-jy
 Version     : v1.0
 date        : 2018-10-15
 Description : V4L2 capture one frame image(yuyv)
 ============================================================================
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
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>
#include <linux/videodev2.h>

#define  DEV_NAME  "/dev/video5"

#define  WIDTH    1920   
#define  HEIGHT   1080

struct v4l2_capability cap;
struct v4l2_queryctrl qctrl;
struct v4l2_control ctrl;
struct v4l2_format Format;
struct v4l2_streamparm Stream_Parm;

// #define XXXX_150

//定义缓冲区结构体
struct buffer
{
    void *start;
    size_t length;
};

struct buffer *buffers;

unsigned long n_buffers;
//unsigned long file_length;

static int cam_fd;

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
             
    if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE)
    {
        printf("Device %s: supports capture.\n",DEV_NAME);
    }
    if ((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING)
    {
        printf("Device %s: supports streaming.\n",DEV_NAME);
    }
    return 0;
}


int get_cap_para(){

/*
 函数   ：int ioctl(intfd, int request, struct v4l2_fmtdesc *argp);
 struct v4l2_fmtdesc  
 {  
    __u32 index;               // 要查询的格式序号，应用程序设置  
    enum v4l2_buf_type type;   // 帧类型，应用程序设置  
    __u32 flags;               // 是否为压缩格式  
    __u8 description[32];      // 格式名称  
    __u32 pixelformat;         // 格式  
    __u32 reserved[4];         // 保留  
 }; 
 VIDIOC_ENUM_FMT    //指令含义：获取当前驱动支持的视频格式
 */
    printf("【**********************所有支持格式：*****************************】\n");    
    struct v4l2_fmtdesc dis_fmtdesc;  
    dis_fmtdesc.index=0;  
    dis_fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;  
    printf("Support format:\n");  
    while(ioctl(cam_fd,VIDIOC_ENUM_FMT,&dis_fmtdesc)!=-1)  
    {  
        printf("\t%d.%s\n",dis_fmtdesc.index+1,dis_fmtdesc.description);  
        dis_fmtdesc.index++;  
    }
    printf("\n");


    printf("【**********************获取当前格式：*****************************】\n");
    Format.type= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(cam_fd,VIDIOC_G_FMT,&Format)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf(">>:  %d * %d\n",Format.fmt.pix.width,Format.fmt.pix.height);
    printf("pix.pixelformat:%c%c%c%c\n", \
            Format.fmt.pix.pixelformat & 0xFF,\
            (Format.fmt.pix.pixelformat >> 8) & 0xFF, \
            (Format.fmt.pix.pixelformat >> 16) & 0xFF,\
            (Format.fmt.pix.pixelformat >> 24) & 0xFF);
    printf("\n");


    printf("【***********************获取帧率：******************************】\n"); 
    Stream_Parm.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if(ioctl(cam_fd,VIDIOC_G_PARM,&Stream_Parm)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf(">>: Frame rate: %u/%u\n",Stream_Parm.parm.capture.timeperframe.numerator,Stream_Parm.parm.capture.timeperframe.denominator);
    printf("\n");
   
   
    printf("【**********************获取白平衡：******************************】\n");
    ctrl.id = V4L2_CID_AUTO_WHITE_BALANCE;
    if(ioctl(cam_fd,VIDIOC_G_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf(">>: 白平衡值: %d \n",ctrl.value);
    printf("\n");
 

    printf("【*********************获取白平衡色温：*****************************】\n");
    ctrl.id= V4L2_CID_WHITE_BALANCE_TEMPERATURE;  //将白平衡设置为指定的开尔文色温
    if(ioctl(cam_fd,VIDIOC_G_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf(">>: 白平衡色温值: %d \n",ctrl.value);
    printf("\n");
 

    printf("【**********************获取亮度值：******************************】\n");
    ctrl.id= V4L2_CID_BRIGHTNESS;
    if(ioctl(cam_fd,VIDIOC_G_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf(">>: 亮度值: %d \n",ctrl.value);
    printf("\n");
 

    printf("【**********************获取对比度：******************************】\n");
    ctrl.id=V4L2_CID_CONTRAST;
    if(ioctl(cam_fd,VIDIOC_G_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf(">>: 对比度值: %d \n",ctrl.value);
    printf("\n");


    printf("【*********************获取颜色饱和度：*****************************】\n");
    ctrl.id = V4L2_CID_SATURATION;
    if(ioctl(cam_fd,VIDIOC_G_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf(">>: 饱和度值: %d \n",ctrl.value);
    printf("\n");

    printf("【***********************获取色度：******************************】\n");
    ctrl.id = V4L2_CID_HUE;
    if(ioctl(cam_fd,VIDIOC_G_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf(">>: 色度: %d \n",ctrl.value);
    printf("\n");

    
    printf("【***********************获取锐度：******************************】\n");
    ctrl.id = V4L2_CID_SHARPNESS;
    if(ioctl(cam_fd,VIDIOC_G_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf(">>: 锐度: %d \n",ctrl.value);
    printf("\n");

    printf("【**********************获取背光补偿：*****************************】\n");
    ctrl.id = V4L2_CID_BACKLIGHT_COMPENSATION;
    if(ioctl(cam_fd,VIDIOC_G_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf(">>: 背光补偿: %d \n",ctrl.value);
    printf("\n");

    printf("【**********************获取伽玛值：******************************】\n");
    ctrl.id = V4L2_CID_GAMMA;
    if(ioctl(cam_fd,VIDIOC_G_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf(">>: 伽玛值: %d \n",ctrl.value);
    printf("\n");

#if 0
//以下参数在该型号（双目122）摄像头，无法调节
    printf("***************************获取增益值************************\n");
    ctrl.id= V4L2_CID_GAIN;
    if(ioctl(cam_fd,VIDIOC_G_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf(">>: 增益: %d \n",ctrl.value);
    printf("\n");


    printf("*************************获取曝光值*****************************\n");
    //(注意：测试发现，在Linux下，V4L2_EXPOSURE_ATUO并不被Firmware认可，要设置自动曝光，需要设置为：V4L2_EXPOSURE_APERTURE_PRIORITY)
    ctrl.id= V4L2_CID_EXPOSURE_AUTO ;
    if(ioctl(cam_fd,VIDIOC_G_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);   
    }
    printf(">: 曝光值: %d\n",ctrl.value);    
    printf("\n");

    printf("*************************获取曝光绝对值****************************\n"); //一般设置曝光是设置它
    struct v4l2_control ctrl;
    ctrl.id=V4L2_CID_EXPOSURE_ABSOLUTE;
    if(ioctl(cam_fd,VIDIOC_G_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf(">: 曝光绝对值: %d\n",ctrl.value);    
    printf("\n");

    printf("***************************获取曝光优先级************************\n");
    struct v4l2_control ctrl;
    ctrl.id= V4L2_CID_EXPOSURE_PRIORITY;
    if(ioctl(cam_fd,VIDIOC_G_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf(">: 曝光优先级: %d \n",ctrl.value);
    printf("\n");
#endif

    return 0;

}


void set_cap_para()
{
    printf("【*********************设置分辨率、格式：****************************】\n");
    memset(&Format,0,sizeof(struct v4l2_format));
    Format.type= V4L2_BUF_TYPE_VIDEO_CAPTURE;
    Format.fmt.pix.width =  WIDTH;
    Format.fmt.pix.height = HEIGHT;
    Format.fmt.pix.pixelformat= V4L2_PIX_FMT_YUYV;
    Format.fmt.pix.field = (enum v4l2_field)1;
    if(ioctl(cam_fd,VIDIOC_S_FMT,&Format)==-1)
    {    
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf("\n");
   
    printf("【***********************设置帧率：******************************】\n");
    Stream_Parm.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    Stream_Parm.parm.capture.timeperframe.denominator =30;
    Stream_Parm.parm.capture.timeperframe.numerator =1;
    if(ioctl(cam_fd,VIDIOC_S_PARM,&Stream_Parm)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf("\n");

    printf("【*********************设置白平衡色温：*****************************】\n");
    ctrl.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
    ctrl.value = 5100;
     if(ioctl(cam_fd,VIDIOC_S_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf("\n");

    printf("【***********************设置亮度：******************************】\n");
    ctrl.id= V4L2_CID_BRIGHTNESS;
    ctrl.value = 40;
    if(ioctl(cam_fd,VIDIOC_S_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf("\n");

    printf("【**********************设置对比度：******************************】\n");
    ctrl.id = V4L2_CID_CONTRAST;
    ctrl.value= 45;
    if(ioctl(cam_fd,VIDIOC_S_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    sleep(1);
    printf("\n");

    printf("【**********************设置饱和度：******************************】\n");
    ctrl.id = V4L2_CID_SATURATION;
    ctrl.value= 60;
    if(ioctl(cam_fd,VIDIOC_S_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf("\n");

    printf("【***********************设置色度：******************************】\n");
    ctrl.id = V4L2_CID_HUE;
    ctrl.value = 1;
    if(ioctl(cam_fd,VIDIOC_S_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf("\n");

    printf("【***********************设置锐度：******************************】\n");
    ctrl.id = V4L2_CID_SHARPNESS;
    ctrl.value = 4;
    if(ioctl(cam_fd,VIDIOC_S_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf("\n");

    
    printf("【**********************设置背光补偿：*****************************】\n");
    ctrl.id = V4L2_CID_BACKLIGHT_COMPENSATION;
    ctrl.value = 3;
    if(ioctl(cam_fd,VIDIOC_S_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf("\n");

    printf("【**********************设置伽玛值：******************************】\n");
    ctrl.id = V4L2_CID_GAMMA;
    ctrl.value = 120;
    if(ioctl(cam_fd,VIDIOC_S_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf("\n");



    printf("【******************验证设置**********************】\n");
    printf("【******************验证设置**********************】\n");


    

#if 0

    /***********设置手动曝光***************************/
    printf("【**********设置手动曝光**************************】\n");
    ctrl.id= V4L2_CID_EXPOSURE_AUTO;
    ctrl.value=V4L2_EXPOSURE_MANUAL;  
    if(ioctl(fd,VIDIOC_S_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf("\n");

    /***********设置曝光绝对值***************************/  
    printf("【***********设置曝光绝对值************************】\n");
    ctrl.id= V4L2_CID_EXPOSURE_ABSOLUTE;
    ctrl.value= value;  //val需填写 
    if(ioctl(fd,VIDIOC_S_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf("\n");

     /***********设置对焦模式***************************/
    printf("【***********设置对焦模式***************************】\n");
    printf("1> 关闭自动对焦\n");
    ctrl.id= V4L2_CID_FOCUS_AUTO;
    ctrl.value= 0;  
    if(ioctl(fd,VIDIOC_S_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf("\n");

    printf("2> 设置焦点值\n");
    ctrl.id= V4L2_CID_FOCUS_ABSOLUTE;
    ctrl.value= val;  //val需填写 
    if(ioctl(fd,VIDIOC_S_CTRL,&ctrl)==-1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }
    printf("\n");


    // /***************设置增益***************************/
    // printf("【***************设置增益***************************】\n");
    // ctrl.id=V4L2_CID_GAIN;
    // if(ioctl(cam_fd,VIDIOC_S_CTRL,&ctrl)==-1)
    // {
    //     perror("ioctl");
    //     exit(EXIT_FAILURE);
    // }
    // printf("\n");

 
    
    printf("**********************All Set Finished!!!*********************\n");

#endif
}




static int set_cap_frame()
{
    struct v4l2_format fmt;
    /*设置摄像头捕捉帧格式及分辨率*/
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = WIDTH;   
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;   //图像存储格式设为YUYV(YUV422)

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

static void init_mmap()
{
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    
    /*申请4个图像缓冲区（位于内核空间）*/
    req.count = 4;    //缓存数量，即缓存队列里保持多少张照片，一般不超过5个  (经测试此版本内核最多申请23个缓存区)
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP; //MMAP内存映射方式
    ioctl(cam_fd, VIDIOC_REQBUFS, &req);

    printf("req.count:%d\n",req.count);

    buffers = calloc(req.count, sizeof(struct buffer));
  
    /*将申请的4个缓冲区映射到用户空间*/
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers)
    {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

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
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        ioctl(cam_fd, VIDIOC_QBUF, &buf);
    }
 
}


static void start_cap()
{   
    /*使能视频设备输出视频流*/
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(cam_fd, VIDIOC_STREAMON, &type);
}

static void process_img(void* addr,int length)
{
    int file_fd;
    file_fd = open("out_image.jpeg", O_RDWR | O_CREAT, 0777);

    write(file_fd, addr, length);  //将图片写入图像件“out_image”

    //usleep(500);
    close(file_fd);
}

/*获取一帧图像*/
static int read_frame(void)
{
    struct v4l2_buffer buf;

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    ioctl(cam_fd, VIDIOC_DQBUF, &buf);  //出队，从队列中取出一个缓冲帧

    process_img(buffers[buf.index].start,buffers[buf.index].length);
    

    ioctl(cam_fd, VIDIOC_QBUF, &buf); //重新入队

    return 1;

}

static void main_loop()
{
    int ret;
    /*select监听*/
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(cam_fd, &fds);
    ret = select(cam_fd + 1, &fds, NULL, NULL, NULL);
    if(ret >0)
    {
        read_frame();
    }
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
    get_cap_para();
    set_cap_para();
    get_cap_para();
    // init_mmap();
    // start_cap();
    // main_loop();
    // stop_cap();
    close_cam();
    return 0;
}
