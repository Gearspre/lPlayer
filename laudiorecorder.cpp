#include "laudiorecorder.h"

#include <QDebug>
#include <QFile>
#include <string.h>

#include "lffdebugtools.h"

extern "C"{
//设备相关API
#include <libavdevice/avdevice.h>

//格式相关API
#include <libavformat/avformat.h>

//工具相关API
#include <libavutil/avutil.h>

//编码相关API
#include <libavcodec/avcodec.h>

//重采样相关API
#include <libswresample/swresample.h>
}

lAudioRecorder::lAudioRecorder(QObject *parent) : QThread(parent)
{
    //初始化libavdevice
    avdevice_register_all();
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

lAudioRecorder::~lAudioRecorder()
{
    this->quit();
    this->wait();
}

AVCodecContext* lAudioRecorder::create_aac_encoder()
{
    AVCodec* encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
    AVCodecContext* codecCtx = avcodec_alloc_context3(encoder);

    codecCtx->sample_fmt = AV_SAMPLE_FMT_S16;               //输入音频的采样格式
    codecCtx->channel_layout = AV_CH_LAYOUT_STEREO;         //输入音频的通道布局
    codecCtx->channels = 2;                                 //输入音频的通道数
    codecCtx->sample_rate = 44100;                          //输入音频的采样率
    codecCtx->bit_rate = 0;// AAC_LC:128K, AAC HE:64K, AAC HE V2:32K  比特率
    codecCtx->profile = FF_PROFILE_AAC_HE_V2; // bit_rate must be 0, if set profile

    if (avcodec_open2(codecCtx, encoder, nullptr) < 0)
    {
        return nullptr;
    }

    return codecCtx;
}

void lAudioRecorder::run()
{
        AVInputFormat* iformat = av_find_input_format(FMT_NAME);
        AVFormatContext* fmtCtx = nullptr;
        QFile pcmdata(FILE_NAME);
        AVPacket* pkt = av_packet_alloc();

        if(!iformat){
            qDebug()<<"找不到输入格式";
            return;
        }

//开启音频输入
        int ret = avformat_open_input(&fmtCtx,        //上下文
                                      DEVICE_NAME, //设备名
                                      iformat,     //格式
                                      nullptr);    //选项

        if(ret < 0){
//            char errbuf[1024] = {0};
//            av_strerror(ret, errbuf, sizeof(errbuf));
//            qDebug()<< QString("opne input error, num:%1, msg:%2").arg(ret).arg(errbuf);
            lFFDebugTools::ffPrintError(ret, "opne input error");
            return;
        }

//音频数据写入磁盘
        if (!pcmdata.open(QIODevice::WriteOnly)){
            qDebug()<<"open file failed";
            avformat_close_input(&fmtCtx);
            return;
        }

//音频数据重采样配置
        SwrContext* swrCtx = nullptr;
        swrCtx = swr_alloc_set_opts(nullptr,                //ctx
                                    AV_CH_LAYOUT_STEREO,    //输出channel布局
                                    AV_SAMPLE_FMT_S32,      //输出采样格式
                                    44100,                  //采样率
                                    AV_CH_LAYOUT_STEREO,    //输入channel布局
                                    AV_SAMPLE_FMT_S16,      //输入采样格式
                                    44100,                  //采样率
                                    0,
                                    nullptr);

        if(!swrCtx){
            qDebug()<<"resample alloc error!";
            return;
        }

        if( ( ret = swr_init(swrCtx) ) < 0){
            lFFDebugTools::ffPrintError(ret, "swr init error");
//            char errbuf[1024] = {0};
//            av_strerror(ret, errbuf, sizeof(errbuf));
//            qDebug()<<QString("swr init error, num:%1, msg:%2").arg(ret).arg(errbuf);
        }

//创建输入缓冲区数组
        // INPUT AUDIO DATA
        uint8_t** srcData = nullptr;
        int srclineSize = 0;
        av_samples_alloc_array_and_samples(&srcData,             //输入缓冲区地址
                                           &srclineSize,        //缓冲区大小
                                           2,                   //通道数
                                           22050,               //单通道的采样个数 (一个pkt->size = 88200 位深2字节 2通道 88200/2/2)
                                           AV_SAMPLE_FMT_S16,   //输入采样格式
                                           0);

//创建输出缓冲区数组
        // OUTPUT AUDIO DATA
        uint8_t** dstData = nullptr;
        int dstlineSize = 0;
        av_samples_alloc_array_and_samples(&dstData,             //输出缓冲区地址
                                           &dstlineSize,        //缓冲区大小
                                           2,                   //通道数
                                           22050,               //单通道的采样个数 (一个pkt->size = 88200 位深2字节 2通道 88200/2/2)
                                           AV_SAMPLE_FMT_S32,   //输入采样格式
                                           0);

        qDebug()<<"start record";
        while(!isInterruptionRequested()){
            ret = av_read_frame(fmtCtx, pkt);

            memcpy(srcData[0], (const void*)pkt->data, pkt->size);

            swr_convert(swrCtx,                       //重采样的上下文
                        dstData,                      //输出结果缓冲区
                        22050,                        //输出单通道的采样数
                        (const uint8_t**)srcData,     //输入缓冲区
                        22050);                       //输入单通道的采样数

            if(ret == 0){
                pcmdata.write((const char*)dstData[0], dstlineSize);
//                qDebug()<<"pkt size: "<<pkt->size;
                qDebug()<<"dst data size: "<<dstlineSize;
                //pointer refrence --
                av_packet_unref(pkt);
            }
            else if(ret == AVERROR(EAGAIN)){//资源不可用
                continue;
            }
            else{
                char errbuf[1024] = {0};
                av_strerror(ret, errbuf, sizeof(errbuf));
                qDebug()<<QString("read frame error, num:%1, msg:%2").arg(ret).arg(errbuf);

                break;
            }

        }

        pcmdata.close();
        av_packet_free(&pkt);
        avformat_close_input(&fmtCtx);
        swr_free(&swrCtx);

        if(srcData){
            av_freep(&srcData[0]);
        }
        av_freep(&srcData);

        if(dstData){
            av_freep(&dstData[0]);
        }
        av_freep(&dstData);

        qDebug()<<"end record";
}
