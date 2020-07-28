#include <gc/gc.h>
#if GC_VERSION_MAJOR == 7
#include "gc7/cordbscs.c"
#elif GC_VERSION_MAJOR == 8
#include "gc8/cordbscs.c"
#else
#error "versione gc non riconosciuta"
#endif
