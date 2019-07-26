// Symbian specific defines and includes

#include <e32def.h>
#include <e32std.h>				// need to link with euser.lib

#define assert(x) __ASSERT_DEBUG(x, User::Exit(255))

