
#ifndef __ui_base_slide_animation_h__
#define __ui_base_slide_animation_h__

#pragma once

#include "linear_animation.h"
#include "tween.h"

namespace ui
{

    // Slide Animation
    //
    // 用于辅助实现可逆动画. 典型用法:
    //
    //     #include "app/slide_animation.h"
    //
    //     class MyClass : public AnimationDelegate {
    //     public:
    //       MyClass() {
    //         animation_.reset(new SlideAnimation(this));
    //         animation_->SetSlideDuration(500);
    //       }
    //       void OnMouseOver() {
    //         animation_->Show();
    //       }
    //       void OnMouseOut() {
    //         animation_->Hide();
    //       }
    //       void AnimationProgressed(const Animation* animation) {
    //         if(animation == animation_.get()) {
    //           Layout();
    //           SchedulePaint();
    //         } else if(animation == other_animation_.get()) {
    //           ...
    //         }
    //       }
    //       void Layout() {
    //         if(animation_->is_animating()) {
    //           hover_image_.SetOpacity(animation_->GetCurrentValue());
    //         }
    //       }
    //     private:
    //       scoped_ptr<SlideAnimation> animation_;
    //     }
    class SlideAnimation : public LinearAnimation
    {
    public:
        explicit SlideAnimation(AnimationDelegate* target);
        virtual ~SlideAnimation();

        // 重置动画状态到0.
        virtual void Reset();
        virtual void Reset(double value);

        // 开始显示动画或者正在隐藏的动画逆过程.
        virtual void Show();

        // 开始隐藏动画或者正在显示的动画逆过程.
        virtual void Hide();

        // 设置滑动持续的时间. 注意如果滑动正在执行, 设置的值不会立即生效.
        virtual void SetSlideDuration(int duration) { slide_duration_ = duration; }
        int GetSlideDuration() const { return slide_duration_; }
        void SetTweenType(Tween::Type tween_type) { tween_type_ = tween_type; }

        double GetCurrentValue() const { return value_current_; }
        bool IsShowing() const { return showing_; }
        bool IsClosing() const { return !showing_ && value_end_<value_current_; }

    private:
        void AnimateToState(double state);

        AnimationDelegate* target_;

        Tween::Type tween_type_;

        // 用于确定动画执行方式.
        bool showing_;

        // 动画值. 基于Animation::state_提供动画的可逆性.
        double value_start_;
        double value_end_;
        double value_current_;

        // in/out动画持续的时间. 缺省是kHoverFadeDurationMS, 可以通过SetDuration
        // 设置新值.
        int slide_duration_;

        DISALLOW_COPY_AND_ASSIGN(SlideAnimation);
    };

} //namespace ui

#endif //__ui_base_slide_animation_h__