
#ifndef __wan_chrome_common_notification_observer_h__
#define __wan_chrome_common_notification_observer_h__

#pragma once

class NotificationDetails;
class NotificationSource;
class NotificationType;

// This is the base class for notification observers. When a matching
// notification is posted to the notification service, Observe is called.
class NotificationObserver
{
public:
    NotificationObserver();
    virtual ~NotificationObserver();

    virtual void Observe(NotificationType type,
        const NotificationSource& source,
        const NotificationDetails& details) = 0;
};

#endif //__wan_chrome_common_notification_observer_h__