/*
 * native.cpp
 *
 *  Created on: 2018年11月22日
 *      Author: ni
 */


#include <jni.h>
#include <android/log.h>
#include <android/bitmap.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <asm/types.h>          /* for videodev2.h */

#include <linux/videodev2.h>
#include <linux/usbdevice_fs.h>

#define  LOG_TAG    "WebCam"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

#define CLEAR(x) memset (&(x), 0, sizeof (x))

struct buffer {
    void *                  start;
    size_t                  length;
};
static int g_fd1;
static int g_fd2;
static unsigned int g_buffers_index1 = 0;
static unsigned int g_buffers_index2 = 0;
struct buffer *g_buffers1  = NULL;
struct buffer *g_buffers2  = NULL;

int OpenAndInitDevice(char *devname,int width,int height,int myindex) {
    struct stat st;
    if (-1 == stat(devname, &st))
    {
        return -1;
    }
    if (!S_ISCHR (st.st_mode)) {
        return -1;
    }
    int fd = -1;
    fd = open(devname, O_RDWR);
    LOGE("----%d,%d",myindex,fd);
    struct v4l2_capability cap;
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1)
    {
        return -1;
    }
    else
    {
        LOGE("driver:%s",cap.driver);
        LOGE("card:%s",cap.card);
        LOGE("bus_info:%s",cap.bus_info);
        LOGE("version:%d",cap.version);
        LOGE("capabilities:%x",cap.capabilities);
        if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == V4L2_CAP_VIDEO_CAPTURE)
        {
            LOGE("Device %s: supports capture.",devname);
        }

        if ((cap.capabilities & V4L2_CAP_STREAMING) == V4L2_CAP_STREAMING)
        {
            LOGE("Device %s: supports streaming.",devname);
        }
    }
    struct v4l2_fmtdesc fmtdec;
    memset(&fmtdec, 0, sizeof(fmtdec));
    fmtdec.index = 0;
    fmtdec.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmtdec) == 0)
    {
        LOGE("%d.%s",fmtdec.index+1,fmtdec.description);
        fmtdec.index++;
    }
    struct v4l2_format fmt;
    CLEAR (fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    LOGE("w,h  %d,%d",width,height);
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (-1 == ioctl(fd, VIDIOC_S_FMT, &fmt))
    {
        LOGE("Unable to set format");
        return -1;
    }
    if(ioctl(fd, VIDIOC_G_FMT, &fmt) == -1)
    {
        LOGE("Unable to get format");
        return -1;
    }
    else
    {
        LOGE("fmt.type:%d",fmt.type);
        LOGE("pix.pixelformat:\t%c%c%c%c",fmt.fmt.pix.pixelformat & 0xFF, (fmt.fmt.pix.pixelformat >> 8) & 0xFF,(fmt.fmt.pix.pixelformat >> 16) & 0xFF, (fmt.fmt.pix.pixelformat >> 24) & 0xFF);
        LOGE("pix.height:%d",fmt.fmt.pix.height);
        LOGE("pix.width:%d",fmt.fmt.pix.width);
        LOGE("pix.field:%d",fmt.fmt.pix.field);
    }
    struct v4l2_requestbuffers req;
    CLEAR (req);
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (-1 == ioctl(fd, VIDIOC_REQBUFS, &req))
    {
        LOGE("request for buffers error");
        return -1;
    }
    struct buffer * buffers = NULL;
    buffers = static_cast<buffer *>(calloc(req.count, sizeof(*buffers)));
    if (!buffers)
    {
        return -1;
    }
    unsigned int n_buffers;
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers)
    {
        struct v4l2_buffer buf;
        CLEAR (buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;
        if (-1 == ioctl(fd, VIDIOC_QUERYBUF, &buf))
        {
            LOGE("query buffer error");
            return -1;
        }
        buffers[n_buffers].length = buf.length;
        LOGE("lenth = %d",buf.length);
        buffers[n_buffers].start =
                mmap(NULL,buf.length,PROT_READ | PROT_WRITE,MAP_SHARED,fd, buf.m.offset);
        if (MAP_FAILED == buffers[n_buffers].start)
        {
            LOGE("buffer map error");
            return -1;
        }
    }
    if(myindex == 0)
    {
        g_fd1 = fd;
        g_buffers1 = buffers;
        g_buffers_index1 = n_buffers;
    }
    else if(myindex == 1)
    {
        g_fd2 = fd;
        g_buffers2 = buffers;
        g_buffers_index2 = n_buffers;
    }
    else
    {
        return -1;
    }
    return 0;
}

int StartCapturing(int myindex)
{
    unsigned int i;
    enum v4l2_buf_type type;
    int n_buffers = -1;
    int fd = -1;
    if(myindex == 0)
    {
        n_buffers = g_buffers_index1;
        fd = g_fd1;
    }
    else if(myindex == 1)
    {
        n_buffers = g_buffers_index2;
        fd = g_fd2;
    }
    else
    {
        return -1;
    }
    for (i = 0; i < n_buffers; ++i)
    {
        struct v4l2_buffer buf;
        CLEAR (buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (-1 == ioctl(fd, VIDIOC_QBUF, &buf))
        {
            LOGE("ioctl eroor %d",errno);
            return -1;
        }
    }
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(fd, VIDIOC_STREAMON, &type))
    {
        LOGE("err :%d",errno);
        return -1;
    }
    return 0;
}

int ReadFrameOne(int myindex)
{
    int n_buffers = -1;
    int fd = -1;
    if(myindex == 0)
    {
        n_buffers = g_buffers_index1;
        fd = g_fd1;
    }
    else if(myindex == 1)
    {
        n_buffers = g_buffers_index2;
        fd = g_fd2;
    }
    else
    {
        return -1;
    }
    struct v4l2_buffer buf;
    CLEAR (buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    LOGE("Read %d",fd);
    if (-1 == ioctl(fd, VIDIOC_DQBUF, &buf))
    {
        LOGE(" Read Frame ioctrl error %d",errno);
        return -1;
    }
    return 0;
}

int StopAndCloseDevice(int myindex)
{
    int n_buffers = -1;
    int fd = -1;
    struct buffer * buffers = NULL;
    if(myindex == 0)
    {
        n_buffers = g_buffers_index1;
        fd = g_fd1;
        buffers = g_buffers1;
    }
    else if(myindex == 1)
    {
        n_buffers = g_buffers_index2;
        fd = g_fd2;
        buffers = g_buffers2;
    }
    else
    {
        return -1;
    }
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == ioctl(fd, VIDIOC_STREAMOFF, &type))
    {
        return -1;
    }
    unsigned int i;
    for (i = 0; i < n_buffers; ++i)
    {
        if (-1 == munmap(buffers[i].start, buffers[i].length))
        {
            return -1;
        }
    }
    free(buffers);
    if (-1 == close(fd))
    {
        fd = -1;
        return -1;
    }
    return 0;
}

int yuyv2rgb24(char  *yuyv, char  *rgb, int  width, int height)
{
    int i, in, rgb_index = 0;
    char y0, u0, y1, v1;
    int r, g, b;
    int out = 0, x, y;

    for(in = 0; in < width * height * 2; in += 4)
    {
        y0 = yuyv[in+0];
        u0 = yuyv[in+1];
        y1 = yuyv[in+2];
        v1 = yuyv[in+3];

        for (i = 0; i < 2; i++)
        {
            if (i)
                y = y1;
            else
                y = y0;
            r = y + (140 * (v1-128))/100;  //r
            g = y - (34 * (u0-128))/100 - (71 * (v1-128))/100; //g
            b = y + (177 * (u0-128))/100; //b
            if(r > 255)   r = 255;
            if(g > 255)   g = 255;
            if(b > 255)   b = 255;
            if(r < 0)     r = 0;
            if(g < 0)     g = 0;
            if(b < 0)     b = 0;

            y = height - rgb_index/width -1;
            x = rgb_index%width;
            rgb[(y*width+x)*3+0] = b;
            rgb[(y*width+x)*3+1] = g;
            rgb[(y*width+x)*3+2] = r;
            rgb_index++;
        }
    }
    return 0;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_huaan_usbcamerademok_camera_UsbCamera_prepareCameraWithBase(JNIEnv *env,
                                                                             jobject instance,
                                                                             jint videoid,
                                                                             jint videobase,jint width,jint height) {
    // TODO
    int ret = -1;
    char devname[256];
    sprintf(devname,"/dev/video%d",videoid);
    LOGE("devname=%s,%d",devname,videobase);
    ret = OpenAndInitDevice(devname,width,height,videobase);
    LOGE("ret = %d", ret);
    if(ret != 0)
    {
        return -1;
    }
    ret = StartCapturing(videobase);
    return ret;

}extern "C"
JNIEXPORT void JNICALL
Java_com_huaan_usbcamerademok_camera_UsbCamera_stopCamera(JNIEnv *env, jobject instance,jint videobase) {
    StopAndCloseDevice(videobase);
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_huaan_usbcamerademok_camera_UsbCamera_getYUVData(JNIEnv *env, jobject instance,
                                                                  jbyteArray bytes_,jint index) {
    jbyte *bytes = env->GetByteArrayElements(bytes_, NULL);
    LOGE("Get data");
    int n_buffers = -1;
    int fd = -1;
    struct buffer * buffers = NULL;
    if(index == 0)
    {
        n_buffers = g_buffers_index1;
        fd = g_fd1;
        buffers = g_buffers1;
    }
    else if(index == 1)
    {
        n_buffers = g_buffers_index2;
        fd = g_fd2;
        buffers = g_buffers2;
    }
    else
    {

    }
    LOGE("Get data1");
    struct v4l2_buffer buf;
    CLEAR (buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    LOGE("Get data2 %d",fd);
    if (-1 == ioctl(fd, VIDIOC_DQBUF, &buf))
    {
        LOGE("ioctrl error %d",errno);
        return -1;
    }
    assert (buf.index < n_buffers);
//    int size = IMG_HEIGHT * IMG_WIDTH * 2;
    LOGE("Get data3 %d %d",buf.index,buffers[buf.index].length);
    memcpy((char * )bytes,buffers[buf.index].start,buffers[buf.index].length);
    LOGE("Get data4 %d ",buffers[buf.index].length);
    if (-1 == ioctl(fd, VIDIOC_QBUF, &buf))
    {
        LOGE("ioctrl error %d",errno);
        return -1;
    }
    env->ReleaseByteArrayElements(bytes_, bytes, 0);
    return  buffers[buf.index].length;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_huaan_usbcamerademok_camera_UsbCamera_yuyv2rgb24(JNIEnv *env, jobject instance,
                                                                  jbyteArray yuvByte_,
                                                                  jbyteArray rgbByte_, jint width,
                                                                  jint height) {
    jbyte *yuvByte = env->GetByteArrayElements(yuvByte_, NULL);
    jbyte *rgbByte = env->GetByteArrayElements(rgbByte_, NULL);

    yuyv2rgb24((char *)yuvByte,(char *) rgbByte,width,height);

    env->ReleaseByteArrayElements(yuvByte_, yuvByte, 0);
    env->ReleaseByteArrayElements(rgbByte_, rgbByte, JNI_COMMIT);
}