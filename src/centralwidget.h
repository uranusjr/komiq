#ifndef CENTRALWIDGET_H
#define CENTRALWIDGET_H

#include <QStack>
#include <QWidget>
#include "image.h"

class QFileInfo;
class QLabel;
class QTapGesture;
class EntryIterator;

class CentralWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CentralWidget(QWidget *parent = nullptr);
    ~CentralWidget() override;

    bool openLocalPaths(const QStringList &paths);
    bool openFiles(const QList<QFileInfo> &infos);

protected:
    bool event(QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void handleTap(QTapGesture *gesture);
    void populateOpenableEntries(const QList<QFileInfo> &infos);
    void nextPage();
    void previousPage();

    Image readNext();
    void refreshLabels();
    bool isVerticalMode() const;

    EntryIterator *iterator;

    Image image1;
    Image image2;
    QStack<Image> fCache;
    QStack<Image> bCache;

    QLabel *label1;
    QLabel *label2;

    QTimer *doubleTapTimer;
};

#endif // CENTRALWIDGET_H
