
#ifndef __ui_base_throb_animation_h__
#define __ui_base_throb_animation_h__

#pragma once

#include "slide_animation.h"

namespace ui
{

    // SlideAnimation的派生类, 实现持续滑动. 所有方法的行为类似SlideAnimation的:
    // 过渡到下一状态. StartThrobbing方法执行一定数量的隐藏/显示变换.
    //
    // ThrobAnimation有两个持续时间: 一个用于实现SlideAnimation行为, 另一个用于
    // 抖动.
    class ThrobAnimation : public SlideAnimation
    {
    public:
        explicit ThrobAnimation(AnimationDelegate* target);
        virtual ~ThrobAnimation() {}

        // 开始抖动. cycles_til_stop指定抖动次数. 负数表示一直抖动.
        void StartThrobbing(int cycles_til_stop);

        // 设置抖动中的滑动动画持续时间.
        void SetThrobDuration(int duration) { throb_duration_ = duration; }

        virtual void Reset();
        virtual void Show();
        virtual void Hide();

        virtual void SetSlideDuration(int duration) { slide_duration_ = duration; }

        // 剩余的抖动次数.
        void set_cycles_remaining(int value) { cycles_remaining_ = value; }
        int cycles_remaining() const { return cycles_remaining_; }

    protected:
        // 重载实现连续抖动(假设正在抖动).
        virtual void Step(base::TimeTicks time_now);

    private:
        // 重置状态, 这样可以实现SlideAnimation行为.
        void ResetForSlide();

        // 滑动动画持续时间.
        int slide_duration_;

        // 抖动时滑动动画持续时间.
        int throb_duration_;

        // 如果正在抖动, 表示剩余的次数.
        int cycles_remaining_;

        // 正在抖动?
        bool throbbing_;

        DISALLOW_COPY_AND_ASSIGN(ThrobAnimation);
    };

} //namespace ui

#endif //__ui_base_throb_animation_h__