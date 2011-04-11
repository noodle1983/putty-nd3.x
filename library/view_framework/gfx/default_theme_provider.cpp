
#include "default_theme_provider.h"

#include "../base/resource_bundle.h"
#include "../widget/widget_win.h"

namespace view
{

    DefaultThemeProvider::DefaultThemeProvider() {}

    DefaultThemeProvider::~DefaultThemeProvider() {}

    void DefaultThemeProvider::Init(Profile* profile) {}

    SkBitmap* DefaultThemeProvider::GetBitmapNamed(int id) const
    {
        return ResourceBundle::GetSharedInstance().GetBitmapNamed(id);
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
        return WidgetWin::IsAeroGlassEnabled();
    }

    bool DefaultThemeProvider::HasCustomImage(int id) const
    {
        return false;
    }
    
    base::RefCountedMemory* DefaultThemeProvider::GetRawData(int id) const
    {
        return NULL;
    }

} //namespace view