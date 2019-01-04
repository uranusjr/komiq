#ifndef ENTRYITERATOR_H
#define ENTRYITERATOR_H

#include <QList>
#include <QUrl>

struct zip_t;

class EntryIterator
{
public:
    explicit EntryIterator(const QList<QUrl> &urls);
    ~EntryIterator();

    QByteArray next();

private:
    QByteArray nextInZip();

    QList<QUrl> urls;
    QList<QUrl>::const_iterator cur;

    zip_t *zip;
    QStringList entries;
    QStringList::const_iterator entit;
};

#endif // ENTRYITERATOR_H
