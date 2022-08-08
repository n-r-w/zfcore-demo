#ifdef UNICODE
#undef UNICODE
#endif
#ifdef _UNICODE
#undef _UNICODE
#endif

#include "refcounter.h"
#include <assert.h>

ULONG STDMETHODCALLTYPE RefCounter::AddRef()
{
	return InterlockedIncrement(&refcounter);
}
ULONG STDMETHODCALLTYPE RefCounter::Release()
{
    assert(refcounter > 0);
    long tmp = InterlockedDecrement(&refcounter);
    if (tmp == 0)
        delete this;
	return tmp;
}
