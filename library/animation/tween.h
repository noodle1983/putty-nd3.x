
#ifndef __animation_tween_h__
#define __animation_tween_h__

#pragma once

#include "base/basic_types.h"

namespace gfx
{
    class Rect;
}

class Tween
{
public:
    enum Type
    {
        LINEAR,        // Linear.
        EASE_OUT,      // Fast in, slow out (default).
        EASE_IN,       // Slow in, fast out.
        EASE_IN_OUT,   // Slow in and out, fast in the middle.
        FAST_IN_OUT,   // Fast in and out, slow in the middle.
        EASE_OUT_SNAP, // Fast in, slow out, snap to final value.
        ZERO,          // Returns a value of 0 always.
    };

    // 根据补间类型返回state的新值. |state|的值从0到1.
    static double CalculateValue(Type type, double state);

    // 方便获取start到end之间的一个值.
    static double ValueBetween(double value, double start, double target);
    static int ValueBetween(double value, int start, int target);
    static gfx::Rect ValueBetween(double value,
        const gfx::Rect& start_bounds, const gfx::Rect& target_bounds);

private:
    Tween();
    ~Tween();

    DISALLOW_COPY_AND_ASSIGN(Tween);
};

#endif //__animation_tween_h__