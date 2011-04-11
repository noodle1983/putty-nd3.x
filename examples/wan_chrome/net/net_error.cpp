
#include "net_error.h"

namespace net
{

    const char kErrorDomain[] = "net";

    const char* ErrorToString(int error)
    {
        if(error == 0)
        {
            return "net::OK";
        }

        switch(error)
        {
#define NET_ERROR(label, value) \
  case ERR_ ## label: \
  return "net::" STRINGIZE_NO_EXPANSION(ERR_ ## label);
#include "net_error_list.h"
#undef NET_ERROR
  default:
      return "net::<unknown>";
        }
    }

} //namespace net