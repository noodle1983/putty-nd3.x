
#ifndef __ui_base_table_model_observer_h__
#define __ui_base_table_model_observer_h__

#pragma once

namespace ui
{

    // Observer for a TableModel. Anytime the model changes, it must notify its
    // observer.
    class TableModelObserver
    {
    public:
        // Invoked when the model has been completely changed.
        virtual void OnModelChanged() = 0;

        // Invoked when a range of items has changed.
        virtual void OnItemsChanged(int start, int length) = 0;

        // Invoked when new items are added.
        virtual void OnItemsAdded(int start, int length) = 0;

        // Invoked when a range of items has been removed.
        virtual void OnItemsRemoved(int start, int length) = 0;

    protected:
        virtual ~TableModelObserver() {}
    };

} //namespace ui

#endif //__ui_base_table_model_observer_h__