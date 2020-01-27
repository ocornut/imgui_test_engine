// Win32 only: activate CRT heap debugging

#ifdef _WIN32
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

//-------------------------------------------------------------------------
// FUNCTIONS
//-------------------------------------------------------------------------
// DebugCrtInit()
// DebugCrtDumpLeaks()
//-------------------------------------------------------------------------

// Call this with break_alloc != 0 to request malloc/new to break on a given allocation.
static inline void DebugCrtInit(long break_alloc = 0)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    if (break_alloc != 0)
        _CrtSetBreakAlloc(break_alloc);
}

static inline void DebugCrtDumpLeaks()
{
    _CrtDumpMemoryLeaks();
}

#endif // _WIN32
