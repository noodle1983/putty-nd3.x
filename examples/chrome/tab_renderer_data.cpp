
#include "tab_renderer_data.h"

TabRendererData::TabRendererData()
: network_state(NETWORK_STATE_NONE),
loading(false),
crashed_status(base::TERMINATION_STATUS_STILL_RUNNING),
show_icon(true),
mini(false),
blocked(false) {}

TabRendererData::~TabRendererData() {}

bool TabRendererData::Equals(const TabRendererData& data)
{
    return favicon.pixelRef() &&
        favicon.pixelRef()==data.favicon.pixelRef() &&
        favicon.pixelRefOffset()==data.favicon.pixelRefOffset() &&
        network_state==data.network_state &&
        title==data.title &&
        url==data.url &&
        loading==data.loading &&
        crashed_status==data.crashed_status &&
        show_icon==data.show_icon &&
        mini==data.mini &&
        blocked==data.blocked;
}