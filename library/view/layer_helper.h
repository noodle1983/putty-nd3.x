
#ifndef __view_layer_helper_h__
#define __view_layer_helper_h__

#pragma once

#include "base/basic_types.h"
#include "base/memory/scoped_ptr.h"

#include "ui_gfx/rect.h"

namespace gfx
{
    class Transform;
}

namespace ui
{
    class Layer;
}

namespace view
{

    class LayerPropertySetter;

    // This is a views-internal API and should not be used externally. View uses
    // this class to manage fields related to accelerated painting.
    namespace internal
    {

        class LayerHelper
        {
        public:
            LayerHelper();
            ~LayerHelper();

            void SetTransform(const gfx::Transform& transform);

            // Only returns non-null if a non-identity transform has been set.
            const gfx::Transform* transform() const { return transform_.get(); }

            void SetLayer(ui::Layer* layer);
            ui::Layer* layer() { return layer_.get(); }

            // Rectangle that needs to be painted.
            void set_clip_rect(const gfx::Rect& rect)
            {
                clip_rect_ = rect;
            }
            const gfx::Rect& clip_rect() const { return clip_rect_; }

            // If true the layer was explicitly turned on.
            void set_paint_to_layer(bool value) { paint_to_layer_ = value; }
            bool paint_to_layer() const { return paint_to_layer_; }

            // See description in View for details
            void set_fills_bounds_opaquely(bool fills_bounds_opaquely)
            {
                fills_bounds_opaquely_ = fills_bounds_opaquely;
            }
            bool fills_bounds_opaquely() const { return fills_bounds_opaquely_; }

            void SetPropertySetter(LayerPropertySetter* setter);
            LayerPropertySetter* property_setter()
            {
                return property_setter_.get();
            }

            // If true the LayerPropertySetter was explicitly set.
            void set_property_setter_explicitly_set(bool value)
            {
                property_setter_explicitly_set_ = value;
            }
            bool property_setter_explicitly_set()
            {
                return property_setter_explicitly_set_;
            }

            // See View::SetExternalTexture for details.
            void set_layer_updated_externally(bool value)
            {
                layer_updated_externally_ = value;
            }
            bool layer_updated_externally() const { return layer_updated_externally_; }

            // Returns true if the layer needs to be used.
            bool ShouldPaintToLayer() const;

        private:
            // The transformation matrix (rotation, translate, scale). If non-null the
            // transform is not the identity transform.
            scoped_ptr<gfx::Transform> transform_;

            scoped_ptr<ui::Layer> layer_;

            scoped_ptr<LayerPropertySetter> property_setter_;

            // Used during painting. If not empty and View::Paint() is invoked, the canvas
            // is created with the specified size.
            // TODO(sky): this should be passed into paint.
            gfx::Rect clip_rect_;

            bool fills_bounds_opaquely_;

            // If true the bitmap is always up to date.
            bool layer_updated_externally_;

            // Should the View paint to a layer?
            bool paint_to_layer_;

            bool property_setter_explicitly_set_;

            bool needs_paint_all_;

            DISALLOW_COPY_AND_ASSIGN(LayerHelper);
        };

    } //namespace internal
} //namespace views

#endif //__view_layer_helper_h__