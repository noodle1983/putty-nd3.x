
#ifndef __ui_base_compositor_observer_h__
#define __ui_base_compositor_observer_h__

#pragma once

namespace ui
{

    // A compositor observer is notified when compositing completes.
    class CompositorObserver
    {
    public:
        // Called when compositing completes.
        virtual void OnCompositingEnded() = 0;

    protected:
        virtual ~CompositorObserver() {}
    };

} //namespace ui

#endif //__ui_base_compositor_observer_h__