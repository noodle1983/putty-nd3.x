
#ifndef __view_native_widget_view_h__
#define __view_native_widget_view_h__

#pragma once

#include "native_widget_delegate.h"
#include "native_widget_views.h"
#include "view/view.h"

namespace view
{

    class NativeWidgetViews;

    namespace internal
    {

        ////////////////////////////////////////////////////////////////////////////////
        // NativeWidgetView
        //
        // This class represents the View that is the "native view" for a
        // NativeWidgetViews. It is the View that is a member of the parent Widget's
        // View hierarchy. It is responsible for receiving relevant events from that
        // hierarchy and forwarding them to its NativeWidgetViews' delegate's hierarchy.
        class NativeWidgetView : public View
        {
        public:
            static const char kViewClassName[];

            explicit NativeWidgetView(NativeWidgetViews* native_widget);
            virtual ~NativeWidgetView();

            Widget* GetAssociatedWidget();

            // Overridden from View:
            virtual void SchedulePaintInternal(const gfx::Rect& r);
            virtual void MarkLayerDirty();
            virtual void CalculateOffsetToAncestorWithLayer(gfx::Point* offset,
                View** ancestor);

        private:
            // Overridden from View:
            virtual void ViewHierarchyChanged(bool is_add,
                View* parent, View* child);
            virtual void OnBoundsChanged(const gfx::Rect& previous_bounds);
            virtual void OnPaint(gfx::Canvas* canvas);
            virtual bool OnMousePressed(const MouseEvent& event);
            virtual bool OnMouseDragged(const MouseEvent& event);
            virtual void OnMouseReleased(const MouseEvent& event);
            virtual void OnMouseCaptureLost();
            virtual void OnMouseMoved(const MouseEvent& event);
            virtual void OnMouseEntered(const MouseEvent& event);
            virtual void OnMouseExited(const MouseEvent& event);
            virtual bool OnKeyPressed(const KeyEvent& event);
            virtual bool OnKeyReleased(const KeyEvent& event);
            virtual bool OnMouseWheel(const MouseWheelEvent& event);
            virtual void OnFocus();
            virtual void OnBlur();
            virtual std::string GetClassName() const;
            virtual void MoveLayerToParent(ui::Layer* parent_layer,
                const gfx::Point& point);
            virtual void DestroyLayerRecurse();
            virtual void UpdateLayerBounds(const gfx::Point& offset);
            virtual void PaintToLayer(const gfx::Rect& dirty_rect);
            virtual void PaintComposite();

            internal::NativeWidgetDelegate* delegate()
            {
                return native_widget_->delegate();
            }

            NativeWidgetViews* native_widget_;

            // Have we sent OnNativeWidgetCreated?
            bool sent_create_;

            DISALLOW_COPY_AND_ASSIGN(NativeWidgetView);
        };

    } //namespace internal
} //namespace view

#endif //__view_native_widget_view_h__