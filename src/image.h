#ifndef IMAGE_H
#define IMAGE_H

#include <QPixmap>

class Image
{
public:
    Image(const QPixmap &pixmap = QPixmap());

    const QPixmap &original() const;
    QPixmap scaledToFit(int w, int h) const;

    bool isNull() const;

private:
    QPixmap orig;
};

#endif // IMAGE_H
