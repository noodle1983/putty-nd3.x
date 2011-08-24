
#include "gantt_view.h"

#include "base/rand_util.h"
#include "ui_base/win/screen.h"
#include "view/widget/widget.h"
#include "gantt/gantt_bar.h"
#include "gantt_view_menu_controller.h"
#include "gantt_util.h"

GanttView::GanttView() : select_frame_(NULL)
{
    set_background(view::Background::CreateSolidBackground(255, 255, 255));
    set_context_menu_controller(GanttViewMenuController::GetInstance());

    for(int i=0; i<10; ++i)
    {
        GanttBar* bar = new GanttBar(base::Time::Now(), base::Time::Now());
        bar->SetProgress(base::RandInt(0, 100));
        gfx::Size preferred_size = bar->GetPreferredSize();
        bar->SetBounds(base::RandInt(0, 800), base::RandInt(0, 400),
            preferred_size.width(), preferred_size.height());
        AddChildView(bar);
    }
}

GanttView::~GanttView()
{
}

bool GanttView::OnMousePressed(const view::MouseEvent& event)
{
    if(event.IsOnlyLeftMouseButton())
    {
        DCHECK(!select_frame_);
        select_frame_ = new view::View();
        select_frame_->set_background(view::Background::CreateSolidBackground(150, 225, 255, 200));
        select_frame_->set_border(view::Border::CreateSolidBorder(1, SkColorSetRGB(51, 153, 255)));

        AddChildView(select_frame_);
        start_select_point_ = ui::Screen::GetCursorScreenPoint();
        view::View::ConvertPointToView(NULL, this, &start_select_point_);
        select_frame_->SetPosition(start_select_point_);

        return true;
    }

    return false;
}

bool GanttView::OnMouseDragged(const view::MouseEvent& event)
{
    if(select_frame_)
    {
        gfx::Point select_point = ui::Screen::GetCursorScreenPoint();
        view::View::ConvertPointToView(NULL, this, &select_point);
        select_frame_->SetBoundsRect(GetNormalizeRect(start_select_point_,
            select_point));
        return true;
    }

    return false;
}

void GanttView::OnMouseReleased(const view::MouseEvent& event)
{
    if(select_frame_)
    {
        select_frame_->SetSize(gfx::Size());
        RemoveChildView(select_frame_);
        delete select_frame_;
        select_frame_ = NULL;
    }
}

void GanttView::OnMouseCaptureLost()
{
    if(select_frame_)
    {
        select_frame_->SetSize(gfx::Size());
        RemoveChildView(select_frame_);
        delete select_frame_;
        select_frame_ = NULL;
    }
}