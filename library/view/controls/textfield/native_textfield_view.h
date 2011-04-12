
#ifndef __view_native_textfield_view_h__
#define __view_native_textfield_view_h__

#pragma once

#include "../../view/view.h"
#include "../menu/simple_menu_model.h"
#include "native_textfield_wrapper.h"

namespace view
{

    class KeyEvent;
    class Menu2;
    class TextfieldViewsModel;

    // A views/skia only implementation of NativeTextfieldWrapper.
    // No platform specific code is used.
    // Following features are not yet supported.
    // * BIDI
    // * IME/i18n support.
    // * X selection (only if we want to support).
    // * STYLE_MULTILINE, STYLE_LOWERCASE text. (These are not used in
    //   chromeos, so we may not need them)
    // * Double click to select word, and triple click to select all.
    // * Undo/Redo
    class NativeTextfieldView : public View,
        public ContextMenuController,
        public NativeTextfieldWrapper,
        public SimpleMenuModel::Delegate
    {
    public:
        explicit NativeTextfieldView(Textfield* parent);
        ~NativeTextfieldView();

        // views::View overrides:
        virtual bool OnMousePressed(const MouseEvent& e);
        virtual bool OnMouseDragged(const MouseEvent& e);
        virtual void OnMouseReleased(const MouseEvent& e, bool canceled);
        virtual bool OnKeyPressed(const KeyEvent& e);
        virtual bool OnKeyReleased(const KeyEvent& e);
        virtual void OnPaint(gfx::Canvas* canvas);
        virtual void OnFocus();
        virtual void OnBlur();
        virtual HCURSOR GetCursorForPoint(EventType event_type,
            const gfx::Point& p);

        // views::ContextMenuController overrides:
        virtual void ShowContextMenuForView(View* source,
            const gfx::Point& p,
            bool is_mouse_gesture);

        // NativeTextfieldWrapper overrides:
        virtual string16 GetText() const;
        virtual void UpdateText();
        virtual void AppendText(const string16& text);
        virtual string16 GetSelectedText() const;
        virtual void SelectAll();
        virtual void ClearSelection();
        virtual void UpdateBorder();
        virtual void UpdateTextColor();
        virtual void UpdateBackgroundColor();
        virtual void UpdateReadOnly();
        virtual void UpdateFont();
        virtual void UpdateIsPassword();
        virtual void UpdateEnabled();
        virtual gfx::Insets CalculateInsets();
        virtual void UpdateHorizontalMargins();
        virtual void UpdateVerticalMargins();
        virtual bool SetFocus();
        virtual View* GetView();
        virtual HWND GetTestingHandle() const;
        virtual bool IsIMEComposing() const;
        virtual void GetSelectedRange(Range* range) const;
        virtual void SelectRange(const Range& range);
        virtual size_t GetCursorPosition() const;
        virtual bool HandleKeyPressed(const KeyEvent& e);
        virtual bool HandleKeyReleased(const KeyEvent& e);
        virtual void HandleFocus();
        virtual void HandleBlur();

        // SimpleMenuModel::Delegate overrides
        virtual bool IsCommandIdChecked(int command_id) const;
        virtual bool IsCommandIdEnabled(int command_id) const;
        virtual bool GetAcceleratorForCommandId(int command_id,
            MenuAccelerator* accelerator);
        virtual void ExecuteCommand(int command_id);

        // class name of internal
        static const char kViewClassName[];

        // Returns true when
        // 1) built with GYP_DEFIENS="touchui=1"
        // 2) enabled by SetEnableTextfieldViews(true)
        // 3) enabled by the command line flag "--enable-textfield-view".
        static bool IsTextfieldViewEnabled();
        // Enable/Disable TextfieldViews implementation for Textfield.
        static void SetEnableTextfieldView(bool enabled);

        enum ClickState
        {
            TRACKING_DOUBLE_CLICK,
            TRACKING_TRIPLE_CLICK,
            NONE,
        };

    protected:
        // View override.
        virtual void OnBoundsChanged(const gfx::Rect& previous_bounds);

    private:
        // A Border class to draw focus border for the text field.
        class TextfieldBorder : public Border
        {
        public:
            TextfieldBorder();

            // Border implementation.
            virtual void Paint(const View& view, gfx::Canvas* canvas) const;
            virtual void GetInsets(gfx::Insets* insets) const;

            // Sets the insets of the border.
            void SetInsets(int top, int left, int bottom, int right);

            // Sets the focus state.
            void set_has_focus(bool has_focus)
            {
                has_focus_ = has_focus;
            }

        private:
            bool has_focus_;
            gfx::Insets insets_;

            DISALLOW_COPY_AND_ASSIGN(TextfieldBorder);
        };

        // Returns the Textfield's font.
        const gfx::Font& GetFont() const;

        // Returns the Textfield's text color.
        SkColor GetTextColor() const;

        // A callback function to periodically update the cursor state.
        void UpdateCursor();

        // Repaint the cursor.
        void RepaintCursor();

        // Update the cursor_bounds and text_offset.
        void UpdateCursorBoundsAndTextOffset();

        void PaintTextAndCursor(gfx::Canvas* canvas);

        // Handle the keyevent.
        bool HandleKeyEvent(const KeyEvent& key_event);

        // Utility function. Gets the character corresponding to a keyevent.
        // Returns 0 if the character is not printable.
        char16 GetPrintableChar(const KeyEvent& key_event);

        // Find a cusor position for given |point| in this views coordinates.
        size_t FindCursorPosition(const gfx::Point& point) const;

        // Mouse event handler. Returns true if textfield needs to be repainted.
        bool HandleMousePressed(const MouseEvent& e);

        // Helper function that sets the cursor position at the location of mouse
        // event.
        void SetCursorForMouseClick(const MouseEvent& e);

        // Utility function to inform the parent textfield (and its controller if any)
        // that the text in the textfield has changed.
        void PropagateTextChange();

        // Does necessary updates when the text and/or the position of the cursor
        // changed.
        void UpdateAfterChange(bool text_changed, bool cursor_changed);

        // Utility function to create the context menu if one does not already exist.
        void InitContextMenuIfRequired();

        // The parent textfield, the owner of this object.
        Textfield* textfield_;

        // The text model.
        scoped_ptr<TextfieldViewsModel> model_;

        // The reference to the border class. The object is owned by View::border_.
        TextfieldBorder* text_border_;

        // The x offset for the text to be drawn, without insets;
        int text_offset_;

        // Cursor's bounds in the textfield's coordinates.
        gfx::Rect cursor_bounds_;

        // True if the textfield is in insert mode.
        bool insert_;

        // The drawing state of cursor. True to draw.
        bool is_cursor_visible_;

        // A runnable method factory for callback to update the cursor.
        ScopedRunnableMethodFactory<NativeTextfieldView> cursor_timer_;

        // Time of last LEFT mouse press. Used for tracking double/triple click.
        base::Time last_mouse_press_time_;

        // Position of last LEFT mouse press. Used for tracking double/triple click.
        gfx::Point last_mouse_press_location_;

        // State variable to track double and triple clicks.
        ClickState click_state_;

        // Context menu and its content list for the textfield.
        scoped_ptr<SimpleMenuModel> context_menu_contents_;
        scoped_ptr<Menu2> context_menu_menu_;

        DISALLOW_COPY_AND_ASSIGN(NativeTextfieldView);
    };

} //namespace view

#endif //__view_native_textfield_view_h__