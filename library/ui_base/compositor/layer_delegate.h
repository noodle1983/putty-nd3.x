
#ifndef __ui_base_layer_delegate_h__
#define __ui_base_layer_delegate_h__

#pragma once

namespace gfx
{
    class Canvas;
}

namespace ui
{

    // A delegate interface implemented by an object that renders to a Layer.
    class LayerDelegate
    {
    public:
        // Paint content for the layer to the specified canvas. It has already been
        // clipped to the Layer's invalid rect.
        virtual void OnPaint(gfx::Canvas* canvas) = 0;

    protected:
        virtual ~LayerDelegate() {}
    };

} //namespace ui

#endif //__ui_base_layer_delegate_h__