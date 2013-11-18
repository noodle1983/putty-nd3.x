#include "PuttyFileDialog.h"
#include "ui_base/shell_dialogs/select_file_policy.h"
#include "zmodem_session.h"


class PuttySelectFilePolicy : public ui::SelectFilePolicy {
public:
	explicit PuttySelectFilePolicy(){}
	virtual ~PuttySelectFilePolicy(){}

	// Overridden from ui::SelectFilePolicy:
	virtual bool CanOpenSelectFileDialog() {return true;}
	virtual void SelectFileDenied() {}

private:

	DISALLOW_COPY_AND_ASSIGN(PuttySelectFilePolicy);
};

PuttyFileDialog::PuttyFileDialog()
	: select_file_dialog_(ui::SelectFileDialog::Create(
        this, new PuttySelectFilePolicy())) {

}


PuttyFileDialog::~PuttyFileDialog()
{

}

void PuttyFileDialog::showOpenDialog(gfx::NativeWindow owning_window, void* frontend)
{
  select_file_dialog_->SelectFile(
      ui::SelectFileDialog::SELECT_OPEN_FILE, string16(),
      FilePath(), NULL,
      0,  // 1-based index for |file_type_info.extensions| to specify default.
      L"",
      owning_window, frontend);
}

// SelectFileDialog::Listener implemenation.
void PuttyFileDialog::FileSelected(const FilePath& path,
						int index, void* params)
{
	ZmodemSession* zSession = (ZmodemSession*) params;
	zSession->onFileSelected(path);
}

void PuttyFileDialog::FileSelectionCanceled(void* params)
{
	ZmodemSession* zSession = (ZmodemSession*) params;
	zSession->reset();
}