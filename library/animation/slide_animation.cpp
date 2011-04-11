
#include "slide_animation.h"

#include <math.h>

// 每秒钟的帧数.
static const int kDefaultFramerateHz = 50;

// 动画缺省的持续时间.
static const int kDefaultDurationMs = 120;

SlideAnimation::SlideAnimation(AnimationDelegate* target)
: LinearAnimation(kDefaultFramerateHz, target),
target_(target),
tween_type_(Tween::EASE_OUT),
showing_(false),
value_start_(0),
value_end_(0),
value_current_(0),
slide_duration_(kDefaultDurationMs) {}

SlideAnimation::~SlideAnimation() {}

void SlideAnimation::Reset()
{
    Reset(0);
}

void SlideAnimation::Reset(double value)
{
    Stop();
    showing_ = static_cast<bool>(value == 1);
    value_current_ = value;
}

void SlideAnimation::Show()
{
    // 如果正在显示(或已完全显示), 什么都不做.
    if(showing_)
    {
        return;
    }

    showing_ = true;
    value_start_ = value_current_;
    value_end_ = 1.0;

    // 确保能做一些事情.
    if(slide_duration_ == 0)
    {
        AnimateToState(1.0); // 跳到动画结尾.
        return;
    }
    else if(value_current_ == value_end_) 
    {
        return;
    }

    // 当前正在发生的动画会被重置.
    SetDuration(static_cast<int>(slide_duration_ * (1 - value_current_)));
    Start();
}

void SlideAnimation::Hide()
{
    // 如果正在隐藏(或已完全隐藏), 什么都不做.
    if(!showing_)
    {
        return;
    }

    showing_ = false;
    value_start_ = value_current_;
    value_end_ = 0.0;

    // 确保能做一些事情.
    if(slide_duration_ == 0)
    {
        AnimateToState(0.0); // 跳到动画结尾(这种情况下等于开头).
        return;
    }
    else if(value_current_ == value_end_)
    {
        return;
    }

    // 当前正在发生的动画会被重置.
    SetDuration(static_cast<int>(slide_duration_ * value_current_));
    Start();
}

void SlideAnimation::AnimateToState(double state)
{
    if(state > 1.0)
    {
        state = 1.0;
    }

    state = Tween::CalculateValue(tween_type_, state);

    value_current_ = value_start_ + (value_end_ - value_start_) * state;

    // 实现吸附.
    if(tween_type_==Tween::EASE_OUT_SNAP &&
        fabs(value_current_-value_end_)<=0.06)
    {
        value_current_ = value_end_;
    }

    // 修正越界值(当前值可能超过1.0, 不给兜圈任何错误机会).
    if((value_end_>=value_start_ && value_current_>value_end_) ||
        (value_end_<value_start_ && value_current_<value_end_))
    {
        value_current_ = value_end_;
    }
}