
#include "animation_container.h"

#include "animation_container_element.h"
#include "animation_container_observer.h"

using base::TimeDelta;
using base::TimeTicks;

namespace ui
{

    AnimationContainer::AnimationContainer()
        : last_tick_time_(TimeTicks::Now()), observer_(NULL) {}

    AnimationContainer::~AnimationContainer()
    {
        // animations拥有这个对象, 删除前先停止. 如果elements_不为空, 说明有
        // 错误发生.
        DCHECK(elements_.empty());
    }

    void AnimationContainer::Start(AnimationContainerElement* element)
    {
        DCHECK(elements_.count(element) == 0); // 只有当元素没运行的时候才启动.

        if(elements_.empty())
        {
            last_tick_time_ = TimeTicks::Now();
            SetMinTimerInterval(element->GetTimerInterval());
        }
        else if(element->GetTimerInterval() < min_timer_interval_)
        {
            SetMinTimerInterval(element->GetTimerInterval());
        }

        element->SetStartTime(last_tick_time_);
        elements_.insert(element);
    }

    void AnimationContainer::Stop(AnimationContainerElement* element)
    {
        DCHECK(elements_.count(element) > 0); // 元素必须是运行的.

        elements_.erase(element);

        if(elements_.empty())
        {
            timer_.Stop();
            if(observer_)
            {
                observer_->AnimationContainerEmpty(this);
            }
        }
        else
        {
            TimeDelta min_timer_interval = GetMinInterval();
            if(min_timer_interval > min_timer_interval_)
            {
                SetMinTimerInterval(min_timer_interval);
            }
        }
    }

    void AnimationContainer::Run()
    {
        // 在更新所有元素后通知观察者. 如果所有元素在更新时都被删除, 那么我们的引用计数
        // 将会变为0, 在通知观察者后也会被删除. 这里添加一次引用保证遍历所有元素后对象
        // 还是合法的.
        scoped_refptr<AnimationContainer> this_ref(this);

        TimeTicks current_time = TimeTicks::Now();

        last_tick_time_ = current_time;

        // 拷贝一份用于遍历, 这样在调用Step时移除任何元素都不会有问题.
        Elements elements = elements_;

        for(Elements::const_iterator i=elements.begin(); i!=elements.end(); ++i)
        {
            // 确保元素是合法的.
            if(elements_.find(*i) != elements_.end())
            {
                (*i)->Step(current_time);
            }
        }

        if(observer_)
        {
            observer_->AnimationContainerProgressed(this);
        }
    }

    void AnimationContainer::SetMinTimerInterval(base::TimeDelta delta)
    {
        // 不考虑和当前元素的时间差, 这对使用Animation/AnimationContainer来说不是问
        // 题.
        timer_.Stop();
        min_timer_interval_ = delta;
        timer_.Start(min_timer_interval_, this, &AnimationContainer::Run);
    }

    TimeDelta AnimationContainer::GetMinInterval()
    {
        DCHECK(!elements_.empty());

        TimeDelta min;
        Elements::const_iterator i = elements_.begin();
        min = (*i)->GetTimerInterval();
        for(++i; i!=elements_.end(); ++i)
        {
            if((*i)->GetTimerInterval() < min)
            {
                min = (*i)->GetTimerInterval();
            }
        }
        return min;
    }

} //namespace ui