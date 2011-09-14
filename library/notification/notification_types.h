
#ifndef __notification_types_h__
#define __notification_types_h__

#pragma once

// This file describes various types used to describe and filter notifications
// that pass through the NotificationService.

namespace notification
{

    enum NotificationType
    {
        NOTIFICATION_START = 0,

        // General -----------------------------------------------------------------

        // Special signal value to represent an interest in all notifications.
        // Not valid when posting a notification.
        NOTIFICATION_ALL = NOTIFICATION_START,

        // The app is done processing user actions, now is a good time to do
        // some background work.
        NOTIFICATION_IDLE,

        // Means that the app has just started doing something in response to a
        // user action, and that background processes shouldn't run if avoidable.
        NOTIFICATION_BUSY,

#include "notification_user_types.h"

        // Custom notifications used by the embedder should start from here.
        NOTIFICATION_END,
    };

} //namespace notification

#endif //__notification_types_h__