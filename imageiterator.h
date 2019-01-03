#ifndef IMAGEITERATOR_H
#define IMAGEITERATOR_H

#include <QList>
#include <QScopedPointer>
#include <QUrl>

struct zip_t;

class ImageIterator
{
public:
    explicit ImageIterator(const QList<QUrl> &urls);
    ~ImageIterator();

    QByteArray next();

private:
    QByteArray nextInZip();

    QList<QUrl> urls;
    QList<QUrl>::const_iterator cur;

    zip_t *zip;
    int zei;
};

#endif // IMAGEITERATOR_H
