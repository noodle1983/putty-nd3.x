#include "cmd_scatter_button.h"
#include "window_interface.h"


CmdScatterMenuModel::CmdScatterMenuModel() : ui::SimpleMenuModel(this)
{
	AddRadioItem(WindowInterface::CMD_DEFAULT,
		WideToUTF16(L"not sharing input(default)"), kGroupMakeDecision);
	AddRadioItem(WindowInterface::CMD_TO_ACTIVE_TAB,
		WideToUTF16(L"sharing input to the active tabs only"), kGroupMakeDecision);
	AddRadioItem(WindowInterface::CMD_TO_WITHIN_WINDOW,
		WideToUTF16(L"sharing input to all tabs within current window only"), kGroupMakeDecision);
	AddRadioItem(WindowInterface::CMD_TO_ALL,
		WideToUTF16(L"sharing input to all tabs among all windows"), kGroupMakeDecision);
	menu_.reset(new view::Menu2(this));
}

void CmdScatterMenuModel::RunMenuAt(const gfx::Point& point)
{
	menu_->RunMenuAt(point, view::Menu2::ALIGN_TOPLEFT);
}

bool CmdScatterMenuModel::IsCommandIdChecked(int command_id) const
{
	if(command_id == WindowInterface::GetInstance()->getCmdScatterState())
	{
		return true;
	}
	return false;
}

bool CmdScatterMenuModel::IsCommandIdEnabled(int command_id) const
{
	return true;
}

bool CmdScatterMenuModel::GetAcceleratorForCommandId(
	int command_id, ui::Accelerator* accelerator)
{
	return false;
}

void CmdScatterMenuModel::ExecuteCommand(int command_id)
{
	WindowInterface::GetInstance()->setCmdScatterState(command_id);
}



CmdScatterMenuButton::CmdScatterMenuButton(const std::wstring& test, bool show_menu_marker)
    : view::MenuButton(NULL, test, this, show_menu_marker) {}

CmdScatterMenuButton::~CmdScatterMenuButton() {}

void CmdScatterMenuButton::RunMenu(view::View* source, const gfx::Point& point)
{
    if(menu_model_ == NULL)
    {
        menu_model_.reset(new CmdScatterMenuModel());
    }
    menu_model_->RunMenuAt(point);
}



