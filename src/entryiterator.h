#ifndef ENTRYITERATOR_H
#define ENTRYITERATOR_H

#include <QList>
#include <QScopedPointer>

class QFileInfo;
struct zip_t;

class EntryIterator
{
public:
    explicit EntryIterator(const QList<QFileInfo> &infos);

    struct SubIterator
    {
        virtual ~SubIterator() {}
        virtual QByteArray next() = 0;
    };

    static bool isValidEntry(const QFileInfo &info);

    QByteArray next();

private:
    QList<QFileInfo> infos;
    QList<QFileInfo>::const_iterator cur;
    QScopedPointer<SubIterator> iter;
};

#endif // ENTRYITERATOR_H
