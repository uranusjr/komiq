#include <QApplication>
#include <QFileInfo>
#include <QGestureEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QTimer>
#include "centralwidget.h"
#include "entryiterator.h"

CentralWidget::CentralWidget(QWidget *parent) :
    QWidget(parent), iterator(nullptr),
    label1(new QLabel()), label2(new QLabel()),
    doubleTapTimer(new QTimer(this))
{
    this->setAcceptDrops(true);
    this->setAutoFillBackground(true);
    this->grabGesture(Qt::TapGesture);
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

    this->doubleTapTimer->setSingleShot(true);
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

bool CentralWidget::event(QEvent *event)
{
    if (event->type() == QEvent::Gesture)
    {
        auto e = dynamic_cast<QGestureEvent *>(event);
        for (auto gesture : e->gestures())
        {
            if (gesture->gestureType() == Qt::TapGesture
                    && gesture->state() == Qt::GestureFinished)
                this->handleTap(dynamic_cast<QTapGesture *>(gesture));
        }
        return true;
    }
    return QWidget::event(event);
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
    // HACK: Is there a way to avoid triggering click events AT ALL at the
    // synthesizer level?
    if (event->source() == Qt::MouseEventSynthesizedByQt)
        return;

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
    if (this->isVerticalMode())
    {
        if (!this->image2.isNull())
        {
            this->fCache.push(this->image2);
            this->image2 = Image();
        }
    }
    else
    {
        if (this->image2.isNull())
            this->image2 = this->readNext();
    }
    this->refreshLabels();
}

void CentralWidget::wheelEvent(QWheelEvent *event)
{
    if (event->delta() < 0)     // Down.
        this->nextPage();
    else
        this->previousPage();
}

void CentralWidget::handleTap(QTapGesture *)
{
    // Trigger double tap event.
    if (this->doubleTapTimer->isActive())
    {
        this->doubleTapTimer->stop();
        this->previousPage();
        return;
    }

    // Wait for the next tap. We reuse the system's double click setting, but
    // cap the upper limit to avoid delaying single taps.
    static QMetaObject::Connection conn;
    if (!conn)
    {
        conn = this->connect(this->doubleTapTimer, &QTimer::timeout,
                             this, &CentralWidget::nextPage,
                             Qt::QueuedConnection);
    }
    this->doubleTapTimer->start(std::min(qApp->doubleClickInterval(), 300));
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

    QString currentName = this->iterator->currentName();

    if (!this->image1.isNull())
        this->bCache.push(this->image1);
    this->image1 = p;

    // Read another image in non-vertical mode, and if the current is not
    // horizontal.
    if (this->isVerticalMode() || p.isHorizontal())
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

    QString title = QString("%1 - %2")
            .arg(qApp->applicationName())
            .arg(currentName);
    this->setWindowTitle(title);
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
    this->image2 = Image();

    // We want another image (if there is one) in non-vertical mode, and if the
    // current image is not horizontal...
    if (this->bCache.size() && !this->isVerticalMode()
            && !this->image1.isHorizontal())
    {
        auto image = this->bCache.pop();
        if (image.isHorizontal())
        {
            // ... But not if the newly-loaded page is itself horizontal.
            this->bCache.push(image);
        }
        else
        {
            this->image2 = this->image1;
            this->image1 = image;
        }
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
    while (true)
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

    // Special case: In vertical mode, if the current image is horizontal,
    // rotate if to view in maximum.
    if (this->isVerticalMode() && this->image1.isHorizontal())
    {
        auto pixmap = this->image1.scaledToFit(h, w);
        QMatrix matrix;
        matrix.rotate(90);
        this->label1->setPixmap(pixmap.transformed(matrix));
    }
}

bool CentralWidget::isVerticalMode() const
{
    return this->width() < this->height();
}
