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

    enum FileType
    {
        Unsuppoerted,
        Directory,
        Image,
        ZipArchive,
    };

    struct SubIterator
    {
        virtual ~SubIterator() {}
        virtual QByteArray next() = 0;
    };

    static FileType fileType(const QFileInfo &info);

    inline static bool isValidEntry(const QFileInfo &info)
    { return fileType(info) != FileType::Unsuppoerted; }

    inline static bool isImageFile(const QFileInfo &info)
    { return fileType(info) == FileType::Image; }

    inline static bool isZipArchive(const QFileInfo &info)
    { return fileType(info) == FileType::ZipArchive; }

    QByteArray next();

private:
    QList<QFileInfo> infos;
    QList<QFileInfo>::const_iterator cur;
    QScopedPointer<SubIterator> iter;
};

#endif // ENTRYITERATOR_H
