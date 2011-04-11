
#ifndef __gfx_transform_h__
#define __gfx_transform_h__

#pragma once

namespace gfx
{

    class Point;
    class Rect;

    // 变换接口.
    // 实现接口的类用于界面组件的变换(比如旋转、缩放等).
    class Transform
    {
    public:
        virtual ~Transform() {}

        // 创建实现接口的对象(比如使用skia的变换机制).
        static Transform* Create();

        // 设置旋转角度.
        virtual void SetRotate(float degree) = 0;

        // 设置缩放参数.
        virtual void SetScaleX(float x) = 0;
        virtual void SetScaleY(float y) = 0;
        virtual void SetScale(float x, float y) = 0;

        // 设置平移参数.
        virtual void SetTranslateX(float x) = 0;
        virtual void SetTranslateY(float y) = 0;
        virtual void SetTranslate(float x, float y) = 0;

        // 在当前变换基础上进行旋转.
        virtual void ConcatRotate(float degree) = 0;

        // 在当前变换基础上进行缩放.
        virtual void ConcatScale(float x, float y) = 0;

        // 在当前变换基础上进行平移.
        virtual void ConcatTranslate(float x, float y) = 0;

        // 在当前变换基础上进行变换(例如 'this = this * transform;').
        virtual bool ConcatTransform(const Transform& transform) = 0;

        // 是否为有效变换?
        virtual bool HasChange() const = 0;

        // 对点应用当前变换.
        virtual bool TransformPoint(Point* point) = 0;

        // 对点应用当前逆变换.
        virtual bool TransformPointReverse(Point* point) = 0;

        // 对矩形应用当前变换.
        virtual bool TransformRect(Rect* rect) = 0;
    };

} //namespace gfx

#endif //__gfx_transform_h__