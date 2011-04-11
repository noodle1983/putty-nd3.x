
#include "transform_skia.h"

#include "SkMatrix.h"

#include "point.h"
#include "rect.h"
#include "skia_util.h"

namespace gfx
{

    // static
    Transform* Transform::Create()
    {
        return new TransformSkia();
    }

    TransformSkia::TransformSkia()
    {
        matrix_.reset(new SkMatrix);
        matrix_->reset();
    }

    TransformSkia::~TransformSkia() {}

    void TransformSkia::SetRotate(float degree)
    {
        matrix_->setRotate(SkFloatToScalar(degree));
    }

    void TransformSkia::SetScaleX(float x)
    {
        matrix_->setScaleX(SkFloatToScalar(x));
    }

    void TransformSkia::SetScaleY(float y)
    {
        matrix_->setScaleY(SkFloatToScalar(y));
    }

    void TransformSkia::SetScale(float x, float y)
    {
        matrix_->setScale(SkFloatToScalar(x), SkFloatToScalar(y));
    }

    void TransformSkia::SetTranslateX(float x)
    {
        matrix_->setTranslateX(SkFloatToScalar(x));
    }

    void TransformSkia::SetTranslateY(float y)
    {
        matrix_->setTranslateY(SkFloatToScalar(y));
    }

    void TransformSkia::SetTranslate(float x, float y)
    {
        matrix_->setTranslate(SkFloatToScalar(x), SkFloatToScalar(y));
    }

    void TransformSkia::ConcatRotate(float degree)
    {
        matrix_->postRotate(SkFloatToScalar(degree));
    }

    void TransformSkia::ConcatScale(float x, float y)
    {
        matrix_->postScale(SkFloatToScalar(x), SkFloatToScalar(y));
    }

    void TransformSkia::ConcatTranslate(float x, float y)
    {
        matrix_->postTranslate(SkFloatToScalar(x), SkFloatToScalar(y));
    }

    bool TransformSkia::ConcatTransform(const Transform& transform)
    {
        return matrix_->setConcat(
            *reinterpret_cast<const TransformSkia&>(transform).matrix_.get(),
            *matrix_.get());
    }

    bool TransformSkia::HasChange() const
    {
        return !matrix_->isIdentity();
    }

    bool TransformSkia::TransformPoint(Point* point)
    {
        SkPoint skp;
        matrix_->mapXY(SkIntToScalar(point->x()), SkIntToScalar(point->y()), &skp);
        point->SetPoint(static_cast<int>(skp.fX), static_cast<int>(skp.fY));
        return true;
    }

    bool TransformSkia::TransformPointReverse(Point* point)
    {
        SkMatrix inverse;
        // WLW TODO: ³¢ÊÔ±ÜÃâÊ¹ÓÃÄæ¾ØÕó.
        if(matrix_->invert(&inverse))
        {
            SkPoint skp;
            inverse.mapXY(SkIntToScalar(point->x()), SkIntToScalar(point->y()), &skp);
            point->SetPoint(static_cast<int>(skp.fX), static_cast<int>(skp.fY));
            return true;
        }
        return false;
    }

    bool TransformSkia::TransformRect(Rect* rect)
    {
        SkRect src = RectToSkRect(*rect);
        if(!matrix_->mapRect(&src))
        {
            return false;
        }
        Rect xrect = SkRectToRect(src);
        rect->SetRect(xrect.x(), xrect.y(), xrect.width(), xrect.height());
        return true;
    }

} //namespace gfx