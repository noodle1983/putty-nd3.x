
#ifndef __view_native_textfield_win_h__
#define __view_native_textfield_win_h__

#pragma once

#include <tom.h> // For ITextDocument, a COM interface to CRichEditCtrl

#include "base/memory/scoped_ptr.h"
#include "base/win/scoped_comptr.h"

#include "gfx/insets.h"
#include "gfx/point.h"

#include "../../base/atl_controls.h"
#include "../../base/message_crack.h"
#include "../menu/simple_menu_model.h"
#include "native_textfield_wrapper.h"

namespace view
{

    class Menu2;
    class NativeViewHost;
    class Textfield;

    static const int kDefaultEditStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN |
        WS_CLIPSIBLINGS;

    // TODO(beng): make a subclass of NativeControlWin instead.
    class NativeTextfieldWin
        : public CWindowImpl<NativeTextfieldWin, CRichEditCtrl,
        CWinTraits<kDefaultEditStyle> >,
        public CRichEditCommands<NativeTextfieldWin>,
        public NativeTextfieldWrapper,
        public SimpleMenuModel::Delegate
    {
    public:
        DECLARE_WND_CLASS(L"ViewTextfieldEdit");

        explicit NativeTextfieldWin(Textfield* parent);
        ~NativeTextfieldWin();

        // Returns true if the current point is close enough to the origin point in
        // space and time that it would be considered a double click.
        static bool IsDoubleClick(const POINT& origin,
            const POINT& current,
            DWORD elapsed_time);

        // Returns true if the virtual key code is a digit coming from the numeric
        // keypad (with or without NumLock on).  |extended_key| should be set to the
        // extended key flag specified in the WM_KEYDOWN/UP where the |key_code|
        // originated.
        static bool IsNumPadDigit(int key_code, bool extended_key);

        // See the code in textfield.cc that calls this for why this is here.
        void AttachHack();

        // Overridden from NativeTextfieldWrapper:
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
        virtual bool HandleKeyPressed(const KeyEvent& event);
        virtual bool HandleKeyReleased(const KeyEvent& event);
        virtual void HandleFocus();
        virtual void HandleBlur();

        // Overridden from SimpleMenuModel::Delegate:
        virtual bool IsCommandIdChecked(int command_id) const;
        virtual bool IsCommandIdEnabled(int command_id) const;
        virtual bool GetAcceleratorForCommandId(
            int command_id,
            MenuAccelerator* accelerator);
        virtual void ExecuteCommand(int command_id);

        // Update accessibility information.
        void InitializeAccessibilityInfo();
        void UpdateAccessibleState(uint32 state_flag, bool set_value);
        void UpdateAccessibleValue(const std::wstring& value);

        // CWindowImpl
        BEGIN_MSG_MAP(Edit)
            VIEW_MSG_WM_CHAR(OnChar)
            VIEW_MSG_WM_CONTEXTMENU(OnContextMenu)
            VIEW_MSG_WM_COPY(OnCopy)
            VIEW_MSG_WM_CUT(OnCut)
            VIEW_MESSAGE_HANDLER_EX(WM_IME_CHAR, OnImeChar)
            VIEW_MESSAGE_HANDLER_EX(WM_IME_STARTCOMPOSITION, OnImeStartComposition)
            VIEW_MESSAGE_HANDLER_EX(WM_IME_COMPOSITION, OnImeComposition)
            VIEW_MESSAGE_HANDLER_EX(WM_IME_ENDCOMPOSITION, OnImeEndComposition)
            VIEW_MSG_WM_KEYDOWN(OnKeyDown)
            VIEW_MSG_WM_LBUTTONDBLCLK(OnLButtonDblClk)
            VIEW_MSG_WM_LBUTTONDOWN(OnLButtonDown)
            VIEW_MSG_WM_LBUTTONUP(OnLButtonUp)
            VIEW_MSG_WM_MBUTTONDOWN(OnNonLButtonDown)
            VIEW_MSG_WM_MOUSEMOVE(OnMouseMove)
            VIEW_MSG_WM_MOUSELEAVE(OnMouseLeave)
            VIEW_MESSAGE_HANDLER_EX(WM_MOUSEWHEEL, OnMouseWheel)
            VIEW_MSG_WM_NCCALCSIZE(OnNCCalcSize)
            VIEW_MSG_WM_NCPAINT(OnNCPaint)
            VIEW_MSG_WM_RBUTTONDOWN(OnNonLButtonDown)
            VIEW_MSG_WM_PASTE(OnPaste)
            VIEW_MSG_WM_SETFOCUS(OnSetFocus)
            VIEW_MSG_WM_SYSCHAR(OnSysChar) // WM_SYSxxx == WM_xxx with ALT down
            VIEW_MSG_WM_SYSKEYDOWN(OnKeyDown)
        END_MSG_MAP()

    private:
        // This object freezes repainting of the edit until the object is destroyed.
        // Some methods of the CRichEditCtrl draw synchronously to the screen.  If we
        // don't freeze, the user will see a rapid series of calls to these as
        // flickers.
        //
        // Freezing the control while it is already frozen is permitted; the control
        // will unfreeze once both freezes are released (the freezes stack).
        class ScopedFreeze
        {
        public:
            ScopedFreeze(NativeTextfieldWin* edit, ITextDocument* text_object_model);
            ~ScopedFreeze();

        private:
            NativeTextfieldWin* const edit_;
            ITextDocument* const text_object_model_;

            DISALLOW_COPY_AND_ASSIGN(ScopedFreeze);
        };

        // This object suspends placing any operations on the edit's undo stack until
        // the object is destroyed.  If we don't do this, some of the operations we
        // perform behind the user's back will be undoable by the user, which feels
        // bizarre and confusing.
        class ScopedSuspendUndo
        {
        public:
            explicit ScopedSuspendUndo(ITextDocument* text_object_model);
            ~ScopedSuspendUndo();

        private:
            ITextDocument* const text_object_model_;

            DISALLOW_COPY_AND_ASSIGN(ScopedSuspendUndo);
        };

        // message handlers
        void OnChar(TCHAR key, UINT repeat_count, UINT flags);
        void OnContextMenu(HWND window, const gfx::Point& point);
        void OnCopy();
        void OnCut();
        LRESULT OnImeChar(UINT message, WPARAM wparam, LPARAM lparam);
        LRESULT OnImeStartComposition(UINT message, WPARAM wparam, LPARAM lparam);
        LRESULT OnImeComposition(UINT message, WPARAM wparam, LPARAM lparam);
        LRESULT OnImeEndComposition(UINT message, WPARAM wparam, LPARAM lparam);
        void OnKeyDown(TCHAR key, UINT repeat_count, UINT flags);
        void OnLButtonDblClk(UINT keys, const gfx::Point& point);
        void OnLButtonDown(UINT keys, const gfx::Point& point);
        void OnLButtonUp(UINT keys, const gfx::Point& point);
        void OnMouseLeave();
        LRESULT OnMouseWheel(UINT message, WPARAM w_param, LPARAM l_param);
        void OnMouseMove(UINT keys, const gfx::Point& point);
        int OnNCCalcSize(BOOL w_param, LPARAM l_param);
        void OnNCPaint(HRGN region);
        void OnNonLButtonDown(UINT keys, const gfx::Point& point);
        void OnPaste();
        void OnSetFocus(HWND hwnd);
        void OnSysChar(TCHAR ch, UINT repeat_count, UINT flags);

        // Helper function for OnChar() and OnKeyDown() that handles keystrokes that
        // could change the text in the edit.
        // Note: This function assumes GetCurrentMessage() returns a MSG with
        // msg > WM_KEYFIRST and < WM_KEYLAST.
        void HandleKeystroke();

        // Every piece of code that can change the edit should call these functions
        // before and after the change.  These functions determine if anything
        // meaningful changed, and do any necessary updating and notification.
        void OnBeforePossibleChange();

        // When a user types a BIDI mirroring character (e.g. left parenthesis
        // U+0028, which should be rendered as '(' in LTR context unless  surrounded
        // by RTL characters in both sides, and it should be rendered as ')' in RTL
        // context unless surrounded by LTR characters in both sides.), the textfield
        // does not properly mirror the character when necessary. However, if we
        // explicitly set the text in the edit to the entire current string, then
        // the BIDI mirroring characters will be mirrored properly. When
        // |should_redraw_text| is true, we explicitly set the text in the edit to
        // the entire current string any time the text changes.
        void OnAfterPossibleChange(bool should_redraw_text);

        // Given an X coordinate in client coordinates, returns that coordinate
        // clipped to be within the horizontal bounds of the visible text.
        //
        // This is used in our mouse handlers to work around quirky behaviors of the
        // underlying CRichEditCtrl like not supporting triple-click when the user
        // doesn't click on the text itself.
        //
        // |is_triple_click| should be true iff this is the third click of a triple
        // click.  Sadly, we need to clip slightly differently in this case.
        LONG ClipXCoordToVisibleText(LONG x, bool is_triple_click) const;

        // Sets whether the mouse is in the edit. As necessary this redraws the
        // edit.
        void SetContainsMouse(bool contains_mouse);

        // Getter for the text_object_model_, used by the ScopedFreeze class.  Note
        // that the pointer returned here is only valid as long as the Edit is still
        // alive.
        ITextDocument* GetTextObjectModel() const;

        // Generates the contents of the context menu.
        void BuildContextMenu();

        // The Textfield this object is bound to.
        Textfield* textfield_;

        // We need to know if the user triple-clicks, so track double click points
        // and times so we can see if subsequent clicks are actually triple clicks.
        bool tracking_double_click_;
        gfx::Point double_click_point_;
        DWORD double_click_time_;

        // Used to discard unnecessary WM_MOUSEMOVE events after the first such
        // unnecessary event.  See detailed comments in OnMouseMove().
        bool can_discard_mousemove_;

        // The text of this control before a possible change.
        string16 text_before_change_;

        // If true, the mouse is over the edit.
        bool contains_mouse_;

        static bool did_load_library_;

        // The contents of the context menu for the edit.
        scoped_ptr<SimpleMenuModel> context_menu_contents_;
        scoped_ptr<Menu2> context_menu_;

        // Border insets.
        gfx::Insets content_insets_;

        // This interface is useful for accessing the CRichEditCtrl at a low level.
        mutable base::ScopedComPtr<ITextDocument> text_object_model_;

        // The position and the length of the ongoing composition string.
        // These values are used for removing a composition string from a search
        // text to emulate Firefox.
        bool ime_discard_composition_;
        int ime_composition_start_;
        int ime_composition_length_;

        // TODO(beng): remove this when we are a subclass of NativeControlWin.
        NativeViewHost* container_view_;

        COLORREF bg_color_;

        //  The accessibility state of this object.
        int accessibility_state_;

        DISALLOW_COPY_AND_ASSIGN(NativeTextfieldWin);
    };

} //namespace view

#endif //__view_native_textfield_win_h__