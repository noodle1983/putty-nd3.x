
#include "menu_item_view.h"

#include <uxtheme.h>
#include <vssym32.h>

#include "base/logging.h"
#include "base/utf_string_conversions.h"
#include "base/win/win_util.h"

#include "gfx/canvas_skia.h"
#include "gfx/native_theme_win.h"

#include "../../accessibility/accessible_view_state.h"
#include "../../base/app_res_ids.h"
#include "../../l10n/l10n_util.h"
#include "../button/menu_button.h"
#include "menu_config.h"
#include "menu_controller.h"
#include "menu_delegate.h"
#include "menu_model.h"
#include "menu_separator.h"
#include "submenu_view.h"

using gfx::NativeTheme;

namespace view
{

    namespace
    {

        // EmptyMenuMenuItem ---------------------------------------------------------

        // EmptyMenuMenuItem is used when a menu has no menu items. EmptyMenuMenuItem
        // is itself a MenuItemView, but it uses a different ID so that it isn't
        // identified as a MenuItemView.
        class EmptyMenuMenuItem : public MenuItemView
        {
        public:
            explicit EmptyMenuMenuItem(MenuItemView* parent)
                : MenuItemView(parent, 0, NORMAL)
            {
                SetTitle(UTF16ToWide(GetStringUTF16(IDS_APP_MENU_EMPTY_SUBMENU)));
                // Set this so that we're not identified as a normal menu item.
                SetID(kEmptyMenuItemViewID);
                SetEnabled(false);
            }

            virtual bool GetTooltipText(const gfx::Point& p, std::wstring* tooltip)
            {
                // Empty menu items shouldn't have a tooltip.
                return false;
            }

        private:
            DISALLOW_COPY_AND_ASSIGN(EmptyMenuMenuItem);
        };

    }

    // Padding between child views.
    static const int kChildXPadding = 8;

    // MenuItemView ---------------------------------------------------------------

    // static
    const int MenuItemView::kMenuItemViewID = 1001;

    // static
    const int MenuItemView::kEmptyMenuItemViewID =
        MenuItemView::kMenuItemViewID + 1;

    // static
    int MenuItemView::label_start_;

    // static
    int MenuItemView::item_right_margin_;

    // static
    int MenuItemView::pref_menu_height_;

    // static
    const char MenuItemView::kViewClassName[] = "view/MenuItemView";

    MenuItemView::MenuItemView(MenuDelegate* delegate)
        : delegate_(delegate),
        controller_(NULL),
        canceled_(false),
        parent_menu_item_(NULL),
        type_(SUBMENU),
        selected_(false),
        command_(0),
        submenu_(NULL),
        has_mnemonics_(false),
        show_mnemonics_(false),
        has_icons_(false)
    {
        // NOTE: don't check the delegate for NULL, UpdateMenuPartSizes supplies a
        // NULL delegate.
        Init(NULL, 0, SUBMENU, delegate);
    }

    MenuItemView::~MenuItemView()
    {
        // TODO(sky): ownership is bit wrong now. In particular if a nested message
        // loop is running deletion can't be done, otherwise the stack gets
        // thoroughly screwed. The destructor should be made private, and
        // MenuController should be the only place handling deletion of the menu.
        // (57890).
        delete submenu_;
    }

    bool MenuItemView::GetTooltipText(const gfx::Point& p, std::wstring* tooltip)
    {
        *tooltip = tooltip_;
        if(!tooltip->empty())
        {
            return true;
        }
        if(GetType() != SEPARATOR)
        {
            gfx::Point location(p);
            ConvertPointToScreen(this, &location);
            *tooltip = GetDelegate()->GetTooltipText(command_, location);
            if(!tooltip->empty())
            {
                return true;
            }
        }
        return false;
    }

    void MenuItemView::GetAccessibleState(AccessibleViewState* state)
    {
        state->role = AccessibilityTypes::ROLE_MENUITEM;
        state->name = accessible_name_;
        switch(GetType())
        {
        case SUBMENU:
            state->state |= AccessibilityTypes::STATE_HASPOPUP;
            break;
        case CHECKBOX:
        case RADIO:
            state->state |= GetDelegate()->IsItemChecked(GetCommand()) ?
                AccessibilityTypes::STATE_CHECKED : 0;
            break;
        case NORMAL:
        case SEPARATOR:
            // No additional accessibility states currently for these menu states.
            break;
        }
    }

    // static
    string16 MenuItemView::GetAccessibleNameForMenuItem(
        const string16& item_text, const string16& accelerator_text)
    {
        string16 accessible_name = item_text;

        // Filter out the "&" for accessibility clients.
        size_t index = 0;
        const char16 amp = '&';
        while((index=accessible_name.find(amp, index))!=string16::npos &&
            index+1<accessible_name.length())
        {
            accessible_name.replace(index, accessible_name.length()-index,
                accessible_name.substr(index+1));

            // Special case for "&&" (escaped for "&").
            if(accessible_name[index] == '&')
            {
                ++index;
            }
        }

        // Append accelerator text.
        if(!accelerator_text.empty())
        {
            accessible_name.push_back(' ');
            accessible_name.append(accelerator_text);
        }

        return accessible_name;
    }

    void MenuItemView::RunMenuAt(HWND parent,
        MenuButton* button,
        const gfx::Rect& bounds,
        AnchorPosition anchor,
        bool has_mnemonics)
    {
        // Show mnemonics if the button has focus or alt is pressed.
        bool show_mnemonics = button ? button->HasFocus() : false;
        // We don't currently need this on gtk as showing the menu gives focus to the
        // button first.
        if(!show_mnemonics)
        {
            show_mnemonics = base::IsAltPressed();
        }
        PrepareForRun(has_mnemonics, show_mnemonics);
        int mouse_event_flags;

        MenuController* controller = MenuController::GetActiveInstance();
        if(controller && !controller->IsBlockingRun())
        {
            // A menu is already showing, but it isn't a blocking menu. Cancel it.
            // We can get here during drag and drop if the user right clicks on the
            // menu quickly after the drop.
            controller->Cancel(MenuController::EXIT_ALL);
            controller = NULL;
        }
        bool owns_controller = false;
        if(!controller)
        {
            // No menus are showing, show one.
            controller = new MenuController(true);
            MenuController::SetActiveInstance(controller);
            owns_controller = true;
        }
        else
        {
            // A menu is already showing, use the same controller.

            // Don't support blocking from within non-blocking.
            DCHECK(controller->IsBlockingRun());
        }

        controller_ = controller;

        // Run the loop.
        MenuItemView* result = controller->Run(parent, button, this, bounds,
            anchor, &mouse_event_flags);

        RemoveEmptyMenus();

        controller_ = NULL;

        if(owns_controller)
        {
            // We created the controller and need to delete it.
            if(MenuController::GetActiveInstance() == controller)
            {
                MenuController::SetActiveInstance(NULL);
            }
            delete controller;
        }
        // Make sure all the windows we created to show the menus have been
        // destroyed.
        DestroyAllMenuHosts();
        if(result && delegate_)
        {
            delegate_->ExecuteCommand(result->GetCommand(), mouse_event_flags);
        }
    }

    void MenuItemView::RunMenuForDropAt(HWND parent,
        const gfx::Rect& bounds,
        AnchorPosition anchor)
    {
        PrepareForRun(false, false);

        // If there is a menu, hide it so that only one menu is shown during dnd.
        MenuController* current_controller = MenuController::GetActiveInstance();
        if(current_controller)
        {
            current_controller->Cancel(MenuController::EXIT_ALL);
        }

        // Always create a new controller for non-blocking.
        controller_ = new MenuController(false);

        // Set the instance, that way it can be canceled by another menu.
        MenuController::SetActiveInstance(controller_);

        controller_->Run(parent, NULL, this, bounds, anchor, NULL);
    }

    void MenuItemView::Cancel()
    {
        if(controller_ && !canceled_)
        {
            canceled_ = true;
            controller_->Cancel(MenuController::EXIT_ALL);
        }
    }

    MenuItemView* MenuItemView::AppendMenuItemFromModel(MenuModel* model,
        int index, int id)
    {
        SkBitmap icon;
        std::wstring label;
        MenuItemView::Type type;
        MenuModel::ItemType menu_type = model->GetTypeAt(index);
        switch(menu_type)
        {
        case MenuModel::TYPE_COMMAND:
            model->GetIconAt(index, &icon);
            type = MenuItemView::NORMAL;
            label = UTF16ToWide(model->GetLabelAt(index));
            break;
        case MenuModel::TYPE_CHECK:
            type = MenuItemView::CHECKBOX;
            label = UTF16ToWide(model->GetLabelAt(index));
            break;
        case MenuModel::TYPE_RADIO:
            type = MenuItemView::RADIO;
            label = UTF16ToWide(model->GetLabelAt(index));
            break;
        case MenuModel::TYPE_SEPARATOR:
            type = MenuItemView::SEPARATOR;
            break;
        case MenuModel::TYPE_SUBMENU:
            type = MenuItemView::SUBMENU;
            label = UTF16ToWide(model->GetLabelAt(index));
            break;
        default:
            NOTREACHED();
            type = MenuItemView::NORMAL;
            break;
        }

        return AppendMenuItemImpl(id, label, icon, type);
    }

    MenuItemView* MenuItemView::AppendMenuItemImpl(int item_id,
        const std::wstring& label,
        const SkBitmap& icon,
        Type type)
    {
        if(!submenu_)
        {
            CreateSubmenu();
        }
        if(type == SEPARATOR)
        {
            submenu_->AddChildView(new MenuSeparator());
            return NULL;
        }
        MenuItemView* item = new MenuItemView(this, item_id, type);
        if(label.empty() && GetDelegate())
        {
            item->SetTitle(GetDelegate()->GetLabel(item_id));
        }
        else
        {
            item->SetTitle(label);
        }
        item->SetIcon(icon);
        if(type == SUBMENU)
        {
            item->CreateSubmenu();
        }
        submenu_->AddChildView(item);
        return item;
    }

    SubmenuView* MenuItemView::CreateSubmenu()
    {
        if(!submenu_)
        {
            submenu_ = new SubmenuView(this);
        }
        return submenu_;
    }

    bool MenuItemView::HasSubmenu() const
    {
        return (submenu_ != NULL);
    }

    SubmenuView* MenuItemView::GetSubmenu() const
    {
        return submenu_;
    }

    void MenuItemView::SetTitle(const std::wstring& title)
    {
        title_ = title;
        accessible_name_ = GetAccessibleNameForMenuItem(title_,
            GetAcceleratorText());
        pref_size_.SetSize(0, 0); // Triggers preferred size recalculation.
    }

    void MenuItemView::SetSelected(bool selected)
    {
        selected_ = selected;
        SchedulePaint();
    }

    void MenuItemView::SetTooltip(const std::wstring& tooltip, int item_id)
    {
        MenuItemView* item = GetMenuItemByID(item_id);
        DCHECK(item);
        item->tooltip_ = tooltip;
    }

    void MenuItemView::SetIcon(const SkBitmap& icon, int item_id)
    {
        MenuItemView* item = GetMenuItemByID(item_id);
        DCHECK(item);
        item->SetIcon(icon);
    }

    void MenuItemView::SetIcon(const SkBitmap& icon)
    {
        icon_ = icon;
        SchedulePaint();
    }

    void MenuItemView::OnPaint(gfx::Canvas* canvas)
    {
        PaintButton(canvas, PB_NORMAL);
    }

    gfx::Size MenuItemView::GetPreferredSize()
    {
        if(pref_size_.IsEmpty())
        {
            pref_size_ = CalculatePreferredSize();
        }
        return pref_size_;
    }

    MenuController* MenuItemView::GetMenuController()
    {
        return GetRootMenuItem()->controller_;
    }

    MenuDelegate* MenuItemView::GetDelegate()
    {
        return GetRootMenuItem()->delegate_;
    }

    MenuItemView* MenuItemView::GetRootMenuItem()
    {
        MenuItemView* item = this;
        while(item)
        {
            MenuItemView* parent = item->GetParentMenuItem();
            if(!parent)
            {
                return item;
            }
            item = parent;
        }
        NOTREACHED();
        return NULL;
    }

    wchar_t MenuItemView::GetMnemonic()
    {
        if(!GetRootMenuItem()->has_mnemonics_)
        {
            return 0;
        }

        const std::wstring& title = GetTitle();
        size_t index = 0;
        do
        {
            index = title.find('&', index);
            if(index != std::wstring::npos)
            {
                if(index+1!=title.size() && title[index+1]!='&')
                {
                    wchar_t char_array[1] = { title[index+1] };
                    return UTF16ToWide(ToLower(WideToUTF16(char_array)))[0];
                }
                index++;
            }
        } while(index != std::wstring::npos);
        return 0;
    }

    MenuItemView* MenuItemView::GetMenuItemByID(int id)
    {
        if(GetCommand() == id)
        {
            return this;
        }
        if(!HasSubmenu())
        {
            return NULL;
        }
        for(int i=0; i<GetSubmenu()->child_count(); ++i)
        {
            View* child = GetSubmenu()->GetChildViewAt(i);
            if(child->GetID() == MenuItemView::kMenuItemViewID)
            {
                MenuItemView* result = static_cast<MenuItemView*>(child)->
                    GetMenuItemByID(id);
                if(result)
                {
                    return result;
                }
            }
        }
        return NULL;
    }

    void MenuItemView::ChildrenChanged()
    {
        MenuController* controller = GetMenuController();
        if(!controller)
        {
            return; // We're not showing, nothing to do.
        }

        // Handles the case where we were empty and are no longer empty.
        RemoveEmptyMenus();

        // Handles the case where we were not empty, but now are.
        AddEmptyMenus();

        controller->MenuChildrenChanged(this);

        if(submenu_)
        {
            // Force a paint and layout. This handles the case of the top level window's
            // size remaining the same, resulting in no change to the submenu's size and
            // no layout.
            submenu_->Layout();
            submenu_->SchedulePaint();
        }
    }

    void MenuItemView::Layout()
    {
        if(!has_children())
        {
            return;
        }

        // Child views are laid out right aligned and given the full height. To right
        // align start with the last view and progress to the first.
        for(int i=child_count()-1,x=width()-item_right_margin_; i>=0; --i)
        {
            View* child = GetChildViewAt(i);
            int width = child->GetPreferredSize().width();
            child->SetBounds(x - width, 0, width, height());
            x -= width - kChildXPadding;
        }
    }

    int MenuItemView::GetAcceleratorTextWidth()
    {
        string16 text = GetAcceleratorText();
        return text.empty() ? 0 : MenuConfig::instance().font.GetStringWidth(text);
    }

    MenuItemView::MenuItemView(MenuItemView* parent,
        int command,
        MenuItemView::Type type)
        : delegate_(NULL),
        controller_(NULL),
        canceled_(false),
        parent_menu_item_(parent),
        type_(type),
        selected_(false),
        command_(command),
        submenu_(NULL),
        has_mnemonics_(false),
        show_mnemonics_(false),
        has_icons_(false)
    {
        Init(parent, command, type, NULL);
    }

    std::string MenuItemView::GetClassName() const
    {
        return kViewClassName;
    }

    // Calculates all sizes that we can from the OS.
    //
    // This is invoked prior to Running a menu.
    void MenuItemView::UpdateMenuPartSizes(bool has_icons)
    {
        MenuConfig::Reset();
        const MenuConfig& config = MenuConfig::instance();

        item_right_margin_ = config.label_to_arrow_padding + config.arrow_width +
            config.arrow_to_edge_padding;

        if(has_icons)
        {
            label_start_ = config.item_left_margin + config.check_width +
                config.icon_to_label_padding;
        }
        else
        {
            // If there are no icons don't pad by the icon to label padding. This
            // makes us look close to system menus.
            label_start_ = config.item_left_margin + config.check_width;
        }
        if(config.render_gutter)
        {
            label_start_ += config.gutter_width + config.gutter_to_label;
        }

        MenuItemView menu_item(NULL);
        menu_item.SetTitle(L"blah"); // Text doesn't matter here.
        pref_menu_height_ = menu_item.GetPreferredSize().height();
    }

    void MenuItemView::Init(MenuItemView* parent,
        int command,
        MenuItemView::Type type,
        MenuDelegate* delegate)
    {
        delegate_ = delegate;
        controller_ = NULL;
        canceled_ = false;
        parent_menu_item_ = parent;
        type_ = type;
        selected_ = false;
        command_ = command;
        submenu_ = NULL;
        show_mnemonics_ = false;
        // Assign our ID, this allows SubmenuItemView to find MenuItemViews.
        SetID(kMenuItemViewID);
        has_icons_ = false;

        MenuDelegate* root_delegate = GetDelegate();
        if(root_delegate)
        {
            SetEnabled(root_delegate->IsCommandEnabled(command));
        }
    }

    void MenuItemView::DropMenuClosed(bool notify_delegate)
    {
        DCHECK(controller_);
        DCHECK(!controller_->IsBlockingRun());
        if(MenuController::GetActiveInstance() == controller_)
        {
            MenuController::SetActiveInstance(NULL);
        }
        delete controller_;
        controller_ = NULL;

        RemoveEmptyMenus();

        if(notify_delegate && delegate_)
        {
            // Our delegate is null when invoked from the destructor.
            delegate_->DropMenuClosed(this);
        }
        // WARNING: its possible the delegate deleted us at this point.
    }

    void MenuItemView::PrepareForRun(bool has_mnemonics, bool show_mnemonics)
    {
        // Currently we only support showing the root.
        DCHECK(!parent_menu_item_);

        // Force us to have a submenu.
        CreateSubmenu();

        canceled_ = false;

        has_mnemonics_ = has_mnemonics;
        show_mnemonics_ = has_mnemonics && show_mnemonics;

        AddEmptyMenus();

        if(!MenuController::GetActiveInstance())
        {
            // Only update the menu size if there are no menus showing, otherwise
            // things may shift around.
            UpdateMenuPartSizes(has_icons_);
        }
    }

    int MenuItemView::GetDrawStringFlags()
    {
        int flags = 0;
        if(base::IsRTL())
        {
            flags |= gfx::Canvas::TEXT_ALIGN_RIGHT;
        }
        else
        {
            flags |= gfx::Canvas::TEXT_ALIGN_LEFT;
        }

        if(has_mnemonics_)
        {
            if(MenuConfig::instance().show_mnemonics ||
                GetRootMenuItem()->show_mnemonics_)
            {
                flags |= gfx::Canvas::SHOW_PREFIX;
            }
            else
            {
                flags |= gfx::Canvas::HIDE_PREFIX;
            }
        }
        return flags;
    }

    void MenuItemView::AddEmptyMenus()
    {
        DCHECK(HasSubmenu());
        if(!submenu_->has_children())
        {
            submenu_->AddChildViewAt(new EmptyMenuMenuItem(this), 0);
        }
        else
        {
            for(int i=0,item_count=submenu_->GetMenuItemCount(); i<item_count;
                ++i)
            {
                MenuItemView* child = submenu_->GetMenuItemAt(i);
                if(child->HasSubmenu())
                {
                    child->AddEmptyMenus();
                }
            }
        }
    }

    void MenuItemView::RemoveEmptyMenus()
    {
        DCHECK(HasSubmenu());
        // Iterate backwards as we may end up removing views, which alters the child
        // view count.
        for(int i=submenu_->child_count()-1; i>=0; --i)
        {
            View* child = submenu_->GetChildViewAt(i);
            if(child->GetID() == MenuItemView::kMenuItemViewID)
            {
                MenuItemView* menu_item = static_cast<MenuItemView*>(child);
                if(menu_item->HasSubmenu())
                {
                    menu_item->RemoveEmptyMenus();
                }
            }
            else if(child->GetID() == EmptyMenuMenuItem::kEmptyMenuItemViewID)
            {
                submenu_->RemoveChildView(child);
            }
        }
    }

    void MenuItemView::AdjustBoundsForRTLUI(gfx::Rect* rect) const
    {
        rect->set_x(GetMirroredXForRect(*rect));
    }

    void MenuItemView::PaintButton(gfx::Canvas* canvas, PaintButtonMode mode)
    {
        const MenuConfig& config = MenuConfig::instance();
        bool render_selection = (mode==PB_NORMAL && IsSelected() &&
            parent_menu_item_->GetSubmenu()->GetShowSelection(this) &&
            !has_children());
        int state = render_selection ? MPI_HOT :
            (IsEnabled() ? MPI_NORMAL : MPI_DISABLED);
        HDC dc = canvas->BeginPlatformPaint();
        NativeTheme::ControlState control_state;

        if(!IsEnabled())
        {
            control_state = NativeTheme::CONTROL_DISABLED;
        }
        else
        {
            control_state = render_selection ? NativeTheme::CONTROL_HIGHLIGHTED :
                NativeTheme::CONTROL_NORMAL;
        }

        // The gutter is rendered before the background.
        if(config.render_gutter && mode==PB_NORMAL)
        {
            gfx::Rect gutter_bounds(label_start_-config.gutter_to_label-
                config.gutter_width, 0, config.gutter_width,
                height());
            AdjustBoundsForRTLUI(&gutter_bounds);
            RECT gutter_rect = gutter_bounds.ToRECT();
            NativeTheme::instance()->PaintMenuGutter(dc, MENU_POPUPGUTTER,
                MPI_NORMAL, &gutter_rect);
        }

        // Render the background.
        if(mode == PB_NORMAL)
        {
            gfx::Rect item_bounds(0, 0, width(), height());
            AdjustBoundsForRTLUI(&item_bounds);
            RECT item_rect = item_bounds.ToRECT();
            NativeTheme::instance()->PaintMenuItemBackground(
                NativeTheme::MENU, dc, MENU_POPUPITEM, state,
                render_selection, &item_rect);
        }

        int top_margin = GetTopMargin();
        int bottom_margin = GetBottomMargin();

        if(type_==CHECKBOX && GetDelegate()->IsItemChecked(GetCommand()))
        {
            PaintCheck(dc, IsEnabled()?MC_CHECKMARKNORMAL:MC_CHECKMARKDISABLED,
                control_state, config.check_height, config.check_width);
        }
        else if(type_==RADIO && GetDelegate()->IsItemChecked(GetCommand()))
        {
            PaintCheck(dc, IsEnabled()?MC_BULLETNORMAL:MC_BULLETDISABLED,
                control_state, config.radio_height, config.radio_width);
        }

        // Render the foreground.
        // Menu color is specific to Vista, fallback to classic colors if can't
        // get color.
        int default_sys_color = render_selection ? COLOR_HIGHLIGHTTEXT :
            (IsEnabled() ? COLOR_MENUTEXT : COLOR_GRAYTEXT);
        SkColor fg_color = NativeTheme::instance()->GetThemeColorWithDefault(
            NativeTheme::MENU, MENU_POPUPITEM, state, TMT_TEXTCOLOR,
            default_sys_color);
        const gfx::Font& font = MenuConfig::instance().font;
        int accel_width = parent_menu_item_->GetSubmenu()->max_accelerator_width();
        int width = this->width() - item_right_margin_ - label_start_ - accel_width;
        gfx::Rect text_bounds(label_start_, top_margin, width, font.GetHeight());
        text_bounds.set_x(GetMirroredXForRect(text_bounds));
        if(mode == PB_FOR_DRAG)
        {
            // With different themes, it's difficult to tell what the correct
            // foreground and background colors are for the text to draw the correct
            // halo. Instead, just draw black on white, which will look good in most
            // cases.
            canvas->AsCanvasSkia()->DrawStringWithHalo(
                GetTitle(), font, 0x00000000, 0xFFFFFFFF, text_bounds.x(),
                text_bounds.y(), text_bounds.width(), text_bounds.height(),
                GetRootMenuItem()->GetDrawStringFlags());
        }
        else
        {
            canvas->DrawStringInt(GetTitle(), font, fg_color,
                text_bounds.x(), text_bounds.y(), text_bounds.width(),
                text_bounds.height(),
                GetRootMenuItem()->GetDrawStringFlags());
        }

        PaintAccelerator(canvas);

        if(icon_.width() > 0)
        {
            gfx::Rect icon_bounds(config.item_left_margin,
                top_margin+(height()-top_margin-
                bottom_margin-icon_.height())/2,
                icon_.width(),
                icon_.height());
            icon_bounds.set_x(GetMirroredXForRect(icon_bounds));
            canvas->DrawBitmapInt(icon_, icon_bounds.x(), icon_bounds.y());
        }

        if(HasSubmenu())
        {
            int state_id = IsEnabled() ? MSM_NORMAL : MSM_DISABLED;
            gfx::Rect arrow_bounds(this->width()-item_right_margin_+
                config.label_to_arrow_padding, 0,
                config.arrow_width, height());
            AdjustBoundsForRTLUI(&arrow_bounds);

            // If our sub menus open from right to left (which is the case when the
            // locale is RTL) then we should make sure the menu arrow points to the
            // right direction.
            NativeTheme::MenuArrowDirection arrow_direction;
            if(base::IsRTL())
            {
                arrow_direction = NativeTheme::LEFT_POINTING_ARROW;
            }
            else
            {
                arrow_direction = NativeTheme::RIGHT_POINTING_ARROW;
            }

            RECT arrow_rect = arrow_bounds.ToRECT();
            NativeTheme::instance()->PaintMenuArrow(NativeTheme::MENU,
                dc, MENU_POPUPSUBMENU, state_id, &arrow_rect,
                arrow_direction, control_state);
        }
        canvas->EndPlatformPaint();
    }

    void MenuItemView::PaintCheck(HDC dc,
        int state_id,
        NativeTheme::ControlState control_state,
        int icon_width,
        int icon_height)
    {
        int top_margin = GetTopMargin();
        int icon_x = MenuConfig::instance().item_left_margin;
        int icon_y = top_margin +
            (height() - top_margin - GetBottomMargin() - icon_height) / 2;
        // Draw the background.
        gfx::Rect bg_bounds(0, 0, icon_x+icon_width, height());
        int bg_state = IsEnabled() ? MCB_NORMAL : MCB_DISABLED;
        AdjustBoundsForRTLUI(&bg_bounds);
        RECT bg_rect = bg_bounds.ToRECT();
        NativeTheme::instance()->PaintMenuCheckBackground(NativeTheme::MENU, dc,
            MENU_POPUPCHECKBACKGROUND, bg_state, &bg_rect);

        // And the check.
        gfx::Rect icon_bounds(icon_x/2, icon_y, icon_width, icon_height);
        AdjustBoundsForRTLUI(&icon_bounds);
        RECT icon_rect = icon_bounds.ToRECT();
        NativeTheme::instance()->PaintMenuCheck(NativeTheme::MENU, dc,
            MENU_POPUPCHECK, state_id, &icon_rect, control_state);
    }

    void MenuItemView::PaintAccelerator(gfx::Canvas* canvas)
    {
        string16 accel_text = GetAcceleratorText();
        if(accel_text.empty())
        {
            return;
        }

        const gfx::Font& font = MenuConfig::instance().font;
        int available_height = height() - GetTopMargin() - GetBottomMargin();
        int max_accel_width = parent_menu_item_->GetSubmenu()->
            max_accelerator_width();
        gfx::Rect accel_bounds(width()-item_right_margin_-max_accel_width,
            GetTopMargin(), max_accel_width, available_height);
        accel_bounds.set_x(GetMirroredXForRect(accel_bounds));
        int flags = GetRootMenuItem()->GetDrawStringFlags() |
            gfx::Canvas::TEXT_VALIGN_MIDDLE;
        flags &= ~(gfx::Canvas::TEXT_ALIGN_RIGHT | gfx::Canvas::TEXT_ALIGN_LEFT);
        if(base::IsRTL())
        {
            flags |= gfx::Canvas::TEXT_ALIGN_LEFT;
        }
        else
        {
            flags |= gfx::Canvas::TEXT_ALIGN_RIGHT;
        }
        canvas->DrawStringInt(
            accel_text, font, TextButton::kDisabledColor,
            accel_bounds.x(), accel_bounds.y(), accel_bounds.width(),
            accel_bounds.height(), flags);
    }

    void MenuItemView::DestroyAllMenuHosts()
    {
        if(!HasSubmenu())
        {
            return;
        }

        submenu_->Close();
        for(int i=0,item_count=submenu_->GetMenuItemCount(); i<item_count; ++i)
        {
            submenu_->GetMenuItemAt(i)->DestroyAllMenuHosts();
        }
    }

    string16 MenuItemView::GetAcceleratorText()
    {
        Accelerator accelerator;
        return (GetDelegate() &&
            GetDelegate()->GetAccelerator(GetCommand(), &accelerator)) ?
            accelerator.GetShortcutText() : string16();
    }

    int MenuItemView::GetTopMargin()
    {
        MenuItemView* root = GetRootMenuItem();
        return root&&root->has_icons_ ? MenuConfig::instance().item_top_margin :
            MenuConfig::instance().item_no_icon_top_margin;
    }

    int MenuItemView::GetBottomMargin()
    {
        MenuItemView* root = GetRootMenuItem();
        return root&&root->has_icons_ ? MenuConfig::instance().item_bottom_margin :
            MenuConfig::instance().item_no_icon_bottom_margin;
    }

    int MenuItemView::GetChildPreferredWidth()
    {
        if(!has_children())
        {
            return 0;
        }

        int width = 0;
        for(int i=0; i<child_count(); ++i)
        {
            if(i)
            {
                width += kChildXPadding;
            }
            width += GetChildViewAt(i)->GetPreferredSize().width();
        }
        return width;
    }

    gfx::Size MenuItemView::CalculatePreferredSize()
    {
        const gfx::Font& font = MenuConfig::instance().font;
        return gfx::Size(
            font.GetStringWidth(title_)+label_start_+item_right_margin_+
            GetChildPreferredWidth(),
            font.GetHeight()+GetBottomMargin()+GetTopMargin());
    }

} //namespace view