
#ifndef __animation_animation_h__
#define __animation_animation_h__

#pragma once

#include "base/memory/ref_counted.h"

#include "animation_container_element.h"

namespace gfx
{
    class Rect;
}

class AnimationContainer;
class AnimationDelegate;

// 动画基类. 可以从LinearAnimation、SlideAnimation、ThrobAnimation或者
// MultiAnimation中派生实现动画, 只有想要实现新的动画种类时才需要从这个类派生.
//
// 子类需要重载Step. 当动画前进时调用, GetCurrentValue()返回动画当时的值.
class Animation : public AnimationContainerElement
{
public:
    explicit Animation(base::TimeDelta timer_interval);
    virtual ~Animation();

    // 启动动画. 如果已经运行, 什么都不做.
    void Start();

    // 停止动画. 如果没有运行, 什么都不做.
    void Stop();

    // 根据使用的动画曲线得到当前状态的值.
    virtual double GetCurrentValue() const = 0;

    // 根据当前值返回一个介于|start|和|target|之间的值. 也就是:
    //   (target - start) * GetCurrentValue() + start
    double CurrentValueBetween(double start, double target) const;
    int CurrentValueBetween(int start, int target) const;
    gfx::Rect CurrentValueBetween(const gfx::Rect& start_bounds,
        const gfx::Rect& target_bounds) const;

    // 设置代理.
    void set_delegate(AnimationDelegate* delegate) { delegate_ = delegate; }

    // 设置管理时钟的容器. 传递NULL表示创建一个新的AnimationContainer.
    void SetContainer(AnimationContainer* container);

    bool is_animating() const { return is_animating_; }

    base::TimeDelta timer_interval() const { return timer_interval_; }

    // 如果代价较高的动画需要渲染返回true.
    // 检查会话类型(比如远程桌面)和访问性设置, 看是否需要渲染代价较高的动画.
    static bool ShouldRenderRichAnimation();

protected:
    // 在Start中调用, 允许派生类为动画做准备.
    virtual void AnimationStarted() {}

    // 从容器中移除后, 在Stop中调用, 此时代理还未被调用.
    virtual void AnimationStopped() {}

    // 动画停止时调用, 确定是否需要调用取消. 如果返回true, 通知代理动画被取消,
    // 否则通知代理动画被停止.
    virtual bool ShouldSendCanceledFromStop() { return false; }

    AnimationContainer* container() { return container_.get(); }
    base::TimeTicks start_time() const { return start_time_; }
    AnimationDelegate* delegate() { return delegate_; }

    virtual void SetStartTime(base::TimeTicks start_time);
    virtual void Step(base::TimeTicks time_now) = 0;
    virtual base::TimeDelta GetTimerInterval() const { return timer_interval_; }

private:
    // 动画间隔.
    const base::TimeDelta timer_interval_;

    // 是否在运行.
    bool is_animating_;

    // 代理; 允许为空.
    AnimationDelegate* delegate_;

    // 所在的容器. 正在动画时不为空.
    scoped_refptr<AnimationContainer> container_;

    // 启动时间.
    base::TimeTicks start_time_;

    DISALLOW_COPY_AND_ASSIGN(Animation);
};

#endif //__animation_animation_h__