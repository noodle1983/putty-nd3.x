//#include "base/basictypes.h"
//#include "base/compiler_specific.h"
#include "base/utf_string_conversions.h"
#include "ui_gfx/canvas.h"
#include "view/controls/label.h"
#include "view/view.h"
//#include "ui/view/widget/widget.h"
#include "view/widget/widget_delegate.h"

class WindowView : public view::WidgetDelegateView {
 public:
  WindowView() : label_(NULL) {
    Init();
  }

  virtual ~WindowView() {}

 private:
  // Overridden from view::View:
  virtual void OnPaint(gfx::Canvas* canvas) OVERRIDE {
	gfx::Rect rect = GetLocalBounds();
    canvas->FillRectInt(SK_ColorWHITE,
		rect.x(),
		rect.y(),
		rect.width(),
		rect.height()
		);
  }
  virtual void Layout() OVERRIDE {
    gfx::Size ps = label_->GetPreferredSize();
    label_->SetBounds((width() - ps.width()) / 2, (height() - ps.height()) / 2, ps.width(), ps.height());
  }
  virtual gfx::Size GetPreferredSize() OVERRIDE {
    gfx::Size ps = label_->GetPreferredSize();
    ps.set_width(ps.width() + 200);
    ps.set_height(ps.height() + 200);
    return ps;
  }

  // Overridden from view::WidgetDelegate:
  virtual string16 GetWindowTitle() const OVERRIDE{
    return ASCIIToUTF16("Hello World Window");
  }
  virtual bool CanResize() const OVERRIDE {
    return true;
  }
  virtual bool CanMaximize() const OVERRIDE {
    return true;
  }
  virtual view::View* GetContentsView() OVERRIDE {
    return this;
  }

  void Init() {
    label_ = new view::Label(ASCIIToUTF16("Hello, World!"));
    AddChildView(label_);
  }

  view::Label* label_;

  DISALLOW_COPY_AND_ASSIGN(WindowView);
};