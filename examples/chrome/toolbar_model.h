
#ifndef __toolbar_model_h__
#define __toolbar_model_h__

#pragma once

#include <string>

#include "base/basic_types.h"
#include "base/string16.h"

class Browser;
class NavigationController;

// This class is the model used by the toolbar, location bar and autocomplete
// edit.  It populates its states from the current navigation entry retrieved
// from the navigation controller returned by GetNavigationController().
class ToolbarModel
{
public:
    explicit ToolbarModel(Browser* browser);
    ~ToolbarModel();

    // Returns the text that should be displayed in the location bar.
    string16 GetText() const;

    // Returns the resource_id of the icon to show to the left of the address,
    // based on the current URL.  This doesn't cover specialized icons while the
    // user is editing; see OmniboxView::GetIcon().
    int GetIcon() const;

    // Getter/setter of whether the text in location bar is currently being
    // edited.
    void set_input_in_progress(bool value) { input_in_progress_ = value; }
    bool input_in_progress() const { return input_in_progress_; }

private:
    // Returns the navigation controller used to retrieve the navigation entry
    // from which the states are retrieved.
    // If this returns NULL, default values are used.
    NavigationController* GetNavigationController() const;

    Browser* browser_;

    // Whether the text in the location bar is currently being edited.
    bool input_in_progress_;

    DISALLOW_IMPLICIT_CONSTRUCTORS(ToolbarModel);
};

#endif //__toolbar_model_h__