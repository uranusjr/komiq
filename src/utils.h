#ifndef UTILS_H
#define UTILS_H

class QFileInfo;

bool isImageFile(const QFileInfo &info);
bool isZipArchive(const QFileInfo &info);

#endif // UTILS_H
