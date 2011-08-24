
#include "gantt_view_delegate.h"

#include "base/logging.h"
#include "ui_gfx/icon_util.h"
#include "ui_gfx/image/image.h"
#include "ui_base/clipboard/clipboard.h"
#include "ui_base/resource/resource_bundle.h"

#include "../wanui_res/resource.h"

GanttViewDelegate::GanttViewDelegate() : default_parent_view_(NULL)
{
    DCHECK(!view::ViewDelegate::view_delegate);
    view::ViewDelegate::view_delegate = this;
}

GanttViewDelegate::~GanttViewDelegate()
{
    view::ViewDelegate::view_delegate = NULL;
}

ui::Clipboard* GanttViewDelegate::GetClipboard() const
{
    if(!clipboard_.get())
    {
        // Note that we need a MessageLoop for the next call to work.
        clipboard_.reset(new ui::Clipboard());
    }
    return clipboard_.get();
}

view::View* GanttViewDelegate::GetDefaultParentView()
{
    return default_parent_view_;
}

bool GanttViewDelegate::GetSavedWindowBounds(const std::wstring& window_name,
                                             gfx::Rect* bounds) const
{
    return false;
}

bool GanttViewDelegate::GetSavedMaximizedState(const std::wstring& window_name,
                                               bool* maximized) const
{
    return false;
}

HICON GanttViewDelegate::GetDefaultWindowIcon() const
{
    return IconUtil::CreateHICONFromSkBitmap(
        ui::ResourceBundle::GetSharedInstance().GetImageNamed(
        IDR_PRODUCT_LOGO_16));
}

int GanttViewDelegate::GetDispositionForEvent(int event_flags)
{
    return 0;
}