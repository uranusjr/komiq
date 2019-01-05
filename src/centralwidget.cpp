#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include "centralwidget.h"
#include "entryiterator.h"

CentralWidget::CentralWidget(QWidget *parent) :
    QWidget(parent), iterator(nullptr),
    label1(new QLabel()), label2(new QLabel())
{
    this->setAcceptDrops(true);
    this->setAutoFillBackground(true);
    this->setMinimumSize(640, 480);

    // Black background.
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::black);
    this->setPalette(palette);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setDirection(QBoxLayout::RightToLeft);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addStretch(1);
    layout->addWidget(this->label1, 0, Qt::AlignCenter);
    layout->addWidget(this->label2, 0, Qt::AlignCenter);
    layout->addStretch(1);
    this->setLayout(layout);
}

CentralWidget::~CentralWidget()
{
    delete this->iterator;
}

bool CentralWidget::openLocalPaths(const QStringList &paths)
{
    QList<QFileInfo> infos;
    for (const QString &path : paths)
    {
        QFileInfo info(path);
        if (!info.exists() || !EntryIterator::isValidEntry(info))
            continue;
        infos.append(info);
    }
    if (infos.isEmpty())
        return false;
    this->populateOpenableEntries(infos);
    return true;
}

void CentralWidget::dragEnterEvent(QDragEnterEvent *event)
{
    for (auto url : event->mimeData()->urls())
    {
        if (!url.isLocalFile())
            continue;
        if (!EntryIterator::isValidEntry(QFileInfo(url.toLocalFile())))
            continue;
        event->acceptProposedAction();
        return;
    }
}

void CentralWidget::dropEvent(QDropEvent *event)
{
    QStringList paths;
    for (auto url : event->mimeData()->urls())
    {
        if (!url.isLocalFile())
            continue;
        paths.append(url.toLocalFile());
    }
    this->openLocalPaths(paths);
}

void CentralWidget::keyPressEvent(QKeyEvent *event)
{
    switch (event->key())
    {
    case Qt::Key::Key_Down:
        this->nextPage();
        break;
    case Qt::Key::Key_Up:
        this->previousPage();
        break;
    case Qt::Key::Key_Right:
        this->nextPage();
        break;
    case Qt::Key::Key_Left:
        this->previousPage();
        break;
    case Qt::Key::Key_Space:
        this->nextPage();
        break;
    default:
        break;
    }
}

void CentralWidget::mouseReleaseEvent(QMouseEvent *event)
{
    switch (event->button())
    {
    case Qt::MouseButton::LeftButton:
        this->nextPage();
        break;
    case Qt::MouseButton::RightButton:
        this->previousPage();
        break;
    default:
        break;
    }
}

void CentralWidget::resizeEvent(QResizeEvent *)
{
    this->refreshLabels();
}

void CentralWidget::wheelEvent(QWheelEvent *event)
{
    if (event->delta() < 0)     // Down.
        this->nextPage();
    else
        this->previousPage();
}

void CentralWidget::populateOpenableEntries(const QList<QFileInfo> &sources)
{
    QList<QFileInfo> infos = sources;
    std::sort(infos.begin(), infos.end(), [](const auto &lhs, const auto& rhs) {
        // TODO: Maybe we should do per-component comparison?
        return lhs.absoluteFilePath() < rhs.absoluteFilePath();
    });

    delete this->iterator;
    this->fCache.clear();
    this->bCache.clear();
    this->iterator = new EntryIterator(infos);

    this->label1->clear();
    this->label2->clear();
    this->nextPage();
}

void CentralWidget::nextPage()
{
    Image p;

    // Read one image. If this fails, we are at the end of the session. Do
    // nothing to stay on the last page.
    p = this->readNext();
    if (p.isNull())
        return;

    if (!this->image1.isNull())
        this->bCache.push(this->image1);
    this->image1 = p;

    // Read another image if the current one is not horizontal.
    if (p.isHorizontal())
        p = Image();
    else
        p = this->readNext();

    // If the the second read image is horizontal, that image needs to be on its
    // own page. Show only one image on the current page, and keep the read
    // horizontal image for the next.
    if (p.isHorizontal())
    {
        this->fCache.push(p);
        p = Image();
    }

    if (!this->image2.isNull())
        this->bCache.push(this->image2);
    this->image2 = p;

    this->refreshLabels();
}

void CentralWidget::previousPage()
{
    if (this->bCache.isEmpty())
        return;

    if (this->label2->pixmap())
        this->fCache.push(*this->label2->pixmap());
    if (this->label1->pixmap())
        this->fCache.push(*this->label1->pixmap());

    this->image1 = this->bCache.pop();
    if (!this->bCache.isEmpty())
    {
        this->image2 = this->image1;
        this->image1 = this->bCache.pop();
    }
    this->refreshLabels();
}

Image CentralWidget::readNext()
{
    if (this->fCache.size())
        return this->fCache.pop();
    if (!this->iterator)
        return Image();

    QPixmap pixmap;
    bool result = false;
    while (!result)
    {
        QByteArray bytes = this->iterator->next();
        if (bytes.isNull()) // No more to read.
            break;
        if (pixmap.loadFromData(bytes))
            break;
    }
    return Image(pixmap);
}

void CentralWidget::refreshLabels()
{
    QList<QLabel *> labels;
    labels.append(this->label1);
    labels.append(this->label2);

    QList<Image> images;
    images.append(this->image1);
    images.append(this->image2);

    int h = this->height();
    int w = this->width();
    if (!this->image2.isNull())
        w /= 2;

    QList<QLabel *>::const_iterator li;
    QList<Image>::const_iterator ii;
    for (li = labels.cbegin(), ii = images.cbegin();
         li != labels.cend() && ii != images.cend(); li++, ii++)
    {
        auto label = *li;
        auto image = *ii;
        if (image.isNull())
            label->clear();
        else
            label->setPixmap(image.scaledToFit(w, h));
        label->setVisible(!image.isNull());
    }
}
