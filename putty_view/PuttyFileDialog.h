#ifndef PUTTYFILEDIALOG_H
#define PUTTYFILEDIALOG_H

#include "ui_base/shell_dialogs/select_file_dialog.h"
#include "base/file_path.h"
#include "Singleton.hpp"

// usage :
//	PuttyFileDialogSingleton::instance()->showOpenDialog(nativeParent, this);

class PuttyFileDialog : public ui::SelectFileDialog::Listener {
public:
	PuttyFileDialog();
	virtual ~PuttyFileDialog();

	void showOpenDialog(gfx::NativeWindow owning_window, void* frontend);

	// SelectFileDialog::Listener implemenation.
	virtual void FileSelected(const FilePath& path,
							int index, void* params) OVERRIDE;
	virtual void FileSelectionCanceled(void* params) OVERRIDE;


private:
	scoped_refptr<ui::SelectFileDialog> select_file_dialog_;

};
typedef DesignPattern::Singleton<PuttyFileDialog> PuttyFileDialogSingleton;

#endif /* PUTTYFILEDIALOG_H */
