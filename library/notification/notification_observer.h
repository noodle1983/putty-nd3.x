
#ifndef __notification_observer_h__
#define __notification_observer_h__

#pragma once

class NotificationDetails;
class NotificationSource;

// This is the base class for notification observers. When a matching
// notification is posted to the notification service, Observe is called.
class NotificationObserver
{
public:
    NotificationObserver();
    virtual ~NotificationObserver();

    virtual void Observe(int type,
        const NotificationSource& source,
        const NotificationDetails& details) = 0;
};

#endif //__notification_observer_h__