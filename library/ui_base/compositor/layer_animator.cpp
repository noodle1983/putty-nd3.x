
#include "layer_animator.h"

#include "base/logging.h"
#include "base/stl_utilinl.h"

#include "ui_gfx/transform.h"
#include "ui_gfx/rect.h"

#include "ui_base/animation/animation_container.h"
#include "ui_base/animation/multi_animation.h"
#include "compositor.h"
#include "layer.h"

namespace
{

    void SetMatrixElement(SkMatrix44& matrix, int index, SkMScalar value)
    {
        int row = index / 4;
        int col = index % 4;
        matrix.set(row, col, value);
    }

    SkMScalar GetMatrixElement(const SkMatrix44& matrix, int index)
    {
        int row = index / 4;
        int col = index % 4;
        return matrix.get(row, col);
    }

}

namespace ui
{

    LayerAnimator::LayerAnimator(Layer* layer)
        : layer_(layer),
        duration_in_ms_(200),
        animation_type_(Tween::EASE_IN) {}

    LayerAnimator::~LayerAnimator()
    {
        for(Elements::iterator i=elements_.begin(); i!=elements_.end(); ++i)
        {
            delete i->second.animation;
        }
        elements_.clear();
    }

    void LayerAnimator::SetAnimationDurationAndType(int duration,
        Tween::Type tween_type)
    {
        DCHECK_GT(duration, 0);
        duration_in_ms_ = duration;
        animation_type_ = tween_type;
    }

    void LayerAnimator::AnimateToPoint(const gfx::Point& target)
    {
        StopAnimating(LOCATION);
        const gfx::Rect& layer_bounds = layer_->bounds();
        if(target == layer_bounds.origin())
        {
            return; // Already there.
        }

        Element& element = elements_[LOCATION];
        element.params.location.start_x = layer_bounds.origin().x();
        element.params.location.start_y = layer_bounds.origin().y();
        element.params.location.target_x = target.x();
        element.params.location.target_y = target.y();
        element.animation = CreateAndStartAnimation();
    }

    void LayerAnimator::AnimateTransform(const gfx::Transform& transform)
    {
        StopAnimating(TRANSFORM);
        const gfx::Transform& layer_transform = layer_->transform();
        if(transform == layer_transform)
        {
            return; // Already there.
        }

        Element& element = elements_[TRANSFORM];
        for(int i=0; i<16; ++i)
        {
            element.params.transform.start[i] =
                GetMatrixElement(layer_transform.matrix(), i);
            element.params.transform.target[i] =
                GetMatrixElement(transform.matrix(), i);
        }
        element.animation = CreateAndStartAnimation();
    }

    void LayerAnimator::AnimationProgressed(const Animation* animation)
    {
        Elements::iterator e = GetElementByAnimation(
            static_cast<const ui::MultiAnimation*>(animation));
        DCHECK(e != elements_.end());
        switch(e->first)
        {
        case LOCATION:
            {
                const gfx::Rect& current_bounds(layer_->bounds());
                gfx::Rect new_bounds = e->second.animation->CurrentValueBetween(
                    gfx::Rect(gfx::Point(e->second.params.location.start_x,
                    e->second.params.location.start_y),
                    current_bounds.size()),
                    gfx::Rect(gfx::Point(e->second.params.location.target_x,
                    e->second.params.location.target_y),
                    current_bounds.size()));
                layer_->SetBounds(new_bounds);
                break;
            }

        case TRANSFORM:
            {
                gfx::Transform transform;
                for(int i=0; i<16; ++i)
                {
                    SkMScalar value = e->second.animation->CurrentValueBetween(
                        e->second.params.transform.start[i],
                        e->second.params.transform.target[i]);
                    SetMatrixElement(transform.matrix(), i, value);
                }
                layer_->SetTransform(transform);
                break;
            }

        default:
            NOTREACHED();
        }
        layer_->compositor()->SchedulePaint();
    }

    void LayerAnimator::AnimationEnded(const ui::Animation* animation)
    {
        Elements::iterator e = GetElementByAnimation(
            static_cast<const MultiAnimation*>(animation));
        DCHECK(e != elements_.end());
        switch (e->first)
        {
        case LOCATION:
            {
                gfx::Rect new_bounds(
                    gfx::Point(e->second.params.location.target_x,
                    e->second.params.location.target_y),
                    layer_->bounds().size());
                layer_->SetBounds(new_bounds);
                break;
            }

        case TRANSFORM:
            {
                gfx::Transform transform;
                for(int i=0; i<16; ++i)
                {
                    SetMatrixElement(transform.matrix(), i,
                        e->second.params.transform.target[i]);
                }
                layer_->SetTransform(transform);
                break;
            }

        default:
            NOTREACHED();
        }
        StopAnimating(e->first);
        // StopAnimating removes from the map, invalidating 'e'.
        e = elements_.end();
        layer_->compositor()->SchedulePaint();
    }

    void LayerAnimator::StopAnimating(AnimationProperty property)
    {
        if(elements_.count(property) == 0)
        {
            return;
        }

        // Reset the delegate so that we don't attempt to update the layer.
        elements_[property].animation->set_delegate(NULL);
        delete elements_[property].animation;
        elements_.erase(property);
    }

    MultiAnimation* LayerAnimator::CreateAndStartAnimation()
    {
        static AnimationContainer* container = NULL;
        if(!container)
        {
            container = new AnimationContainer;
            container->AddRef();
        }
        MultiAnimation::Parts parts;
        parts.push_back(MultiAnimation::Part(duration_in_ms_, animation_type_));
        MultiAnimation* animation = new MultiAnimation(parts);
        animation->SetContainer(container);
        animation->set_delegate(this);
        animation->set_continuous(false);
        animation->Start();
        return animation;
    }

    LayerAnimator::Elements::iterator LayerAnimator::GetElementByAnimation(
        const MultiAnimation* animation)
    {
        for(Elements::iterator i=elements_.begin(); i!=elements_.end(); ++i)
        {
            if(i->second.animation == animation)
            {
                return i;
            }
        }
        NOTREACHED();
        return elements_.end();
    }

} //namespace ui