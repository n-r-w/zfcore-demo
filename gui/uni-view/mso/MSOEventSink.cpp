#ifdef UNICODE
#undef UNICODE
#endif
#ifdef _UNICODE
#undef _UNICODE
#endif

#include "MSOEventSink.h"
#include "msoglobals.h"
#include "msoinc.h"

extern const IID IID_IWordAppEvents2 =  
	{0x000209FE,0x0000,0x0000, {0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
extern const IID IID_IExcelAppEvents2 = 
	 {0x00024413,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
 
enum EnumMSODisps
{
	msoDispWordSelectionChanged		= 0x0000000CL,
	msoDispWordBeforePrint			= 0x00000007L,
	msoDispExcelSelectionChanged	= 0x00000616L
};

MSOEventSink::MSOEventSink()
    : pWordApplication(nullptr)
    , pWordThisDocument(nullptr)
    , pExcelApplication(nullptr)
    , pExcelThisDocument(nullptr)
    , msoType(0)
{
}

MSOEventSink::~MSOEventSink()
{
    RELEASE(pWordApplication)
    RELEASE(pWordThisDocument)
    RELEASE(pExcelApplication)
    RELEASE(pExcelThisDocument)
}

bool MSOEventSink::SetApplication(Word::_Application * pWordApp)
{
    RELEASE(pWordApplication)

    if (FAILED(pWordApp->QueryInterface(__uuidof(Word::_Application), (void**)&pWordApplication))) {
        DebugTrap();
		return false;
    }

    return true;
}

bool MSOEventSink::SetApplication(Excel::_Application * pExcelApp)
{
    RELEASE(pExcelApplication)

    if (FAILED(pExcelApp->QueryInterface(__uuidof(Excel::_Application), (void**)&pExcelApplication))) {
        DebugTrap();
		return false;
    }

    return true;
}

bool MSOEventSink::SetWordThisDocument(Word::_Document * _pWordThisDocument)
{
	RELEASE(pWordThisDocument);

	if( FAILED(_pWordThisDocument->QueryInterface(__uuidof(Word::_Document), (void**)&pWordThisDocument)) )
	{
		DebugTrap();
		return false;
	}

	return true;
}

bool MSOEventSink::SetExcelThisDocument(Excel::_Workbook * _pExcelThisDocument)
{
	RELEASE(pExcelThisDocument);

	if( FAILED(_pExcelThisDocument->QueryInterface(__uuidof(Excel::_Workbook), (void**)&pExcelThisDocument)) )
	{
		DebugTrap();
		return false;
	}

	return true;
}

void MSOEventSink::SetMSOType(int _msoType)
{
	msoType = _msoType;
}

int MSOEventSink::GetMSOType() const
{
	return msoType;
}

HRESULT MSOEventSink::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = nullptr;

    if (IsEqualGUID(riid, IID_IUnknown))
		*ppvObject = reinterpret_cast<void**>(this);

	if (IsEqualGUID(riid, IID_IWordAppEvents2))
		*ppvObject = reinterpret_cast<void**>(this);

	if (IsEqualGUID(riid, IID_IDispatch))
		*ppvObject = reinterpret_cast<void**>(this);

	if (IsEqualGUID(riid, IID_IExcelAppEvents2))
		*ppvObject = reinterpret_cast<void**>(this);

	if (*ppvObject)
	{
		((IUnknown*)*ppvObject)->AddRef();
		return S_OK;
	}
	else return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE MSOEventSink::AddRef()
{
	//_LOGINFO("%s: AddRef, %d", __FUNCTION__, refcounter);
	return RefCounter::AddRef();
}

ULONG STDMETHODCALLTYPE MSOEventSink::Release()
{
	//_LOGINFO("%s: Release, %d", __FUNCTION__, refcounter);
	return RefCounter::Release();
}

STDMETHODIMP MSOEventSink::GetTypeInfoCount(unsigned int FAR* pctinfo)
{
	*pctinfo = 0;
	return S_OK;
}

STDMETHODIMP MSOEventSink::GetTypeInfo(unsigned int /*iTInfo*/, LCID  /*lcid*/, ITypeInfo FAR* FAR*  /*ppTInfo*/)
{
	return E_NOTIMPL;
}

STDMETHODIMP MSOEventSink::GetIDsOfNames(REFIID /*riid*/, OLECHAR FAR* FAR* /*rgszNames*/, unsigned int /*cNames*/, LCID /*lcid*/, DISPID FAR* /*rgDispId*/)
{
	return E_NOTIMPL;
}

HRESULT MSOEventSink::DoWordInvoke(DISPID dispIdMember, DISPPARAMS* pDispParams)
{
	HRESULT hr = S_OK;

	Word::Window * activeWindow = 0;
	if( FAILED(hr = pWordApplication->get_ActiveWindow(&activeWindow)) )
		return hr;

	Word::_Document * activeDocument = 0;
	if( FAILED(hr = activeWindow->get_Document(&activeDocument)) )
	{
		RELEASE(activeWindow);
		return hr;
	}

	if( activeDocument != pWordThisDocument )
	{
		RELEASE(activeDocument);
		RELEASE(activeWindow);
		
		return S_OK; // not our window
	}
	RELEASE(activeDocument);
	RELEASE(activeWindow);

	if( dispIdMember == msoDispWordSelectionChanged && pDispParams->rgvarg[0].vt == VT_DISPATCH )
	{ // WindowSelectionChange
		Word::Selection * selection = 0;

		hr = pDispParams->rgvarg[0].pdispVal->QueryInterface(__uuidof(Word::Selection), (void**)&selection);

		if( SUCCEEDED(hr) )
		{ // clear selection to avoid copying
			long start = 0;
			selection->get_Start(&start);
			selection->put_Start(start);
			selection->put_End(start);
		} // clear selection to avoid copying
		RELEASE(selection);
	} // WindowSelectionChange

	if( dispIdMember == msoDispWordBeforePrint && 
		pDispParams->cArgs == 2 &&
		pDispParams->rgvarg[1].vt == (VT_BYREF | VT_BOOL) )
	{ // DocumentBeforePrint
		Word::Window * activeWindow = 0;
		if( FAILED(hr = pWordApplication->get_ActiveWindow(&activeWindow)) )
			return hr;

		Word::_Document * activeDocument = 0;
		if( FAILED(hr = activeWindow->get_Document(&activeDocument)) )
		{
			RELEASE(activeWindow);
			return hr;
		}

		*pDispParams->rgvarg[1].pboolVal = VARIANT_TRUE; // Cancel printing
	} // DocumentBeforePrint

	return hr;
}

HRESULT MSOEventSink::DoExcelInvoke(DISPID dispIdMember, DISPPARAMS* pDispParams)
{
	HRESULT hr = S_OK;

	Excel::_Workbook * activeDocument = pExcelApplication->GetActiveWorkbook();

	if( !activeDocument )
		return S_OK;

	if( activeDocument != pExcelThisDocument )
	{
		RELEASE(activeDocument);
		return S_OK;// not our workbook
	}
	RELEASE(activeDocument);

	if( dispIdMember == msoDispExcelSelectionChanged && 
		pDispParams->cArgs == 2 &&
		pDispParams->rgvarg[1].vt == VT_DISPATCH )
	{ // SheetSelectionChange
		Excel::Range * range = 0;

		hr = pDispParams->rgvarg[1].pdispVal->QueryInterface(__uuidof(Excel::Range), (void**)&range);

		if( SUCCEEDED(hr) )
		{ // clear selection to avoid copying
			
		} // clear selection to avoid copying
		RELEASE(range);
	} // SheetSelectionChange

	return hr;
}

STDMETHODIMP MSOEventSink::Invoke(DISPID dispIdMember, REFIID /*riid*/, LCID /*lcid*/,
										 WORD /*wFlags*/, DISPPARAMS* pDispParams, VARIANT* /*pVarResult*/,
										 EXCEPINFO * /*pExcepInfo*/, UINT * /*puArgErr*/)
{
	#ifdef _DEBUG
    //	_debugOutput("%s: dispid 0x%08X\n", __FUNCTION__, dispIdMember);
#endif // _DEBUG

	if( !pDispParams || pDispParams->cArgs < 1 || !pDispParams->rgvarg )
		return S_OK; // no parameters

	switch( GetMSOType() )
	{
		case MSO_WORD:
			DoWordInvoke(dispIdMember, pDispParams);
			break;
		case MSO_EXCEL:
			DoExcelInvoke(dispIdMember, pDispParams);
			break;
	}

	return S_OK;
}

