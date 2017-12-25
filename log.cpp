
#include <iostream>

#include "log.hpp"

namespace kvak::log {

perror_class perror;

log_wrapper debug("\x1b[97m[DBG] ", std::cerr);
log_wrapper info ("\x1b[1m\x1b[94m[INF] ", std::cerr);
log_wrapper error("\x1b[1m\x1b[91m[ERR] ", std::cerr);

}
