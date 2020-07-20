#ifndef AUDIOHANDLER_H
#define AUDIOHANDLER_H

#include <QObject>
#include <QCamera>
#include <QCameraInfo>
#include <QVideoProbe>
#include <QMediaPlayer>
#include <QMediaRecorder>
#include <QAudioDeviceInfo>
#include <QObject>
#include <QVideoFrame>
#include <QCameraImageCapture>
extern "C" {
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavdevice/avdevice.h>
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/frame.h"
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libavutil/dict.h"
#include "libavutil/eval.h"
#include "libavutil/fifo.h"
#include "libavutil/pixfmt.h"
#include "libavutil/rational.h"
#include "libswresample/swresample.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavcodec/avcodec.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/frame.h"
#include "libavutil/opt.h"
#include "libswresample/swresample.h"
#include "libavutil/time.h"

}
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <QAudioDeviceInfo>
#include <QtConcurrent/QtConcurrent>
#include "sockethandler.h"



class AudioHandler //: public QObject
{
    //Q_OBJECT
public:
    AudioHandler(QString cDeviceName, std::mutex* _writeLock,int64_t time,SocketHandler*, int bufferSize);
    int grabFrames();
    int init();

private:
    int mBufferSize;
    int64_t time;
    QString aDeviceName;
    QString cDeviceName;
    std::mutex* writeLock;
    void cleanup();
    int openInputFile();
    int openOutputFile();
    void initPacket(AVPacket *packet);
    int loadEncodeAndWrite();
    int encodeAudioFrame(AVFrame*,int*);
    int initOutputFrame(AVFrame **,int);
    int readDecodeConvertAndStore(int *);
    int convertSamples(const uint8_t **,uint8_t **, const int);
    int addSamplesToFifo(uint8_t **,const int );
    int initConvertedSamples(uint8_t ***,int);
    int decodeAudioFrame(AVFrame *,int *, int *);
    int writeOutputFileHeader();
    int initFifo();
    int initResampler();
    SocketHandler *mSocketHandler;
    AVFormatContext *inputFormatContext;
    AVFormatContext *outputFormatContext;
    AVCodecContext *inputCodecContext;
    AVCodecContext *outputCodecContext;
    SwrContext *resampleContext;
    AVAudioFifo *fifo;
    void changeAudioInputDevice(QString deviceName);
    QVariantList getAudioInputDevices();
    static int audioCustomSocketWrite(void* opaque, uint8_t *buffer, int buffer_size);
    AVDictionary *options = NULL;
};

#endif // AUDIOHANDLER_H
