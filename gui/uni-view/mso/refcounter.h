#pragma once

#include <Windows.h>

class RefCounter
{
protected:
	volatile mutable long refcounter;
	RefCounter(void) : refcounter(1) {}
	virtual ~RefCounter(){}
public:
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);
};
