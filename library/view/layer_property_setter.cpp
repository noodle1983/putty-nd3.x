
#include "layer_property_setter.h"

#include "base/memory/scoped_ptr.h"

#include "ui_base/compositor/compositor.h"
#include "ui_base/compositor/layer.h"
#include "ui_base/compositor/layer_animator.h"

namespace view
{

    namespace
    {

        // DefaultSetter ---------------------------------------------------------------

        class DefaultSetter : public LayerPropertySetter
        {
        public:
            DefaultSetter();

            // LayerPropertySetter:
            virtual void Installed(ui::Layer* layer);
            virtual void Uninstalled(ui::Layer* layer);
            virtual void SetTransform(ui::Layer* layer,
                const gfx::Transform& transform);
            virtual void SetBounds(ui::Layer* layer,
                const gfx::Rect& bounds);

        private:
            DISALLOW_COPY_AND_ASSIGN(DefaultSetter);
        };

        DefaultSetter::DefaultSetter() {}

        void DefaultSetter::Installed(ui::Layer* layer) {}

        void DefaultSetter::Uninstalled(ui::Layer* layer) {}

        void DefaultSetter::SetTransform(ui::Layer* layer,
            const gfx::Transform& transform)
        {
            layer->SetTransform(transform);
        }

        void DefaultSetter::SetBounds(ui::Layer* layer, const gfx::Rect& bounds)
        {
            layer->SetBounds(bounds);
        }

        // AnimatingSetter -------------------------------------------------------------

        class AnimatingSetter : public LayerPropertySetter
        {
        public:
            AnimatingSetter();

            // LayerPropertySetter:
            virtual void Installed(ui::Layer* layer);
            virtual void Uninstalled(ui::Layer* layer);
            virtual void SetTransform(ui::Layer* layer,
                const gfx::Transform& transform);
            virtual void SetBounds(ui::Layer* layer, const gfx::Rect& bounds);

        private:
            scoped_ptr<ui::LayerAnimator> animator_;

            DISALLOW_COPY_AND_ASSIGN(AnimatingSetter);
        };

        AnimatingSetter::AnimatingSetter() {}

        void AnimatingSetter::Installed(ui::Layer* layer)
        {
            animator_.reset(new ui::LayerAnimator(layer));
        }

        void AnimatingSetter::Uninstalled(ui::Layer* layer)
        {
            animator_.reset();
        }

        void AnimatingSetter::SetTransform(ui::Layer* layer,
            const gfx::Transform& transform)
        {
            animator_->AnimateTransform(transform);
        }

        void AnimatingSetter::SetBounds(ui::Layer* layer, const gfx::Rect& bounds)
        {
            if(bounds.size() == animator_->layer()->bounds().size())
            {
                animator_->AnimateToPoint(bounds.origin());
            }
            else
            {
                animator_->StopAnimatingToPoint();
            }
        }

    }

    // LayerPropertySetter ---------------------------------------------------------

    // static
    LayerPropertySetter* LayerPropertySetter::CreateDefaultSetter()
    {
        return new DefaultSetter;
    }

    // static
    LayerPropertySetter* LayerPropertySetter::CreateAnimatingSetter()
    {
        return new AnimatingSetter();
    }

} //namespace view