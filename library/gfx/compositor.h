
#ifndef __gfx_compositor_h__
#define __gfx_compositor_h__

#pragma once

#include "base/ref_counted.h"

namespace gfx
{
    class Transform;

    typedef unsigned int TextureID;

    // Compositor object to take care of GPU painting.
    // A Browser compositor object is responsible for generating the final
    // displayable form of pixels comprising a single widget's contents. It draws an
    // appropriately transformed texture for each transformed view in the widget's
    // view hierarchy. The initial implementation uses GL for this purpose.
    // Future CLs will adapt this to ongoing Skia development.
    class Compositor : public base::RefCounted<Compositor>
    {
    public:
        // Create a compositor from the provided handle.
        static Compositor* Create(HWND widget);

        // Notifies the compositor that compositing is about to start.
        virtual void NotifyStart() = 0;

        // Notifies the compositor that compositing is complete.
        virtual void NotifyEnd() = 0;

        // Draws the given texture with the given transform.
        virtual void DrawTextureWithTransform(TextureID txt,
            const Transform& transform) = 0;

        // Save the current transformation that can be restored with RestoreTransform.
        virtual void SaveTransform() = 0;

        // Restore a previously saved transformation using SaveTransform.
        virtual void RestoreTransform() = 0;

    protected:
        virtual ~Compositor() {}

    private:
        friend class base::RefCounted<Compositor>;
    };

} //namespace gfx

#endif //__gfx_compositor_h__