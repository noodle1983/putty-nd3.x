
#include "default_theme_provider.h"

#include "ui_base/resource/resource_bundle.h"

#include "view/widget/native_widget_win.h"

namespace view
{

    DefaultThemeProvider::DefaultThemeProvider() {}

    DefaultThemeProvider::~DefaultThemeProvider() {}

    void DefaultThemeProvider::Init(Profile* profile) {}

    SkBitmap* DefaultThemeProvider::GetBitmapNamed(int id) const
    {
        return ui::ResourceBundle::GetSharedInstance().GetBitmapNamed(id);
    }

    SkColor DefaultThemeProvider::GetColor(int id) const
    {
        // Return debugging-blue.
        return 0xff0000ff;
    }

    bool DefaultThemeProvider::GetDisplayProperty(int id, int* result) const
    {
        return false;
    }

    bool DefaultThemeProvider::ShouldUseNativeFrame() const
    {
        return NativeWidgetWin::IsAeroGlassEnabled();
    }

    bool DefaultThemeProvider::HasCustomImage(int id) const
    {
        return false;
    }
    
    RefCountedMemory* DefaultThemeProvider::GetRawData(int id) const
    {
        return NULL;
    }

} //namespace view