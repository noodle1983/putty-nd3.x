
#ifndef __view_native_widget_view_h__
#define __view_native_widget_view_h__

#pragma once

#include "native_widget_delegate.h"
#include "native_widget_views.h"
#include "view/view.h"

namespace view
{

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

            void set_delete_native_widget(bool delete_native_widget)
            {
                delete_native_widget_ = delete_native_widget;
            }

            void set_cursor(HCURSOR cursor) { cursor_ = cursor; }

            // Overridden from View:
            virtual void CalculateOffsetToAncestorWithLayer(
                gfx::Point* offset,
                ui::Layer** layer_parent);

        private:
            // Overridden from View:
            virtual void ViewHierarchyChanged(bool is_add,
                View* parent, View* child);
            virtual void OnBoundsChanged(const gfx::Rect& previous_bounds);
            virtual void OnPaint(gfx::Canvas* canvas);
            virtual bool OnSetCursor(const gfx::Point& p);
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
            virtual void VisibilityChanged(View* starting_from, bool is_visible);
            virtual void OnFocus();
            virtual void OnBlur();
            virtual std::string GetClassName() const;
            virtual void MoveLayerToParent(ui::Layer* parent_layer,
                const gfx::Point& point);
            virtual void DestroyLayerRecurse();
            virtual void UpdateLayerBounds(const gfx::Point& offset);
            virtual void CreateLayerIfNecessary();

            internal::NativeWidgetDelegate* delegate()
            {
                return native_widget_->delegate();
            }

            NativeWidgetViews* native_widget_;

            // Have we sent OnNativeWidgetCreated?
            bool sent_create_;

            bool delete_native_widget_;

            // The cursor set for the associated widget.
            HCURSOR cursor_;

            DISALLOW_COPY_AND_ASSIGN(NativeWidgetView);
        };

    } //namespace internal
} //namespace view

#endif //__view_native_widget_view_h__