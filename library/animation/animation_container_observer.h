
#ifndef __animation_animation_container_observer_h__
#define __animation_animation_container_observer_h__

#pragma once

class AnimationContainer;

// 容器管理的animations每次更新都会通知观察者.
class AnimationContainerObserver
{
public:
    // 容器管理的时钟触发时, 在所有animations更新完成后调用本函数.
    virtual void AnimationContainerProgressed(
        AnimationContainer* container) = 0;

    // 当容器管理的animations为空时调用.
    virtual void AnimationContainerEmpty(AnimationContainer* container) = 0;

protected:
    virtual ~AnimationContainerObserver() {}
};

#endif //__animation_animation_container_observer_h__