
#ifndef __ui_base_animation_delegate_h__
#define __ui_base_animation_delegate_h__

#pragma once

namespace ui
{

    class Animation;

    // AnimationDelegate
    //
    // 想要接收animation的状态通知, 需要实现这个接口.
    class AnimationDelegate
    {
    public:
        // animation完成时调用.
        virtual void AnimationEnded(const Animation* animation) {}

        // animation步进时调用.
        virtual void AnimationProgressed(const Animation* animation) {}

        // animation取消时调用.
        virtual void AnimationCanceled(const Animation* animation) {}

    protected:
        virtual ~AnimationDelegate() {}
    };

} //namespace ui

#endif //__ui_base_animation_delegate_h__