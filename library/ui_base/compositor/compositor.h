
#ifndef __ui_base_compositor_h__
#define __ui_base_compositor_h__

#pragma once

#include "base/memory/ref_counted.h"

class SkBitmap;

namespace gfx
{
    class Point;
    class Rect;
    class Size;
    class Transform;
}

namespace ui
{

    // Textures are created by a Compositor for managing an accelerated view.
    // Any time a View with a texture needs to redraw itself it invokes SetBitmap().
    // When the view is ready to be drawn Draw() is invoked.
    //
    // Texture is really a proxy to the gpu. Texture does not itself keep a copy of
    // the bitmap.
    //
    // Views own the Texture.
    class Texture : public base::RefCounted<Texture>
    {
    public:
        // Sets the bitmap of this texture. The bitmaps origin is at |origin|.
        // |overall_size| gives the total size of texture.
        virtual void SetBitmap(const SkBitmap& bitmap,
            const gfx::Point& origin,
            const gfx::Size& overall_size) = 0;

        // Draws the texture.
        virtual void Draw(const gfx::Transform& transform) = 0;

    protected:
        virtual ~Texture() {}

    private:
        friend class base::RefCounted<Texture>;
    };

    // Compositor object to take care of GPU painting.
    // A Browser compositor object is responsible for generating the final
    // displayable form of pixels comprising a single widget's contents. It draws an
    // appropriately transformed texture for each transformed view in the widget's
    // view hierarchy.
    class Compositor : public base::RefCounted<Compositor>
    {
    public:
        // Create a compositor from the provided handle.
        static Compositor* Create(HWND widget);

        // Creates a new texture. The caller owns the returned object.
        virtual Texture* CreateTexture() = 0;

        // Notifies the compositor that compositing is about to start.
        virtual void NotifyStart() = 0;

        // Notifies the compositor that compositing is complete.
        virtual void NotifyEnd() = 0;

        // Blurs the specific region in the compositor.
        virtual void Blur(const gfx::Rect& bounds) = 0;

        // Schedules a paint on the widget this Compositor was created for.
        virtual void SchedulePaint() = 0;

    protected:
        virtual ~Compositor() {}

    private:
        friend class base::RefCounted<Compositor>;
    };

} //namespace ui

#endif //__ui_base_compositor_h__