#include <QDirIterator>
#include "zip/zip.h"
#include "entryiterator.h"
#include "utils.h"

namespace
{

class ImageFileIterator : public EntryIterator::SubIterator
{
public:
    ImageFileIterator(const QFileInfo &info) : info(info), done(false) {}

    QByteArray next()
    {
        QByteArray data;
        if (this->done)
            return data;
        QFile file(this->info.absoluteFilePath());
        file.open(QIODevice::ReadOnly);
        data = file.readAll();
        file.close();
        this->done = true;
        return data;
    }

private:
    QFileInfo info;
    bool done;
};

class DirectoryIterator : public EntryIterator::SubIterator
{
public:
    DirectoryIterator(const QDir &dir) :
        iter(dir, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks)
    {}

    QByteArray next()
    {
        while (true)
        {
            if (this->iter.next().isEmpty())
                return QByteArray();
            auto info = this->iter.fileInfo();
            if (!isImageFile(info))
                continue;
            QFile file(info.absoluteFilePath());
            file.open(QIODevice::ReadOnly);
            QByteArray data = file.readAll();
            file.close();
            return data;
        }
    }

private:
    QDirIterator iter;
};

class ZipArchiveIterator : public EntryIterator::SubIterator
{
public:
    ZipArchiveIterator(const QFileInfo &info) : zip(nullptr)
    {
        // TODO: This cannot handle non-ASCII characters. How viable is it to
        // patch zip.c to use Qt's file API instead?
        auto filename = QDir::toNativeSeparators(info.absoluteFilePath());
        this->zip = zip_open(filename.toLocal8Bit().constData(), 0, 'r');
        if (!this->zip)
            return;
        for (int i = 0; i < zip_total_entries(this->zip); i++)
        {
            zip_entry_openbyindex(this->zip, i);
            QString name = QString::fromLocal8Bit(zip_entry_name(this->zip));
            this->entries.append(name);
        }
        // TODO: Maybe we should do per-component comparison?
        std::sort(this->entries.begin(), this->entries.end());
        this->iter = this->entries.cbegin();
    }

    ~ZipArchiveIterator()
    {
        if (this->zip)
            zip_close(this->zip);
    }

    QByteArray next()
    {
        if (!this->zip || this->iter == this->entries.cend())
            return QByteArray();
        zip_entry_open(this->zip, (*this->iter).toLocal8Bit().constData());
        auto bufsize = zip_entry_size(this->zip);
        QByteArray bytes(static_cast<int>(bufsize), '\0');
        zip_entry_noallocread(this->zip, bytes.data(), bufsize);
        this->iter++;
        return bytes;
    }

private:
    zip_t *zip;
    QStringList entries;
    QStringList::const_iterator iter;
};

}   // (anonymous namespace)

EntryIterator::EntryIterator(const QList<QFileInfo> &infos) : infos(infos)
{
    this->cur = this->infos.cbegin();
}

bool EntryIterator::isValidEntry(const QFileInfo &info)
{
    return (info.isDir() || isImageFile(info) || isZipArchive(info));
}

QByteArray EntryIterator::next()
{
    while (true)
    {
        // Try the current subiterator.
        QByteArray bytes;
        if (this->iter)
        {
            bytes = this->iter->next();
            if (!bytes.isNull())
                return bytes;
        }

        // The current subiterator is finished. Try to move to next entry.

        // No more to read. Give up.
        if (this->cur == this->infos.cend())
            return bytes;

        // Open a new one to read.
        SubIterator *next;
        auto info = *this->cur;
        if (info.isDir())
            next = new DirectoryIterator(info.dir());
        else if (isImageFile(info))
            next = new ImageFileIterator(info);
        else if (isZipArchive(info))
            next = new ZipArchiveIterator(info);
        else
            return QByteArray();
        this->iter.reset(next);
        this->cur++;
    }
}
