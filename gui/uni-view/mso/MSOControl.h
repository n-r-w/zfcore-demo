#pragma once

/*
Выдрано из experium с минимальными доработками
*/

#include "MSOEventSink.h"
#include "MSOObject.h"
#include "msoinc.h"

#include <QSize>
#include <QString>

#define LIGHT_MSO_WINDOW() 1

class MSOControl : public IMSOObjectSite, public RefCounter
{
    // MSOControl

    MSOObject* pMSO = nullptr;
    CLSID msoCLSID = GUID_NULL;
    HWND hWindow = 0;
    int elemType = 0;
#if LIGHT_MSO_WINDOW()
    bool enableLightMSO = true;
    HWND hInternalMSOWnd = 0;
#endif // LIGHT_MSO_WINDOW()
    bool bReadOnly = false;

    IConnectionPointContainer* pConnectionPtContainer = nullptr;
    IConnectionPoint* pConnectionPt = nullptr;
    MSOEventSink* pMSOEventSink = nullptr;
    DWORD dwCookie = 0;

public:
    MSOControl(HWND _hWindow, int _elemType);
    virtual ~MSOControl();

    static LRESULT CALLBACK WndProc(
        HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);

    HWND GetWindow() const;
#if LIGHT_MSO_WINDOW()
    HWND GetInternalMSOWnd() const;
#endif // LIGHT_MSO_WINDOW()
    bool IsReadOnly() const;
    bool IsValid() const;

    void setLightMSO(bool b);
    bool loadFromFile(const QString& file_name, bool web_view);
    bool setReadOnly(bool read_only);
    void resize(const QSize& size);

    void onFocus(bool b);
    LRESULT wnd_OnFocus(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);

    void onActivateApp();

    int getViewType();
    bool setViewType(int type);

    bool print();

    bool isDirty() const;
    bool save();
    bool saveAs(const QString& file_name);
    static bool saveWordAsPdf(const QString& file_office, const QString& file_pdf);
    static bool saveExcelAsPdf(const QString& file_office, const QString& file_pdf);

    //	LRESULT OnCommand(HWND hWindow, UINT message, WPARAM wParam, LPARAM lParam);

    void PrepareToDestroy();
    HANDLE GetAppProcessHandle();
    void ClearEventContainers();

    template <typename _TDoc> HRESULT GetMSODocument(_TDoc** pMSODocument);

    //	HRESULT DoConnectToEvents();
    HRESULT DoSwitchToReadOnly();

    // Word specific methods
    HRESULT GetWordApplication(Word::_Application** pApplication);
    HRESULT SwitchReadOnly_Word();
    HRESULT ConnectToWordEvents();
    HRESULT HighlightWords(const char* strWords);
#if LIGHT_MSO_WINDOW()
    HRESULT SwitchToLightWord();
    HRESULT SwitchToLightExcel();
    HRESULT _SwitchToLightWord2003();
    HRESULT _SwitchToLightWord71013();
	HRESULT _SwitchToLightExcel2003();
	HRESULT _SwitchToLightExcel71013();
	#endif // LIGHT_MSO_WINDOW()

	// Excel specific methods
	HRESULT GetExcelApplication(Excel::_Application ** pApplication);
	HRESULT SwitchReadOnly_Excel();
	HRESULT ConnectToExcelEvents();

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
}; // MSOControl

bool IsMSOAvailable( WORD msoStyle );
bool IsMSOAvailableWithXP( WORD msoStyle );
bool IsXFormatAvailable( WORD msoStyle );
int GetWordVersion();
int GetExcelVersion();

struct SCellStyle
{ // SCellStyle
    QString fontName;
    float fontSize = 0;
    BOOL bold = false;
    BOOL textWrap = false;
    int horAlign = 0;
    int verAlign = 0;
    QString title;
    int width = 0;
    COLORREF background = 0;
    QString cellFormat;

    void Init()
    {
        fontName.clear();
        cellFormat.clear();
        fontSize = -1;
        bold = -1;
        textWrap = -1;
        horAlign = -1;
        verAlign = -1;
        width = -1;
        background = (COLORREF)-1;
    }
}; // SCellStyle

class ExcelAutomation
{ // ExcelAutomation
    Excel::_ApplicationPtr application = nullptr;
    Excel::_WorkbookPtr workbook = nullptr;
    Excel::_WorksheetPtr activeSheet = nullptr;

public:
    ExcelAutomation();
	~ExcelAutomation();

	HRESULT CreateExcelApplication();
	void ShutdownExcelThread();

	Excel::_ApplicationPtr GetApplication();
	Excel::_WorkbookPtr GetWorkbook();
	Excel::_WorksheetPtr GetWorkSheet();
	Excel::RangePtr GetCells();
	Excel::RangePtr GetRows();

	HRESULT InitDefaultPrintSetup();
	HRESULT SwitchPageToLandscape();
	HRESULT SwitchPageToPortrait();
	HRESULT InitDefaultWindow();

	void Freeze();
	void UnFreeze();

	HRESULT FillRange(Excel::RangePtr range, SCellStyle & style);
	HRESULT FillCell(int y, int x, SCellStyle & style);
	HRESULT PutBordersForRange(Excel::RangePtr range, int borderStyle, int borderWeight, COLORREF borderColor);
	HRESULT GetPrintPagesCount(long & pagesCount);
	HRESULT FitSheetToOnePage();

	// helpers
	static double FromPx2Chars(double pixels);
	static Excel::RangePtr GetRangeFromDispatch(IDispatch * pDispatch);
	static void MakeExcelSheetNameSafe(const char * str, char ** safeStr);
protected:
	HRESULT _SwitchPageOrientation(Excel::XlPageOrientation value);
private:
	ExcelAutomation(const ExcelAutomation &);
	ExcelAutomation & operator=(const ExcelAutomation &);

}; // ExcelAutomation

HRESULT ConvertWordDocument( const char * strInFile, const char * strOutFile );
