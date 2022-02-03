#ifndef LAUDIORECORDER_H
#define LAUDIORECORDER_H

#include <QObject>
#include <QThread>

class lAudioRecorder : public QThread
{
    Q_OBJECT
#ifdef Q_OS_WIN
private:
    const char* FMT_NAME = "dshow";
    const char* DEVICE_NAME = "audio=耳机 (HUAWEI FreeBuds 4 Hands-Free AG Audio)";
    const QString FILE_NAME = "D:/out.pcm";
#elif //MAC OS
    const char* FMT_NAME = "avfoundation"
    const char* DEVICE_NAME = ":0"
#endif

public:
    explicit lAudioRecorder(QObject *parent = nullptr);
    ~lAudioRecorder();

public:
    AVCodecContext* encoder();

private:
    void run() override;
public slots:

signals:

private:
    bool isEnable = false;
};

#endif // LAUDIORECORDER_H
