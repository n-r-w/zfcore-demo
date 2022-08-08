#define _CRT_SECURE_NO_WARNINGS

#ifdef UNICODE
#undef UNICODE
#endif
#ifdef _UNICODE
#undef _UNICODE
#endif

#include "zf_core.h"

#include "MSOControl.h"
#include "pointer.h"

#include <QApplication>
#include <QDebug>
#include <QString>
#include <QWidget>
#include <QDir>

#pragma warning(disable:4530) // warning C4530: C++ exception handler used, but unwind semantics are not enabled

const WCHAR MSO_WORD_PROGID[]		= L"Word.Document";
const WCHAR MSO_EXCEL_PROGID[]		= L"Excel.Sheet";
#define MSOCONTROL_WIN "MSOCONTROL"

struct SFindWnd
{
    HWND hFoundWnd;
    const char* wndClassName;
};

static BOOL CALLBACK EnumWndChilds(HWND hwnd, LPARAM lParam)
{
    SFindWnd* pFindWnd = reinterpret_cast<SFindWnd*>(lParam);

    char* buffer = reinterpret_cast<char*>(calloc(KB, sizeof(char)));

    if (pFindWnd && GetClassName(hwnd, buffer, KB) && strcmp(pFindWnd->wndClassName, buffer) == 0) {
        pFindWnd->hFoundWnd = hwnd;
        free(buffer);
        return FALSE;
    }

    free(buffer);
    return TRUE;
}

#ifdef UNICODE
#undef UNICODE
#endif
#ifdef _UNICODE
#undef _UNICODE
#endif

static HWND FindChildWndWithClassName(HWND hParentWnd, const char* wndClassName)
{
    if (!IsWindow(hParentWnd)) {
        DebugTrap();
        return nullptr;
    }

    SFindWnd findWnd;
    findWnd.hFoundWnd = nullptr;
    findWnd.wndClassName = wndClassName;

    EnumChildWindows(hParentWnd, EnumWndChilds, (LPARAM)&findWnd);

    return findWnd.hFoundWnd;
}


static bool _GetMSOVersion(WORD msoStyle, QString& version)
{
    QString msoAppKey;

    switch (msoStyle) {
        case MSO_WORD: {
            msoAppKey = "Word.Application";
            break;
        }
        case MSO_EXCEL: {
            msoAppKey = "Excel.Application";
            break;
        }
        default: {
            zf::Core::logError("unsupported MSO document");
            return false;
        }
    }
    msoAppKey += "\\CurVer";

    HKEY hMSOApp = nullptr;

    if (RegOpenKey(HKEY_CLASSES_ROOT, msoAppKey.toLatin1().constData(), &hMSOApp) != ERROR_SUCCESS)
        return false;

    DWORD msoAppIDSize = 128;
    char* msoAppID = static_cast<char*>(calloc(msoAppIDSize, sizeof(char)));
    if (RegQueryValueEx(hMSOApp, nullptr, nullptr, nullptr, (BYTE*)msoAppID, &msoAppIDSize) != ERROR_SUCCESS) {
        RegCloseKey(hMSOApp);
        return false;
    }

    version = QString::fromLatin1(msoAppID);

    RegCloseKey(hMSOApp);

    return true;
}

static bool IsWord2013()
{
    QString version;
    if (!_GetMSOVersion(MSO_WORD, version) || version.isEmpty())
        return false;

    if (version.contains(".15")) // MSO2013
        return true;
    if (version.contains(".16")) // MSO .16 B
        return true;

    return false;
}

bool IsExcel2013()
{
    QString version;
    if (!_GetMSOVersion(MSO_EXCEL, version) || version.isEmpty())
        return false;

    if (version.contains(".15")) // MSO2013
        return true;
    if (version.contains(".16")) // MSO .16 B
        return true;

    return false;
}

bool IsXFormatAvailable( WORD msoStyle )
{
    QString version;
    if (!_GetMSOVersion(msoStyle, version) || version.isEmpty())
        return false;

    bool isCompatibleVersion = false;
    if (version.contains(".16")) // MSO .16 B
        isCompatibleVersion = true;
    if (version.contains(".15")) // MSO2013
        isCompatibleVersion = true;
    if (version.contains(".14")) // MSO2010
        isCompatibleVersion = true;
    if (version.contains(".12")) // MSO2007
        isCompatibleVersion = true;

    return isCompatibleVersion;
}

int GetWordVersion()
{
    QString version;
    if (!_GetMSOVersion(MSO_WORD, version) || version.isEmpty())
        return 0;

    if (version.contains(".16")) // MSO .16 B
        return 2016;
    if (version.contains(".15")) // MSO2013
        return 2013;
    if (version.contains(".14")) // MSO2010
        return 2010;
    if (version.contains(".12")) // MSO2007
        return 2007;
    if (version.contains(".11")) // MSO2003
        return 2003;

    return 0;
}

int GetExcelVersion()
{
    QString version;
    if (!_GetMSOVersion(MSO_EXCEL, version) || version.isEmpty())
        return 0;

    if (version.contains(".16")) // MSO .16 B
        return 2016;
    if (version.contains(".15")) // MSO2013
        return 2013;
    if (version.contains(".14")) // MSO2010
        return 2010;
    if (version.contains(".12")) // MSO2007
        return 2007;
    if (version.contains(".11")) // MSO2003
        return 2003;

    return 0;
}

bool IsMSOAvailable( WORD msoStyle )
{
    QString version;
    if (!_GetMSOVersion(msoStyle, version) || version.isEmpty())
        return false;

    bool isCompatibleVersion = false;
    if (version.contains(".16")) // MSO .16 B
        isCompatibleVersion = true;
    if (version.contains(".15")) // MSO2013
        isCompatibleVersion = true;
    if (version.contains(".14")) // MSO2010
        isCompatibleVersion = true;
    if (version.contains(".12")) // MSO2007
        isCompatibleVersion = true;
    if (version.contains(".11")) // MSO2003
        isCompatibleVersion = true;

    return isCompatibleVersion;
}

bool IsMSOAvailableWithXP( WORD msoStyle )
{
    QString version;
    if (!_GetMSOVersion(msoStyle, version) || version.isEmpty())
        return false;

    bool isCompatibleVersion = false;
    if (version.contains(".16")) // MSO .16 B
        isCompatibleVersion = true;
    if (version.contains(".15")) // MSO2013
        isCompatibleVersion = true;
    if (version.contains(".14")) // MSO2010
        isCompatibleVersion = true;
    if (version.contains(".12")) // MSO2007
        isCompatibleVersion = true;
    if (version.contains(".11")) // MSO2003
        isCompatibleVersion = true;
    if (version.contains(".10")) // MSO2002(XP)
        isCompatibleVersion = true;

    return isCompatibleVersion;
}

static wchar_t* a2w(const char* astr, size_t alen = MAX_SIZE_T, size_t* p_wlen = nullptr, uint cp = CP_ACP)
{
    wchar_t* result = 0;
    if (p_wlen)
        *p_wlen = 0;
    if (astr) {
        if (alen == MAX_SIZE_T)
            alen = strlen(astr);
        int wlen = MultiByteToWideChar(cp, 0, astr, static_cast<int>(alen), 0, 0); // mbstowcs(0,astr,0);
        if (wlen > 0) {
            result = reinterpret_cast<wchar_t*>(calloc(wlen + 1, sizeof(wchar_t)));
            int ret = MultiByteToWideChar(cp, 0, astr, static_cast<int>(alen), result, wlen); // mbstowcs(result,astr,len);
            result[ret] = L'\0';
            if (p_wlen)
                *p_wlen = ret;
        }
    }
    return result;
}

static BSTR Char2SysStrAllocated(const TCHAR* str)
{
    BSTR bstrVal = nullptr;
#ifdef UNICODE
    bstrVal = SysAllocString(str);
#else
    if (!str)
        str = "";
    wchar_t* buffer = a2w(str);
    bstrVal = SysAllocString(buffer);
    free(buffer);
#endif
    return bstrVal;
}

static void GUIDGenWithStrictResult(BSTR* bstrGUID)
{
    *bstrGUID = Char2SysStrAllocated("A07A3855-453C-4e78-965C-91A3F5A5B85B");
}

//////////////////////////////////////////////////////////////////////////
//
// IServiceProvider
//
STDMETHODIMP MSOControl::QueryService(REFGUID /*guidService*/, REFIID /*riid*/, void ** /*ppvObject*/)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////
//
// IUnknown
//

HRESULT MSOControl::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = nullptr;

    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = reinterpret_cast<void**>(this);

    if (IsEqualGUID(riid, IID_IMSOObjectSite))
        *ppvObject = reinterpret_cast<void**>(this);

    if (*ppvObject) {
        (reinterpret_cast<IUnknown*>(*ppvObject))->AddRef();
        return S_OK;
    } else
        return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE MSOControl::AddRef()
{
	return RefCounter::AddRef();
}

ULONG STDMETHODCALLTYPE MSOControl::Release()
{
	return RefCounter::Release();
}

//////////////////////////////////////////////////////////////////////////
//
// IMSOObjectSite
//

STDMETHODIMP MSOControl::GetWindow(HWND* phWnd)
{
	*phWnd = hWindow;
	return S_OK;
}

STDMETHODIMP MSOControl::GetBorder(LPRECT prcBorder)
{
    if (hWindow == nullptr)
        return E_FAIL;

    GetClientRect(hWindow, prcBorder);

    return S_OK;
}

STDMETHODIMP MSOControl::SetStatusText(LPCOLESTR /*pszText*/)
{
	return S_OK;
}

STDMETHODIMP MSOControl::GetHostName(LPWSTR * /*ppwszHostName*/)
{
	return E_NOTIMPL;
}

STDMETHODIMP MSOControl::SysMenuCommand(UINT /*uiCharCode*/)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////
//
// MSOControl
//
MSOControl::MSOControl(HWND _hWindow, int _elemType)
    : pMSO(nullptr)
#if LIGHT_MSO_WINDOW()
    , enableLightMSO(false)
    , hInternalMSOWnd(nullptr)
#endif // LIGHT_MSO_WINDOW()
{
    elemType = _elemType;

    WCHAR msoObjectType[128];

    hWindow = _hWindow; // parent

    pConnectionPtContainer = nullptr;
    pConnectionPt = nullptr;
    pMSOEventSink = nullptr;
    dwCookie = 0;

    pMSO = MSOObject::CreateInstance(this);

    if (elemType < 0) { // get type from styles
        long wndStyle = GetWindowLong(hWindow, GWL_STYLE);

        if ((wndStyle & MSO_WORD) == MSO_WORD) {
            wcscpy(msoObjectType, MSO_WORD_PROGID);
            elemType = VIEWFORM_TYPE_MSWORD;

            if (IsWord2013())
                pMSO->SetDisableDocValidation(true);
        } else if ((wndStyle & MSO_EXCEL) == MSO_EXCEL) {
            wcscpy(msoObjectType, MSO_EXCEL_PROGID);
            elemType = VIEWFORM_TYPE_MSEXCEL;

            if (IsExcel2013())
                pMSO->SetDisableDocValidation(true);
        } else
            DebugTrap();
        bReadOnly = ((wndStyle & MSO_READONLY) == MSO_READONLY);

    } // get type from styles
    else { // get from element type
        if (elemType == VIEWFORM_TYPE_MSWORD) {
            wcscpy(msoObjectType, MSO_WORD_PROGID);
            if (IsWord2013())
                pMSO->SetDisableDocValidation(true);
        } else if (elemType == VIEWFORM_TYPE_MSEXCEL) {
            wcscpy(msoObjectType, MSO_EXCEL_PROGID);
            if (IsExcel2013())
                pMSO->SetDisableDocValidation(true);
        } else
            DebugTrap();
        bReadOnly = true; // always read-only in view mode
    } // get from element type

    HRESULT hr = S_OK;

    if (FAILED(hr = CLSIDFromProgID(msoObjectType, &msoCLSID))) {
#if 0
		{
			char buffer[1024];
			_snprintf_s(buffer, SIZEOFARRAY(buffer), _TRUNCATE, "CLSIDFromProgID: Call failed, HR 0x%08X", hr);
			MessageBox(0, buffer, "Debug", MB_OK | MB_TASKMODAL);
		}
#endif // IVAN
        DebugTrap();
        delete pMSO;
        return;
    }

    RECT rect;
    GetClientRect(hWindow, &rect);

    if (FAILED(hr = pMSO->CreateDocObject(msoCLSID))) {
#if 0
		{
			char buffer[1024];
			_snprintf_s(buffer, SIZEOFARRAY(buffer), _TRUNCATE, "CreateDocObject: Call failed, HR 0x%08X", hr);
			MessageBox(0, buffer, "Debug", MB_OK | MB_TASKMODAL);
		}
#endif // IVAN
        if (hr == DSO_E_INVALIDSERVER) { // The associated COM server does not support ActiveX Document embedding
            assert(false);
        } // The associated COM server does not support ActiveX Document embedding
        if (hr == RPC_E_SERVERFAULT) {
            // 0x80010105 (RPC_E_SERVERFAULT): The server threw an exception
            // Ask Ivan
            DebugTrap();
        } else {
            if (FAILED(hr = pMSO->CreateDocObject(msoCLSID))) {
                DebugTrap();
            }
        }
        delete pMSO;
        pMSO = nullptr;
        return;
    }

#if LIGHT_MSO_WINDOW()
    enableLightMSO = false;
#endif // LIGHT_MSO_WINDOW()

    //    SetWindowSubclass(_hWindow, WndProc, 0, 0);
}

MSOControl::~MSOControl()
{
	PrepareToDestroy();
}

void MSOControl::resize(const QSize& size)
{
    if (!pMSO)
        return;

    RECT rect = {};
    rect.right = size.width();
    rect.bottom = size.height();

    ODS("%s: W %d height %d", __FUNCTION__, rect.right, rect.bottom);

    pMSO->OnNotifySizeChange(&rect);

	#if LIGHT_MSO_WINDOW()
	if( IsWindow(GetInternalMSOWnd()) )
	{
		MoveWindow(GetInternalMSOWnd(), 0, 0, rect.right, rect.bottom, TRUE);
	}
	#endif // LIGHT_MSO_WINDOW()	
}

bool MSOControl::loadFromFile(const QString& file_name, bool web_view)
{      
    if (pMSO == nullptr)
        pMSO = MSOObject::CreateInstance(this);

    // IV: Research
    if (pMSO->GetOleObject() != nullptr)
        pMSO->Close();

    BIND_OPTS opts = {};
    opts.cbStruct = sizeof(opts);
    // IV: Don't set STGM_SHARE_EXCLUSIVE flag - it's crash MSO2003
    // opts.grfMode = STGM_READWRITE | STGM_SHARE_DENY_NONE;
    opts.grfMode = STGM_READWRITE | STGM_SHARE_EXCLUSIVE;
    opts.grfFlags = BIND_MAYBOTHERUSER;
    opts.dwTickCountDeadline = 20000;

    HRESULT hr = pMSO->CreateFromFile(QSTR_TO_WSTR(file_name), msoCLSID, &opts);

    if (SUCCEEDED(hr)) { // activate OLE
#if LIGHT_MSO_WINDOW()
        if (enableLightMSO) { // not disabled light version of MSO
            switch (elemType) {
                case VIEWFORM_TYPE_MSWORD:
                    pMSO->SetUpdateDoc(FALSE);
                    pMSO->OnNotifyChangeToolState(FALSE);
                    break;
            }
        } // not disabled light version of MSO
#endif // LIGHT_MSO_WINDOW
        if (FAILED(hr = pMSO->IPActivateView()))
            return FALSE;
        if (VIEWFORM_TYPE_MSWORD == elemType && web_view)
            setViewType(WORDOLE_WEB_VIEW);

    } // activate OLE
    else {
        DebugTrap();
        return FALSE;
    }

#if LIGHT_MSO_WINDOW()
    if (enableLightMSO) { // not disabled light version of MSO
        switch (elemType) {
            case VIEWFORM_TYPE_MSWORD:
                Sleep(500); // #42516
                SwitchToLightWord();
                ShowWindow(pMSO->GetDocWindow(), SW_SHOW);
                break;
            case VIEWFORM_TYPE_MSEXCEL:
                Sleep(500); // #42516
                SwitchToLightExcel();
                ShowWindow(pMSO->GetDocWindow(), SW_SHOW);
                break;
        }
    } // not disabled light version of MSO
#endif // LIGHT_MSO_WINDOW

    pMSO->IPActivateView();

    return TRUE;
}

bool MSOControl::setReadOnly(bool read_only)
{
    //    if (bReadOnly == read_only)
    //        return true;

    bReadOnly = read_only;
    if (pMSO == nullptr)
        return true;

    IDispatch* pDisp = pMSO->GetMSODocumentIDispatch();
    if (pDisp != nullptr) { // document created
        RELEASE(pDisp)

        //        if (FAILED(DoConnectToEvents()))
        //            return FALSE;

        if (SUCCEEDED(DoSwitchToReadOnly()))
            return true;
        else
            return false;
    } // document created
    else
        return true;
}

bool MSOControl::setViewType(int type)
{
    if (!pMSO || !pMSO->IsWordObject())
        return false;

    Word::_Document* document = nullptr;
    if (SUCCEEDED(GetMSODocument(&document)) && document) {
        Word::Window* wnd = nullptr;
        if (SUCCEEDED(document->get_ActiveWindow(&wnd)) && wnd) {
            Word::View* view = nullptr;
            if (SUCCEEDED(wnd->get_View(&view)) && view) {
                if (type == WORDOLE_PRINT_VIEW)
                    view->put_Type(Word::WdViewType::wdPrintView);
                else if (type == WORDOLE_WEB_VIEW)
                    view->put_Type(Word::WdViewType::wdWebView);
                else if (type == WORDOLE_PRINT_PREVIEW)
                    view->put_Type(Word::WdViewType::wdPrintPreview);
                else
                    DebugTrap();

                RELEASE(view)
            }
            RELEASE(wnd)
        }
        RELEASE(document)
    }

    return true;
}

bool MSOControl::print()
{
    if (pMSO) {
        switch (elemType) {
            case VIEWFORM_TYPE_MSWORD: {
                Word::_Document* document = 0;
                if (SUCCEEDED(GetMSODocument(&document))) {
                    HRESULT hr = document->PrintOut();
                    RELEASE(document);

                    if (SUCCEEDED(hr))
                        return true;
                }
                break;
            }
            case VIEWFORM_TYPE_MSEXCEL: {
                HRESULT hr = S_OK;
                Excel::_Workbook* workbook = 0;
                if (SUCCEEDED(hr = GetMSODocument(&workbook))) {
                    try {
                        hr = workbook->_PrintOut();
                    } catch (...) {
                        hr = E_FAIL;
                    }

                    RELEASE(workbook);

                    if (SUCCEEDED(hr))
                        return true;
                    else
                        return false;
                }
                break;
            }
            default:
                DebugTrap();
                return false;
        }
    }

    return true;
}

bool MSOControl::isDirty() const
{
    return pMSO ? pMSO->IsDirty() : false;
}

bool MSOControl::save()
{
    if (pMSO)
        return SUCCEEDED(pMSO->Save());

    return false;
}

bool MSOControl::saveAs(const QString& file_name)
{
    if (pMSO)
        return SUCCEEDED(pMSO->SaveToFile(QSTR_TO_WSTR(file_name), true));

    return false;
}

bool MSOControl::saveWordAsPdf(const QString& file_office, const QString& file_pdf)
{
    bool res = false;

    QString file_office_win = QDir::toNativeSeparators(file_office);
    QString file_pdf_win = QDir::toNativeSeparators(file_pdf);

    Word::_ApplicationPtr application;
    if (FAILED(application.CreateInstance("Word.Application")))
        return false;

    Word::Documents* documents;
    if (SUCCEEDED(application->get_Documents(&documents))) {        
        VARIANT varFileName;
        VariantInit(&varFileName);
        V_VT(&varFileName) = VT_BSTR;
        V_BSTR(&varFileName) = QSTR_TO_BSTR(file_office_win);

        VARIANT varFalse;
        VariantInit(&varFalse);
        V_VT(&varFalse) = VT_BOOL;
        V_BOOL(&varFalse) = VARIANT_FALSE;

        VARIANT varTrue;
        VariantInit(&varTrue);
        V_VT(&varTrue) = VT_BOOL;
        V_BOOL(&varTrue) = VARIANT_TRUE;

        Word::_Document* document = nullptr;

        if (SUCCEEDED(documents->Open(&varFileName, // FileName
                &varFalse, // ConfirmConversions
                &varTrue, // ReadOnly
                &varFalse, // AddToRecentFiles
                &vtMissing, // PasswordDocument
                &vtMissing, // PasswordTemplate
                &vtMissing, // Revert
                &vtMissing, // WritePasswordDocument
                &vtMissing, // WritePasswordTemplate
                &vtMissing, // Format
                &vtMissing, // msoEncodingAutoDetect
                &varFalse, // Visible
                &varFalse, // OpenAndRepair
                &vtMissing, // DocumentDirection
                &vtMissing, // NoEncodingDialog
                &vtMissing, // XMLTransform
                &document))) {
            if (document != nullptr) {
                if (SUCCEEDED(document->ExportAsFixedFormat(QSTR_TO_BSTR(file_pdf_win), Word::wdExportFormatPDF, FALSE,
                        Word::wdExportOptimizeForOnScreen, Word::wdExportAllDocument, 0, 0,
                        Word::wdExportDocumentContent, TRUE, TRUE, Word::wdExportCreateWordBookmarks, TRUE, TRUE,
                        FALSE)))
                    res = true;
                document->Close(&varFalse);
                document->Release();
            }
        }
        documents->Close();
        documents->Release();
    }
    application->Quit();

    return res;
}

bool MSOControl::saveExcelAsPdf(const QString& file_office, const QString& file_pdf)
{
    bool res = false;

    QString file_office_win = QDir::toNativeSeparators(file_office);
    QString file_pdf_win = QDir::toNativeSeparators(file_pdf);

    VARIANT varFalse;
    VariantInit(&varFalse);
    V_VT(&varFalse) = VT_BOOL;
    V_BOOL(&varFalse) = VARIANT_FALSE;

    VARIANT varTrue;
    VariantInit(&varTrue);
    V_VT(&varTrue) = VT_BOOL;
    V_BOOL(&varTrue) = VARIANT_TRUE;

    Excel::_ApplicationPtr application;
    if (FAILED(application.CreateInstance("Excel.Application")))
        return false;

    application->PutVisible(0, FALSE);
    Excel::Workbooks* workbooks = application->Workbooks;
    if (workbooks != nullptr) {
        Excel::_Workbook* workbook = workbooks->Open(QSTR_TO_BSTR(file_office_win));
        if (workbook != nullptr) {
            VARIANT varFileName;
            VariantInit(&varFileName);
            V_VT(&varFileName) = VT_BSTR;
            V_BSTR(&varFileName) = QSTR_TO_BSTR(file_pdf_win);
            if (SUCCEEDED(workbook->ExportAsFixedFormat(Excel::xlTypePDF, varFileName)))
                res = true;

            workbook->Close(varFalse);
            workbook->Release();
        }
        workbooks->Close();
        workbooks->Release();
    }

    application->Quit();

    return res;
}

int MSOControl::getViewType()
{
    int retVal = -1;
    Word::_Document* document = nullptr;
    if (SUCCEEDED(GetMSODocument(&document)) && document) {
        Word::Window* wnd = nullptr;
        if (SUCCEEDED(document->get_ActiveWindow(&wnd)) && wnd) {
            Word::View* view = nullptr;
            if (SUCCEEDED(wnd->get_View(&view)) && view) {
                Word::WdViewType viewType;
                if (SUCCEEDED(view->get_Type(&viewType))) {
                    switch (viewType) {
                        case Word::WdViewType::wdPrintView:
                            retVal = WORDOLE_PRINT_VIEW;
                            break;
                        case Word::WdViewType::wdWebView:
                            retVal = WORDOLE_WEB_VIEW;
                            break;
                        case Word::WdViewType::wdPrintPreview:
                            retVal = WORDOLE_PRINT_PREVIEW;
                            break;
                    }
                }

                RELEASE(view)
            }
            RELEASE(wnd)
        }
        RELEASE(document)
    }

    return retVal;
}

void MSOControl::setLightMSO(bool b)
{
    enableLightMSO = b;
}

void MSOControl::onActivateApp()
{
    if (!pMSO)
        return;

    ODS("%s: state %d thread %d curthread %d\n", __FUNCTION__, wParam, lParam, GetCurrentThreadId());

#if LIGHT_MSO_WINDOW()
    pMSO->OnNotifyAppActivate(true, GetCurrentThreadId());
#endif // LIGHT_MSO_WINDOW()
}

void MSOControl::onFocus(bool b)
{
    if (!pMSO)
        return;

    ODS("%s: ", __FUNCTION__);

    pMSO->OnNotifyControlFocus(b);
}

// LRESULT CALLBACK MSOControl::WndProc(
//    HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/)
//{
//    qDebug() << message;

//    BEGIN_MSG_MAP(hWindow, MSOControl)
//    //    MSG_HANDLER(WM_PAINT, OnPaint)
//    //    MSG_HANDLER(WM_DESTROY, OnDestroy)
//    //    MSG_HANDLER(WM_SIZE, OnSize)
//    MSG_HANDLER(WM_SETFOCUS, wnd_OnFocus)
//    //    MSG_HANDLER(WM_COMMAND, OnCommand)
//    END_MSG_MAP()

//    return DefSubclassProc(hWindow, message, wParam, lParam);
//}

LRESULT MSOControl::wnd_OnFocus(HWND, UINT, WPARAM, LPARAM)
{
    if (!pMSO)
        return FALSE;

    pMSO->OnNotifyControlFocus(TRUE);
    return TRUE;
}

/*
LRESULT MSOControl::OnCommand(HWND , UINT , WPARAM wParam, LPARAM lParam)
{
    if(WM_COMMAND_NOTIFYID(wParam, lParam) == IDM_VIEWCOPYTOCLIPBOARD)
    {
        char* docbody=0;
        int docbodylen=GetObjDocBody(OBJTYPE_DOC,lParam,DOCOBJ_DOCBODY,docbody);

        if(docbodylen > 0)
        {
            const SelectQuary* pSelectQuary=GetObjSelectQuary(OBJTYPE_DOC,lParam,DOCOBJ_DOCADD);
            if(pSelectQuary)
            {
                int filetype=(int)FILETYPE2MAINFILETYPE(pSelectQuary->GetIntField(0,DOCADD_FILETYPE));
                char* cnvmem=0;

                int textlen=ExtractTextFromDocument(filetype,cnvmem,docbodylen,docbody);

                if(textlen > 0)
                    PutStringToClipboard(cnvmem);

                DELETESTR(cnvmem);
            }
        }

        DELETESTR(docbody);
        return TRUE;
    }

    if(WM_COMMAND_NOTIFYID(wParam, lParam) == IDM_DOCPRINT)
    {
        if(pMSO)
        {
            switch( elemType )
            {
            case VIEWFORM_TYPE_MSWORD:
                {
                    Word::_Document * document = 0;
                    if( SUCCEEDED(GetMSODocument(&document)) )
                    {
                        HRESULT hr = document->PrintOut();
                        RELEASE(document);

                        if( SUCCEEDED(hr) )
                            return TRUE;
                    }
                    break;
                }
            case VIEWFORM_TYPE_MSEXCEL:
                {
                    HRESULT hr = S_OK;
                    Excel::_Workbook * workbook = 0;
                    if( SUCCEEDED(hr = GetMSODocument(&workbook)) )
                    {
                        try
                        {
                            hr = workbook->_PrintOut();
                        }
                        catch(...)
                        {
                            hr = E_FAIL;
                        }

                        RELEASE(workbook);

                        if( SUCCEEDED(hr) )
                            return TRUE;
                        else
                            return FALSE;
                    }
                    break;
                }
            default:
                DebugTrap();
                return E_UNEXPECTED;
            }
        }
    }

    if(WM_COMMAND_NOTIFYID(wParam, lParam) == DBM_SAVEIFDIRTY)
    {
        if(pMSO)
        {
            switch( elemType )
            {
                case VIEWFORM_TYPE_MSWORD:
                {
                    BOOL isDirty = pMSO->IsDirty();
                    if(isDirty && IDYES == YesNoBox(hWindow, LS(IDS_16654)))
                        pMSO->Save();
                    else
                        return FALSE;
                }
                break;
            }
        }

        return TRUE;
    }

    return FALSE;
}
*/

HWND MSOControl::GetWindow() const
{
	return hWindow;
}
#if LIGHT_MSO_WINDOW()
HWND MSOControl::GetInternalMSOWnd() const
{
	return hInternalMSOWnd;
}
#endif // LIGHT_MSO_WINDOW()

bool MSOControl::IsReadOnly() const
{
	return bReadOnly;
}

bool MSOControl::IsValid() const
{
    return (pMSO != nullptr);
}

void MSOControl::PrepareToDestroy()
{
    ClearEventContainers();

    if (pMSO) {
        if (pMSO->GetOleObject() != nullptr) {
            pMSO->IPDeactivateView();
            pMSO->Close();
        }
        delete pMSO;
    }
    pMSO = nullptr;
}

template<typename _TDoc>
HRESULT MSOControl::GetMSODocument(_TDoc ** pMSODocument )
{
    HRESULT hr = S_OK;

    IDispatch* pDocObj = pMSO->GetMSODocumentIDispatch();

    if (pDocObj == nullptr) {
        DebugTrap();
        return E_POINTER;
    }

    if (FAILED(hr = pDocObj->QueryInterface(__uuidof(_TDoc), (void**)pMSODocument))) {
        // !!!! Если вдруг это перестало работать, то:
        // берём UUID запрошенного. К примеру для Word::_Document это
        // 0002096b-0000-0000-c000-000000000046. Открываем regedit, подставляем этот uid в путь:
        // HKEY_CLASSES_ROOT\Interface\{0002096B-0000-0000-C000-000000000046}\TypeLib
        // там два значения. Запоминаем Version (к примеру для 2010 офиса это 8.5)
        // и другой uid в поле "по умолчанию". К примеру это {00020905-0000-0000-C000-000000000046}
        // Подставляем это значение в ветку:
        // HKEY_CLASSES_ROOT\TypeLib\{00020905-0000-0000-C000-000000000046}
        // В ней - ветки с версиями. 8.5 (т.е. та версия, которую мы запомнили чуть выше)
        // будет содержать что-то полезное.
        // Но рядом с 8.5 могут быть другие, пустые. Вот все остальные пустые нужно удалить
        // После установки 2016-го офиса, к примеру, появляется 8.7. Простое удаление решает.
        // Перезагрузка требуется
        // Источник: https://stackoverflow.com/questions/12957595/error-accessing-com-components

        // +1
        DebugTrap();
        return hr;
    }

    return hr;
}

HRESULT MSOControl::GetWordApplication(Word::_Application ** pApplication)
{
    if (pMSO == nullptr) {
        DebugTrap(); // object not created yet
        return E_POINTER;
    }

    HRESULT hr = S_OK;

    Word::_Document* pMSODocument = nullptr;
    if (FAILED(hr = GetMSODocument<Word::_Document>(&pMSODocument))) {
        DebugTrap();
        return hr;
    }

    if (FAILED(hr = pMSODocument->get_Application(pApplication))) {
        RELEASE(pMSODocument)
        return hr;
    }

    RELEASE(pMSODocument)

    return hr;
}

HRESULT MSOControl::GetExcelApplication(Excel::_Application ** pApplication)
{
    if (pMSO == nullptr) {
        DebugTrap(); // object not created yet
        return E_POINTER;
    }

    HRESULT hr = S_OK;

    Excel::_Workbook* pMSODocument = nullptr;
    if (FAILED(hr = GetMSODocument<Excel::_Workbook>(&pMSODocument))) {
        DebugTrap();
        return hr;
    }

    if (FAILED(hr = pMSODocument->get_Application(pApplication))) {
        RELEASE(pMSODocument)
        return hr;
    }

    RELEASE(pMSODocument)

    return hr;
}

void MSOControl::ClearEventContainers()
{
    if (pConnectionPt) {
        pConnectionPt->Unadvise(dwCookie);
    }
    dwCookie = 0;

    RELEASE(pConnectionPtContainer)
    RELEASE(pConnectionPt)
    RELEASE(pMSOEventSink)
}

HRESULT MSOControl::ConnectToWordEvents()
{
    HRESULT hr = S_OK;

    ClearEventContainers();

    Word::_Application* pWordApp = nullptr;
    if (FAILED(hr = GetWordApplication(&pWordApp))) {
        DebugTrap();
        return hr;
    }

    IConnectionPointContainer* pConnectionPtContainer = nullptr;
    if (FAILED(hr = pWordApp->QueryInterface(__uuidof(IConnectionPointContainer), (void**)&pConnectionPtContainer))) {
        DebugTrap();
        RELEASE(pWordApp)
        ClearEventContainers();
        return hr;
    }

    if (FAILED(hr = pConnectionPtContainer->FindConnectionPoint(IID_IWordAppEvents2, &pConnectionPt))) {
        DebugTrap();
        RELEASE(pWordApp)
        ClearEventContainers();
        return hr;
    }

    pMSOEventSink = new MSOEventSink;

    pMSOEventSink->SetApplication(pWordApp);

    Word::_Document* thisDocument = nullptr;
    if (FAILED(hr = GetMSODocument(&thisDocument))) {
        DebugTrap();
        RELEASE(pWordApp)
        ClearEventContainers();
        return hr;
    }
    pMSOEventSink->SetWordThisDocument(thisDocument);
    pMSOEventSink->SetMSOType(MSO_WORD);

    if (FAILED(hr = pConnectionPt->Advise(pMSOEventSink, &dwCookie))) {
        DebugTrap();
        RELEASE(pWordApp)
        ClearEventContainers();
        return hr;
    }

    RELEASE(pWordApp)

    return hr;
}

HRESULT MSOControl::ConnectToExcelEvents()
{
    HRESULT hr = S_OK;

    ClearEventContainers();

    Excel::_Application* pExcelApp = nullptr;
    if (FAILED(hr = GetExcelApplication(&pExcelApp))) {
        DebugTrap();
        return hr;
    }

    IConnectionPointContainer* pConnectionPtContainer = nullptr;
    if (FAILED(hr = pExcelApp->QueryInterface(__uuidof(IConnectionPointContainer), (void**)&pConnectionPtContainer))) {
        DebugTrap();
        RELEASE(pExcelApp)
        ClearEventContainers();
        return hr;
    }

    if (FAILED(hr = pConnectionPtContainer->FindConnectionPoint(IID_IExcelAppEvents2, &pConnectionPt))) {
        DebugTrap();
        RELEASE(pExcelApp)
        ClearEventContainers();
        return hr;
    }

    pMSOEventSink = new MSOEventSink;

    pMSOEventSink->SetApplication(pExcelApp);
    Excel::_Workbook* thisDocument = nullptr;
    if (FAILED(hr = GetMSODocument(&thisDocument))) {
        DebugTrap();
        RELEASE(pExcelApp)
        ClearEventContainers();
        return hr;
    }
    pMSOEventSink->SetExcelThisDocument(thisDocument);
    pMSOEventSink->SetMSOType(MSO_EXCEL);

    if (FAILED(hr = pConnectionPt->Advise(pMSOEventSink, &dwCookie))) {
        DebugTrap();
        RELEASE(pExcelApp)
        ClearEventContainers();
        return hr;
    }

    RELEASE(pExcelApp)

    return hr;
}

HRESULT MSOControl::SwitchReadOnly_Word()
{
    HRESULT hr = S_OK;

    if (pMSO == nullptr)
        return E_POINTER;

    Word::_Document* pMSODocument = 0;
    if (FAILED(hr = GetMSODocument<Word::_Document>(&pMSODocument))) {
        DebugTrap();
        return hr;
    }

    // protect document
    VARIANT varPasswd;
    VariantInit(&varPasswd);
    varPasswd.vt = VT_BSTR;
    GUIDGenWithStrictResult(&varPasswd.bstrVal);

    if (IsReadOnly()) { // protect document
        VARIANT varTrue;
        VariantInit(&varTrue);
        varTrue.vt = VT_BOOL;
        varTrue.boolVal = VARIANT_TRUE;

        VARIANT varFalse;
        VariantInit(&varFalse);
        varFalse.vt = VT_BOOL;
        varFalse.boolVal = VARIANT_FALSE;

        hr = pMSODocument->Protect(Word::wdAllowOnlyReading, &varFalse, &varPasswd, &vtMissing, &varTrue);

        VariantClear(&varTrue);
        VariantClear(&varFalse);

    } // protect document
    else
        pMSODocument->Unprotect(&varPasswd);

    VariantClear(&varPasswd);

    RELEASE(pMSODocument)

    return hr;
}

HRESULT MSOControl::HighlightWords(const char * /*strWords*/)
{
#if 0 
	if( IsEmpty(strWords) )
		return S_FALSE;

	char * wordsCollection = STRDUP(strWords);

	char wordsSep[] = {' ', '\0'};

	ArrayT<BSTR> wordsAr(2);
	char * ctx = 0;
	for( char * s = strtok_s(wordsCollection, wordsSep, &ctx), * next = 0; s; s = next )
	{
		next = strtok_s(0, wordsSep, &ctx);
		wordsAr.Add(Char2SysStrAllocated(s));
	}

	size_t len = wordsAr.size();

	HRESULT hr = S_OK;

	Word::_Application * application = 0;
	if( FAILED(hr = GetWordApplication(&application)) )
	{
		DebugTrap();
		return hr;
	}

	Word::_Document * document = 0;
	if( FAILED(GetMSODocument(&document)) || !document )
		return S_FALSE;

	Word::Words * words = 0;
	if( FAILED(document->get_Words(&words)) || !words )
	{
		RELEASE(document);
		RELEASE(application);
		DebugTrap();
		return S_FALSE;
	}

	long wordsCount = 0;
	words->get_Count(&wordsCount);

	for(long i = 0; i < wordsCount; i++)
	{
		Word::Range * range = 0;
		if( FAILED(words->Item(i, &range)) || !range )
			continue;

		BSTR bstrText = 0;
		range->get_Text(&bstrText);

		if( !bstrText )continue;

		bool found = false;

		for(size_t j = 0; j < len; ++j)
			if( wcsstr(bstrText, wordsAr[j]) )
			{
				found = true;
				break;
			}

		if(found)
			range->put_HighlightColorIndex(Word::WdColorIndex::wdBrightGreen);

		SysFreeString(bstrText);
		
		RELEASE(range);
	}

	RELEASE(application);

	for(size_t j = 0; j < len; ++j)
		SysFreeString(wordsAr[j]);

	return hr;
#else
	return E_NOTIMPL;
#endif // 0
}

#if LIGHT_MSO_WINDOW()
HRESULT MSOControl::_SwitchToLightWord2003()
{
    HRESULT hr = S_OK;

    if (elemType != VIEWFORM_TYPE_MSWORD) {
        DebugTrap();
        return E_UNEXPECTED;
    }

    const char* concrete_WWU = "_WwU";
    const char* concrete_WWV = "_WwV";
    const char* concrete_WWB = "_WwB";

    HWND hDocWnd = pMSO->GetDocWindow();
    if (!IsWindow(hDocWnd)) {
        _LOGERR("%s: DOS framer window not found", __FUNCTION__);
        DebugTrap();
        return E_UNEXPECTED;
    }

    _LOGERR("%s: found DSO framer window", __FUNCTION__);

    HWND wwu = FindChildWndWithClassName(hDocWnd, concrete_WWU);
    if (!IsWindow(wwu)) {
        _LOGERR("%s: %s MSO window not found under DSO framer", __FUNCTION__, concrete_WWU);
        DebugTrap();
        return E_UNEXPECTED;
    }

    _LOGERR("%s: found %s MSO window", __FUNCTION__, concrete_WWU);

    HWND wwv = FindChildWndWithClassName(wwu, concrete_WWV);
    if (!IsWindow(wwv)) {
        _LOGERR("%s: %s MSO window not found under %s", __FUNCTION__, concrete_WWV, concrete_WWU);
        DebugTrap();
    }

    _LOGERR("%s: found %s MSO window", __FUNCTION__, concrete_WWV);
    HWND wwb = FindChildWndWithClassName(wwv, concrete_WWB);
    if (!IsWindow(wwb)) {
        _LOGERR("%s: %s MSO window not found under %s", __FUNCTION__, concrete_WWB, concrete_WWV);
        DebugTrap();
    }

    _LOGERR("%s: found %s MSO window", __FUNCTION__, concrete_WWB);

    ShowWindow(wwu, SW_HIDE); // temporary hide
    MoveWindow(wwu, 0, 0, 0, 0, TRUE);

    hInternalMSOWnd = wwb;

    LONG lStyle = GetWindowLong(wwb, GWL_STYLE);
    lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
    SetWindowLong(hInternalMSOWnd, GWL_STYLE, lStyle);

    SetParent(hInternalMSOWnd, hDocWnd);

    RECT rect;
    GetClientRect(hDocWnd, &rect);
    MoveWindow(hInternalMSOWnd, 0, 0, rect.right, rect.bottom, TRUE);

    DRef<Word::_Application> application;
    if (SUCCEEDED(hr = GetWordApplication(application.GetPtrPtr()))) {
        DRef<Word::Window> window;
        if (SUCCEEDED(hr = application->get_ActiveWindow(window.GetPtrPtr()))) {
            window->put_DisplayRulers(VARIANT_FALSE);

            DRef<Word::View> view;
            if (SUCCEEDED(hr = window->get_View(view.GetPtrPtr()))) {
                DRef<Word::Zoom> zoom;
                if (SUCCEEDED(hr = view->get_Zoom(zoom.GetPtrPtr()))) {
                    zoom->put_PageFit(Word::wdPageFitBestFit);
                    if (IsWindow(GetInternalMSOWnd())) {
                        InvalidateRect(GetInternalMSOWnd(), nullptr, TRUE);
                        UpdateWindow(GetInternalMSOWnd());
                    }
                } else {
                    _LOGERR("%s: failed to get active doicument's window's view zoom object, hr code 0x%08X",
                        __FUNCTION__, hr);
                    DebugTrap();
                }
            } else {
                _LOGERR("%s: failed to get active doicument's window's view object, hr code 0x%08X", __FUNCTION__, hr);
                DebugTrap();
            }
        } else {
            _LOGERR("%s: failed to get active doicument's window object, hr code 0x%08X", __FUNCTION__, hr);
            DebugTrap();
        }
    } else {
        _LOGERR("%s: failed to get Microsoft Word application object, hr code 0x%08X", __FUNCTION__, hr);
        DebugTrap();
    }

    ShowWindow(wwu, SW_SHOW);

    return S_OK;
}

HRESULT MSOControl::_SwitchToLightWord71013()
{
	_LOGINFO(__FUNCTION__" Enter");
	HRESULT hr = S_OK;

	if( elemType != VIEWFORM_TYPE_MSWORD )
	{
		DebugTrap();
		return E_UNEXPECTED;
	}

	const char * concrete_WWT = "_WwT";
	const char * concrete_WWU = "_WwU";
	const char * concrete_WWB = "_WwB";

	HWND hDocWnd = pMSO->GetDocWindow();
	if( !IsWindow(hDocWnd) )
	{
		_LOGERR("%s: DOS framer window not found", __FUNCTION__);
		DebugTrap();
		return E_UNEXPECTED;
	}

	_LOGERR("%s: found DSO framer window", __FUNCTION__);

	HWND wwt = FindChildWndWithClassName(hDocWnd, concrete_WWT);
	if( !IsWindow(wwt) )
	{
		_LOGERR("%s: %s MSO window not found under DSO framer", __FUNCTION__, concrete_WWT);
		DebugTrap();
		return E_UNEXPECTED;
	}

	_LOGERR("%s: found %s MSO window", __FUNCTION__, concrete_WWT);

	HWND wwu = FindChildWndWithClassName(wwt, concrete_WWU);
	if( !IsWindow(wwu) )
	{
		_LOGERR("%s: %s MSO window not found under %s", __FUNCTION__, concrete_WWU, concrete_WWT);
		DebugTrap();
	}

	_LOGERR("%s: found %s MSO window", __FUNCTION__, concrete_WWU);
	HWND wwb = FindChildWndWithClassName(wwu, concrete_WWB);
	if( !IsWindow(wwb) )
	{
		_LOGERR("%s: %s MSO window not found under %s", __FUNCTION__, concrete_WWB, concrete_WWU);
		//DebugTrap();
		return S_FALSE;
	}

	_LOGERR("%s: found %s MSO window", __FUNCTION__, concrete_WWB);

	ShowWindow(wwt, SW_HIDE); // temporary hide
	MoveWindow(wwt, 0, 0, 0, 0, TRUE);

	hInternalMSOWnd = wwb;

	LONG lStyle = GetWindowLong(wwb, GWL_STYLE);
	lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
	SetWindowLong(hInternalMSOWnd, GWL_STYLE, lStyle);

	SetParent(hInternalMSOWnd, hDocWnd);

	RECT rect;
	GetClientRect(hDocWnd, &rect);
	MoveWindow(hInternalMSOWnd, 0, 0, rect.right, rect.bottom, TRUE);

	DRef<Word::_Application> application;
	if( SUCCEEDED(hr = GetWordApplication(application.GetPtrPtr())) )
	{
		DRef<Word::Window> window;
		if( SUCCEEDED(hr = application->get_ActiveWindow(window.GetPtrPtr())) )
		{
			window->put_DisplayRulers(VARIANT_FALSE);
			
			DRef<Word::View> view;
			if( SUCCEEDED(hr = window->get_View(view.GetPtrPtr())) )
			{
				DRef<Word::Zoom> zoom;
				if( SUCCEEDED(hr = view->get_Zoom(zoom.GetPtrPtr())) )
				{
					zoom->put_PageFit(Word::wdPageFitBestFit);
					if( IsWindow(GetInternalMSOWnd()) )
					{
						InvalidateRect(GetInternalMSOWnd(), nullptr, TRUE);
						UpdateWindow(GetInternalMSOWnd());
					}
				}
				else
				{
					_LOGERR("%s: failed to get active doicument's window's view zoom object, hr code 0x%08X", __FUNCTION__, hr);
					DebugTrap();
				}
			}
			else
			{
				_LOGERR("%s: failed to get active document's window's view object, hr code 0x%08X", __FUNCTION__, hr);
				DebugTrap();
			}
		}
		else
		{
			_LOGERR("%s: failed to get active doicument's window object, hr code 0x%08X", __FUNCTION__, hr);
			DebugTrap();
		}
	}
	else
	{
		_LOGERR("%s: failed to get Microsoft Word application object, hr code 0x%08X", __FUNCTION__, hr);
		DebugTrap();
	}

	ShowWindow(wwt, SW_SHOW);

	return S_OK;
}

HRESULT MSOControl::SwitchToLightWord()
{
	if( elemType != VIEWFORM_TYPE_MSWORD )
	{
		DebugTrap();
		return E_UNEXPECTED;
	}

	int version = GetWordVersion();
	switch(version)
	{
		case 2007:
		case 2010:
		case 2013:
		case 2016:
			return _SwitchToLightWord71013();
		case 2003:
			return _SwitchToLightWord2003();
		default:
			{
				_LOGERR("%s: unsupported Microsoft Word version (%d) to switch to light mode", __FUNCTION__, version);
				DebugTrap();
				return S_FALSE;
			}
	}
}

HRESULT MSOControl::_SwitchToLightExcel2003()
{
	return S_OK;
}

HRESULT MSOControl::_SwitchToLightExcel71013()
{
	if( elemType != VIEWFORM_TYPE_MSEXCEL )
	{
		DebugTrap();
		return E_UNEXPECTED;
	}

	const char * concrete_EXCEL2 = "EXCEL2"; // ribbon
	const char * concrete_EXCEL9 = "EXCEL9";
	const char * concrete_EXCEL7 = "EXCEL7";
	
	HWND hDocWnd = pMSO->GetDocWindow();
	if( !IsWindow(hDocWnd) )
	{
		_LOGERR("%s: DOS framer window not found", __FUNCTION__);
		DebugTrap();
		return E_UNEXPECTED;
	}

	_LOGERR("%s: found DSO framer window", __FUNCTION__);

	HWND excel2Wnd = FindChildWndWithClassName(hDocWnd, concrete_EXCEL2);
	if( !IsWindow(excel2Wnd) )
	{
		_LOGERR("%s: %s MSO window not found under DSO framer", __FUNCTION__, concrete_EXCEL2);
		DebugTrap();
		return E_UNEXPECTED;
	}

	_LOGERR("%s: found ribbon %s MSO window", __FUNCTION__, concrete_EXCEL2);
	MoveWindow(excel2Wnd, 0, 0, 0, 0, TRUE);

	HWND excel9Wnd = FindChildWndWithClassName(hDocWnd, concrete_EXCEL9);
	if( !IsWindow(excel9Wnd) )
	{
		_LOGERR("%s: %s MSO window not found under DSO framer", __FUNCTION__, concrete_EXCEL9);
		DebugTrap();
		return E_UNEXPECTED;
	}

	_LOGERR("%s: found %s MSO window", __FUNCTION__, concrete_EXCEL9);

	HWND excel7wnd = FindChildWndWithClassName(excel9Wnd, concrete_EXCEL7);
	if( !IsWindow(excel7wnd) )
	{
		_LOGERR("%s: %s MSO window not found under %s", __FUNCTION__, concrete_EXCEL7, concrete_EXCEL9);
		DebugTrap();
	}

	_LOGERR("%s: found %s MSO window", __FUNCTION__, concrete_EXCEL7);

	ShowWindow(excel7wnd, SW_HIDE); // temporary hide
	MoveWindow(excel7wnd, 0, 0, 0, 0, TRUE);

	hInternalMSOWnd = excel7wnd;

	LONG lStyle = GetWindowLong(hInternalMSOWnd, GWL_STYLE);
	lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
	SetWindowLong(hInternalMSOWnd, GWL_STYLE, lStyle);

	SetParent(hInternalMSOWnd, hDocWnd);

	RECT rect;
	GetClientRect(hDocWnd, &rect);
	MoveWindow(hInternalMSOWnd, 0, 0, rect.right, rect.bottom, TRUE);

	ShowWindow(excel7wnd, SW_SHOW);

	return S_OK;
}

HRESULT MSOControl::SwitchToLightExcel()
{
	return S_FALSE;

	// IV: Excel workbook window cannot be safely detached from it's parent
#if 0
	if( elemType != VIEWFORM_TYPE_MSEXCEL )
	{
		DebugTrap();
		return E_UNEXPECTED;
	}


	int version = GetExcelVersion();
	switch(version)
	{
		case 2007:
		case 2010:
		case 2013:
		case 2016:
			return _SwitchToLightExcel71013();
		case 2003:
			return _SwitchToLightExcel2003();
		default:
			{
				_LOGERR("%s: unsupported Microsoft Excel version (%d) to switch to light mode", __FUNCTION__, version);
				DebugTrap();
				return S_FALSE;
			}
	}
#endif // 0
}
#endif // LIGHT_MSO_WINDOW()

HRESULT MSOControl::SwitchReadOnly_Excel()
{
	HRESULT hr = S_OK;

	Excel::_Workbook * pMSODocument = 0;

	if( FAILED(hr = GetMSODocument<Excel::_Workbook>(&pMSODocument)) )
	{
		DebugTrap();
		return hr;
	}

	VARIANT varPasswd;
	VariantInit(&varPasswd);
	varPasswd.vt = VT_BSTR;
	GUIDGenWithStrictResult(&varPasswd.bstrVal);

	Excel::Sheets * sheetCollection = 0;
	if( FAILED(hr = pMSODocument->get_Sheets(&sheetCollection)) )
	{
		DebugTrap();
		VariantClear(&varPasswd);
		return hr;
	}

	long count = 0;

	if( FAILED(hr = sheetCollection->get_Count(&count)) )
	{
		DebugTrap();
		RELEASE(sheetCollection);
		VariantClear(&varPasswd);
		return hr;
	}

	VARIANT varIndex, varTrue;
	VariantInit(&varIndex);
	varIndex.vt = VT_I4;

	VariantInit(&varTrue);
	varTrue.vt = VT_BOOL;
	varTrue.boolVal = VARIANT_TRUE;

	for(long i = 1; i <= count; ++i )
	{ // protect each sheet
		varIndex.intVal = i;

		IDispatch * dispSheet = 0;
		if( FAILED(hr = sheetCollection->get_Item(varIndex, &dispSheet)) || !dispSheet )
		{
			DebugTrap();
			RELEASE(sheetCollection);
			VariantClear(&varPasswd);
			VariantClear(&varTrue);
			if( hr == S_OK )
				hr = E_POINTER;
			return hr;
		}
		
		Excel::_Worksheet * sheet = 0;

		if( FAILED(hr = dispSheet->QueryInterface(__uuidof(Excel::_Worksheet), (void**)&sheet)) )
		{
			DebugTrap();
			RELEASE(dispSheet);
			RELEASE(sheetCollection);
			VariantClear(&varPasswd);
			VariantClear(&varTrue);
			return hr;
		}
		RELEASE(dispSheet);

		if( IsReadOnly() )
		{ // protect
			if( FAILED(hr = sheet->Protect(
				varPasswd,
				vtMissing,
				vtMissing,
				vtMissing,
				varTrue)) )
			{
				DebugTrap();
				RELEASE(sheet);
				RELEASE(sheetCollection);
				VariantClear(&varPasswd);
				VariantClear(&varTrue);
				return hr;
			}
			sheet->put_EnableSelection(Excel::xlNoSelection);
		} // protect
		else
		{ // unprotect
			if( FAILED(hr = sheet->Unprotect(varPasswd)) )
			{
				DebugTrap();
				RELEASE(sheet);
				RELEASE(sheetCollection);
				VariantClear(&varPasswd);
				VariantClear(&varTrue);
				return hr;
			}
			sheet->put_EnableSelection(Excel::xlNoRestrictions);
		} // unprotect

		RELEASE(sheet);
	} // protect each sheet
	VariantClear(&varIndex);
	VariantClear(&varTrue);

	try
	{
		if( IsReadOnly() )
		{
			hr = pMSODocument->_Protect(varPasswd);
		}
		else
			hr = pMSODocument->Unprotect(varPasswd);
	}
	catch(_com_error &)
	{
		// IV: possible exception during inplace opening and editng from Open file dialog, safe to bypass
	}
	
	VariantClear(&varPasswd);

	pMSODocument->Release();

	return hr;
}

HANDLE MSOControl::GetAppProcessHandle()
{
	switch( elemType )
	{
	case VIEWFORM_TYPE_MSWORD:
		{
			//Word::_A
		}
	case VIEWFORM_TYPE_MSEXCEL:
		{
		}
	default:
		DebugTrap();
		return 0;
	}
}

HRESULT MSOControl::DoSwitchToReadOnly()
{
	switch( elemType )
	{
		case VIEWFORM_TYPE_MSWORD:
			return SwitchReadOnly_Word();
		case VIEWFORM_TYPE_MSEXCEL:
			return SwitchReadOnly_Excel();
		default:
			DebugTrap();
			return E_UNEXPECTED;
	}
}

/*
HRESULT MSOControl::DoConnectToEvents()
{
    bool mayExportDoc = 0!=(IsHomeConnect() ? MayCopyToClipboardHome() : MayCopyToClipboard());

    if( mayExportDoc )
        return S_OK; // don't disable copying

    switch( elemType )
    {
    case VIEWFORM_TYPE_MSWORD:
        return ConnectToWordEvents();
    case VIEWFORM_TYPE_MSEXCEL:
        return ConnectToExcelEvents();
    default:
        DebugTrap();
        return E_UNEXPECTED;
    }
}
*/

//-------------------------------------------------------------------------------------------------
struct MSOConverterSite : public IMSOObjectSite, public RefCounter
{
	MSOConverterSite(){}
	virtual ~MSOConverterSite(){}

	//  IUnknown
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
		__RPC__deref_out void __RPC_FAR *__RPC_FAR *ppvObject);
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	// IServiceProvider
	STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppvObject);

	// IMSOObjectSite
	STDMETHOD(GetWindow)(HWND* phWnd);
	STDMETHOD(GetBorder)(LPRECT prcBorder);
	STDMETHOD(SetStatusText)(LPCOLESTR pszText);
	STDMETHOD(GetHostName)(LPWSTR *ppwszHostName);
	STDMETHOD(SysMenuCommand)(UINT uiCharCode);
};
//////////////////////////////////////////////////////////////////////////
//
// IServiceProvider
//
STDMETHODIMP MSOConverterSite::QueryService(REFGUID /*guidService*/, REFIID /*riid*/, void ** /*ppvObject*/)
{
	return E_NOTIMPL;
}

//////////////////////////////////////////////////////////////////////////
//
// IUnknown
//

HRESULT MSOConverterSite::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = nullptr;

    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = reinterpret_cast<void**>(this);

    if (IsEqualGUID(riid, IID_IMSOObjectSite))
        *ppvObject = reinterpret_cast<void**>(this);

    if (*ppvObject) {
        ((IUnknown*)*ppvObject)->AddRef();
        return S_OK;
    } else
        return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE MSOConverterSite::AddRef()
{
	return RefCounter::AddRef();
}

ULONG STDMETHODCALLTYPE MSOConverterSite::Release()
{
	return RefCounter::Release();
}

//////////////////////////////////////////////////////////////////////////
//
// IMSOObjectSite
//

STDMETHODIMP MSOConverterSite::GetWindow(HWND* phWnd)
{
    if (QApplication::topLevelWidgets().isEmpty())
        *phWnd = nullptr;
    else
        *phWnd = reinterpret_cast<HWND>(QApplication::topLevelWidgets().first()->winId());

    return S_OK;
}

STDMETHODIMP MSOConverterSite::GetBorder(LPRECT prcBorder)
{
	prcBorder->left = 0;
	prcBorder->right = 0;
	prcBorder->top = 0;
	prcBorder->bottom = 0;

	return S_OK;
}

STDMETHODIMP MSOConverterSite::SetStatusText(LPCOLESTR /*pszText*/)
{
	return S_OK;
}

STDMETHODIMP MSOConverterSite::GetHostName(LPWSTR * /*ppwszHostName*/)
{
	return E_NOTIMPL;
}

STDMETHODIMP MSOConverterSite::SysMenuCommand(UINT /*uiCharCode*/)
{
	return E_NOTIMPL;
}


HRESULT ConvertWordDocument( const char * strInFile, const char * strOutFile )
{
    /*
    HRESULT hr = S_OK;

    WCHAR msoObjectType[128];
    wcscpy(msoObjectType, MSO_WORD_PROGID);

    CLSID msoCLSID;
    if( FAILED(hr = CLSIDFromProgID(msoObjectType, &msoCLSID)) )
    {
        DebugTrap();
        return hr;
    }

    IDispatch * dispWordDoc = 0;
    if( FAILED(hr = CoCreateInstance(msoCLSID, 0, CLSCTX_LOCAL_SERVER, IID_IDispatch, (void **)&dispWordDoc)) )
        return hr;

    Word::_Document * wordDocument = 0;
    if( FAILED(hr = dispWordDoc->QueryInterface(__uuidof(Word::_Document), (void**)&wordDocument)) )
    {
        RELEASE(dispWordDoc);
        return hr;
    }

    Word::_Application * wordApp = 0;
    if( FAILED(hr = wordDocument->get_Application(&wordApp)) )
    {
        RELEASE(dispWordDoc);
        RELEASE(wordDocument);
        return hr;
    }

    Word::Documents * documents = 0;
    if( FAILED(hr = wordApp->get_Documents(&documents)) )
    {
        RELEASE(wordApp);
        RELEASE(dispWordDoc);
        RELEASE(wordDocument);
        return hr;
    }

    VARIANT varInFileName;
    VariantInit(&varInFileName);
    varInFileName.bstrVal = Char2SysStrAllocated(strInFile);
    varInFileName.vt = VT_BSTR;
    VARIANT varTrue;
    VariantInit(&varTrue);
    varTrue.boolVal = VARIANT_TRUE;
    varTrue.vt = VT_BOOL;

    Word::_Document * activeDocument = 0;
    if( FAILED(hr = documents->Open(&varInFileName,
        &vtMissing, &vtMissing, &vtMissing, &vtMissing, &vtMissing, &vtMissing, &vtMissing,
        &vtMissing, &vtMissing, &vtMissing, &vtMissing, &vtMissing, &vtMissing, &varTrue, &vtMissing, &activeDocument))
    )
    {
        VariantClear(&varInFileName);
        RELEASE(documents);
        RELEASE(wordApp);
        RELEASE(dispWordDoc);
        RELEASE(wordDocument);
        return hr;
    }

    VARIANT varOutFileName;
    VariantInit(&varOutFileName);
    varOutFileName.bstrVal = Char2SysStrAllocated(strOutFile);
    varOutFileName.vt = VT_BSTR;

    VARIANT varFileFormat;
    VariantInit(&varFileFormat);
    varFileFormat.intVal = Word::wdFormatText;
    varFileFormat.vt = VT_I4;

    hr = activeDocument->SaveAs(&varOutFileName, &varFileFormat );

    VariantClear(&varInFileName);
    VariantClear(&varOutFileName);
    VariantClear(&varFileFormat);


    RELEASE(documents);
    RELEASE(dispWordDoc);
    RELEASE(wordDocument);
    RELEASE(wordApp);

    return hr;

    */

    HRESULT hr = S_OK;

    WCHAR wInFileName[_MAX_PATH];
    mbstowcs(wInFileName, strInFile, _MAX_PATH);

    MSOConverterSite* pMSOSite = new MSOConverterSite;
    MSOObject* pMSO = MSOObject::CreateInstance(pMSOSite);

    WCHAR msoObjectType[128];
    wcscpy(msoObjectType, MSO_WORD_PROGID);

    CLSID msoCLSID;
    if (FAILED(hr = CLSIDFromProgID(msoObjectType, &msoCLSID))) {
        DebugTrap();
        RELEASE(pMSO)
        delete pMSOSite;
        return hr;
    }

    BIND_OPTS opts = {};
    opts.cbStruct = sizeof(opts);
    opts.grfMode = STGM_READ | STGM_SHARE_EXCLUSIVE;
    opts.grfFlags = BIND_MAYBOTHERUSER;
    opts.dwTickCountDeadline = 20000;

    if (pMSO->GetOleObject() != 0)
        pMSO->Close();

    hr = pMSO->CreateFromFile(wInFileName, msoCLSID, &opts);
    if (FAILED(hr)) {
        DebugTrap();
        RELEASE(pMSO);
        delete pMSOSite;
        return hr;
    }

    if (FAILED(hr = pMSO->IPActivateView())) {
        DebugTrap();
        RELEASE(pMSO);
        delete pMSOSite;
        return hr;
    }

    IDispatch* pDocObj = pMSO->GetMSODocumentIDispatch();

    if (pDocObj == 0) {
        DebugTrap();
        RELEASE(pMSO);
        delete pMSOSite;
        return E_POINTER;
    }

    Word::_Document* pMSODocument = 0;

    if (FAILED(hr = pDocObj->QueryInterface(__uuidof(Word::_Document), (void**)&pMSODocument))) {
        DebugTrap();
        RELEASE(pDocObj);
        pMSO->Close();
        RELEASE(pMSO);
        delete pMSOSite;
        return hr;
    }

    pMSODocument->put_ShowGrammaticalErrors(VARIANT_FALSE);
    pMSODocument->put_ShowSpellingErrors(VARIANT_FALSE);
    pMSODocument->put_ShowRevisions(VARIANT_FALSE);

    Word::_Application* pApplication = 0;
    if (FAILED(hr = pMSODocument->get_Application(&pApplication))) {
        DebugTrap();
        RELEASE(pDocObj);
        pMSO->Close();
        RELEASE(pMSO);
        delete pMSOSite;
        return hr;
    }

    pApplication->put_DisplayAlerts(Word::wdAlertsNone);

    VARIANT varOutFileName;
    VariantInit(&varOutFileName);
    varOutFileName.bstrVal = Char2SysStrAllocated(strOutFile);
    varOutFileName.vt = VT_BSTR;

    VARIANT varFileFormat;
    VariantInit(&varFileFormat);
    varFileFormat.intVal = Word::wdFormatText;
    varFileFormat.vt = VT_I4;

    hr = pMSODocument->SaveAs(&varOutFileName, &varFileFormat);

    if (hr != S_OK)
        DebugTrap();
    VariantClear(&varOutFileName);
    VariantClear(&varFileFormat);

    RELEASE(pApplication);
    RELEASE(pMSODocument);
    RELEASE(pDocObj);

    pMSO->Close();
    RELEASE(pMSO);
    delete pMSOSite;

    return hr;
    //
}

///////////////////////////////////////////////////////////////////////////////
// ExcelAutomation
//

ExcelAutomation::ExcelAutomation():
activeSheet(0),
application(0),
workbook(0)
{
}

ExcelAutomation::~ExcelAutomation()
{
	if(application)
		UnFreeze();
}

void ExcelAutomation::ShutdownExcelThread()
{
	if(!application)
	{
		DebugTrap();
		return;
	}

	// IV: Due Microsoft Excel leaks interfaces inside itself the only way to shutdownh process
	// we create is find window of integrated application and terminate associated process.

	long excelHwnd = 0;
	application->get_Hwnd(&excelHwnd);

	if( excelHwnd )
	{
		DWORD processId = (DWORD)-1;
        GetWindowThreadProcessId((HWND)(LONG_PTR)(excelHwnd), &processId);
        HANDLE excelProcess = 0;
        if( processId && (excelProcess = OpenProcess(PROCESS_TERMINATE, 0, processId)) !=0 )
		{
			TerminateProcess(excelProcess, 0);
			CloseHandle(excelProcess);
		}
		else
			DebugTrap();
	}
	else
		DebugTrap();
}

void ExcelAutomation::Freeze()
{
	application->put_DisplayAlerts(0, VARIANT_FALSE);
	application->put_ScreenUpdating(0, VARIANT_FALSE);
	application->put_EnableEvents(VARIANT_FALSE);
}

void ExcelAutomation::UnFreeze()
{
	application->put_DisplayAlerts(0, VARIANT_TRUE);
	application->put_ScreenUpdating(0, VARIANT_TRUE);
	application->put_EnableEvents(VARIANT_TRUE);
}

Excel::_ApplicationPtr ExcelAutomation::GetApplication()
{
    assert(application != 0);
    return application;
}

Excel::_WorksheetPtr ExcelAutomation::GetWorkSheet()
{
    assert(activeSheet != 0);
    return activeSheet;
}

Excel::_WorkbookPtr ExcelAutomation::GetWorkbook()
{
    assert(workbook != 0);
    return workbook;
}

Excel::RangePtr ExcelAutomation::GetCells()
{
    //	GetWorkSheet()->Rows;
    Excel::RangePtr cells = GetWorkSheet()->Cells;
	if( !cells )
	{
		DebugTrap();
		return Excel::RangePtr(0);
	}

	return cells;
}

Excel::RangePtr ExcelAutomation::GetRows()
{
	Excel::RangePtr rows = GetWorkSheet()->Rows;
	if( !rows )
	{
		DebugTrap();
		return Excel::RangePtr(0);
	}

	return rows;
}

HRESULT ExcelAutomation::CreateExcelApplication()
{
	HRESULT hr = S_OK;

	try
	{ // try
		if( FAILED(hr = application.CreateInstance("Excel.Application")) )
		{
			DebugTrap();
			return E_FAIL;
		}

		Excel::WorkbooksPtr workbooks = application->GetWorkbooks();
		if( !workbooks )
		{
			DebugTrap();
			return E_POINTER;
		}

		VARIANT varType;
		VariantInit(&varType);
		varType.vt = VT_INT;
		varType.intVal = Excel::xlWorksheet;
		workbook = workbooks->Add(varType);
		if( !workbook )
		{
			DebugTrap();
			return E_POINTER;
		}

		activeSheet = workbook->GetActiveSheet();

		if( !activeSheet )
		{
			DebugTrap();
			return E_POINTER;
		}
	} // try
	catch(...)
	{
		DebugTrap();
		return E_FAIL;
	}

	return S_OK;
}

HRESULT ExcelAutomation::InitDefaultPrintSetup()
{
	Excel::PageSetupPtr pageSetup = GetWorkSheet()->GetPageSetup();
	if( pageSetup )
	{ // have page setup object
		pageSetup->PutOrientation(Excel::xlLandscape);
		pageSetup->PutLeftMargin(1.2);
		pageSetup->PutRightMargin(1.2);
		pageSetup->PutTopMargin(1.2);
		pageSetup->PutBottomMargin(1.2);

		return S_OK;
	} // have page setup object
	else
		return E_FAIL;
}

HRESULT ExcelAutomation::FitSheetToOnePage()
{
	Excel::PageSetupPtr pageSetup = GetWorkSheet()->GetPageSetup();
	if( pageSetup )
	{ // have page setup object
		VARIANT varFit;
		VariantInit(&varFit);
		varFit.vt = VT_INT;
		varFit.intVal = 1;
		
		VARIANT varZoom;
		VariantInit(&varZoom);
		varZoom.vt = VT_BOOL;
		varZoom.boolVal = VARIANT_FALSE;
		pageSetup->PutZoom(varZoom);
		pageSetup->PutFitToPagesTall(varFit);
		pageSetup->PutFitToPagesWide(varFit);

		return S_OK;
	} // have page setup object
	else
		return E_FAIL;
}

HRESULT ExcelAutomation::_SwitchPageOrientation(Excel::XlPageOrientation value)
{
	Excel::PageSetupPtr pageSetup = GetWorkSheet()->GetPageSetup();
	if( pageSetup )
	{ // have page setup object
		pageSetup->PutOrientation(value);

		return S_OK;
	} // have page setup object
	else
		return E_FAIL;
}

HRESULT ExcelAutomation::SwitchPageToLandscape()
{
	return _SwitchPageOrientation(Excel::xlLandscape);
}

HRESULT ExcelAutomation::SwitchPageToPortrait()
{
	return _SwitchPageOrientation(Excel::xlPortrait);
}

HRESULT ExcelAutomation::InitDefaultWindow()
{
	Excel::WindowPtr activeWindow = GetApplication()->GetActiveWindow();
	if( !activeWindow )
	{ // maximize window
		DebugTrap();
		return E_UNEXPECTED;
	} // maximize window
	activeWindow->PutWindowState(Excel::xlMaximized);
	VARIANT varZoom;
	VariantInit(&varZoom);
	varZoom.vt = VT_INT;
	varZoom.intVal = 100;
	activeWindow->PutZoom(varZoom);

	return S_OK;
}

double ExcelAutomation::FromPx2Chars(double pixels)
{
	return ((pixels - 5) / 7) + 3;
}

Excel::RangePtr ExcelAutomation::GetRangeFromDispatch(IDispatch * pDispatch)
{
	if( !pDispatch )
	{
		DebugTrap();
		return 0;
	}

	return Excel::RangePtr(pDispatch);
}

void ExcelAutomation::MakeExcelSheetNameSafe(const char * str, char ** safeStr)
{
    size_t strLen = strlen(str);
    if (strLen >= 31)
        strLen = 30; // Excel restriction

    *safeStr = reinterpret_cast<char*>(calloc(strLen + 1, sizeof(char)));

    static char restrChars[] = ":/\?*[]<>|\"";
    static size_t restrCharsLen = ARRAYSIZE(restrChars) - 1;

    bool restricted = false;
    size_t curPos = 0;
    for (size_t i = 0; i < strLen; ++i) {
        restricted = false;
        for (size_t t = 0; t < restrCharsLen; ++t)
            if (str[i] == restrChars[t]) {
                restricted = true;
                break;
            }
        if (!restricted)
            (*safeStr)[curPos++] = str[i];
    }
    (*safeStr)[curPos] = 0;
}

HRESULT ExcelAutomation::FillRange(Excel::RangePtr range, SCellStyle & style)
{
	HRESULT hr = S_OK;

	try
	{ // try
		Excel::FontPtr cellFont = range->GetFont();

		if( !cellFont )
		{
			DebugTrap();
			return E_UNEXPECTED;
		}

		// general TRUE and FALSE
        VARIANT varTrue, varFalse;
        VariantInit(&varTrue);
        VariantInit(&varFalse);
        varTrue.vt = varFalse.vt = VT_BOOL;
        varTrue.boolVal = VARIANT_TRUE;
        varFalse.boolVal = VARIANT_FALSE;

        // font parameters
        if (!style.fontName.isEmpty()) { // font name
            VARIANT varFontName;
            VariantInit(&varFontName);
            varFontName.vt = VT_BSTR;
            varFontName.bstrVal = Char2SysStrAllocated(style.fontName.toLocal8Bit().constData());
            cellFont->PutName(varFontName);
            VariantClear(&varFontName);
        } // font name

        if (style.fontSize != -1) { // font size
            VARIANT varFontSize;
            VariantInit(&varFontSize);
            varFontSize.vt = VT_R4;
            varFontSize.fltVal = style.fontSize;
            cellFont->PutSize(varFontSize);
        } // font size

        if (style.bold != -1) { // bold
            cellFont->PutBold(style.bold == TRUE ? varTrue : varFalse);
        } // bold

        if (style.textWrap != -1) { // text wrapping
            range->PutWrapText(style.textWrap == TRUE ? varTrue : varFalse);
        } // text wrapping

        if (style.verAlign != -1) { // vertical alignment
            VARIANT varVAlignment;
            VariantInit(&varVAlignment);
            varVAlignment.vt = VT_I4;
            varVAlignment.intVal = style.verAlign;
            range->PutVerticalAlignment(varVAlignment);
        } // vertical alignment

        if (style.horAlign != -1) { // horizontal alignment
            VARIANT varHAlignment;
            VariantInit(&varHAlignment);
            varHAlignment.vt = VT_I4;
            varHAlignment.intVal = style.horAlign;
            range->PutHorizontalAlignment(varHAlignment);
        } // horizontal alignment

        if (!style.cellFormat.isEmpty()) { // set spec cells format explicitly
            VARIANT varTextFormat;
            VariantInit(&varTextFormat);
            varTextFormat.vt = VT_BSTR;
            varTextFormat.bstrVal = Char2SysStrAllocated(style.cellFormat.toLocal8Bit().constData());
            range->PutNumberFormat(varTextFormat);
            VariantClear(&varTextFormat);
        } // set spec cells format explicitly

        if (!style.title.isEmpty()) { // cell title
            VARIANT varValue;
            VariantInit(&varValue);
            varValue.vt = VT_BSTR;
            varValue.bstrVal = Char2SysStrAllocated(style.title.toLocal8Bit().constData());

            if (style.cellFormat.isEmpty()) { // implicit format detection
                bool isOnlyDigits = true;
                size_t len = style.title.length();
                for (size_t i = 0; i < len; ++i)
                    if (!style.title.at(static_cast<int>(i)).isDigit()) {
                        isOnlyDigits = false;
                        break;
                    }

                if (!isOnlyDigits) { // not a digit in title or forced text
                    VARIANT varTextFormat;
                    VariantInit(&varTextFormat);
                    varTextFormat.vt = VT_BSTR;
                    varTextFormat.bstrVal = Char2SysStrAllocated("@");
                    range->PutNumberFormat(varTextFormat);
                    VariantClear(&varTextFormat);
                } // not a digit in title or forced text
            } // implicit format detection

            range->PutFormulaR1C1(varValue);
            VariantClear(&varValue);
        } // cell title

        if (style.width != -1) { // cell width
            VARIANT varValue;
            VariantInit(&varValue);
            if (style.width > 1000)
                style.width = 1000;
            varValue.vt = VT_R8;
            varValue.dblVal = ExcelAutomation::FromPx2Chars(style.width);
            range->PutColumnWidth(varValue);
        } // cell width

        if (style.background != (COLORREF)-1) { // background color
            Excel::InteriorPtr interior = range->GetInterior();
            if (interior) { // have interior object
                VARIANT varRGB;
                VariantInit(&varRGB);
                varRGB.vt = VT_INT;
                varRGB.lVal = style.background;
                interior->PutColor(varRGB);
            } // have interior object
            else
                return E_FAIL;
        } // background color
    } // try
    catch (_com_error& e) {
        UNREFERENCED_PARAMETER(e);
#ifdef _DEBUG
        //		_debugOutput("Exception code 0x%08X Message %s\n", e.Error(), e.ErrorMessage());
#endif // _DEBUG
        DebugTrap();
        return e.Error();
    }
    return hr;
}

HRESULT ExcelAutomation::FillCell(int y, int x, SCellStyle & style)
{
    HRESULT hr = S_OK;

    try { // try
        VARIANT varX, varY;
        VariantInit(&varX);
        VariantInit(&varY);
        varX.vt = varY.vt = VT_INT;
        varY.intVal = y;
        varX.intVal = x;

        Excel::RangePtr theCell = ExcelAutomation::GetRangeFromDispatch(GetCells()->GetItem(varY, varX).pdispVal);

        if (!theCell) {
            DebugTrap();
            return hr;
        }

        return FillRange(theCell, style);
    } // try
    catch (_com_error& e) {
        UNREFERENCED_PARAMETER(e);
#ifdef _DEBUG
//        _debugOutput("Exception code 0x%08X Message %s\n", e.Error(), e.ErrorMessage());
#endif // _DEBUG
        DebugTrap();
        return e.Error();
    }
}

HRESULT ExcelAutomation::PutBordersForRange(
    Excel::RangePtr range, int borderStyle, int borderWeight, COLORREF borderColor)
{
	HRESULT hr = S_OK;

	try
	{ // try
		Excel::BordersPtr usedRangeBorders = range->GetBorders();
		if( range )
		{ // put cell border's style and color
			VARIANT varLineStyle;
			VariantInit(&varLineStyle);
			varLineStyle.vt = VT_I4;
			varLineStyle.intVal = borderStyle;
			usedRangeBorders->PutLineStyle(varLineStyle);
			VARIANT varWeight;
			VariantInit(&varWeight);
			varWeight.vt = VT_I4;
			varWeight.intVal = borderWeight;
			usedRangeBorders->PutWeight(varWeight);
			VARIANT varRGB;
			VariantInit(&varRGB);
			varRGB.vt = VT_INT;
			varRGB.lVal = borderColor;
			usedRangeBorders->PutColor(varRGB);
		} // put cell border's style and color
		else
			return E_UNEXPECTED;
	} // try
	catch(_com_error & e)
	{
        UNREFERENCED_PARAMETER(e);
#ifdef _DEBUG
        //		_debugOutput("Exception code 0x%08X Message %s\n", e.Error(), e.ErrorMessage());
#endif // _DEBUG
        DebugTrap();
        return e.Error();
	}
	return hr;
}

HRESULT ExcelAutomation::GetPrintPagesCount(long & pagesCount)
{
	try
	{ // try
		Excel::HPageBreaksPtr HorPageBreaks = GetWorkSheet()->GetHPageBreaks();
		Excel::VPageBreaksPtr VerPageBreaks = GetWorkSheet()->GetVPageBreaks();

		pagesCount = (HorPageBreaks->Count + 1) * (VerPageBreaks->Count + 1);
	} // try
	catch(_com_error & e)
	{
        UNREFERENCED_PARAMETER(e);
#ifdef _DEBUG
        //		_debugOutput("Exception code 0x%08X Message %s\n", e.Error(), e.ErrorMessage());
#endif // _DEBUG
        DebugTrap();
        return e.Error();
	}

	return S_OK;
}

#pragma warning(default:4530)
