
#include "notification_details.h"

NotificationDetails::NotificationDetails() : ptr_(NULL) {}

NotificationDetails::NotificationDetails(const NotificationDetails& other)
: ptr_(other.ptr_) {}

NotificationDetails::NotificationDetails(const void* ptr) : ptr_(ptr) {}

NotificationDetails::~NotificationDetails() {}