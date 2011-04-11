
#ifndef __animation_animation_delegate_h__
#define __animation_animation_delegate_h__

#pragma once

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

#endif //__animation_animation_delegate_h__