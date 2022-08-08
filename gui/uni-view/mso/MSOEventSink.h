#pragma once

#include "refcounter.h"

namespace Word
{
struct _Application;
struct _Document;
} // namespace Word

namespace Excel
{
struct _Application;
struct _Workbook;
} // namespace Excel

extern const IID IID_IWordAppEvents2;
extern const IID IID_IExcelAppEvents2;

class MSOEventSink : public IDispatch, public RefCounter
{
public:
	MSOEventSink();

	/*  IUnknown */
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
		__RPC__deref_out void __RPC_FAR *__RPC_FAR *ppvObject);

	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	/*  IDispatch */
	HRESULT STDMETHODCALLTYPE GetTypeInfoCount(__RPC__out UINT *pctinfo);
	
	HRESULT STDMETHODCALLTYPE GetTypeInfo (UINT iTInfo, LCID lcid, 
		__RPC__deref_out_opt ITypeInfo **ppTInfo);
	
	HRESULT STDMETHODCALLTYPE GetIDsOfNames(__RPC__in REFIID riid,
		__RPC__in_ecount_full(cNames) LPOLESTR *rgszNames,
		UINT cNames, LCID lcid, __RPC__out_ecount_full(cNames) DISPID *rgDispId);

	STDMETHOD (Invoke)(DISPID dispidMember,
		REFIID riid,
		LCID lcid,
		WORD wFlags,
		DISPPARAMS* pdispparams,
		VARIANT* pvarResult,
		EXCEPINFO* pexcepinfo,
		UINT* puArgErr);

	bool	SetApplication(Word::_Application * pWordApp);
	bool	SetApplication(Excel::_Application * pExcelApp);
	void	SetMSOType(int _msoType);
	int		GetMSOType() const;
	bool	SetWordThisDocument(Word::_Document * _pWordThisDocument);
	bool	SetExcelThisDocument(Excel::_Workbook * _pExcelThisDocument);

private:
    Word::_Application* pWordApplication = nullptr;
    Word::_Document* pWordThisDocument = nullptr;
    Excel::_Application* pExcelApplication = nullptr;
    Excel::_Workbook* pExcelThisDocument = nullptr;
    int msoType = 0;

    ~MSOEventSink();
    void	Clear();

	HRESULT DoWordInvoke(DISPID dispIdMember, DISPPARAMS* pDispParams);
	HRESULT DoExcelInvoke(DISPID dispIdMember, DISPPARAMS* pDispParams);
};
