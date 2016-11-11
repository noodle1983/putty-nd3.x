#include "KfifoBuffer.h"
#include "string.h"

//-----------------------------------------------------------------------------

KfifoBuffer::KfifoBuffer(const int theSizePower)
{
    // limit the size to 1k ~ 64M
    int sizePower = theSizePower < 10 ? 10
              : theSizePower > 26 ? 26
              : theSizePower;
    sizeM = 1 << sizePower; //1MB
    maskM = sizeM - 1;
    rawM = new char[sizeM];
    readIndexM = 0;
    writeIndexM = 0;
    highWaterMarkM = sizeM *4 / 5; // 80%
    lowWaterMarkM = sizeM >> 1;    // 50%
}

//-----------------------------------------------------------------------------

KfifoBuffer::~KfifoBuffer()
{
    release();
}

//-----------------------------------------------------------------------------

void KfifoBuffer::init()
{
}

//-----------------------------------------------------------------------------

void KfifoBuffer::release()
{
    delete[] rawM;
    rawM = NULL;
    readIndexM = 0;
    writeIndexM = 0;
}

//-----------------------------------------------------------------------------

unsigned KfifoBuffer::put(const char* const theBuffer, const unsigned theLen)
{
    unsigned leftSize = sizeM - writeIndexM + readIndexM;
    unsigned putLen = (theLen < leftSize) ? theLen : leftSize;

    unsigned leftEnd = sizeM - (writeIndexM & maskM);
    unsigned firstPartLen = (putLen < leftEnd) ? putLen : leftEnd;
    memcpy(rawM + (writeIndexM & maskM), theBuffer, firstPartLen);
    memcpy(rawM, theBuffer + firstPartLen, putLen - firstPartLen);
    writeIndexM += putLen;
    return putLen;
}

//-----------------------------------------------------------------------------

unsigned KfifoBuffer::get(char* const theBuffer, const unsigned theLen)
{
    unsigned usedSize = writeIndexM - readIndexM;
    unsigned getLen = (theLen < usedSize) ? theLen : usedSize;

    unsigned readIndexToEnd = sizeM - (readIndexM & maskM);
    unsigned firstPartLen = (getLen < readIndexToEnd) ? getLen : readIndexToEnd;
    if (theBuffer)
    {
        memcpy(theBuffer, rawM + (readIndexM & maskM), firstPartLen);
        memcpy(theBuffer + firstPartLen, rawM, getLen - firstPartLen);
    }
    readIndexM += getLen;
    return getLen;
}

//-----------------------------------------------------------------------------

unsigned KfifoBuffer::peek(char* const theBuffer, const unsigned theLen)
{
    unsigned usedSize = writeIndexM - readIndexM;
    unsigned getLen = (theLen < usedSize) ? theLen : usedSize;

    unsigned readIndexToEnd = sizeM - (readIndexM & maskM);
    unsigned firstPartLen = (getLen < readIndexToEnd) ? getLen : readIndexToEnd;
    memcpy(theBuffer, rawM + (readIndexM & maskM), firstPartLen);
    memcpy(theBuffer + firstPartLen, rawM, getLen - firstPartLen);
    return getLen;
}

//-----------------------------------------------------------------------------

unsigned KfifoBuffer::putn(const char* const theBuffer, const unsigned theLen)
{
    unsigned leftSize = sizeM - writeIndexM + readIndexM;
    if (theLen > leftSize)
        return 0;
    unsigned putLen = theLen;

    unsigned leftEnd = sizeM - (writeIndexM & maskM);
    unsigned firstPartLen = (putLen < leftEnd) ? putLen : leftEnd;
    memcpy(rawM + (writeIndexM & maskM), theBuffer, firstPartLen);
    memcpy(rawM, theBuffer + firstPartLen, putLen - firstPartLen);
    writeIndexM += putLen;
    return putLen;
}

//-----------------------------------------------------------------------------

unsigned KfifoBuffer::getn(char* const theBuffer, const unsigned theLen)
{
    unsigned usedSize = writeIndexM - readIndexM;
    if (theLen > usedSize)
        return 0;
    unsigned getLen = theLen;

    unsigned readIndexToEnd = sizeM - (readIndexM & maskM);
    unsigned firstPartLen = (getLen < readIndexToEnd) ? getLen : readIndexToEnd;
    if (theBuffer)
    {
        memcpy(theBuffer, rawM + (readIndexM & maskM), firstPartLen);
        memcpy(theBuffer + firstPartLen, rawM, getLen - firstPartLen);
    }
    readIndexM += getLen;
    return getLen;
}

//-----------------------------------------------------------------------------

unsigned KfifoBuffer::peekn(char* const theBuffer, const unsigned theLen)
{
    unsigned usedSize = writeIndexM - readIndexM;
    if (theLen > usedSize)
        return 0;
    unsigned getLen = theLen;

    unsigned readIndexToEnd = sizeM - (readIndexM & maskM);
    unsigned firstPartLen = (getLen < readIndexToEnd) ? getLen : readIndexToEnd;
    memcpy(theBuffer, rawM + (readIndexM & maskM), firstPartLen);
    memcpy(theBuffer + firstPartLen, rawM, getLen - firstPartLen);
    return getLen;
}

//-----------------------------------------------------------------------------

unsigned KfifoBuffer::commitRead(const unsigned theLen)
{
    readIndexM += theLen;
    return theLen;
}


