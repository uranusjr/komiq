#include <QImageReader>
#include <QMimeDatabase>
#include "utils.h"

static QMimeDatabase mdb;

bool isImageFile(const QFileInfo &info)
{
    auto mime = mdb.mimeTypeForFile(info);
    for (auto name : QImageReader::supportedMimeTypes())
    {
        if (mime.inherits(QString::fromLocal8Bit(name)))
            return true;
    }
    return false;
}

bool isZipArchive(const QFileInfo &info)
{
    return mdb.mimeTypeForFile(info).inherits("application/zip");
}
