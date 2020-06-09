#ifndef VIDEOHANDLER_H
#define VIDEOHANDLER_H

#include <QObject>
#include <QCamera>
#include <QCameraInfo>
#include <QVideoProbe>
#include <QMediaPlayer>
#include <QMediaRecorder>
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
}
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>


class VideoHandler : public QObject
{
    Q_OBJECT
public:
    VideoHandler(QObject* parent = 0);
    QVideoProbe* videoProbe;
    QCamera* camera;
    void setup(QObject* qmlCamera);
    QMediaRecorder* recorder;
    QVector<uchar*> frames;

public slots:
    void handleFrame(QVideoFrame);
};

#endif // VIDEOHANDLER_H
