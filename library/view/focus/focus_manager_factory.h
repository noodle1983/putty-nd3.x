
#ifndef __view_focus_manager_factory_h__
#define __view_focus_manager_factory_h__

#pragma once

#include "base/basic_types.h"

namespace view
{

    class FocusManager;
    class Widget;

    // A factory to create FocusManager. This is used in unit tests
    // to inject a custom factory.
    class FocusManagerFactory
    {
    public:
        // Create a FocusManager for the given |widget| using installe Factory.
        static FocusManager* Create(Widget* widget);

        // Installs FocusManagerFactory. If |factory| is NULL, it resets
        // to the default factory which creates plain FocusManager.
        static void Install(FocusManagerFactory* factory);

    protected:
        FocusManagerFactory();
        virtual ~FocusManagerFactory();

        // Create a FocusManager for the given |widget|.
        virtual FocusManager* CreateFocusManager(Widget* widget) = 0;

    private:
        DISALLOW_COPY_AND_ASSIGN(FocusManagerFactory);
    };

} //namespace view

#endif //__view_focus_manager_factory_h__