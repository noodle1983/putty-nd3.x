
#ifndef __tab_renderer_data_h__
#define __tab_renderer_data_h__

#pragma once

#include "base/process_util.h"
#include "base/string16.h"

#include "SkBitmap.h"

#include "url.h"

// Wraps the state needed by the renderers.
struct TabRendererData
{
    // Different types of network activity for a tab. The NetworkState of a tab
    // may be used to alter the UI (e.g. show different kinds of loading
    // animations).
    enum NetworkState
    {
        NETWORK_STATE_NONE,     // no network activity.
        NETWORK_STATE_WAITING,  // waiting for a connection.
        NETWORK_STATE_LOADING,  // connected, transferring data.
		NETWORK_STATE_DISCONNECTED      // disconnected
    };

    TabRendererData();
    ~TabRendererData();

    // This interprets the crashed status to decide whether or not this
    // render data represents a tab that is "crashed" (i.e. the render
    // process died unexpectedly).
    bool IsCrashed() const
    {
        return (crashed_status==base::TERMINATION_STATUS_PROCESS_WAS_KILLED ||
            crashed_status==base::TERMINATION_STATUS_PROCESS_CRASHED ||
            crashed_status==base::TERMINATION_STATUS_ABNORMAL_TERMINATION);
    }

    // Returns true if the TabRendererData is same as given |data|. Two favicons
    // are considered equals if two SkBitmaps point to the same SkPixelRef object.
    bool Equals(const TabRendererData& data);

    SkBitmap favicon;
    NetworkState network_state;
    string16 title;
	string16 index;
    Url url;
    bool loading;
    base::TerminationStatus crashed_status;
    bool show_icon;
    bool mini;
    bool blocked;
};

#endif //__tab_renderer_data_h__