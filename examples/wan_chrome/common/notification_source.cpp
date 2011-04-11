
#include "notification_source.h"

NotificationSource::NotificationSource(const NotificationSource& other)
: ptr_(other.ptr_) {}

NotificationSource::NotificationSource(const void* ptr) : ptr_(ptr) {}

NotificationSource::~NotificationSource() {}