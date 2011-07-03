
#ifndef __ui_base_layer_animator_h__
#define __ui_base_layer_animator_h__

#pragma once

#include <map>

#include "base/basic_types.h"

#include "SkScalar.h"

#include "ui_gfx/point.h"

#include "ui_base/animation/animation_delegate.h"
#include "ui_base/animation/tween.h"

namespace gfx
{
    class Transform;
}

namespace ui
{

    class Layer;
    class MultiAnimation;

    // LayerAnimator manages animating various properties of a Layer.
    class LayerAnimator : public AnimationDelegate
    {
    public:
        explicit LayerAnimator(Layer* layer);
        virtual ~LayerAnimator();

        Layer* layer() { return layer_; }

        // Sets the duration (in ms) and type of animation. This does not effect
        // existing animations, only newly created animations.
        void SetAnimationDurationAndType(int duration, Tween::Type tween_type);

        // Animates the layer to the specified point. The point is relative to the
        // parent layer.
        void AnimateToPoint(const gfx::Point& target);
        void StopAnimatingToPoint()
        {
            StopAnimating(LOCATION);
        }

        // Animates the transform from from the current transform to |transform|.
        void AnimateTransform(const gfx::Transform& transform);
        void StopAnimatingTransform()
        {
            StopAnimating(TRANSFORM);
        }

        // AnimationDelegate:
        virtual void AnimationProgressed(const Animation* animation);
        virtual void AnimationEnded(const Animation* animation);

    private:
        // Types of properties that can be animated.
        enum AnimationProperty
        {
            LOCATION,
            TRANSFORM
        };

        // Parameters used when animating the location.
        struct LocationParams
        {
            int start_x;
            int start_y;
            int target_x;
            int target_y;
        };

        // Parameters used whe animating the transform.
        struct TransformParams
        {
            // TODO: make 4x4 whe Transform is updated.
            SkScalar start[9];
            SkScalar target[9];
        };

        union Params
        {
            LocationParams location;
            TransformParams transform;
        };

        // Used for tracking the animation of a particular property.
        struct Element
        {
            Params params;
            MultiAnimation* animation;
        };

        typedef std::map<AnimationProperty, Element> Elements;

        // Stops animating the specified property. This does not set the property
        // being animated to its final value.
        void StopAnimating(AnimationProperty property);

        // Creates an animation.
        MultiAnimation* CreateAndStartAnimation();

        // Returns an iterator into |elements_| that matches the specified animation.
        Elements::iterator GetElementByAnimation(const MultiAnimation* animation);

        // The layer.
        Layer* layer_;

        // Properties being animated.
        Elements elements_;

        // Duration in ms for newly created animations.
        int duration_in_ms_;

        // Type of animation for newly created animations.
        Tween::Type animation_type_;

        DISALLOW_COPY_AND_ASSIGN(LayerAnimator);
    };

} //namespace ui

#endif //__ui_base_layer_animator_h__