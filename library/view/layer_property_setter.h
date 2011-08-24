
#ifndef __view_layer_property_setter_h__
#define __view_layer_property_setter_h__

#pragma once

namespace gfx
{
    class Rect;
    class Transform;
}

namespace ui
{
    class Layer;
}

namespace view
{

    // When a property of layer needs to be changed it is set by way of
    // LayerPropertySetter. This enables LayerPropertySetter to animate property
    // changes.
    class LayerPropertySetter
    {
    public:
        // Creates a LayerPropertySetter that immediately sets the values on the
        // layer. Ownership returns to caller.
        static LayerPropertySetter* CreateDefaultSetter();

        // Creates a LayerPropertySetter that animates changes. Ownership returns to
        // caller.
        static LayerPropertySetter* CreateAnimatingSetter();

        virtual ~LayerPropertySetter() {}

        // Invoked when the LayerPropertySetter is to be used for a particular Layer.
        // This is invoked whenever a new Layer is created. A LayerPropertySetter is
        // only associated with one Layer at a time.
        virtual void Installed(ui::Layer* layer) = 0;

        // Invoked when the Layer is destroyed.
        virtual void Uninstalled(ui::Layer* layer) = 0;

        // Sets the transform on the Layer.
        virtual void SetTransform(ui::Layer* layer,
            const gfx::Transform& transform) = 0;

        // Sets the bounds of the layer.
        virtual void SetBounds(ui::Layer* layer, const gfx::Rect& bounds) = 0;
    };

} //namespace view

#endif //__view_layer_property_setter_h__