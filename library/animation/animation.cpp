
#include "animation.h"

#include "base/win/windows_version.h"

#include "gfx/rect.h"

#include "tween.h"
#include "animation_container.h"
#include "animation_delegate.h"

Animation::Animation(base::TimeDelta timer_interval)
: timer_interval_(timer_interval),
is_animating_(false), delegate_(NULL) {}

Animation::~Animation()
{
    // 在析构函数中不要发出通知. 可能代理拥有这个对象, 且也正被删除.
    if(is_animating_)
    {
        container_->Stop(this);
    }
}

void Animation::Start()
{
    if(is_animating_)
    {
        return;
    }

    if(!container_.get())
    {
        container_ = new AnimationContainer();
    }

    is_animating_ = true;

    container_->Start(this);

    AnimationStarted();
}

void Animation::Stop()
{
    if(!is_animating_)
    {
        return;
    }

    is_animating_ = false;

    // 先通知容器, 因为代理有可能会删除这个对象.
    container_->Stop(this);

    AnimationStopped();

    if(delegate_)
    {
        if(ShouldSendCanceledFromStop())
        {
            delegate_->AnimationCanceled(this);
        }
        else
        {
            delegate_->AnimationEnded(this);
        }
    }
}

double Animation::CurrentValueBetween(double start, double target) const
{
    return Tween::ValueBetween(GetCurrentValue(), start, target);
}

int Animation::CurrentValueBetween(int start, int target) const
{
    return Tween::ValueBetween(GetCurrentValue(), start, target);

}

gfx::Rect Animation::CurrentValueBetween(const gfx::Rect& start_bounds,
                                         const gfx::Rect& target_bounds) const
{
    return Tween::ValueBetween(GetCurrentValue(), start_bounds, target_bounds);
}

void Animation::SetContainer(AnimationContainer* container)
{
    if(container == container_.get())
    {
        return;
    }

    if(is_animating_)
    {
        container_->Stop(this);
    }

    if(container)
    {
        container_ = container;
    }
    else
    {
        container_ = new AnimationContainer();
    }

    if(is_animating_)
    {
        container_->Start(this);
    }
}

// static
bool Animation::ShouldRenderRichAnimation()
{
    if(base::GetWinVersion() >= base::WINVERSION_VISTA)
    {
        BOOL result;
        // 获取"关闭所有不必要的动画"值.
        if(SystemParametersInfo(SPI_GETCLIENTAREAANIMATION, 0, &result, 0))
        {
            // 貌似MSDN文档(2009五月)有一个错误:
            //   http://msdn.microsoft.com/en-us/library/ms724947(VS.85).aspx
            // 文档中说当动画禁止时返回TRUE, 但实际上是启用时返回TRUE.
            return !!result;
        }
    }
    return !::GetSystemMetrics(SM_REMOTESESSION);
}

void Animation::SetStartTime(base::TimeTicks start_time)
{
    start_time_ = start_time;
}