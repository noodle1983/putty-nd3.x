
#include "focus_manager_factory.h"

#include "focus_manager.h"

namespace
{

    class DefaultFocusManagerFactory : public view::FocusManagerFactory
    {
    public:
        DefaultFocusManagerFactory() : view::FocusManagerFactory() {}
        virtual ~DefaultFocusManagerFactory() {}

    protected:
        virtual view::FocusManager* CreateFocusManager(
            view::Widget* widget)
        {
            return new view::FocusManager(widget);
        }

    private:
        DISALLOW_COPY_AND_ASSIGN(DefaultFocusManagerFactory);
    };

    view::FocusManagerFactory* focus_manager_factory = NULL;

}

namespace view
{

    FocusManagerFactory::FocusManagerFactory() {}

    FocusManagerFactory::~FocusManagerFactory() {}

    // static
    FocusManager* FocusManagerFactory::Create(Widget* widget)
    {
        if(!focus_manager_factory)
        {
            focus_manager_factory = new DefaultFocusManagerFactory();
        }
        return focus_manager_factory->CreateFocusManager(widget);
    }

    // static
    void FocusManagerFactory::Install(FocusManagerFactory* f)
    {
        if(f == focus_manager_factory)
        {
            return;
        }
        delete focus_manager_factory;
        focus_manager_factory = f ? f : new DefaultFocusManagerFactory();
    }

} //namespace view