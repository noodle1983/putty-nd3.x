
#ifndef __animation_animation_container_element_h__
#define __animation_animation_container_element_h__

#include "base/time.h"

#pragma once

// AnimationContainer容器的元素接口, 由Animation实现.
class AnimationContainerElement
{
public:
    // 设置animation的启动时间. 由AnimationContainer::Start调用.
    virtual void SetStartTime(base::TimeTicks start_time) = 0;

    // 当animation步进时调用.
    virtual void Step(base::TimeTicks time_now) = 0;

    // 返回animation的时间间隔. 如果一个元素想要改变这个值, 需要先
    // 调用Stop, 然后Start.
    virtual base::TimeDelta GetTimerInterval() const = 0;

protected:
    virtual ~AnimationContainerElement() {}
};

#endif //__animation_animation_container_element_h__