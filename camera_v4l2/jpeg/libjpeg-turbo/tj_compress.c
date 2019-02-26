#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "turbojpeg.h"


typedef unsigned char uchar;

typedef struct tjp_info {
  int outwidth;
  int outheight;
  unsigned long jpg_size;
}tjp_info_t;

/*获取当前ms数*/
static int get_timer_now ()
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return(now.tv_sec * 1000 + now.tv_usec / 1000);
}

/*读取文件到内存*/
uchar *read_file2buffer(char *filepath, tjp_info_t *tinfo)
{
  FILE *fd;
  struct stat fileinfo;
  stat(filepath,&fileinfo);
  tinfo->jpg_size = fileinfo.st_size;

  fd = fopen(filepath,"rb");
  if (NULL == fd) {
    printf("file not open\n");
    return NULL;
  }
  uchar *data = (uchar *)malloc(sizeof(uchar) * fileinfo.st_size);
  fread(data,1,fileinfo.st_size,fd);
  fclose(fd);
  return data;
}

/*写内存到文件*/
void write_buffer2file(char *filename, uchar *buffer, int size)
{
  FILE *fd = fopen(filename,"wb");
  if (NULL == fd) {
    return;
  }
  fwrite(buffer,1,size,fd);
  fclose(fd);
}

/*图片解压缩*/
uchar *tjpeg_decompress(uchar *jpg_buffer, tjp_info_t *tinfo)
{
  tjhandle handle = NULL;
  int img_width,img_height,img_subsamp,img_colorspace;
  int flags = 0, pixelfmt = TJPF_RGB;
  /*创建一个turbojpeg句柄*/
  handle = tjInitDecompress();
  if (NULL == handle)  {
    return NULL;
  }
  /*获取jpg图片相关信息但并不解压缩*/
  int ret = tjDecompressHeader3(handle,jpg_buffer,tinfo->jpg_size,&img_width,&img_height,&img_subsamp,&img_colorspace);
  if (0 != ret) {
    tjDestroy(handle);
    return NULL;
  }
  /*输出图片信息*/
  printf("jpeg width:%d\n",img_width);
  printf("jpeg height:%d\n",img_height);
  printf("jpeg subsamp:%d\n",img_subsamp);
  printf("jpeg colorspace:%d\n",img_colorspace);
  /*计算1/4缩放后的图像大小,若不缩放，那么直接将上面的尺寸赋值给输出尺寸*/
  tjscalingfactor sf;
  sf.num = 1;
  sf.denom = 1;  //缩放比例，为4代表1/4
  tinfo->outwidth = TJSCALED(img_width, sf);
  tinfo->outheight = TJSCALED(img_height, sf);
  printf("w:%d,h:%d\n",tinfo->outwidth,tinfo->outheight);
  /*解压缩时，tjDecompress2（）会自动根据设置的大小进行缩放，但是设置的大小要在它的支持范围，如1/2 1/4等*/
  flags |= 0;
  int size = tinfo->outwidth * tinfo->outheight * 3;
  uchar *rgb_buffer = (uchar *)malloc(sizeof(uchar) * size);
  ret = tjDecompress2(handle, jpg_buffer, tinfo->jpg_size, rgb_buffer, tinfo->outwidth, 0,tinfo->outheight, pixelfmt, flags);
  if (0 != ret) {
    tjDestroy(handle);
    return NULL;
  }
  tjDestroy(handle);
  return rgb_buffer;
}

/*压缩图片*/
int tjpeg_compress(uchar *rgb_buffer, tjp_info_t *tinfo, int quality, uchar **outjpg_buffer, unsigned long *outjpg_size)
{
  tjhandle handle = NULL;
  int flags = 0;
  int subsamp = TJSAMP_422;
  int pixelfmt = TJPF_RGB;
  /*创建一个turbojpeg句柄*/
  handle=tjInitCompress();
  if (NULL == handle) {
    return -1;
  }
   
  /** 
 * tjCompress2()将RGB，灰度或CMYK图像压缩为JPEG图像。//tjCompressFromYUV()将YUV平面图像压缩为JPEG图像。
 * @handle  	处理TurboJPEG压缩器或解压实例的句柄
 * @srcBuf  	指向包含rgb平面图像的图像缓冲区的指针
 * @width   	源图像的宽度（以像素为单位）
 * @pitch   	源图像中每一行的字节数，默认0即可
 * @height   	源图像的高度
 * @pixelFormat 源中使用的像素格式,参见TJPF，默认TJPF_RGB
 * @jpegBuf     指向要接收的图像缓冲区的指针的地址
 * @jpegSize    指向接受图像的长度
 * @jpegSubsamp 在使用时使用的色度次抽样的水平，参照TJSAMP，默认TJSAMP_422
 * @jpegQual    生成的JPEG图像的图像质量（1 =最差，100 =最佳）
 * @flags       标记位，默认为0
 * @如果成功返回0，如果发生错误则返回-1（请参阅#tjGetErrorStr（））
 */
  int ret = tjCompress2(handle, rgb_buffer,tinfo->outwidth,0,tinfo->outheight,pixelfmt,outjpg_buffer,outjpg_size,subsamp,quality, flags);
  if (0 != ret) {
    tjDestroy(handle);
    return -1;
  }
  tjDestroy(handle);
  return 0;
}

/*测试程序*/
int tj_test()
{
  tjp_info_t tinfo;
    char *filename = "./test.jpg";
 
  /*读图像*/
    uchar *jpeg_buffer = read_file2buffer(filename,&tinfo);
    if (NULL == jpeg_buffer) {
        printf("read file failed\n");
        return 1;
    }

  /*解压缩*/
    uchar *rgb = tjpeg_decompress(jpeg_buffer,&tinfo);
    if (NULL == rgb) {
        printf("error\n");
    free(jpeg_buffer);
    return -1;
    }

    uchar *outjpeg=NULL;
    unsigned long outjpegsize;
  /*压缩*/
    tjpeg_compress(rgb,&tinfo,70,&outjpeg,&outjpegsize);
    printf("out jpeg size = %lu\n",outjpegsize);

    char *outfile = "./tjout.jpg";
  
    write_buffer2file(outfile,outjpeg,outjpegsize);
  
  free(jpeg_buffer);
  free(rgb);
  return 0;
}

int tj_CompressFromMem(uchar *rgb_buffer,int quality)
{
  tjhandle handle = NULL;
  int flags = 0;
  int subsamp = TJSAMP_422;
  int pixelfmt = TJPF_RGB;
  /*创建一个turbojpeg句柄*/
  handle=tjInitCompress();
  if (NULL == handle) {
    return -1;
  }

  uchar *outjpg_buf=NULL;
  unsigned long outjpg_size;

  /*压缩*/
  int ret = tjCompress2(handle, rgb_buffer,WIDTH,0,HEIGHT,pixelfmt,outjpg_buf,outjpg_size,subsamp,quality, flags);
  if (0 != ret) {
    tjDestroy(handle);
    return -1;
  }
  tjDestroy(handle);
  printf("out jpg size = %lu\n",outjpg_size);
  
  FILE *fp_jpg = fopen("./tjout.jpg","wb");
  if (NULL == fp_jpg) {
    return;
  }
  fwrite(outjpg_buf,1,outjpg_size,fp_jpg);
  fclose(fp_jpg);

  return 0;
}


int main()
{
    tj_test();
    return 0;
}
