#include "../trp.h"
#if GC_VERSION_MAJOR == 7
#include "gc7/cordxtra.c"
#elif GC_VERSION_MAJOR == 8
#include "gc8/cordxtra.c"
#else
#error "versione gc non riconosciuta"
#endif
