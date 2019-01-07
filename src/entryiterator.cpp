#include <QDirIterator>
#include <QImageReader>
#include <QMimeDatabase>
#ifdef Q_OS_WIN
#include <Windows.h>
#endif
#include "zip/zip.h"
#include "entryiterator.h"

static QMimeDatabase mdb;

// This needs to be out-of-line to make Clang happy.
// https://stackoverflow.com/questions/28786473
EntryIterator::SubIterator::~SubIterator()
{
}

#ifdef Q_OS_WIN
static zip_t *zip_open_unicode(const QFileInfo &info, int level, char mode)
{
    auto path = QDir::toNativeSeparators(info.absoluteFilePath());
    if (path.isEmpty())
        return nullptr;

    // Convert path components to short names, which allows fopen_s (used by
    // miniz internally) to handle non-ASCII paths.

    wchar_t *full = new wchar_t[static_cast<size_t>(path.size()) + 1];
    full[path.toWCharArray(full)] = '\0';

    auto len = ::GetShortPathNameW(full, nullptr, 0);
    wchar_t *shrt = new wchar_t[len];
    ::GetShortPathNameW(full, shrt, len);

    auto shortened = QString::fromWCharArray(shrt, static_cast<int>(len));
    delete [] full;
    delete [] shrt;

    return zip_open(shortened.toLocal8Bit().constData(), level, mode);
}
#else
static zip_t *zip_open_unicode(const QFileInfo &info, int level, char mode)
{
    auto filename = QDir::toNativeSeparators(info.absoluteFilePath());
    return zip_open(filename.toLocal8Bit().constData(), level, mode);
}
#endif

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

    QString name() const { return this->info.absoluteFilePath(); }

private:
    QFileInfo info;
    bool done;
};

class ZipArchiveIterator : public EntryIterator::SubIterator
{
public:
    ZipArchiveIterator(const QFileInfo &info) :
        info(info), zip(zip_open_unicode(info, 0, 'r'))
    {
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

    QString name() const { return this->info.absoluteFilePath(); }

private:
    QFileInfo info;
    zip_t *zip;
    QStringList entries;
    QStringList::const_iterator iter;
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
            if (this->sub)
            {
                auto next = this->sub->next();
                if (!next.isNull())
                    return next;
            }
            if (this->iter.next().isEmpty())
                return QByteArray();
            auto info = this->iter.fileInfo();
            switch (EntryIterator::fileType(info))
            {
            case EntryIterator::Unsuppoerted:
            case EntryIterator::Directory:
                break;
            case EntryIterator::Image:
                this->sub.reset(new ImageFileIterator(info));
                break;
            case EntryIterator::ZipArchive:
                this->sub.reset(new ZipArchiveIterator(info));
                break;
            }
        }
    }

    QString name() const { return this->iter.fileInfo().absoluteFilePath(); }

private:
    QDirIterator iter;
    QScopedPointer<EntryIterator::SubIterator> sub;
};

}   // (anonymous namespace)

EntryIterator::EntryIterator(const QList<QFileInfo> &infos) : infos(infos)
{
    this->cur = this->infos.cbegin();
}

EntryIterator::FileType EntryIterator::fileType(const QFileInfo &info)
{
    if (info.isDir())
        return Directory;
    auto mime = mdb.mimeTypeForFile(info);
    if (mime.inherits("application/zip"))
        return ZipArchive;
    for (auto name : QImageReader::supportedMimeTypes())
    {
        if (mime.inherits(QString::fromLocal8Bit(name)))
            return Image;
    }
    return Unsuppoerted;
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
        switch (fileType(info))
        {
        case Unsuppoerted:
            return bytes;
        case Directory:
            next = new DirectoryIterator(QDir(info.absoluteFilePath()));
            break;
        case Image:
            next = new ImageFileIterator(info);
            break;
        case ZipArchive:
            next = new ZipArchiveIterator(info);
            break;
        }
        this->iter.reset(next);
        this->cur++;
    }
}

QString EntryIterator::currentName() const
{
    return this->iter->name();
}
