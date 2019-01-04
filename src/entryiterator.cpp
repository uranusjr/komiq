#include <QDebug>
#include <QDir>
#include "zip/zip.h"
#include "entryiterator.h"

EntryIterator::EntryIterator(const QList<QUrl> &urls) :
    urls(urls), zip(nullptr)
{
    this->cur = this->urls.cbegin();
}

EntryIterator::~EntryIterator()
{
    if (this->zip)
        zip_close(this->zip);
}

QByteArray EntryIterator::next()
{
    // Try the current zip.
    QByteArray bytes = this->nextInZip();
    if (!bytes.isNull())
        return bytes;

    // The current zip is finished. Try to move to next zip.

    // No more to read. Give up.
    if (this->cur == this->urls.cend())
        return bytes;

    // Open a new one to read.
    // TODO: This cannot handle non-ASCII characters. How viable is it to patch
    // zip.c to use Qt's file API instead?
    auto filename = QDir::toNativeSeparators((*this->cur).toLocalFile());
    this->zip = zip_open(filename.toLocal8Bit().constData(), 0, 'r');
    if (!this->zip)
        qDebug() << "Failed to open" << filename;
    this->entries.clear();
    if (this->zip)
    {
        for (int i = 0; i < zip_total_entries(this->zip); i++)
        {
            zip_entry_openbyindex(this->zip, i);
            QString name = QString::fromLocal8Bit(zip_entry_name(this->zip));
            this->entries.append(name);
        }
        std::sort(this->entries.begin(), this->entries.end());
        this->entit = this->entries.cbegin();
    }
    this->cur++;

    return this->nextInZip();
}

QByteArray EntryIterator::nextInZip()
{
    if (!this->zip)
        return QByteArray();

    // Already at the end of the current zip. Close it.
    if (this->entit == this->entries.cend())
    {
        zip_close(this->zip);
        this->zip = nullptr;
        return QByteArray();
    }

    zip_entry_open(this->zip, (*entit).toLocal8Bit().constData());
    auto bufsize = zip_entry_size(this->zip);
    QByteArray bytes(static_cast<int>(bufsize), '\0');
    zip_entry_noallocread(this->zip, bytes.data(), bufsize);
    this->entit++;

    return bytes;
}
