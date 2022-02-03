#ifndef LFFDEBUGTOOLS_H
#define LFFDEBUGTOOLS_H

#include <QString>

class lFFDebugTools
{
public:
    lFFDebugTools();

public:
    static QString ffPrintError(int errNumber, const QString& errSrc);
};

#endif // LFFDEBUGTOOLS_H
