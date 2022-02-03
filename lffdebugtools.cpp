#include "lffdebugtools.h"
#include <libavutil/avutil.h>

lFFDebugTools::lFFDebugTools()
{

}

QString lFFDebugTools::ffPrintError(int errNumber, const QString &errSrc)
{
    char errbuf[1024] = {0};
    av_strerror(errNumber, errbuf, sizeof(errbuf));
    qDebug()<< QString("%1 errNum:%2, errMsg:%3").arg(errSrc).arg(ret).arg(errbuf).trimmed();
}

