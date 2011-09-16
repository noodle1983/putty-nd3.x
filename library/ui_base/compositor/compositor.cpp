
#include "compositor.h"

#include "compositor_observer.h"
#include "layer.h"

namespace ui
{

    Compositor::Compositor(CompositorDelegate* delegate, const gfx::Size& size)
        : delegate_(delegate),
        size_(size),
        root_layer_(NULL) {}

    Compositor::~Compositor() {}

    void Compositor::Draw(bool force_clear)
    {
        if(!root_layer_)
        {
            return;
        }

        NotifyStart(force_clear);
        root_layer_->DrawTree();
        NotifyEnd();
    }

    void Compositor::AddObserver(CompositorObserver* observer)
    {
        observer_list_.AddObserver(observer);
    }

    void Compositor::RemoveObserver(CompositorObserver* observer)
    {
        observer_list_.RemoveObserver(observer);
    }

    void Compositor::NotifyStart(bool clear)
    {
        OnNotifyStart(clear);
    }

    void Compositor::NotifyEnd()
    {
        OnNotifyEnd();
        FOR_EACH_OBSERVER(CompositorObserver,
            observer_list_,
            OnCompositingEnded());
    }

} //namespace ui