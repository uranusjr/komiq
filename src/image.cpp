#include "image.h"

Image::Image(const QPixmap &pixmap) : orig(pixmap)
{
}

const QPixmap &Image::original() const
{
    return this->orig;
}

QPixmap Image::scaledToFit(int w, int h) const
{
    if ((1.0 * w / h) > (1.0 * this->orig.width() / this->orig.height()))
        return this->orig.scaledToHeight(h, Qt::SmoothTransformation);
    return this->orig.scaledToWidth(w, Qt::SmoothTransformation);
}

bool Image::isNull() const
{
    return this->orig.isNull();
}

bool Image::isHorizontal() const
{
    if (this->orig.isNull())
        return false;
    return (this->orig.width() > this->orig.height());
}
