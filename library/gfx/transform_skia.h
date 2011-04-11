
#ifndef __gfx_transform_skia_h__
#define __gfx_transform_skia_h__

#pragma once

#include "base/basic_types.h"
#include "base/scoped_ptr.h"

#include "transform.h"

class SkMatrix;

namespace gfx
{

    // 使用skia变换机制实现的Transformation.
    class TransformSkia : public Transform
    {
    public:
        TransformSkia();
        virtual ~TransformSkia();

        // Transform接口.
        virtual void SetRotate(float degree);
        virtual void SetScaleX(float x);
        virtual void SetScaleY(float y);
        virtual void SetScale(float x, float y);
        virtual void SetTranslateX(float x);
        virtual void SetTranslateY(float y);
        virtual void SetTranslate(float x, float y);
        virtual void ConcatRotate(float degree);
        virtual void ConcatScale(float x, float y);
        virtual void ConcatTranslate(float x, float y);
        virtual bool ConcatTransform(const Transform& transform);
        virtual bool HasChange() const;
        virtual bool TransformPoint(Point* point);
        virtual bool TransformPointReverse(Point* point);
        virtual bool TransformRect(Rect* rect);

    private:
        friend class CanvasSkia;
        scoped_ptr<SkMatrix> matrix_;

        DISALLOW_COPY_AND_ASSIGN(TransformSkia);
    };

} //namespace gfx

#endif //__gfx_transform_skia_h__