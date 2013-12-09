#include "cmd_scatter_button.h"



CmdScatterMenuModel::CmdScatterMenuModel() : ui::SimpleMenuModel(this),
	current_encoding_command_id_(kCommandSelectAscii)
{
	AddRadioItem(kCommandSelectAscii,
		WideToUTF16(L"ASCII"), kGroupMakeDecision);
	AddRadioItem(kCommandSelectUtf8,
		WideToUTF16(L"UTF-8"), kGroupMakeDecision);
	AddRadioItem(kCommandSelectUtf16,
		WideToUTF16(L"UTF-16"), kGroupMakeDecision);
	menu_.reset(new view::Menu2(this));
}

void CmdScatterMenuModel::RunMenuAt(const gfx::Point& point)
{
	menu_->RunMenuAt(point, view::Menu2::ALIGN_TOPRIGHT);
}

bool CmdScatterMenuModel::IsCommandIdChecked(int command_id) const
{
	// Radio items.
	if(command_id == current_encoding_command_id_)
	{
		return true;
	}

	// Check items.
	if(checked_fruits_.find(command_id) != checked_fruits_.end())
	{
		return true;
	}

	return false;
}

bool CmdScatterMenuModel::IsCommandIdEnabled(int command_id) const
{
	// All commands are enabled except for kCommandGoHome.
	return command_id != kCommandGoHome;
}

bool CmdScatterMenuModel::GetAcceleratorForCommandId(
	int command_id, ui::Accelerator* accelerator)
{
	// We don't use this in the example.
	return false;
}

void CmdScatterMenuModel::ExecuteCommand(int command_id)
{
	switch(command_id)
	{
	case kCommandDoSomething:
		{
			LOG(INFO) << "Done something";
			break;
		}

		// Radio items.
	case kCommandSelectAscii:
		{
			current_encoding_command_id_ = kCommandSelectAscii;
			LOG(INFO) << "Selected ASCII";
			break;
		}
	case kCommandSelectUtf8:
		{
			current_encoding_command_id_ = kCommandSelectUtf8;
			LOG(INFO) << "Selected UTF-8";
			break;
		}
	case kCommandSelectUtf16:
		{
			current_encoding_command_id_ = kCommandSelectUtf16;
			LOG(INFO) << "Selected UTF-16";
			break;
		}

		// Check items.
	case kCommandCheckApple:
	case kCommandCheckOrange:
	case kCommandCheckKiwi:
		{
			// Print what fruit is checked.
			const char* checked_fruit = "";
			if(command_id == kCommandCheckApple)
			{
				checked_fruit = "Apple";
			}
			else if(command_id == kCommandCheckOrange)
			{
				checked_fruit = "Orange";
			}
			else if(command_id == kCommandCheckKiwi)
			{
				checked_fruit = "Kiwi";
			}
			LOG(INFO) << "Checked " << checked_fruit;

			// Update the check status.
			std::set<int>::iterator iter = checked_fruits_.find(command_id);
			if(iter == checked_fruits_.end())
			{
				checked_fruits_.insert(command_id);
			}
			else
			{
				checked_fruits_.erase(iter);
			}
			break;
		}
	}
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



