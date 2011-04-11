
#ifndef __animation_linear_animation_h__
#define __animation_linear_animation_h__

#pragma once

#include "animation.h"

class AnimationDelegate;

// 线性时间限定的动画. 动画步进时调AnimateToState被调用.
class LinearAnimation : public Animation
{
public:
    // 初始化所有值, 除了持续时间.
    //
    // 如果调用者使用这个构造函数, 还必须调用SetDuration()函数. 最好一次
    // 构造好, 但有时持续时间在对象构造完成到调用Start()之间可能会改变,
    // 所以需要暴露这个接口.
    LinearAnimation(int frame_rate, AnimationDelegate* delegate);

    // 初始化所有成员.
    LinearAnimation(int duration, int frame_rate, AnimationDelegate* delegate);

    // 根据使用的动画曲线得到当前状态的值. LinearAnimation仅提供线性关系, 可以在
    // 派生类重载这个函数提供其它类型的值.
    virtual double GetCurrentValue() const;

    // 结束当前动画.
    void End();

    // 修改动画的持续时间. 会重置动画到开始状态.
    void SetDuration(int duration);

protected:
    // 动画步进的时候调用. 派生类重载这个函数更新状态.
    virtual void AnimateToState(double state) = 0;

    // 当动画需要前进时, 由AnimationContainer调用. 应该使用|time_now|而不是
    // Time::Now, 这样可以避免同一时间触发的动画不一致.
    virtual void Step(base::TimeTicks time_now);

    virtual void AnimationStopped();

    virtual bool ShouldSendCanceledFromStop();

private:
    base::TimeDelta duration_;

    // 当前状态, 范围是[0.0, 1.0].
    double state_;

    // 如果为true, 表示动画结束. AnimationStopped函数用它判断动画是否已经结束.
    bool in_end_;

    DISALLOW_COPY_AND_ASSIGN(LinearAnimation);
};

#endif //__animation_linear_animation_h__