#ifndef CENTRALWIDGET_H
#define CENTRALWIDGET_H

#include <QStack>
#include <QUrl>
#include <QWidget>
#include "image.h"

class QLabel;
class EntryIterator;

class CentralWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CentralWidget(QWidget *parent = nullptr);
    ~CentralWidget() override;

    bool openUrls(const QList<QUrl> &urls);
    bool openLocalPaths(const QStringList &paths);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

signals:
    void filesAccepted(QList<QUrl> urls);
    void sessionOpened();

private:
    void populateOpenableUrls(QList<QUrl> urls);
    void nextPage();
    void previousPage();

    Image readNext();
    void refreshLabels();

    EntryIterator *iterator;

    Image image1;
    Image image2;
    QStack<Image> fCache;
    QStack<Image> bCache;

    QLabel *label1;
    QLabel *label2;
};

#endif // CENTRALWIDGET_H
