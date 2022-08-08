#ifdef UNICODE
#undef UNICODE
#endif
#ifdef _UNICODE
#undef _UNICODE
#endif

#ifdef Q_OS_WIN
#pragma warning(disable : 4311)
#pragma warning(disable : 4302)
#endif

#include "MSOObject.h"
#include "rbbinder.h"

#include <QDebug>
#include <QUuid>

MSOObject::MSOObject()
{
	ODS("MSOObject::MSOObject\n");
	bDisableDocValidation = false;
	m_cRef = 1;
	m_fDisplayTools = TRUE;
	m_bUpdateDocs = TRUE;
}

MSOObject::~MSOObject(void)
{
	ODS("MSOObject::~MSOObject\n");
	if (m_pole)	Close();
	if (m_hwnd) DestroyWindow(m_hwnd);

	SAFE_FREESTRING(m_pwszUsername);
	SAFE_FREESTRING(m_pwszPassword);
	SAFE_FREESTRING(m_pwszHostName);

	SAFE_RELEASE_INTERFACE(m_pstgroot);
	SAFE_RELEASE_INTERFACE(m_punkIPPResource);
	SAFE_RELEASE_INTERFACE(m_pcmdCtl);
	SAFE_RELEASE_INTERFACE(m_psiteCtl);
}

void MSOObject::SetDisableDocValidation(bool disable)
{
	bDisableDocValidation = disable;
}

bool MSOObject::GetDisableDocValidation() const
{
	return bDisableDocValidation;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::CreateNewDocObject
//
//  Static Creation Function.
//
STDMETHODIMP_(MSOObject*) MSOObject::CreateInstance(IMSOObjectSite* phost)
{
	ODS("MSOObject::CreateInstance()\n");
    MSOObject* pnew = new MSOObject();
    if ((pnew) && FAILED(pnew->InitializeNewInstance(phost))) {
        pnew->Release();
        pnew = nullptr;
    }
    return pnew;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::InitializeNewInstance
//
//  Sets up new docobject class. We must a control site to attach this
//  window to. It will call back to host for menu and IOleCommandTarget.
//
STDMETHODIMP MSOObject::InitializeNewInstance(IMSOObjectSite* phost)
{
	HRESULT hr = E_UNEXPECTED;
	WNDCLASS wndclass;
    HWND hwndCtl;

    ODS("MSOObject::InitializeNewInstance()\n");

    // We need valid IMSOObjectSite...
    if ((phost == nullptr) || FAILED(phost->GetWindow(&hwndCtl)))
        return hr;

    // As an AxDoc site, we need a valid parent window...
    if ((!hwndCtl) || (!IsWindow(hwndCtl)))
        return hr;

    // Create a temp storage for this docobj site (if one already exists, bomb out)...
    if ((m_pstgroot)
        || FAILED(hr = StgCreateDocfile(nullptr,
                      STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE | STGM_DELETEONRELEASE, 0,
                      &m_pstgroot)))
        return hr;

    // If our site window class has not been registered before, we should register it...

    // This is protected by a critical section just for fun. The fact we had to single
    // instance the OCX because of the host hook makes having multiple instances conflict here
    // very unlikely. However, that could change sometime, so better to be safe than sorry.
    EnterCriticalSection(&v_csecThreadSynch);

    const auto hProgInstance = reinterpret_cast<HINSTANCE>(GetModuleHandle(nullptr));

    if (GetClassInfo(hProgInstance, "DSOFramerDocWnd", &wndclass) == 0) {
        memset(&wndclass, 0, sizeof(WNDCLASS));
        wndclass.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
        wndclass.lpfnWndProc = MSOObject::FrameWindowProc;
        wndclass.hInstance = hProgInstance;
        wndclass.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wndclass.lpszClassName = "DSOFramerDocWnd";
        if (RegisterClass(&wndclass) == 0)
            hr = E_WIN32_LASTERROR;
    }

    LeaveCriticalSection(&v_csecThreadSynch);
    if (FAILED(hr))
        return hr;

    // Get the position RECT (and validate as needed)...
    hr = phost->GetBorder(&m_rcViewRect);
    if (FAILED(hr))
        return hr;

    if (m_rcViewRect.top > m_rcViewRect.bottom) {
        m_rcViewRect.top = 0;
        m_rcViewRect.bottom = 0;
    }
    if (m_rcViewRect.left > m_rcViewRect.right) {
        m_rcViewRect.left = 0;
        m_rcViewRect.right = 0;
    }

    // Create our site window at the give location (we are child of the control window)...
    m_hwnd = CreateWindowEx(0, "DSOFramerDocWnd", nullptr, WS_CHILD | WS_CLIPCHILDREN, m_rcViewRect.left,
        m_rcViewRect.top, (m_rcViewRect.right - m_rcViewRect.left), (m_rcViewRect.bottom - m_rcViewRect.top), hwndCtl,
        nullptr, hProgInstance, nullptr);

    if (!m_hwnd)
        return E_OUTOFMEMORY;

#ifdef _WIN64
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
#else
    SetWindowLong(m_hwnd, GWL_USERDATA, (LONG)this);
#endif

    // Save the control info for this object...
    m_hwndCtl = hwndCtl;
    (m_psiteCtl = phost)->AddRef();

    m_psiteCtl->QueryInterface(IID_IOleCommandTarget, (void**)&m_pcmdCtl);
    m_psiteCtl->GetHostName(&m_pwszHostName);
    if (m_pwszHostName == nullptr)
        m_pwszHostName = DsoCopyString(L"ExperiumMSOFramerControl");

    return S_OK;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::CreateDocObject (From CLSID)
//
//  This does embedding of new object, or loads a copy from storage.
//  To activate and show the object, you must call IPActivateView().
//
STDMETHODIMP MSOObject::CreateDocObject(REFCLSID rclsid)
{
    HRESULT hr;
    CLSID clsid;
    DWORD dwMiscStatus = 0;
    IOleObject* pole = nullptr;
    IPersistStorage* pipstg = nullptr;
    BOOL fInitNew = (m_pstgfile == nullptr);

    ODS("MSOObject::CreateDocObject(CLSID)\n");
    assert(!(m_pole));

    // Don't load if an object has already been loaded...
    if (m_pole) return E_UNEXPECTED;

	// It is possible that someone picked an older ProgId/CLSID that
	// will AutoConvert on CoCreate, so fix up the storage with the
	// new CLSID info. We we actually call CoCreate on the new CLSID...
	if (fInitNew && SUCCEEDED(OleGetAutoConvert(rclsid, &clsid)))
	{
		OleDoAutoConvert(m_pstgfile, &clsid);
	}
	else clsid = rclsid;

	// First, check the server to make sure it is AxDoc server...
	if (FAILED(hr = ValidateDocObjectServer(rclsid)))
		return hr;

	// If we haven't loaded a storage, create a new one and remember to
	// call InitNew (instead of Load) later on...
	if (fInitNew && FAILED(hr = CreateObjectStorage(rclsid)))
		return hr;

	// We are ready to create an instance. Call CoCreate to make an
	// inproc handler and ask for IOleObject (all docobjs must support this)...
	if (FAILED(hr = InstantiateDocObjectServer(clsid, &pole)))
		return hr;

	// Do a quick check to see if server wants us to set client site before the load..
	hr = pole->GetMiscStatus(DVASPECT_CONTENT, &dwMiscStatus);
	if (dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST)
		hr = pole->SetClientSite((IOleClientSite*)&m_xOleClientSite);

	// Load up the bloody thing...
	if (SUCCEEDED(hr = pole->QueryInterface(IID_IPersistStorage, (void**)&pipstg)))
	{
		// Remember to InitNew if this is a new storage...			
		hr = ((fInitNew) ? pipstg->InitNew(m_pstgfile) : pipstg->Load(m_pstgfile));
		pipstg->Release();
	}

	// Assuming all the above worked we should have an OLE Embeddable
	// object and should finish the initialization (set object running)...
	if (SUCCEEDED(hr))
	{
		// Save the IOleObject* and do a disconnect on quit...
		SAFE_SET_INTERFACE(m_pole, pole);
		m_fDisconnectOnQuit = TRUE;

		// Keep server CLSID for this object
		m_clsidObject = clsid;

		// Ensure server is running and locked...
		EnsureOleServerRunning(TRUE);

		// If we didn't do so already, set our client site...
		if (!(dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST))
			hr = m_pole->SetClientSite((IOleClientSite*)&m_xOleClientSite);

		// Set the host names for OLE embedding...
		m_pole->SetHostNames(m_pwszHostName, m_pwszHostName);

        // Ask object to save (if dirty)...
        if (IsDirty())
            SaveObjectStorage();
    } else { // Be sure they disconnect from our site if we failed load...
        pole->SetClientSite(nullptr);
    }

    // This will free the OLE server if anything above failed...
    SAFE_RELEASE_INTERFACE(pole);
    return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::CreateDocObject (From IStorage*)
//
//  This does embedding of new object, or loads a copy from storage.
//  To activate and show the object, you must call IPActivateView().
//
STDMETHODIMP MSOObject::CreateDocObject(IStorage *pstg)
{
    HRESULT hr;
    CLSID clsid;

    ODS("MSOObject::CreateDocObject(IStorage*)\n");
    CHECK_NULL_RETURN(pstg, E_POINTER);

    // Read the clsid from the storage..
    if (FAILED(hr = ReadClassStg(pstg, &clsid)))
        return hr;

    // Validate the server is AxDoc server...
	if (FAILED(hr = ValidateDocObjectServer(clsid)))
		return hr;

	// Create a new storage for this CLSID...
    if (FAILED(hr = CreateObjectStorage(clsid)))
        return hr;

    // Copy data into the new storage and commit the change...
    hr = pstg->CopyTo(0, nullptr, nullptr, m_pstgfile);
    if (SUCCEEDED(hr))
        hr = m_pstgfile->Commit(STGC_OVERWRITE);

    // Then do normal create on existing storage made from copy...
    return CreateDocObject(clsid);
}


////////////////////////////////////////////////////////////////////////
// MSOObject::CreateFromFile
//
//  Loads document into the framer control. The bind options determine if
//  file should be loaded read-only or read-write. An alternate CLSID can 
//  be provided if the document type is not normally associated with a 
//  a DocObject server in OLE.
//
//  To activate and show the object, you must call IPActivateView().
//
//  The code will attempt to open the file in one of three ways:
//
//   1.) Using IMoniker, we will attempt to open and load existing file
//       via a passed moniker, similar to how IE loads files.
//
//   2.) Using IPersistFile, we will attempt to open via direct file path.
//
//   3.) Finally, if above two options don't work, we will try to open the 
//       storage, make a copy and load using IPersistStorage. This last way follows
//       the (old) Binder behavior, and should *by spec* always work, but is a bit 
//       clunky since we open a copy, not the real file.
//
STDMETHODIMP MSOObject::CreateFromFile(LPWSTR pwszFile, REFCLSID rclsid, LPBIND_OPTS pbndopts)
{
    HRESULT hr;
    CLSID clsid;
    CLSID clsidConv;
    IOleObject* pole = nullptr;
    IBindCtx* pbctx = nullptr;
    IMoniker* pmkfile = nullptr;
    IStorage* pstg = nullptr;
    BOOL fLoadFromAltCLSID = (rclsid != GUID_NULL);

    // Sanity check of parameters...
    if (!(pwszFile) || ((*pwszFile) == L'\0') || (pbndopts == nullptr))
        return E_INVALIDARG;

    TRACE2("MSOObject::CreateFromFile(%S, %x)\n", pwszFile, pbndopts->grfMode);

    // First. we'll try to find the associated CLSID for the given file,
    // and then set it to the alternate if not found. If we don't have a
    // CLSID by the end of this, because user didn't specify alternate
    // and GetClassFile failed, then we error out...
    if (FAILED(GetClassFile(pwszFile, &clsid)) && !(fLoadFromAltCLSID))
		return DSO_E_INVALIDSERVER;

	// We should try to load from alternate CLSID if provided one...
	if (fLoadFromAltCLSID) clsid = rclsid;

	// We should also handle auto-convert to start "newest" server...
    if (SUCCEEDED(OleGetAutoConvert(clsid, &clsidConv)))
        clsid = clsidConv;

    // Validate that we have a DocObject server...
    if ((clsid == GUID_NULL) || FAILED(ValidateDocObjectServer(clsid)))
        return DSO_E_INVALIDSERVER;

    // Check for IE cache items since these are read-only as far as user is concerned...
    if (FIsIECacheFile(pwszFile)) {
        pbndopts->grfMode &= ~(STGM_READWRITE);
		pbndopts->grfMode &= ~(STGM_WRITE);
		pbndopts->grfMode |= STGM_READ;
    }

    // First, we try to bind by moniker (same as IE). We'll need a bind context 
	// and a file moniker for the orginal source...
	if (SUCCEEDED(hr = CreateBindCtx(0, &pbctx)))
	{
		if (SUCCEEDED(hr = pbctx->SetBindOptions(pbndopts)) && 
			SUCCEEDED(hr = CreateFileMoniker(pwszFile, &pmkfile)))
		{
			// IV: never bind to file object using this shit cos OOo & AOO 
			// register themself as an inplace activator's but actually cannot work properly.
			#if 0
			// Bind to the object moniker refers to...
			hr = pmkfile->BindToObject(pbctx, nullptr, IID_IOleObject, (void**)&pole);

			// If that failed, try to bind direct to file in new server...
			if (FAILED(hr))
#endif // 0
            {
                IPersistFile* pipfile = nullptr;

                if (SUCCEEDED(hr = InstantiateDocObjectServer(clsid, &pole))
                    && SUCCEEDED(hr = pole->QueryInterface(IID_IPersistFile, (void**)&pipfile))) {
                    hr = pipfile->Load(pwszFile, pbndopts->grfMode);
                    pipfile->Release();
                }
            }

            // If either solution worked, setup the rest of the bind info...
            if (SUCCEEDED(hr)) {
                // Save the IOleObject* and do a disconnect on quit...
				SAFE_SET_INTERFACE(m_pole, pole);
				m_fDisconnectOnQuit = TRUE;

				// Keep server CLSID for this object
				m_clsidObject = clsid;

				// Keep the moniker and bind ctx...
				SAFE_SET_INTERFACE(m_pmkSourceFile, pmkfile);
				SAFE_SET_INTERFACE(m_pbctxSourceFile, pbctx);

				// Set out client site...
				m_pole->SetClientSite((IOleClientSite*)&m_xOleClientSite);

                // We don't normally set host name for moniker bind, but MSWORD object
                // requires it to IP activate it instead of link & show external...
                if (IsWordObject())
                    m_pole->SetHostNames(m_pwszHostName, m_pwszHostName);

            } else
                pole->SetClientSite(nullptr);

            // This will release the moniker if above failed...
            pmkfile->Release();
        }
        // This will release the bind ctx if above failed...
		pbctx->Release();
	}

    // If binding by moniker failed, try the old fashion way of bind to OLE storage...
    if (FAILED(hr)) {
        // Try to open file as OLE storage (native OLE DocFile)...
        if (SUCCEEDED(hr = StgOpenStorage(pwszFile, nullptr, pbndopts->grfMode, nullptr, 0, &pstg))) {
            // Create our substorage, and copy the data over to it...
            if (SUCCEEDED(hr = CreateObjectStorage(clsid))
                && SUCCEEDED(hr = pstg->CopyTo(0, nullptr, nullptr, m_pstgfile))) {
                m_pstgfile->Commit(STGC_OVERWRITE);

                // Then create the object from the storage copy...
                if (SUCCEEDED(hr = CreateDocObject(clsid))) {
                    SAFE_SET_INTERFACE(m_pstgSourceFile, pstg);
                }
            }
            // This will release the storage if above failed...
            pstg->Release();
        }
    }

    // If all went well, we should save file name and whether it opened read-only...
    if (SUCCEEDED(hr)) {
        m_pwszSourceFile = DsoCopyString(pwszFile);
		m_idxSourceName = CalcDocNameIndex(m_pwszSourceFile);
		m_fOpenReadOnly = ((pbndopts->grfMode & STGM_WRITE) == 0) &&
			((pbndopts->grfMode & STGM_READWRITE) == 0);
    }

    // This will free the OLE server if anything above failed...
	SAFE_RELEASE_INTERFACE(pole);
	return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::CreateFromURL
//
//  Loads document from a URL resource. The document can be open read-only
//  using URLMON in the same context as the host client (i.e., it can be 
//  treated as normal navigation in host if host supports URLMON, like IE).
//  Or you can choose to open read-write, which will attempt to use the 
//  Microsoft Internet Publishing Provider (MSDAIPP) to author on the web.
//
//  The default is to open read-only with URLMON. If client wants read-write,
//  we will try MSDAIPP for DAV first, then MSDAIPP for FPSE/WSS/SPS.
//
STDMETHODIMP MSOObject::CreateFromURL(LPWSTR pwszUrlFile, REFCLSID rclsid, LPBIND_OPTS pbndopts, LPWSTR pwszUserName, LPWSTR pwszPassword)
{
    HRESULT hr;
    BOOL fReadOnly;
    LPWSTR pwszTempFile;
    IStream* pstmWebResource = nullptr;

    // First, a sanity check on the parameters passed...
    if ((pwszUrlFile == nullptr) || (pbndopts == nullptr))
        return E_INVALIDARG;

    TRACE2("MSOObject::CreateFromURL(%S, %x)\n", pwszUrlFile, pbndopts->grfMode);

    // Get a temp path for the download...
    if (!GetTempPathForURLDownload(pwszUrlFile, &pwszTempFile))
		return E_ACCESSDENIED;

	// Save out the user name and password (if provided) for IAuthenticate...
	if (pwszUserName)
	{
		SAFE_FREESTRING(m_pwszUsername);
		m_pwszUsername = DsoCopyString(pwszUserName);

		SAFE_FREESTRING(m_pwszPassword);
		m_pwszPassword = DsoCopyString(pwszPassword);
	}

	// Determine if they want to open read-only using URLMON or read-write using MSDAIPP...
	fReadOnly = ((pbndopts->grfMode & STGM_WRITE) == 0) &&
		((pbndopts->grfMode & STGM_READWRITE) == 0);

	if (fReadOnly) // If using URLMON...
	{
		hr = URLDownloadFile((IUnknown*)&m_xAuthenticate, pwszUrlFile, pwszTempFile);
	}
	else // Otherwise we need access to MSDAIPP for writable stream...
	{
		hr = IPPDownloadWebResource(pwszUrlFile, pwszTempFile, &pstmWebResource);
	}

	// If all goes well with the download, we can then open the temp file for write access to
	// use as local file during the editing. Then we can save back on the stream if requested.
	if (SUCCEEDED(hr))
	{
		BIND_OPTS bopts = {sizeof(BIND_OPTS), BIND_MAYBOTHERUSER, STGM_READWRITE|STGM_SHARE_EXCLUSIVE, 10000};
		hr = CreateFromFile(pwszTempFile, rclsid, &bopts);
		if (SUCCEEDED(hr))
		{
			m_fOpenReadOnly = fReadOnly;
			if (!m_fOpenReadOnly)
			{
				m_pwszWebResource = DsoCopyString(pwszUrlFile);
				SAFE_SET_INTERFACE(m_pstmWebResource, pstmWebResource);
			}
		}
	}

	SAFE_RELEASE_INTERFACE(pstmWebResource);
	DsoMemFree(pwszTempFile);
	return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::CreateFromRunningObject
//
//  Grabs a pre-existing object (such as a running object) and attempts
//  to embed it. If that fails, it will make a copy of the object and open
//  the copy read-only.
//
STDMETHODIMP MSOObject::CreateFromRunningObject(LPUNKNOWN punkObj, LPWSTR /*pwszObjectName*/, LPBIND_OPTS pbndopts)
{
	HRESULT hr;
	IPersistStorage *prststg;
	IOleObject *pole;
    CLSID clsid;
    BOOL fReadOnly;

    ODS("MSOObject::CreateFromRunningObject()\n");
    CHECK_NULL_RETURN(punkObj, E_POINTER);
    CHECK_NULL_RETURN(pbndopts, E_POINTER);

    // We only can support read-only since a running object is external, and in order
    // to get a copy inplace active, we end up making a copy of the storage, which means
    // this control will have a copy and not control the orginal source. Since the orginal
    // can be locked and not available, we have to treat this as read-only.
    fReadOnly = ((pbndopts->grfMode & STGM_WRITE) == 0) && ((pbndopts->grfMode & STGM_READWRITE) == 0);

    if (!fReadOnly)
        return STG_E_LOCKVIOLATION;

    // First, ensure the object passed supports embedding (i.e., we expect
	// something like a Word.Document or Excel.Sheet object, not a Word.Application
	// object or something else that is not a document type)...
	hr = punkObj->QueryInterface(IID_IOleObject, (void**)&pole);
	if (FAILED(hr)) return hr;

	// Ask the object to save its persistent state. We also collect its CLSID and
	// validate that the object type supports DocObject embedding...
	hr = pole->QueryInterface(IID_IPersistStorage, (void**)&prststg);
	if (SUCCEEDED(hr))
	{
		hr = prststg->GetClassID(&clsid); // Validate the object type...
		if (FAILED(hr) || FAILED(ValidateDocObjectServer(clsid)))
		{
            hr = DSO_E_INVALIDSERVER;
        } else {
            // Create a matching storage for the object and save the current
            // state of the file in our storage (this is our private copy)...
            if (SUCCEEDED(hr = CreateObjectStorage(clsid)) && SUCCEEDED(hr = prststg->Save(m_pstgfile, FALSE))
                && SUCCEEDED(hr = prststg->SaveCompleted(nullptr))) {
                // At this point we have a read-only copy...
                prststg->HandsOffStorage();
                m_fOpenReadOnly = TRUE;

                // Create the embedding...
                hr = CreateDocObject(clsid);
            }
        }

        prststg->Release();
    }

    pole->Release();
	return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::IPActivateView
//
//  Activates the object for inplace viewing.
//
STDMETHODIMP MSOObject::IPActivateView()
{
    HRESULT hr = E_UNEXPECTED;
    ODS("MSOObject::IPActivateView()\n");
    assert(m_pole);

    // Make sure the site window is made visible...
    if (GetUpdateDoc())
        if (!IsWindowVisible(m_hwnd))
            ShowWindow(m_hwnd, SW_SHOW);

    // If we have an IOleDocument pointer, we can use the Show method for
    // inplace activation...
    if (m_pdocv)
	{
        hr = m_pdocv->Show(TRUE);
    } else if (m_pole) {
        // Try creating a document view and loading it...
        hr = m_xOleDocumentSite.ActivateMe(nullptr);

        if (FAILED(hr)) // If that fails, go the old OLE route...
        {
            RECT rcView = {};
            if (GetUpdateDoc())
                GetClientRect(m_hwnd, &rcView);

            // First call IOleObject::DoVerb with OLEIVERB_INPLACEACTIVATE
            // (or OLEIVERB_SHOW), and our view rect and IOleClientSite pointer...
            hr = m_pole->DoVerb(
                OLEIVERB_INPLACEACTIVATE, nullptr, (IOleClientSite*)&m_xOleClientSite, (UINT)-1, m_hwnd, &rcView);

            // If the server doesn't recognize IP verb, try OLEIVERB_SHOW instead...
            if (hr == OLEOBJ_E_INVALIDVERB)
                hr = m_pole->DoVerb(
                    OLEIVERB_SHOW, nullptr, (IOleClientSite*)&m_xOleClientSite, (UINT)-1, m_hwnd, &rcView);

            // There is an issue with Visio 2002 rejecting DoVerb when it is called while
            // Visio is still loading up the main app window. OLE servers normally return RPC
            // retry later error, which will spin us in the msg-filter, but Visio just rejects
            // the call all together, which pops us out of the filter early. This causes the
            // embed attempt to fail. So to workaround this, we will check for the condition,
            // sleep a bit, and then try our own semi-msg-filter loop...
            if ((hr == RPC_E_CALL_REJECTED) && IsVisioObject()) {
                DWORD dwLoopCnt = 0;
                do {
                    Sleep((200 * ++dwLoopCnt));
                    hr = m_pole->DoVerb(
                        OLEIVERB_SHOW, nullptr, (IOleClientSite*)&m_xOleClientSite, (UINT)-1, m_hwnd, &rcView);
                } while ((hr == RPC_E_CALL_REJECTED) && (dwLoopCnt < 4));
            }
        }
    }

    // Go ahead and UI activate now...
    if (SUCCEEDED(hr))
		hr = UIActivateView();

	return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::IPDeactivateView
//
//  Deactivates the object ready for close.
//
STDMETHODIMP MSOObject::IPDeactivateView()
{
	HRESULT hr = S_OK;
	ODS("MSOObject::IPDeactivateView()\n");

	// If we still have a UI active object, tell it to UI deactivate...
	if (m_pipactive)
		UIDeactivateView();

	// Next hide the active object...
	if (m_pdocv)
		m_pdocv->Show(FALSE);

	// Notify object our intention to IP deactivate...
	if (m_pipobj)
		m_pipobj->InPlaceDeactivate();

    // Close the object down and release pointers...
    if (m_pdocv) {
        hr = m_pdocv->CloseView(0);
        m_pdocv->SetInPlaceSite(nullptr);
    }

    SAFE_RELEASE_INTERFACE(m_pcmdt);
    SAFE_RELEASE_INTERFACE(m_pdocv);
    SAFE_RELEASE_INTERFACE(m_pipobj);

	// Hide the site window...
	ShowWindow(m_hwnd, SW_HIDE);

	return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::UIActivateView
//
//  UI Activates the object (which should bring up toolbars and caret).
//
STDMETHODIMP MSOObject::UIActivateView()
{
	HRESULT hr = S_FALSE;
	ODS("MSOObject::UIActivateView()\n");

    if (m_pdocv) // Go UI active...
    {
        if (!GetUpdateDoc())
            hr = m_pdocv->UIActivate(TRUE);
    } else if (m_pole) // We should never get here, but just in case pdocv is nullptr, signal UI active the old way...
    {
        RECT rcView = {};
        if (GetUpdateDoc())
            GetClientRect(m_hwnd, &rcView);
        m_pole->DoVerb(OLEIVERB_UIACTIVATE, nullptr, (IOleClientSite*)&m_xOleClientSite, (UINT)-1, m_hwnd, &rcView);
    }

    // Forward focus to the IP object...
    if (SUCCEEDED(hr) && (m_hwndIPObject))
        SetFocus(m_hwndIPObject);

    return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::UIDeactivateView
//
//  UI Deactivates the object (which should hide toolbars and caret).
//
STDMETHODIMP MSOObject::UIDeactivateView()
{
	HRESULT hr = S_FALSE;
	ODS("MSOObject::UIDeactivateView()\n");
	if (m_pdocv)
	{
		__try
		{
			hr = m_pdocv->UIActivate(FALSE);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			// Ask Ivan about this
			DebugTrap();
		}
	}
	return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::IsDirty
//
//  Determines if the file is dirty (by asking IPersistXXX::IsDirty).
//
STDMETHODIMP_(BOOL) MSOObject::IsDirty()
{
	BOOL fDirty = TRUE; // Assume we are dirty unless object says we are not
    IPersistStorage* pprststg;
    IPersistFile* pprst;

    // Can't be dirty without object
    CHECK_NULL_RETURN(m_pole, FALSE);

    // Ask object its dirty state...
    if ((m_pmkSourceFile) && SUCCEEDED(m_pole->QueryInterface(IID_IPersistFile, (void**)&pprst))) {
        fDirty = ((pprst->IsDirty() == S_FALSE) ? FALSE : TRUE);
        pprst->Release();
    } else if (SUCCEEDED(m_pole->QueryInterface(IID_IPersistStorage, (void**)&pprststg))) {
        fDirty = ((pprststg->IsDirty() == S_FALSE) ? FALSE : TRUE);
        pprststg->Release();
    }

    return fDirty;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::Save
//
//  Does default save action for existing document (if it is not read-only).
//
//  There are three types of loaded doc objects we can save:
//   (1) Files obtained by URL write bind via MSDAIPP; 
//   (2) Files opened from local file source; and 
//   (3) Objects already running or files linked to via OLE moniker.
//
//  This function determines which type we should do the save, and call 
//  the right code for that type.
//
STDMETHODIMP MSOObject::Save()
{
	HRESULT	hr = DSO_E_NOTBEENSAVED;

	// We can't do  save default if file was open read-only,
	// caller must do save with new file name...
    if (!IsReadOnly()) {
        // If we have a URL (write-access) resource, do MSDAIPP save...
        if (m_pstmWebResource) {
            hr = SaveToURL(nullptr, TRUE, nullptr, nullptr);
        }
        // Else if it is local file, do a local save...
        else if (m_pwszSourceFile) {
            hr = SaveToFile(nullptr, TRUE);
        }
    }

    return hr;
}

STDMETHODIMP MSOObject::SaveToFile(LPWSTR pwszFile, BOOL fOverwriteFile)
{
    HRESULT hr = E_UNEXPECTED;
    IStorage* pstg;
    LPWSTR pwszFullName = nullptr;
    LPWSTR pwszRename = nullptr;
    BOOL fDoNormalSave = FALSE;
    BOOL fDoOverwriteOps = FALSE;
    BOOL fFileOpSuccess = FALSE;

    TRACE2("MSOObject::SaveToFile(%S, %d)\n", ((pwszFile) ? pwszFile : L"[Default]"), fOverwriteFile);
    CHECK_NULL_RETURN(m_pole, hr);

    // If they passed no file, use the default if current file is not read-only...
    if ((fDoNormalSave = (pwszFile == nullptr)) != 0) {
        // File must be read-write for normal save...
        if (IsReadOnly())
            return DSO_E_DOCUMENTREADONLY;

        // If we are not moniker bound, we'll use the existing file path for ole save...
        if (m_pwszSourceFile == nullptr)
            return DSO_E_NOTBEENSAVED;

        pwszFile = (pwszFullName = DsoCopyString(m_pwszSourceFile));
        if (pwszFile == nullptr)
            return E_OUTOFMEMORY;
    } else {
        // Make sure a file extension exists for string passed by caller...
        if (ValidateFileExtension(pwszFile, &pwszFullName))
            pwszFile = pwszFullName;
    }

    // See if we will be overwriting, and error unless given permission to do so...
    if (((fDoOverwriteOps = FFileExists(pwszFile)) != 0 && !(fOverwriteFile)) != 0)
        return STG_E_FILEALREADYEXISTS;

    // If we had a previous lock on storage, we have to free it...
	SAFE_RELEASE_INTERFACE(m_pstgSourceFile);

	// If we are overwriting, we do a little Shell Operation here. This is done
	// for two reasons: (1) it keeps the server from asking us to overwrite the
	//  file as it normally would in case of normal save; and (2) it lets us
	// restore the original if the save fails...
	if (fDoOverwriteOps)
	{
		pwszRename = DsoCopyStringCat(pwszFile, L".dstmp");
		fFileOpSuccess = ((pwszRename) && FPerformShellOp(FO_RENAME, pwszFile, pwszRename));
	}

	// Time to do the save to new file.

	// First we try to save by moniker using IPersistMoniker/IPersistFile. Check for defaut
	// save to the current document. If to a new doc, create new moniker and save ...
	if ((fDoNormalSave) && (m_pmkSourceFile))
	{
        hr = SaveDocToMoniker(m_pmkSourceFile, m_pbctxSourceFile, TRUE);
    } else if (pwszFile) {
        // Create a new moniker for the save...
        IMoniker* pnewmk = nullptr;
        if (SUCCEEDED(hr = CreateFileMoniker(pwszFile, &pnewmk))) {
            // If current document is read-only, we want new one to be read-write...
            if ((m_fOpenReadOnly) && (m_pbctxSourceFile)) {
                BIND_OPTS bndopts; bndopts.cbStruct = sizeof(BIND_OPTS);
				if (SUCCEEDED(m_pbctxSourceFile->GetBindOptions(&bndopts)))
				{
                    bndopts.grfMode = (STGM_TRANSACTED | STGM_SHARE_DENY_WRITE | STGM_READWRITE);
                    hr = m_pbctxSourceFile->SetBindOptions(&bndopts);
                }
            } else if (m_pbctxSourceFile == nullptr) {
                // May need to create a new bind context if we haven't already...
                if (SUCCEEDED(hr = CreateBindCtx(0, &m_pbctxSourceFile))) {
                    BIND_OPTS bndopts;
                    bndopts.cbStruct = sizeof(BIND_OPTS);
                    bndopts.grfFlags = BIND_MAYBOTHERUSER;
					bndopts.grfMode = (STGM_TRANSACTED | STGM_SHARE_DENY_WRITE | STGM_READWRITE);
                    bndopts.dwTickCountDeadline = 10000;
                    hr = m_pbctxSourceFile->SetBindOptions(&bndopts);
                }
            }

            // Do the save with moniker bind option and keep lock...
            if (SUCCEEDED(hr = SaveDocToMoniker(pnewmk, m_pbctxSourceFile, TRUE))) {
                SAFE_RELEASE_INTERFACE(m_pmkSourceFile);
                SAFE_SET_INTERFACE(m_pmkSourceFile, pnewmk);
                m_fOpenReadOnly = FALSE; // Should be read-write now...
            }

            pnewmk->Release();
        }
    }

    // If we are not using moniker binding, or the save with new moniker failed, try to use
    // a save copy approach and we will overwrite the file...
    if (!(m_pmkSourceFile) || FAILED(hr))
	{
		// Update the internal storage to the current doc state...
		if (((m_pstgfile) || SUCCEEDED(hr = CreateObjectStorage(m_clsidObject))) &&
			SUCCEEDED(hr = SaveObjectStorage()))
		{
			// Ask for server to save to file in "Save Copy As" fashion. This produces a native
			// OLE document file more similar to what happens in normal SaveAs dialog for BIFF file...
			hr = SaveDocToFile(pwszFile, FALSE);

            // If that doesn't work, save out the storage as OLE storage and ...
            if (FAILED(hr)
                && SUCCEEDED(hr = StgCreateDocfile(pwszFile,
                                 STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_CREATE, 0, &pstg))) {
                WriteClassStg(pstg, m_clsidObject);

                if (SUCCEEDED(hr = m_pstgfile->CopyTo(0, nullptr, nullptr, pstg)))
                    hr = pstg->Commit(STGC_OVERWRITE);

                pstg->Release();
            }
        }

        // If that worked, and we once had a moniker, we have to dump it now...
		if (SUCCEEDED(hr) && (m_pmkSourceFile))
		{
			SAFE_RELEASE_INTERFACE(m_pmkSourceFile);
			SAFE_RELEASE_INTERFACE(m_pbctxSourceFile);
		}
	}

	// If we made a copy to protect on overwrite, either restore or delete it as needed...
	if ((fDoOverwriteOps) && (fFileOpSuccess) && (pwszRename))
	{
		FPerformShellOp((FAILED(hr) ? FO_RENAME : FO_DELETE), pwszRename, pwszFile);
    }

    // If this is an exisitng file save, or the operation failed, relock the
    // the original file save source...
    if ((m_pstgfile) && !(m_pmkSourceFile) && (m_pwszSourceFile) && ((fDoNormalSave) || (FAILED(hr)))) {
        StgOpenStorage(m_pwszSourceFile, nullptr,
            ((m_pwszWebResource) ? (STGM_READ | STGM_SHARE_DENY_WRITE)
                                 : (STGM_TRANSACTED | STGM_SHARE_DENY_WRITE | STGM_READWRITE)),
            nullptr, 0, &m_pstgSourceFile);
    } else if (SUCCEEDED(hr)) {
        // Otherwise if we succeeded, free any existing file info we have it and
        // save the new file info for later re-saves (and lock)...
        SAFE_FREESTRING(m_pwszSourceFile);
        SAFE_FREESTRING(m_pwszWebResource);
		SAFE_RELEASE_INTERFACE(m_pstmWebResource);

		// Save the name, and try to lock the file for editing...
        if ((m_pwszSourceFile = DsoCopyString(pwszFile)) != 0) {
            m_idxSourceName = CalcDocNameIndex(m_pwszSourceFile);

            if (!(m_pmkSourceFile) && (m_pstgfile))
                StgOpenStorage(m_pwszSourceFile, nullptr, (STGM_TRANSACTED | STGM_SHARE_DENY_WRITE | STGM_READWRITE),
                    nullptr, 0, &m_pstgSourceFile);
        }
    }

    if (pwszRename)
        DsoMemFree(pwszRename);

    if (pwszFullName)
		DsoMemFree(pwszFullName);

	return hr;
}

STDMETHODIMP MSOObject::SaveToURL(LPWSTR pwszURL, BOOL fOverwriteFile, LPWSTR pwszUserName, LPWSTR pwszPassword)
{
	HRESULT	 hr = DSO_E_DOCUMENTREADONLY;

	// If we have no URL to save to and no previously open web stream, fail...
	if ((!pwszURL) && (!m_pstmWebResource))
		return hr;

	// Save out the user name and password (if provided) for IAuthenticate...
	if (pwszUserName)
	{
		SAFE_FREESTRING(m_pwszUsername);
		m_pwszUsername = DsoCopyString(pwszUserName);

		SAFE_FREESTRING(m_pwszPassword);
        m_pwszPassword = DsoCopyString(pwszPassword);
    }

    if ((pwszURL) && (m_pwszWebResource) && (DsoCompareStringsEx(pwszURL, -1, m_pwszWebResource, -1) == CSTR_EQUAL))
        pwszURL = nullptr;

    if (pwszURL) {
        IStream* pstmT = nullptr;
        LPWSTR pwszFullUrl = nullptr;
        LPWSTR pwszTempFile;

        IStream* pstmBkupStm;
        IStorage* pstgBkupStg;
        LPWSTR    pwszBkupFile, pwszBkupUrl;
		UINT      idxBkup;

		if (!GetTempPathForURLDownload(pwszURL, &pwszTempFile))
			return E_INVALIDARG;

        if (ValidateFileExtension(pwszURL, &pwszFullUrl))
            pwszURL = pwszFullUrl;

        // We are going to save out the current file info in case of
        // an error we can restore it to do native saves back to open location...
        pstmBkupStm = m_pstmWebResource;
        m_pstmWebResource = nullptr;
        pwszBkupUrl = m_pwszWebResource;
        m_pwszWebResource = nullptr;
        pwszBkupFile = m_pwszSourceFile;
        m_pwszSourceFile = nullptr;
        pstgBkupStg = m_pstgSourceFile;
        m_pstgSourceFile = nullptr;
        idxBkup = m_idxSourceName;
        m_idxSourceName = 0;

        // Save the object to a new (temp) file on the local drive...
        if (SUCCEEDED(hr = SaveToFile(pwszTempFile, TRUE))) {
            // Then upload from that file...
			hr = IPPUploadWebResource(pwszTempFile, &pstmT, pwszURL, fOverwriteFile);
        }

        // If both calls succeed, we can free the old file/url location info
		// and save the new information, otherwise restore the old info from backup...
        if (SUCCEEDED(hr)) {
            SAFE_RELEASE_INTERFACE(pstgBkupStg);

            if ((pstmBkupStm) && (pwszBkupFile))
                FPerformShellOp(FO_DELETE, pwszBkupFile, nullptr);

            SAFE_RELEASE_INTERFACE(pstmBkupStm);
            SAFE_FREESTRING(pwszBkupUrl);
            SAFE_FREESTRING(pwszBkupFile);

            m_pstmWebResource = pstmT;
			m_pwszWebResource = DsoCopyString(pwszURL);
			//m_pwszSourceFile already saved in SaveStorageToFile
			//m_pstgSourceFile already saved in SaveStorageToFile
			//m_idxSourceName already calced in SaveStorageToFile;

        } else {
            if (m_pstgSourceFile)
                m_pstgSourceFile->Release();

            if (m_pwszSourceFile) {
                FPerformShellOp(FO_DELETE, m_pwszSourceFile, nullptr);
                DsoMemFree(m_pwszSourceFile);
            }

            m_pstmWebResource = pstmBkupStm;
            m_pwszWebResource = pwszBkupUrl;
            m_pwszSourceFile   = pwszBkupFile;
			m_pstgSourceFile   = pstgBkupStg;
			m_idxSourceName    = idxBkup;
        }

        if (pwszFullUrl)
            DsoMemFree(pwszFullUrl);

        DsoMemFree(pwszTempFile);

    } else if ((m_pstmWebResource) && (m_pwszSourceFile)) {
        if (SUCCEEDED(hr = SaveToFile(nullptr, TRUE)))
            hr = IPPUploadWebResource(m_pwszSourceFile, &m_pstmWebResource, nullptr, TRUE);
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::DoOleCommand
//
//  Calls IOleCommandTarget::Exec on the active object to do a specific
//  command (like Print, SaveCopy, Zoom, etc.). 
//
STDMETHODIMP MSOObject::DoOleCommand(DWORD dwOleCmdId, DWORD dwOptions, VARIANT* vInParam, VARIANT* vInOutParam)
{
	HRESULT hr;
	OLECMD cmd = {dwOleCmdId, 0};
	TRACE2("MSOObject::DoOleCommand(cmd=%d, Opts=%d\n", dwOleCmdId, dwOptions);

    // Can't issue OLECOMMANDs when in print preview mode (object calls us)...
    if (InPrintPreview())
        return E_ACCESSDENIED;

    // The server must support IOleCommandTarget, the CmdID being requested, and
    // the command should be enabled. If this is the case, do the command...
    if ((m_pcmdt) && SUCCEEDED(m_pcmdt->QueryStatus(nullptr, 1, &cmd, nullptr))
        && ((cmd.cmdf & OLECMDF_SUPPORTED) && (cmd.cmdf & OLECMDF_ENABLED))) {
        TRACE1("QueryStatus say supported = 0x%X\n", cmd.cmdf);

        // Do the command asked by caller on default command group...
        hr = m_pcmdt->Exec(nullptr, cmd.cmdID, dwOptions, vInParam, vInOutParam);
        TRACE1("DocObj_IOleCommandTarget::Exec() = 0x%X\n", hr);

        if ((dwOptions == OLECMDEXECOPT_PROMPTUSER)) {
            // Handle bug issue for PPT when printing using prompt...
            if ((hr == E_INVALIDARG) && (cmd.cmdID == OLECMDID_PRINT) && IsPPTObject()) {
                ODS("Retry command for PPT\n");
                hr = m_pcmdt->Exec(nullptr, cmd.cmdID, OLECMDEXECOPT_DODEFAULT, vInParam, vInOutParam);
                TRACE1("DocObj_IOleCommandTarget::Exec() = 0x%X\n", hr);
            }

            // If user canceled an Office dialog, that's OK...
            if (hr == 0x80040103)
                hr = S_FALSE;
        }
    } else {
        TRACE1("Command Not supportted (%d)\n", cmd.cmdf);
        hr = DSO_E_COMMANDNOTSUPPORTED;
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::Close
//
//  Close down the object and disconnect us from any handlers/proxies.
//
STDMETHODIMP MSOObject::Close()
{
	HRESULT hr;

	ODS("MSOObject::Close\n");
	m_fInClose = TRUE;

	// Make sure we are not in print preview before close...
	if (InPrintPreview())
	{
		/*ExitPrintPreview(TRUE);*/
	}

	// Go ahead an IP deactivate the object...
	hr = IPDeactivateView();

	SAFE_RELEASE_INTERFACE(m_pprtprv);
	SAFE_RELEASE_INTERFACE(m_pcmdt);
	SAFE_RELEASE_INTERFACE(m_pdocv);
	SAFE_RELEASE_INTERFACE(m_pipactive);
	SAFE_RELEASE_INTERFACE(m_pipobj);

	// Release the OLE object and cleanup...
	if (m_pole)
	{
        // Free running lock if we set it...
        FreeRunningLock();

        // Tell server to release our client site...
        m_pole->SetClientSite(nullptr);

        // Finally, close the object and release the pointer...
        hr = m_pole->Close(OLECLOSE_NOSAVE);

        if (m_fDisconnectOnQuit)
            CoDisconnectObject((IUnknown*)m_pole, 0);

		SAFE_RELEASE_INTERFACE(m_pole);
	}

	SAFE_RELEASE_INTERFACE(m_pstgSourceFile);
	SAFE_RELEASE_INTERFACE(m_pmkSourceFile);
    SAFE_RELEASE_INTERFACE(m_pbctxSourceFile);

    // Free any temp file we might have...
    if ((m_pstmWebResource) && (m_pwszSourceFile))
        FPerformShellOp(FO_DELETE, m_pwszSourceFile, nullptr);

    SAFE_RELEASE_INTERFACE(m_pstmWebResource);
    SAFE_FREESTRING(m_pwszWebResource);
    SAFE_FREESTRING(m_pwszSourceFile);
    m_idxSourceName = 0;

	if (m_fDisconnectOnQuit)
	{
		CoDisconnectObject((IUnknown*)this, 0);
		m_fDisconnectOnQuit = FALSE;
	}

	SAFE_RELEASE_INTERFACE(m_pstmview);
	SAFE_RELEASE_INTERFACE(m_pstgfile);

	ClearMergedMenu();
	m_fInClose = FALSE;
	return S_OK;
}


////////////////////////////////////////////////////////////////////////
// MSOObject Notification Functions - The OCX should call these to
//  let the doc site update the object as needed.
//  

////////////////////////////////////////////////////////////////////////
// MSOObject::OnNotifySizeChange
//
//  Resets the size of the site window and tells UI active object to 
//  resize as well. If we are UI active, we'll call ResizeBorder to 
//  re-negotiate toolspace (allow toolbars to shrink and grow), otherwise
//  we'll just set the IP active view rect (minus any toolspace, which
//  should be none since object is not UI active!). 
//
STDMETHODIMP_(void) MSOObject::OnNotifySizeChange(LPRECT prc)
{
	RECT rc;

	SetRect(&rc, 0, 0, (prc->right - prc->left), (prc->bottom - prc->top));
	if (rc.right < 0) rc.right = 0;
	if (rc.top < 0) rc.top = 0;

    // First, resize our frame site window tot he new size (don't change focus)...
    if (m_hwnd) {
        m_rcViewRect = *prc;

        SetWindowPos(
            m_hwnd, nullptr, m_rcViewRect.left, m_rcViewRect.top, rc.right, rc.bottom, SWP_NOACTIVATE | SWP_NOZORDER);

        UpdateWindow(m_hwnd);
    }

    if (GetUpdateDoc()) {
        // If we have an active object (i.e., Document is still UI active) we should
		// tell it of the resize so it can re-negotiate border space...
		if ((m_fObjectUIActive) && (m_pipactive))
		{
			m_pipactive->ResizeBorder(&rc, (IOleInPlaceUIWindow*)&m_xOleInPlaceFrame, TRUE);
		}
		else if ((m_fObjectIPActive) && (m_pdocv))
		{
			rc.left   += m_bwToolSpace.left;   rc.right  -= m_bwToolSpace.right;
			rc.top    += m_bwToolSpace.top;    rc.bottom -= m_bwToolSpace.bottom;
			m_pdocv->SetRect(&rc);
		}
    }

    return;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::OnNotifyAppActivate
//
//  Notify doc object when the top-level frame window goes active and 
//  deactive so it can handle window focs and paiting correctly. Failure
//  to not forward this notification leads to bad behavior.
// 
STDMETHODIMP_(void) MSOObject::OnNotifyAppActivate(BOOL fActive, DWORD /*dwThreadID*/)
{
	// This is critical for DocObject servers, so forward these messages
	// when the object is UI active...
	if (m_pipactive)
	{
		// We should always tell obj server when our frame activates, but
		// don't tell it to go deactive if the thread gaining focus is 
		// the server's since our frame may have lost focus because of
		// a top-level modeless dialog (ex., the RefEdit dialog of Excel)...
		if (!m_fObjectInModalCondition)
			m_pipactive->OnFrameWindowActivate(fActive);
	}

	m_fAppWindowActive = fActive;
}

STDMETHODIMP_(void) MSOObject::OnNotifyControlFocus(BOOL fGotFocus)
{
	HWND hwnd;

	// For UI Active DocObject server, we will tell them we gain/lose control focus...
	if ((m_pipactive) && (!m_fObjectInModalCondition))
	{
        // TODO: Normally we would notify object of loss of control focus (such as user
        // moving focus from framer control to another text box or something on the same
        // form), but this can cause PPT to drop its toolbar and XL to drop its formula
        // bar, so we are skipping this call. We really should determine if it is needed
        // for another host (like Visio or something?) but that is to do...
        //
        //        m_pipactive->OnDocWindowActivate(fGotFocus);

        // We should forward the focus to the active window if window with the focus
        // is not already one that is parented to us...
        if ((fGotFocus) && !IsWindowChild(m_hwnd, GetFocus()) &&
			SUCCEEDED(m_pipactive->GetWindow(&hwnd)))
			SetFocus(hwnd);
	}
}

////////////////////////////////////////////////////////////////////////
// MSOObject::OnNotifyPaletteChanged
//
//  Give the object first chance at realizing a palette. Important on
//  256 color machines, but not so critical these days when everyone is
//  running full 32-bit True Color graphic cards.
// 
STDMETHODIMP_(void) MSOObject::OnNotifyPaletteChanged(HWND hwndPalChg)
{
	if ((m_fObjectUIActive) && (m_hwndUIActiveObj))
		SendMessage(m_hwndUIActiveObj, WM_PALETTECHANGED, (WPARAM)hwndPalChg, 0L);
}

////////////////////////////////////////////////////////////////////////
// MSOObject::OnNotifyChangeToolState
//
//  This should be called to get object to show/hide toolbars as needed.
//
STDMETHODIMP_(void) MSOObject::OnNotifyChangeToolState(BOOL fShowTools)
{
	// Can't change toolbar state in print preview (sorry)...
	if (InPrintPreview()) return;

	// If we want to show/hide toolbars, we can do the following...
	if (fShowTools != (BOOL)m_fDisplayTools)
	{
		OLECMD cmd;
        cmd.cmdID = OLECMDID_HIDETOOLBARS;

        m_fDisplayTools = fShowTools;

        // Use IOleCommandTarget(OLECMDID_HIDETOOLBARS) to toggle on/off. We have
        // to check that server supports it and if its state matches our own so
        // when toggle, we do the correct thing by the user...
        if ((m_pcmdt) && SUCCEEDED(m_pcmdt->QueryStatus(nullptr, 1, &cmd, nullptr))
            && ((cmd.cmdf & OLECMDF_SUPPORTED) || (cmd.cmdf & OLECMDF_ENABLED))) {
            if (((m_fDisplayTools) && ((cmd.cmdf & OLECMDF_LATCHED) == OLECMDF_LATCHED))
                || (!(m_fDisplayTools) && !((cmd.cmdf & OLECMDF_LATCHED) == OLECMDF_LATCHED))) {
                m_pcmdt->Exec(nullptr, OLECMDID_HIDETOOLBARS, OLECMDEXECOPT_PROMPTUSER, nullptr, nullptr);
            } else if (m_fDisplayTools && (IsWordObject() || IsPPTObject())) {
                // HACK: Word and PowerPoint 2007 do not report the correct latched state for the ribbon,
                // so if we are trying to show the tools after hiding them, they will say they are already
                // visible when the ribbon is not. This is an apparant trick they are using to force the
                // ribbon visible in IE embed cases, but it is causing us problems.
                m_pcmdt->Exec(nullptr, OLECMDID_HIDETOOLBARS, OLECMDEXECOPT_PROMPTUSER, nullptr, nullptr);
            }

            // There can be focus issues when turning them off, so make sure
            // the object is on top of the z-order...
            if ((!m_fDisplayTools) && (m_hwndIPObject))
                BringWindowToTop(m_hwndIPObject);

            // If user toggles off the toolbar while the object is UI active, and
            // we are not still in activation process, we need to explictly tell Office
            // apps to also hide the "Web" toolbar. For Office, OLECMDID_HIDETOOLBARS puts
            // the app into a "web view" which (in some apps) brings up the web toolbar.
            // Since we intend to have no tools, we have to turn it off by code...
            if ((m_fObjectUIActive) && (m_fObjectActivateComplete))
                TurnOffWebToolbar(!m_fDisplayTools);

        } else if (m_pdocv) {
            // If we have a DocObj server, but no IOleCommandTarget, do things the hard
            // way and resize. When server attempts to resize window it will have to
            // re-negotiate BorderSpace and we fail there, so server "should" not
            // display its tools (at least that is the idea!<g>)...
            RECT rc;
            GetClientRect(m_hwnd, &rc);
            MapWindowPoints(m_hwnd, m_hwndCtl, (LPPOINT)&rc, 2);
            OnNotifySizeChange(&rc);
        }
    }
    return;
}


////////////////////////////////////////////////////////////////////////
// MSOObject Protected Functions -- Helpers
//

////////////////////////////////////////////////////////////////////////
// MSOObject::InstantiateDocObjectServer (protected)
//
//  Startup OLE Document Object server and get IOleObject pointer.
//
STDMETHODIMP MSOObject::InstantiateDocObjectServer(REFCLSID rclsid, IOleObject **ppole)
{
    HRESULT hr;
    IUnknown* punk = nullptr;

    ODS("MSOObject::InstantiateDocObjectServer()\n");

    /*
    IV: don't call to create inproc server due hanging other instance of loaded Microsoft Word.
    //||
    //SUCCEEDED(hr = CoCreateInstance(rclsid, nullptr, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&punk))
    */

    // We perform custom create in order of local server then inproc server...
    if (SUCCEEDED(hr = CoCreateInstance(rclsid, nullptr, CLSCTX_LOCAL_SERVER, IID_IUnknown, (void**)&punk))) {
        // Ask for IOleObject interface...
        if (SUCCEEDED(hr = punk->QueryInterface(IID_IOleObject, (void**)ppole))) {
            // TODO: Add strong connection to remote server (IExternalConnection??)...
        }
        punk->Release();
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::CreateObjectStorage (protected)
//
//  Makes the internal IStorage to host the object and assigns the CLSID.
//
STDMETHODIMP MSOObject::CreateObjectStorage(REFCLSID rclsid)
{
	HRESULT hr;
	LPWSTR pwszName;
	DWORD dwid;
	CHAR szbuf[256];

	if ((!m_pstgroot)) return E_UNEXPECTED;

	// Next, create a new object storage (with unique name) in our
	// temp root storage "file" (this keeps an OLE integrity some servers
	// need to function correctly instead of IP activating from file directly).

	// We make a fake object storage name...
	dwid = ((rclsid.Data1)|GetTickCount());
    wsprintf(szbuf, "OLEDocument%X", dwid);

    if ((pwszName = DsoConvertToLPWSTR(szbuf)) == nullptr)
        return E_OUTOFMEMORY;

    // Create the sub-storage...
	hr = m_pstgroot->CreateStorage(pwszName,
		STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &m_pstgfile);

	DsoMemFree(pwszName);

	if (FAILED(hr)) return hr;

	// We'll also create a stream for OLE view settings (non-critical)...
	if( (pwszName = DsoConvertToLPWSTR(szbuf)) != 0)
	{
		m_pstgroot->CreateStream(pwszName,
			STGM_DIRECT | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &m_pstmview);
		DsoMemFree(pwszName);
	}

	// Finally, write out the CLSID for the new substorage...
	hr = WriteClassStg(m_pstgfile, rclsid);
	return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::SaveObjectStorage (protected)
//
//  Saves the object back to the internal IStorage. Returns S_FALSE if
//  there is no file storage for this document (it depends on how file
//  was loaded). In most cases we should have one, and this copies data
//  into the internal storage for save.
//
STDMETHODIMP MSOObject::SaveObjectStorage()
{
    HRESULT hr = S_FALSE;
    IPersistStorage* pipstg = nullptr;

    // Got to have object to save state...
    if (!m_pole)
        return E_UNEXPECTED;

    // If we have file storage, ask for IPersist and Save (commit changes)...
    if ((m_pstgfile) && SUCCEEDED(hr = m_pole->QueryInterface(IID_IPersistStorage, (void**)&pipstg))) {
        if (SUCCEEDED(hr = pipstg->Save(m_pstgfile, TRUE)))
            hr = pipstg->SaveCompleted(nullptr);

        hr = m_pstgfile->Commit(STGC_DEFAULT);
        pipstg->Release();
    }

    // Go ahead and save the view state if view still active (non-critical)...
    if ((m_pdocv) && (m_pstmview))
	{
		m_pdocv->SaveViewState(m_pstmview);
		m_pstmview->Commit(STGC_DEFAULT);
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::SaveDocToMoniker (protected)
//
//  Saves document to location spcified by the moniker passed. The server
//  needs to support either IPersistMoniker or IPersistFile.  This is used
//  primarily when document was opened by moniker instead of storage.
//
STDMETHODIMP MSOObject::SaveDocToMoniker(IMoniker *pmk, IBindCtx *pbc, BOOL fKeepLock)
{
    HRESULT hr = E_FAIL;
    IPersistMoniker* prstmk;
    LPOLESTR pwszFullName = nullptr;

    CHECK_NULL_RETURN(m_pole, E_UNEXPECTED);
    CHECK_NULL_RETURN(pmk, E_POINTER);

    // Get IPersistMoniker interface and ask it to save context if it existing moniker...
    if (SUCCEEDED(hr = m_pole->QueryInterface(IID_IPersistMoniker, (void**)&prstmk))) {
        hr = prstmk->Save(pmk, pbc, fKeepLock);
        prstmk->Release();
    }

    // If that failed to work, switch to IPersistFile and use full path to the new file...
    if (FAILED(hr) && SUCCEEDED(hr = pmk->GetDisplayName(pbc, nullptr, &pwszFullName))) {
        hr = SaveDocToFile(pwszFullName, fKeepLock);
        CoTaskMemFree(pwszFullName);
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::SaveDocToFile (protected)
//
//  Saves document to location spcified by the path passed. The server
//  needs to support either IPersistFile. 
//
STDMETHODIMP MSOObject::SaveDocToFile(LPWSTR pwszFullName, BOOL fKeepLock)
{
    HRESULT hr = E_FAIL;
    IPersistFile* pipfile = nullptr;

    BOOL fSameAsOpen = ((m_pwszSourceFile)
        && DsoCompareStringsEx(pwszFullName, lstrlenW(pwszFullName), m_pwszSourceFile, lstrlenW(m_pwszSourceFile))
            == CSTR_EQUAL);

    if (SUCCEEDED(hr = m_pole->QueryInterface(IID_IPersistFile, (void**)&pipfile))) {
#ifdef DSO_WORD12_PERSIST_BUG
        LPOLESTR pwszWordCurFile = nullptr;
        // HACK: Word 2007 RTM has a bug in its IPersistFile::Save method which will cause it to save
        // to the old file location and not the new one, so if the document being saved is a Word
        // object and we are saving to a new file, then got to run this hack to copy the saved bits
        // to the correct location...
        if (IsWordObject()) {
            hr = pipfile->GetCurFile(&pwszWordCurFile);
            if ((fSameAsOpen) && (pwszWordCurFile))
			{
				// Check again if Word thinks the file is the same as we do...
                fSameAsOpen = (DsoCompareStringsEx(
                                   pwszFullName, lstrlenW(pwszFullName), pwszWordCurFile, lstrlenW(pwszWordCurFile))
                    == CSTR_EQUAL);
                if (fSameAsOpen) { // If it is the same file after all, we don't need to do the hack...
                    CoTaskMemFree(pwszWordCurFile);
                    pwszWordCurFile = nullptr;
                }
            }
        }
#endif

        // Do the save using file path or nullptr if we are saving to current file...
        hr = pipfile->Save((fSameAsOpen ? nullptr : pwszFullName), fKeepLock);

#ifdef DSO_WORD12_PERSIST_BUG
		// HACK: If we have the pwszWordCurFile, we'll assume Word 12 RTM might have saved this, and we
		// need to check if Word saved to the right path or not. So check the paths, and if the new 
		// file is not there, copy the file Word saved to the new location...
		if (pwszWordCurFile)
		{
			if (SUCCEEDED(hr) && !FFileExists(pwszFullName) && FFileExists(pwszWordCurFile))
			{
				if (!FPerformShellOp(FO_COPY, pwszWordCurFile, pwszFullName))
					hr = E_ACCESSDENIED;
			}
			CoTaskMemFree(pwszWordCurFile);
		}
		// Hopefully, Word should have this fixed by Office 2007 SP1 and we can remove this hack!
#endif
        pipfile->Release();
    }

    return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::ValidateDocObjectServer (protected)
//
//  Quick validation check to see if CLSID is for DocObject server.
//
//  Officially, the only way to determine if a server supports ActiveX
//  Document embedding is to IP activate it and ask for IOleDocument, 
//  but that means going through the IP process just to fail if IOleDoc
//  is not supported. Therefore, we are going to rely on the server's 
//  honesty in setting its reg keys to include the "DocObject" sub key 
//  under their CLSID.
//
//  This is 99% accurate. For those servers that fail, too bad charlie!
//
STDMETHODIMP MSOObject::ValidateDocObjectServer(REFCLSID rclsid)
{
	if( GetDisableDocValidation() )
		return S_OK;

	HRESULT hr = DSO_E_INVALIDSERVER;
	CHAR  szKeyCheck[256];
	LPSTR pszClsid;
	HKEY  hkey;

	// We don't handle MSHTML even though it is DocObject server. If you plan
	// to view web pages in browser-like context, best to use WebBrowser control...
	if (rclsid == CLSID_MSHTML_DOCUMENT)
		return hr;

	// Convert the CLSID to a string and check for DocObject sub key...
	if ( (pszClsid = DsoCLSIDtoLPSTR(rclsid)) != 0)
	{
		wsprintf(szKeyCheck, "CLSID\\%s\\DocObject", pszClsid);

		if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szKeyCheck, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
		{
			hr = S_OK;
			RegCloseKey(hkey);
		}

		DsoMemFree(pszClsid);
	}
	else hr = E_OUTOFMEMORY;

	return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::GetDocumentTypeAndFileExtension 
//
//  Returns default file type and extension for a save of embedded object.
//
STDMETHODIMP_(BOOL) MSOObject::GetDocumentTypeAndFileExtension(WCHAR** ppwszFileType, WCHAR** ppwszFileExt)
{
    DWORD dwType, dwSize;
    LPWSTR pwszExt = nullptr;
    LPWSTR pwszType = nullptr;
    LPSTR pszType = nullptr;
    LPSTR pszClsid;
    CHAR szkey[255];
    CHAR szbuf[255];
    HKEY hk;

    if ((ppwszFileType == nullptr) && (ppwszFileExt == nullptr))
        return FALSE;

    pszClsid = DsoCLSIDtoLPSTR(m_clsidObject);
    if (!pszClsid)
        return FALSE;

    wsprintf(szkey, "CLSID\\%s\\DefaultExtension", pszClsid);
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT, szkey, 0, KEY_READ, &hk) == ERROR_SUCCESS) {
        LPSTR pszT = szbuf;
        szbuf[0] = '\0';
        dwSize = 255;
        if (RegQueryValueEx(hk, nullptr, 0, &dwType, (BYTE*)pszT, &dwSize) == ERROR_SUCCESS) {
            while (*(pszT++) && (*pszT != ','))
                (void)(0);

            if (*pszT == ',')
                (pszType = pszT)++;

            *pszT = '\0';
        } else
            lstrcpy(szbuf, ".ole");

        RegCloseKey(hk);
    } else
        lstrcpy(szbuf, ".ole");

    pwszExt = DsoConvertToLPWSTR(szbuf);
    if (ppwszFileExt) *ppwszFileExt = pwszExt;

	if (ppwszFileType)
	{
		ULONG cb1, cb2;
		LPWSTR pwszCombined;
		CHAR szUnknownType[255];

		if (!(*pszType))
		{
			szUnknownType[0] = '\0';
			wsprintf(szUnknownType, "Native Document (*%s)", szbuf);
			pszType = szUnknownType;
		}
		pwszType = DsoConvertToLPWSTR(pszType);

		cb1 = lstrlenW(pwszExt);
		cb2 = lstrlenW(pwszType);

		pwszCombined = (LPWSTR)DsoMemAlloc(((cb1 + cb2 + 4) * sizeof(WCHAR)));
		if (pwszCombined)
		{
			memcpy(&pwszCombined[0], pwszType,(cb2 * sizeof(WCHAR)));
			pwszCombined[cb2] = L'\0';

			pwszCombined[cb2 + 1] = L'*';
			memcpy(&pwszCombined[cb2 + 2], pwszExt, (cb1 * sizeof(WCHAR)));
			pwszCombined[cb2 + 2 + cb1] = L'\0';
			pwszCombined[cb2 + 3 + cb1] = L'\0';

			DsoMemFree(pwszType);
        }

        *ppwszFileType = ((pwszCombined) ? pwszCombined : pwszType);
    }

    if ((ppwszFileExt == nullptr) && (pwszExt))
        DsoMemFree(pwszExt);

    DsoMemFree(pszClsid);
    return (((ppwszFileExt) && (*ppwszFileExt)) || ((ppwszFileType) && (*ppwszFileType)));
}

////////////////////////////////////////////////////////////////////////
// MSOObject::ValidateFileExtension (protected)
//
//  Adds a default extension to save file path if user didn't provide
//  one (uses CLSID and registry to determine default extension).
//
STDMETHODIMP_(BOOL) MSOObject::ValidateFileExtension(WCHAR* pwszFile, WCHAR** ppwszOut)
{
    BOOL fHasExt = FALSE;
    BOOL fChangedExt = FALSE;
    LPWSTR pwszT;
    LPWSTR pwszExt = nullptr;
    DWORD dw = 0;

    if (((pwszFile) != 0 && (dw = lstrlenW(pwszFile)) != 0 && (ppwszOut)) != 0) {
        *ppwszOut = nullptr;

        pwszT = (pwszFile + dw);
        while ((pwszT != pwszFile) && (*(--pwszT)) && ((*pwszT != L'\\') && (*pwszT != L'/'))) {
            if (*pwszT == L'.')
                fHasExt = TRUE;
        }

        if (!(fHasExt) && GetDocumentTypeAndFileExtension(nullptr, &pwszExt)) {
            *ppwszOut = DsoCopyStringCat(pwszFile, pwszExt);
            fChangedExt = ((*ppwszOut) != nullptr);
        }
    }

    return fChangedExt;
}



////////////////////////////////////////////////////////////////////////
// MSOObject::EnsureOleServerRunning (protected)
//
//  Verifies the DocObject server is running and is optionally locked
//  while the embedding is taking place.
//
STDMETHODIMP MSOObject::EnsureOleServerRunning(BOOL fLockRunning)
{
	HRESULT hr = S_FALSE;
	IBindCtx *pbc;
	IRunnableObject *pro;
    IOleContainer* pocnt;
    BIND_OPTS bnd = {sizeof(BIND_OPTS), BIND_MAYBOTHERUSER, (STGM_READWRITE | STGM_SHARE_EXCLUSIVE), 10000};

    TRACE1("MSOObject::EnsureOleServerRunning(%d)\n", (DWORD)fLockRunning);
    if (m_pole == nullptr)
        return E_FAIL;

    // If we are already locked, don't need to do this again...
    if (m_fLockedServerRunning)
        return hr;

    // Create a bind ctx...
	if (FAILED(CreateBindCtx(0, &pbc)))
		return E_UNEXPECTED;

	// Setup default bind options for the run operation...
	pbc->SetBindOptions(&bnd);

	// Get IRunnableObject and set server to run as OLE object. We check the
	// running state first since this is proper OLE, but note that this is not
	// returned from out-of-proc server, but the in-proc handler. Also note, we
	// specify a timeout in case the object never starts, and "check" the object
	// runs without IMessageFilter errors...
	if (SUCCEEDED(m_pole->QueryInterface(IID_IRunnableObject, (void**)&pro)))
	{

		// If the object is not currently running, let's run it...
		if (!(pro->IsRunning()))
			hr = pro->Run(pbc);

		// Set the object server as a contained object (i.e., OLE object)...
		pro->SetContainedObject(TRUE);

		// Lock running if desired...
		if (fLockRunning)
			m_fLockedServerRunning = SUCCEEDED(pro->LockRunning(TRUE, TRUE));

		pro->Release();
	}
	else if (SUCCEEDED(m_pole->QueryInterface(IID_IOleContainer, (void**)&pocnt)))
	{
		if (fLockRunning)
			m_fLockedServerRunning = SUCCEEDED(pocnt->LockContainer(TRUE));

		pocnt->Release();
	}

	pbc->Release();
	return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::FreeRunningLock (protected)
//
//  Free any previous lock made by EnsureOleServerRunning.
//
STDMETHODIMP_(void) MSOObject::FreeRunningLock()
{
    IRunnableObject* pro;
    IOleContainer* pocnt;

    ODS("MSOObject::FreeRunningLock(%d)\n");
    assert(m_pole);

    // Don't do anything if we didn't lock the server...
    if (m_fLockedServerRunning == FALSE)
        return;

    // Get IRunnableObject and free lock...
    if (SUCCEEDED(m_pole->QueryInterface(IID_IRunnableObject, (void**)&pro))) {
        pro->LockRunning(FALSE, TRUE);
		pro->Release();
    } else if (SUCCEEDED(m_pole->QueryInterface(IID_IOleContainer, (void**)&pocnt))) {
        pocnt->LockContainer(FALSE);
		pocnt->Release();
    }

    m_fLockedServerRunning = FALSE;
	return;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::SetRunningServerLock 
//
//  Sets an external lock on the running object server.
//
STDMETHODIMP MSOObject::SetRunningServerLock(BOOL fLock)
{
	HRESULT hr;
    TRACE1("MSOObject::SetRunningServerLock(%d)\n", (DWORD)fLock);

    // Word doesn't obey normal running lock, but does have method to explicitly
    // lock the server for mail, so we'll use that in the Word case...
    if (IsWordObject()) {
        IClassFactory* pcf = nullptr;
        interface ifoo : public IUnknown
        {
            STDMETHOD(_uncall)() PURE;
            STDMETHOD(lock)(BOOL f) PURE;
        }
        *pi = nullptr;
        const GUID iidifoo = {0x0006729A, 0x0000, 0x0000, {0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

        SEH_TRY
        hr = CoGetClassObject(CLSID_WORD_DOCUMENT_DOC, CLSCTX_LOCAL_SERVER, nullptr, IID_IClassFactory, (void**)&pcf);
        if (SUCCEEDED(hr) && (pcf)) {
            hr = pcf->QueryInterface(iidifoo, (void**)&pi);
            if (SUCCEEDED(hr) && (pi)) {
                hr = pi->lock(fLock);
				pi->Release();
            }
            pcf->Release();
        }
        SEH_EXCEPT(hr)
    } else {
        if (fLock) {
            hr = EnsureOleServerRunning(TRUE);
        } else {
            hr = (FreeRunningLock(), S_OK);
        }
    }

    return hr;
}


////////////////////////////////////////////////////////////////////////
// MSOObject::CreateIPPBindResource (protected)
//
//  Returns an instance of MSDAIPP URL resource provider which is used by
//  Office/Windows for Web Folders (DAV/FPSE server). 
//
STDMETHODIMP_(IUnknown*) MSOObject::CreateIPPBindResource()
{
    HRESULT hr;
    IDBProperties* pdbprops = nullptr;
    IBindResource* pres = nullptr;
    DBPROPSET rdbpset;
    DBPROP rdbp[4];
    BSTR bstrLock;
    DWORD dw = 256;
    CHAR szUserName[256];

    if (FAILED(CoCreateInstance(CLSID_MSDAIPP_BINDER, nullptr, CLSCTX_INPROC, IID_IDBProperties, (void**)&pdbprops)))
        return nullptr;

    bstrLock = (GetUserName(szUserName, &dw) ? DsoConvertToBSTR(szUserName) : nullptr);

    memset(rdbp, 0, sizeof(rdbp));

    rdbpset.cProperties = 4;
    rdbpset.guidPropertySet = DBPROPSET_DBINIT;
    rdbpset.rgProperties = rdbp;

	rdbp[0].dwPropertyID = DBPROP_INIT_BINDFLAGS;
	rdbp[0].vValue.vt = VT_I4;
	rdbp[0].vValue.lVal = DBBINDURLFLAG_OUTPUT;

	rdbp[1].dwPropertyID = DBPROP_INIT_LOCKOWNER;
	rdbp[1].vValue.vt = VT_BSTR;
	rdbp[1].vValue.bstrVal = bstrLock;

	rdbp[2].dwPropertyID = DBPROP_INIT_LCID;
	rdbp[2].vValue.vt = VT_I4;
	rdbp[2].vValue.lVal = GetThreadLocale();

	rdbp[3].dwPropertyID = DBPROP_INIT_PROMPT;
	rdbp[3].vValue.vt = VT_I2;
	rdbp[3].vValue.iVal = DBPROMPT_COMPLETE;

	if (pdbprops->SetProperties(1, &rdbpset) == S_OK)
	{
#ifdef DSO_MSDAIPP_USE_DAVONLY
		BSTR  bstrClsid = SysAllocString(L"{9FECD570-B9D4-11D1-9C78-0000F875AC61}");
		if (bstrClsid)
		{
			rdbpset.cProperties = 1; 
			rdbpset.guidPropertySet = DBPROPSET_MSDAIPP_INIT;
			rdbpset.rgProperties = rdbp;

			rdbp[0].dwPropertyID = 6; // DBPROP_INIT_PROTOCOLPROVIDER;
			rdbp[0].vValue.vt = VT_BSTR;
			rdbp[0].vValue.bstrVal = bstrClsid;

			hr = pdbprops->SetProperties(1, &rdbpset);
			SysFreeString(bstrClsid);
		}
#endif
		hr = pdbprops->QueryInterface(IIDX_IBindResource, (void**)&pres);
	}

	SAFE_FREEBSTR(bstrLock);
	pdbprops->Release();

	return (IUnknown*)pres;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::IPPDownloadWebResource (protected)
//
//  Downloads the file specified by the URL to the given temp file. Locks
//  the web resource for editing if ppstmKeepForSave is requested.
//
STDMETHODIMP MSOObject::IPPDownloadWebResource(LPWSTR pwszURL, LPWSTR pwszFile, IStream** ppstmKeepForSave)
{
    HRESULT hr = E_UNEXPECTED;
    IStream* pstm = nullptr;
    IBindResource* pres = nullptr;
    BYTE* rgbBuf;
    DWORD dwStatus, dwBindFlags;
    ULONG cbRead, cbWritten;
    HANDLE hFile;

    // Sanity check...
    if ((pwszURL == nullptr) || (pwszFile == nullptr) || (ppstmKeepForSave == nullptr))
        return E_INVALIDARG;

    // We need MSDAIPP for full write access...
    if ((m_punkIPPResource) == 0 && (m_punkIPPResource = CreateIPPBindResource()) == 0)
        return DSO_E_REQUIRESMSDAIPP;

	rgbBuf = new BYTE[10240]; //a 10-k buffer for reading
	if (!rgbBuf) return E_OUTOFMEMORY;

	// Use IBindResource::Bind to open an IStream and copy out the date to
	// the file given. This will then be used to load the object from the file...
    if (SUCCEEDED(m_punkIPPResource->QueryInterface(IIDX_IBindResource, (void**)&pres))) {
        dwBindFlags
            = (DBBINDURLFLAG_READ | DBBINDURLFLAG_WRITE | DBBINDURLFLAG_OUTPUT | DBBINDURLFLAG_SHARE_DENY_WRITE);
    callagain:
        if (SUCCEEDED(hr = pres->Bind(nullptr, pwszURL, dwBindFlags, DBGUIDX_STREAM, IID_IStream,
                          (IAuthenticate*)&m_xAuthenticate, nullptr, &dwStatus, (IUnknown**)&pstm))) {
            LARGE_INTEGER lintStart;
            lintStart.QuadPart = 0;
            hr = pstm->Seek(lintStart, STREAM_SEEK_SET, nullptr);

            if (FOpenLocalFile(pwszFile, GENERIC_WRITE, 0, CREATE_ALWAYS, &hFile)) {
                while (SUCCEEDED(hr)) {
                    if (FAILED(hr = pstm->Read((void*)rgbBuf, 10240, &cbRead)) || (cbRead == 0))
                        break;

                    if (FALSE == WriteFile(hFile, rgbBuf, cbRead, &cbWritten, nullptr)) {
                        hr = E_WIN32_LASTERROR;
                        break;
                    }
                }

                CloseHandle(hFile);
            } else
                hr = E_WIN32_LASTERROR;

            SAFE_SET_INTERFACE(*ppstmKeepForSave, pstm);

            pstm->Release();
        } else if ((hr == DB_E_NOTSUPPORTED) && ((dwBindFlags & DBBINDURLFLAG_OUTPUT) == DBBINDURLFLAG_OUTPUT)) {
            // WEC4 does not support DBBINDURLFLAG_OUTPUT flag, but if we are using WEC
            // we don't really need the flag since this is not an HTTP GET call. Flip the
            // flag off and call the method again to connect to server...
            dwBindFlags &= ~DBBINDURLFLAG_OUTPUT;
            goto callagain;
        }

        pres->Release();
    }

    // Map an OLEDB error to a common "file" error so a user
    // (and VB/VBScript) would better understand...
    if (FAILED(hr))
	{
		switch (hr)
		{
		case DB_E_NOTFOUND:             hr = STG_E_FILENOTFOUND; break;
		case DB_E_READONLY:
		case DB_E_RESOURCELOCKED:       hr = STG_E_LOCKVIOLATION; break;
		case DB_SEC_E_PERMISSIONDENIED:
		case DB_SEC_E_SAFEMODE_DENIED:  hr = E_ACCESSDENIED; break;
		case DB_E_CANNOTCONNECT:
		case DB_E_TIMEOUT:              hr = E_VBA_NOREMOTESERVER; break;
		case E_NOINTERFACE:             hr = E_UNEXPECTED; break;
		}
	}

	delete [] rgbBuf;
	return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::IPPUploadWebResource (protected)
//
//  Uploads the file to a URL. The code can be used two ways:
//
//  1.) If ppstmSave contains a pointer to an existing IStream*, then we
//      just upload to the existing stream. This allows for normal "Save"
//      on an open web resource.
//
//  2.) If ppstmSave is nullptr (or contains a nullptr IStream*), we create a new
//      web resource at the location given by pwszURLSaveTo and save to its
//      stream. If ppstmSave is passed, we return the new IStream* to the
//      caller who can then use it to do the other type of save next time.
//
STDMETHODIMP MSOObject::IPPUploadWebResource(LPWSTR pwszFile, IStream** ppstmSave, LPWSTR pwszURLSaveTo, BOOL fOverwriteFile)
{
    HRESULT hr = E_UNEXPECTED;
    ICreateRow* pcrow = nullptr;
    IStream* pstm = nullptr;
    BYTE* rgbBuf;
    HANDLE hFile;
    BOOL fstmIn = FALSE;
    ULONG       cbRead, cbWritten;
	DWORD       dwStatus, dwBindFlags;

	// We need MSDAIPP for full write access...
	if ((m_punkIPPResource) == 0 && (m_punkIPPResource = CreateIPPBindResource()) == 0)
		return DSO_E_REQUIRESMSDAIPP;

	// Check if this a "Save" on existing IStream* and jump to loop...
	if ((ppstmSave) != 0 && (fstmIn = (BOOL)(pstm = *ppstmSave)) != 0)
		goto uploadfrominstm;

	// Check the URL string and ask for ICreateRow (to make new web resource)...
    if (!(pwszURLSaveTo) || !LooksLikeHTTP(pwszURLSaveTo)
        || FAILED(m_punkIPPResource->QueryInterface(IIDX_ICreateRow, (void**)&pcrow)))
        return hr;

    dwBindFlags = (DBBINDURLFLAG_READ | DBBINDURLFLAG_WRITE | DBBINDURLFLAG_SHARE_DENY_WRITE
        | (fOverwriteFile ? DBBINDURLFLAG_OVERWRITE : 0));

    if (SUCCEEDED(hr = pcrow->CreateRow(nullptr, pwszURLSaveTo, dwBindFlags, DBGUIDX_STREAM, IID_IStream,
                      (IAuthenticate*)&m_xAuthenticate, nullptr, &dwStatus, nullptr, (IUnknown**)&pstm))) {
        // Once we are here, we have a stream (either handed in or opened from above).
        // We just loop through and read from the file to the stream...
    uploadfrominstm:
        if ((rgbBuf = new BYTE[10240]) != 0) // a 10-k buffer for reading
        {
            if (FOpenLocalFile(pwszFile, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &hFile)) {
                LARGE_INTEGER lintStart;
                lintStart.QuadPart = 0;
                hr = pstm->Seek(lintStart, STREAM_SEEK_SET, nullptr);

                while (SUCCEEDED(hr)) {
                    if (FALSE == ReadFile(hFile, rgbBuf, 10240, &cbRead, nullptr)) {
                        hr = E_WIN32_LASTERROR;
                        break;
                    }

                    if (0 == cbRead)
                        break;

                    if (FAILED(hr = pstm->Write((void*)rgbBuf, cbRead, &cbWritten)))
                        break;
                }

                // Need to commit the changes to make it official...
                if (SUCCEEDED(hr))
                    hr = pstm->Commit(STGC_DEFAULT);

                CloseHandle(hFile);
            } else
                hr = E_WIN32_LASTERROR;

            delete[] rgbBuf;
        } else
            hr = E_OUTOFMEMORY;

        // If we are not using a passed in IStream (and therefore created one), we
		// should AddRef and pass back (if caller asked us to)...
		if (!fstmIn)
		{
			if (SUCCEEDED(hr) && (ppstmSave) && (!(*ppstmSave)))
			{
				SAFE_SET_INTERFACE(*ppstmSave, pstm);
            }
            pstm->Release();
        }
    }

    // Map an OLEDB error to a common "file" error so a user
    // (and VB/VBScript) would better understand...
    if (FAILED(hr)) {
        switch (hr) {
            case DB_E_RESOURCEEXISTS:
                hr = STG_E_FILEALREADYEXISTS;
                break;
            case DB_E_NOTFOUND:
                hr = STG_E_PATHNOTFOUND;
                break;
            case DB_E_READONLY:
            case DB_E_RESOURCELOCKED:
                hr = STG_E_LOCKVIOLATION;
                break;
            case DB_SEC_E_PERMISSIONDENIED:
            case DB_SEC_E_SAFEMODE_DENIED:
                hr = E_ACCESSDENIED;
                break;
            case DB_E_CANNOTCONNECT:
            case DB_E_TIMEOUT:
                hr = E_VBA_NOREMOTESERVER;
                break;
            case DB_E_OUTOFSPACE:
                hr = STG_E_MEDIUMFULL;
                break;
            case E_NOINTERFACE:
                hr = E_UNEXPECTED;
                break;
        }
    }

    if (pcrow)
        pcrow->Release();

	return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::TurnOffWebToolbar (protected)
//
//  This function "turns off" the Web toolbar used by Office apps to 
//  do in-site navigation. The problem is when toggling tools off the
//  bar may still be visible, so we have to explicitly turn it off if
//  we want a true "no tool" state.
//
//  This should *only* be called when turnning off the bars after object
//  if inplace active. The call to OLECMDID_HIDETOOLBARS should hide all
//  the primay toolbars, so we are just catching those attached to the 
//  document collection itself that are not in primary set.
//
STDMETHODIMP_(void) MSOObject::TurnOffWebToolbar(BOOL fTurnedOff)
{
    IDispatch* pdisp;
    VARIANT vtT[5];
    for (int i = 0; i < SIZEOFARRAY(vtT); i++)
        VariantInit(&vtT[i]);

    ODS("MSOObject::TurnOffWebToolbar()\n");

    // Can't change toolbar state in print preview...
    if (InPrintPreview())
        return;

    if ((m_pipactive) && (SUCCEEDED(m_pipactive->QueryInterface(IID_IDispatch, (void**)&pdisp))) && (pdisp)) {
        // Get the commandbars on the document object....
        if (SUCCEEDED(DsoDispatchInvoke(
                pdisp, const_cast<wchar_t*>(L"CommandBars"), 0, DISPATCH_PROPERTYGET, 0, nullptr, &vtT[0]))) {
            // Turn off Web toolbar...
            assert(vtT[0].vt == VT_DISPATCH);
            vtT[1].vt = VT_BSTR;
            vtT[1].bstrVal = SysAllocString(L"Web");

            if (SUCCEEDED(DsoDispatchInvoke(
                    vtT[0].pdispVal, const_cast<wchar_t*>(L"Item"), 0, DISPATCH_PROPERTYGET, 1, &vtT[1], &vtT[2]))) {
                assert(vtT[2].vt == VT_DISPATCH);
                vtT[3].vt = VT_BOOL;
                vtT[3].boolVal = 0;
                DsoDispatchInvoke(
                    vtT[2].pdispVal, const_cast<wchar_t*>(L"Visible"), 0, DISPATCH_PROPERTYPUT, 1, &vtT[3], &vtT[2]);
                VariantClear(&vtT[2]);
            }

            VariantClear(&vtT[1]);

            // Word 2002/2003 also displays Reviewing toolbar, so kill it too...
            if (fTurnedOff && IsWordObject()) {
                assert(vtT[0].vt == VT_DISPATCH);
                vtT[1].vt = VT_BSTR;
                vtT[1].bstrVal = SysAllocString(L"Reviewing");

                if (SUCCEEDED(DsoDispatchInvoke(vtT[0].pdispVal, const_cast<wchar_t*>(L"Item"), 0, DISPATCH_PROPERTYGET,
                        1, &vtT[1], &vtT[2]))) {
                    assert(vtT[2].vt == VT_DISPATCH);
                    vtT[3].vt = VT_BOOL;
                    vtT[3].boolVal = 0;
                    DsoDispatchInvoke(vtT[2].pdispVal, const_cast<wchar_t*>(L"Visible"), 0, DISPATCH_PROPERTYPUT, 1,
                        &vtT[3], &vtT[2]);
                    VariantClear(&vtT[2]);
                }

                VariantClear(&vtT[1]);
            }

            VariantClear(&vtT[0]);
        }

        pdisp->Release();
    }
}

////////////////////////////////////////////////////////////////////////
// MSOObject::ClearMergedMenu (protected)
//
//  Frees the merged menu set by host.
//
STDMETHODIMP_(void) MSOObject::ClearMergedMenu()
{	
	if (m_hMenuMerged)
	{
		int cbMenuCnt = GetMenuItemCount(m_hMenuMerged);
        for (int i = cbMenuCnt; i >= 0; --i)
            RemoveMenu(m_hMenuMerged, i, MF_BYPOSITION);

        //        MyDestroyMenu(m_hMenuMerged);
        m_hMenuMerged = nullptr;
    }
}

////////////////////////////////////////////////////////////////////////
// MSOObject::CalcDocNameIndex (protected)
//
//  Calculates position of the name portion of the full path string.
//
STDMETHODIMP_(DWORD) MSOObject::CalcDocNameIndex(LPCWSTR pwszPath)
{
	DWORD cblen, idx = 0;
	if ((pwszPath) && ((cblen = lstrlenW(pwszPath)) > 1))
	{
		for (idx = cblen; idx > 0; --idx)
		{
			if (pwszPath[idx] == L'\\')
				break;
		}

		if ((idx) && !(++idx < cblen))
			idx = 0;
	}
	return idx;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::OnDraw (protected)
//
//  Site drawing (does nothing in this version).
//
STDMETHODIMP_(void) MSOObject::OnDraw(DWORD /*dvAspect*/, HDC /*hdcDraw*/, LPRECT /*prcBounds*/, LPRECT /*prcWBounds*/, HDC /*hicTargetDev*/, BOOL /*fOptimize*/)
{
	// Don't have to draw anything, object does all this because we are
	// always UI active. If we allowed for multiple objects and had some
	// non-UI active, we would have to do some drawing, but that will not
	// happen in this sample.
}


////////////////////////////////////////////////////////////////////////
//
// ActiveX Document Site Interfaces
//

////////////////////////////////////////////////////////////////////////
//
// MSOObject IUnknown Interface Methods
//
//   STDMETHODIMP         QueryInterface(REFIID riid, void ** ppv);
//   STDMETHODIMP_(ULONG) AddRef(void);
//   STDMETHODIMP_(ULONG) Release(void);
//
STDMETHODIMP MSOObject::QueryInterface(REFIID riid, void** ppv)
{
    ODS("MSOObject::QueryInterface\n");
    CHECK_NULL_RETURN(ppv, E_POINTER);

    HRESULT hr = S_OK;

    if (IID_IUnknown == riid) {
        *ppv = (IUnknown*)this;
    } else if (IID_IOleClientSite == riid) {
        *ppv = (IOleClientSite*)&m_xOleClientSite;
    } else if ((IID_IOleInPlaceSite == riid) || (IID_IOleWindow == riid)) {
        *ppv = (IOleInPlaceSite*)&m_xOleInPlaceSite;
    } else if (IID_IOleDocumentSite == riid) {
        *ppv = (IOleDocumentSite*)&m_xOleDocumentSite;
    } else if ((IID_IOleInPlaceFrame == riid) || (IID_IOleInPlaceUIWindow == riid)) {
        *ppv = (IOleInPlaceFrame*)&m_xOleInPlaceFrame;
    } else if (IID_IOleCommandTarget == riid) {
        *ppv = (IOleCommandTarget*)&m_xOleCommandTarget;
    } else if (IIDX_IAuthenticate == riid) {
        *ppv = (IAuthenticate*)&m_xAuthenticate;
    } else if (IID_IServiceProvider == riid) {
        *ppv = (IServiceProvider*)&m_xServiceProvider;
    } else if (IID_IContinueCallback == riid) {
        *ppv = (IContinueCallback*)&m_xContinueCallback;
    } else if (IID_IOlePreviewCallback == riid) {
        *ppv = (IOlePreviewCallback*)&m_xPreviewCallback;
    } else {
        *ppv = nullptr;
        hr = E_NOINTERFACE;
    }

    if (nullptr != *ppv)
        ((IUnknown*)(*ppv))->AddRef();

    return hr;
}

STDMETHODIMP_(ULONG) MSOObject::AddRef(void)
{
	TRACE1("MSOObject::AddRef - %d\n", m_cRef + 1);
	return ++m_cRef;
}

STDMETHODIMP_(ULONG) MSOObject::Release(void)
{
	TRACE1("MSOObject::Release - %d\n", m_cRef - 1);
	return --m_cRef;
}


////////////////////////////////////////////////////////////////////////
//
// MSOObject::XOleClientSite -- IOleClientSite Implementation
//
//	 STDMETHODIMP SaveObject(void);
//	 STDMETHODIMP GetMoniker(DWORD dwAssign, DWORD dwWhich, LPMONIKER* ppmk);
//	 STDMETHODIMP GetContainer(LPOLECONTAINER* ppContainer);
//	 STDMETHODIMP ShowObject(void);
//	 STDMETHODIMP OnShowWindow(BOOL fShow);
//	 STDMETHODIMP RequestNewObjectLayout(void);
//
IMPLEMENT_INTERFACE_UNKNOWN(MSOObject, OleClientSite)

STDMETHODIMP MSOObject::XOleClientSite::SaveObject(void)
{
	ODS("MSOObject::XOleClientSite::SaveObject\n");
	return S_OK;
}

STDMETHODIMP MSOObject::XOleClientSite::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker** ppmk)
{
    HRESULT hr = OLE_E_CANT_GETMONIKER;
    METHOD_PROLOGUE(MSOObject, OleClientSite);

    TRACE2("MSOObject::XOleClientSite::GetMoniker(%d, %d)\n", dwAssign, dwWhichMoniker);
    CHECK_NULL_RETURN(ppmk, E_POINTER);
    *ppmk = nullptr;

    // Provide moniker for object opened by moniker...
    switch (dwWhichMoniker) {
        case OLEWHICHMK_OBJREL:
        case OLEWHICHMK_OBJFULL: {
            if (pThis->m_pmkSourceFile)
			{
				*ppmk = pThis->m_pmkSourceFile;
				hr = S_OK;
			}
			else if (dwAssign == OLEGETMONIKER_FORCEASSIGN)
			{
				// TODO: Should we allow frce create of moniker if we don't start with it?
			}
        } break;
    }

    // Need to AddRef returned value to caller on success...
    if ((hr == S_OK) && (*ppmk))
		((IUnknown*)*ppmk)->AddRef();

	return hr;
}

STDMETHODIMP MSOObject::XOleClientSite::GetContainer(IOleContainer** ppContainer)
{
    ODS("MSOObject::XOleClientSite::GetContainer\n");
    if (ppContainer)
        *ppContainer = nullptr;
    return E_NOINTERFACE;
}

STDMETHODIMP MSOObject::XOleClientSite::ShowObject(void)
{
	ODS("MSOObject::XOleClientSite::ShowObject\n");
	return S_OK;
}

STDMETHODIMP MSOObject::XOleClientSite::OnShowWindow(BOOL /*fShow*/)
{
	ODS("MSOObject::XOleClientSite::OnShowWindow\n");
	return S_OK;
}

STDMETHODIMP MSOObject::XOleClientSite::RequestNewObjectLayout(void)
{
	ODS("MSOObject::XOleClientSite::RequestNewObjectLayout\n");
	return S_OK;
}

////////////////////////////////////////////////////////////////////////
//
// MSOObject::XOleInPlaceSite -- IOleInPlaceSite Implementation
//
//	 STDMETHODIMP GetWindow(HWND* phWnd);
//	 STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);
//	 STDMETHODIMP CanInPlaceActivate(void);
//	 STDMETHODIMP OnInPlaceActivate(void);
//	 STDMETHODIMP OnUIActivate(void);
//	 STDMETHODIMP GetWindowContext(LPOLEINPLACEFRAME* ppIIPFrame, LPOLEINPLACEUIWINDOW* ppIIPUIWindow, LPRECT prcPos, LPRECT prcClip, LPOLEINPLACEFRAMEINFO pFI);
//	 STDMETHODIMP Scroll(SIZE sz);
//	 STDMETHODIMP OnUIDeactivate(BOOL fUndoable);
//	 STDMETHODIMP OnInPlaceDeactivate(void);
//	 STDMETHODIMP DiscardUndoState(void);
//	 STDMETHODIMP DeactivateAndUndo(void);
//	 STDMETHODIMP OnPosRectChange(LPCRECT prcPos);
//
IMPLEMENT_INTERFACE_UNKNOWN(MSOObject, OleInPlaceSite)

STDMETHODIMP MSOObject::XOleInPlaceSite::GetWindow(HWND* phwnd)
{
	METHOD_PROLOGUE(MSOObject, OleInPlaceSite);
	ODS("MSOObject::XOleInPlaceSite::GetWindow\n");
	if (phwnd) *phwnd = pThis->m_hwnd;
	return S_OK;
}

STDMETHODIMP MSOObject::XOleInPlaceSite::ContextSensitiveHelp(BOOL /*fEnterMode*/)
{
	ODS("MSOObject::XOleInPlaceSite::ContextSensitiveHelp\n");
	return S_OK;
}

STDMETHODIMP MSOObject::XOleInPlaceSite::CanInPlaceActivate(void)
{
	ODS("MSOObject::XOleInPlaceSite::CanInPlaceActivate\n");
	return S_OK;
}

STDMETHODIMP MSOObject::XOleInPlaceSite::OnInPlaceActivate(void)
{
	METHOD_PROLOGUE(MSOObject, OleInPlaceSite);
	ODS("MSOObject::XOleInPlaceSite::OnInPlaceActivate\n");

	if ((!pThis->m_pole) || 
		FAILED(pThis->m_pole->QueryInterface(IID_IOleInPlaceObject, (void **)&(pThis->m_pipobj))))
		return E_UNEXPECTED;

	pThis->m_fObjectIPActive = TRUE;
	return S_OK;
}

STDMETHODIMP MSOObject::XOleInPlaceSite::OnUIActivate(void)
{
	METHOD_PROLOGUE(MSOObject, OleInPlaceSite);
	ODS("MSOObject::XOleInPlaceSite::OnUIActivate\n");
	pThis->m_fObjectUIActive = TRUE;
	pThis->m_pipobj->GetWindow(&(pThis->m_hwndIPObject));
	return S_OK;
}

STDMETHODIMP MSOObject::XOleInPlaceSite::GetWindowContext(IOleInPlaceFrame** ppFrame,
														  IOleInPlaceUIWindow** ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
	METHOD_PROLOGUE(MSOObject, OleInPlaceSite);
    ODS("MSOObject::XOleInPlaceSite::GetWindowContext\n");

    if (ppFrame) {
        SAFE_SET_INTERFACE(*ppFrame, &(pThis->m_xOleInPlaceFrame));
    }

    if (ppDoc)
        *ppDoc = nullptr;

    if (lprcPosRect)
        *lprcPosRect = pThis->m_rcViewRect;

    if (lprcClipRect)
        *lprcClipRect = *lprcPosRect;

	memset(lpFrameInfo, 0, sizeof(OLEINPLACEFRAMEINFO));
	lpFrameInfo->cb = sizeof(OLEINPLACEFRAMEINFO);
	lpFrameInfo->hwndFrame = pThis->m_hwnd;
	return S_OK;
}

STDMETHODIMP MSOObject::XOleInPlaceSite::Scroll(SIZE /*sz*/)
{
	ODS("MSOObject::XOleInPlaceSite::Scroll\n");
	return S_OK;
}

STDMETHODIMP MSOObject::XOleInPlaceSite::OnUIDeactivate(BOOL /*fUndoable*/)
{
    METHOD_PROLOGUE(MSOObject, OleInPlaceSite);
    ODS("MSOObject::XOleInPlaceSite::OnUIDeactivate\n");

    pThis->m_fObjectUIActive = FALSE;
    pThis->m_xOleInPlaceFrame.SetMenu(nullptr, nullptr, nullptr);
    SetFocus(pThis->m_hwnd);

    return S_OK;
}

STDMETHODIMP MSOObject::XOleInPlaceSite::OnInPlaceDeactivate(void)
{
    METHOD_PROLOGUE(MSOObject, OleInPlaceSite);
    ODS("MSOObject::XOleInPlaceSite::OnInPlaceDeactivate\n");

    pThis->m_fObjectIPActive = FALSE;
    pThis->m_hwndIPObject = nullptr;
    SAFE_RELEASE_INTERFACE((pThis->m_pipobj));

    pThis->m_fAttemptPptPreview = FALSE;
    pThis->m_fObjectActivateComplete = FALSE;
    return S_OK;
}

STDMETHODIMP MSOObject::XOleInPlaceSite::DiscardUndoState(void)
{
	ODS("MSOObject::XOleInPlaceSite::DiscardUndoState\n");
	return S_OK;
}

STDMETHODIMP MSOObject::XOleInPlaceSite::DeactivateAndUndo(void)
{
	METHOD_PROLOGUE(MSOObject, OleInPlaceSite);
	ODS("MSOObject::XOleInPlaceSite::DeactivateAndUndo\n");
	if (pThis->m_pipobj) pThis->m_pipobj->InPlaceDeactivate();
	return S_OK;
}

STDMETHODIMP MSOObject::XOleInPlaceSite::OnPosRectChange(LPCRECT /*lprcPosRect*/)
{
	ODS("MSOObject::XOleInPlaceSite::OnPosRectChange\n");
	return S_OK;
}

////////////////////////////////////////////////////////////////////////
//
// MSOObject::XOleDocumentSite -- IOleDocumentSite Implementation
//
//	 STDMETHODIMP ActivateMe(IOleDocumentView* pView);
//
IMPLEMENT_INTERFACE_UNKNOWN(MSOObject, OleDocumentSite)

STDMETHODIMP MSOObject::XOleDocumentSite::ActivateMe(IOleDocumentView* pView)
{
	METHOD_PROLOGUE(MSOObject, OleDocumentSite);
	Sleep(1000); // #71718
    ODS("MSOObject::XOleDocumentSite::ActivateMe\n");

    HRESULT hr = E_FAIL;
    IOleDocument* pmsodoc;

    // If we're passed a nullptr view pointer, then try to get one from
    // the document object (the object within us).
    if (pView) {
        // Make sure that the view has our client site
        hr = pView->SetInPlaceSite((IOleInPlaceSite*)&(pThis->m_xOleInPlaceSite));

        pView->AddRef(); // we will be keeping the object if successful..
    } else if (pThis->m_pole) {
        // Create a new view from the OleDocument...
        if (FAILED(pThis->m_pole->QueryInterface(IID_IOleDocument, (void**)&pmsodoc)))
            return E_FAIL;

        hr = pmsodoc->CreateView((IOleInPlaceSite*)&(pThis->m_xOleInPlaceSite), pThis->m_pstmview, 0, &pView);

        pmsodoc->Release();
    }

    // If we have the view, apply view state...
    if (SUCCEEDED(hr) && (pThis->m_pstmview))
        hr = pView->ApplyViewState(pThis->m_pstmview);

	// If any of the above failed, release the view and return...
	if (FAILED(hr))
	{
		SAFE_RELEASE_INTERFACE(pView);
		return hr;
	}

	// keep the view pointer...
	pThis->m_pdocv = pView;

	// Get a command target (if available)...
	pView->QueryInterface(IID_IOleCommandTarget, (void**)&(pThis->m_pcmdt));

	// Make sure that the view has our client site
	pView->SetInPlaceSite((IOleInPlaceSite*)&(pThis->m_xOleInPlaceSite));

	// This sets up toolbars and menus first    
	if (SUCCEEDED(hr = pView->UIActivate(TRUE)))
	{
		// Set the window size sensitive to new toolbars
		pView->SetRect(&(pThis->m_rcViewRect));

		// Makes it all active
		pView->Show(TRUE);

		pThis->m_fAppWindowActive = TRUE;
		pThis->m_fObjectActivateComplete = TRUE;

		// Toogle tools off if that's what user wants...
		if (!(pThis->m_fDisplayTools))
		{
			pThis->m_fDisplayTools = TRUE;
			pThis->OnNotifyChangeToolState(FALSE);
		}
	}

	return hr;
}

////////////////////////////////////////////////////////////////////////
//
// MSOObject::XOleInPlaceFrame -- IOleInPlaceFrame Implementation
//
//   STDMETHODIMP GetWindow(HWND* phWnd);
//   STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);
//   STDMETHODIMP GetBorder(LPRECT prcBorder);
//   STDMETHODIMP RequestBorderSpace(LPCBORDERWIDTHS pBW);
//   STDMETHODIMP SetBorderSpace(LPCBORDERWIDTHS pBW);
//   STDMETHODIMP SetActiveObject(LPOLEINPLACEACTIVEOBJECT pIIPActiveObj, LPCOLESTR pszObj);
//   STDMETHODIMP InsertMenus(HMENU hMenu, LPOLEMENUGROUPWIDTHS pMGW);
//   STDMETHODIMP SetMenu(HMENU hMenu, HOLEMENU hOLEMenu, HWND hWndObj);
//   STDMETHODIMP RemoveMenus(HMENU hMenu);
//   STDMETHODIMP SetStatusText(LPCOLESTR pszText);
//   STDMETHODIMP EnableModeless(BOOL fEnable);
//   STDMETHODIMP TranslateAccelerator(LPMSG pMSG, WORD wID);
//
IMPLEMENT_INTERFACE_UNKNOWN(MSOObject, OleInPlaceFrame)

STDMETHODIMP MSOObject::XOleInPlaceFrame::GetWindow(HWND* phWnd)
{
	METHOD_PROLOGUE(MSOObject, OleInPlaceFrame);
	ODS("MSOObject::XOleInPlaceFrame::GetWindow\n");
	return pThis->m_xOleInPlaceSite.GetWindow(phWnd);
}

STDMETHODIMP MSOObject::XOleInPlaceFrame::ContextSensitiveHelp(BOOL fEnterMode)
{
	METHOD_PROLOGUE(MSOObject, OleInPlaceFrame);
	ODS("MSOObject::XOleInPlaceFrame::ContextSensitiveHelp\n");
	return pThis->m_xOleInPlaceSite.ContextSensitiveHelp(fEnterMode);
}


STDMETHODIMP MSOObject::XOleInPlaceFrame::GetBorder(LPRECT prcBorder)
{
    METHOD_PROLOGUE(MSOObject, OleInPlaceFrame);
    ODS("MSOObject::XOleInPlaceFrame::GetBorder\n");
    CHECK_NULL_RETURN(prcBorder, E_POINTER);

    // If we don't allow Toolspace, and we are already active, give
    // no space for tools (ie, hide toolabrs), otherwise give as much we can...
    if (!(pThis->m_fDisplayTools) && (pThis->m_pipactive))
        SetRectEmpty(prcBorder);
    else
		GetClientRect(pThis->m_hwnd, prcBorder);

	TRACE_LPRECT("prcBorder", prcBorder);

	return S_OK;
}

STDMETHODIMP MSOObject::XOleInPlaceFrame::RequestBorderSpace(LPCBORDERWIDTHS pBW)
{
    ODS("MSOObject::XOleInPlaceFrame::RequestBorderSpace\n");
    CHECK_NULL_RETURN(pBW, E_POINTER);
    TRACE_LPRECT("pBW", pBW);
    return S_OK;
}

STDMETHODIMP MSOObject::XOleInPlaceFrame::SetBorderSpace(LPCBORDERWIDTHS pBW)
{
	RECT rc;

	METHOD_PROLOGUE(MSOObject, OleInPlaceFrame);
	ODS("MSOObject::XOleInPlaceFrame::SetBorderSpace\n");

	if( !pThis->GetUpdateDoc() )
		return S_OK;

	if (pBW){TRACE_LPRECT("pBW", pBW);}

	GetClientRect(pThis->m_hwnd, &rc);
	SetRectEmpty((RECT*)&(pThis->m_bwToolSpace));

	if (pBW)
	{
		pThis->m_bwToolSpace = *pBW;
		rc.left   += pBW->left;   rc.right  -= pBW->right;
		rc.top    += pBW->top;    rc.bottom -= pBW->bottom;
	}

	// Save the current view RECT (space minus tools)...
	pThis->m_rcViewRect = rc;

	// Update the active document (if alive)...
	if (pThis->m_pdocv)
		pThis->m_pdocv->SetRect(&(pThis->m_rcViewRect));

	return S_OK;
}

STDMETHODIMP MSOObject::XOleInPlaceFrame::SetActiveObject(LPOLEINPLACEACTIVEOBJECT pIIPActiveObj, LPCOLESTR /*pszObj*/)
{
    METHOD_PROLOGUE(MSOObject, OleInPlaceFrame);
    ODS("MSOObject::XOleInPlaceFrame::SetActiveObject\n");

    SAFE_RELEASE_INTERFACE((pThis->m_pipactive));
    pThis->m_hwndUIActiveObj = nullptr;
    pThis->m_dwObjectThreadID = 0;

    if (pIIPActiveObj) {
        SAFE_SET_INTERFACE(pThis->m_pipactive, pIIPActiveObj);
        pIIPActiveObj->GetWindow(&(pThis->m_hwndUIActiveObj));
        pThis->m_dwObjectThreadID = GetWindowThreadProcessId(pThis->m_hwndUIActiveObj, nullptr);
    }

    return S_OK;
}

STDMETHODIMP MSOObject::XOleInPlaceFrame::InsertMenus(HMENU /*hMenu*/, LPOLEMENUGROUPWIDTHS /*pMGW*/)
{
	ODS("MSOObject::XOleInPlaceFrame::InsertMenus\n");
	return S_OK;
}

STDMETHODIMP MSOObject::XOleInPlaceFrame::SetMenu(HMENU hMenu, HOLEMENU hOLEMenu, HWND hWndObj)
{
	METHOD_PROLOGUE(MSOObject, OleInPlaceFrame);
	ODS("MSOObject::XOleInPlaceFrame::SetMenu\n");

	//pThis->CheckForPPTPreviewChange();

	// We really don't do anything here. We will merge the menu and set it
	// later if the menubar is visible or user chooses dropdown from titlebar.
	// All we do is stash current values or clear them depending on call.
	if (hMenu)
	{
		pThis->m_hMenuActive = hMenu;
		pThis->m_holeMenu = hOLEMenu;
        pThis->m_hwndMenuObj = hWndObj;

        // Handle a special case where menu is being updated after initial set
        // and not in close. In such a case we should force redraw control we
        // are in so new menu changes are visible as soon as possible...
        if ((!(pThis->m_fInClose)) && (pThis->m_hwndCtl))
            InvalidateRect(pThis->m_hwndCtl, nullptr, FALSE);
    } else {
        pThis->m_hMenuActive = nullptr;
        pThis->m_holeMenu = nullptr;
        pThis->m_hwndMenuObj = nullptr;
    }

    // Regardless of call, make sure to cleanup a merged menu if
    // one was previously created for the control...
    pThis->ClearMergedMenu();

    return S_OK;
}

STDMETHODIMP MSOObject::XOleInPlaceFrame::RemoveMenus(HMENU /*hMenu*/)
{
	ODS("MSOObject::XOleInPlaceFrame::RemoveMenus\n");
	return S_OK;
}

STDMETHODIMP MSOObject::XOleInPlaceFrame::SetStatusText(LPCOLESTR pszText)
{
	METHOD_PROLOGUE(MSOObject, OleInPlaceFrame);
	ODS("MSOObject::XOleInPlaceFrame::SetStatusText\n");
	if ((pszText) && (*pszText)){TRACE1(" Status Text = %S \n", pszText);}
	return ((pThis->m_psiteCtl) ? pThis->m_psiteCtl->SetStatusText(pszText) : S_OK);
}

STDMETHODIMP MSOObject::XOleInPlaceFrame::EnableModeless(BOOL fEnable)
{
	METHOD_PROLOGUE(MSOObject, OleInPlaceFrame);
	TRACE1("MSOObject::XOleInPlaceFrame::EnableModeless(%d)\n", fEnable);
	pThis->m_fObjectInModalCondition = !fEnable;
	SendMessage(pThis->m_hwndCtl, DSO_WM_ASYNCH_STATECHANGE, DSO_STATE_MODAL, (LPARAM)fEnable);
	return S_OK;
}

STDMETHODIMP MSOObject::XOleInPlaceFrame::TranslateAccelerator(LPMSG /*pMSG*/, WORD /*wID*/)
{
	ODS("MSOObject::XOleInPlaceFrame::TranslateAccelerator\n");
	return S_FALSE;
}

////////////////////////////////////////////////////////////////////////
//
// MSOObject::XOleCommandTarget -- IOleCommandTarget Implementation
//
//   STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText);
//   STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvaIn, VARIANTARG *pvaOut);            
//
IMPLEMENT_INTERFACE_UNKNOWN(MSOObject, OleCommandTarget)

STDMETHODIMP MSOObject::XOleCommandTarget::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    METHOD_PROLOGUE(MSOObject, OleCommandTarget);

    if (pguidCmdGroup != nullptr)
        return OLECMDERR_E_UNKNOWNGROUP;

    if (prgCmds == nullptr)
        return E_INVALIDARG;

    for (int i = 0; i < (int)cCmds; i++) {
        switch (prgCmds[i].cmdID) {
            case OLECMDID_NEW:
            case OLECMDID_OPEN:
            case OLECMDID_SAVE:
            case OLECMDID_SAVEAS:
            case OLECMDID_PRINT:
            case OLECMDID_PAGESETUP:
            case OLECMDID_PROPERTIES:
            case OLECMDID_PRINTPREVIEW:
                prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                break;
            default:
                prgCmds[i].cmdf = 0;
                break;
        }
    }

    if (pCmdText) {
        pCmdText->cmdtextf = MSOCMDTEXTF_NONE;
		pCmdText->cwActual = 0;
    }

    return S_OK;
}

STDMETHODIMP MSOObject::XOleCommandTarget::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG * /*pvaIn*/, VARIANTARG * /*pvaOut*/)
{
    METHOD_PROLOGUE(MSOObject, OleCommandTarget);

    ODS("CDsoFramerControl::XOleCommandTarget::Exec\n");

    if ((pguidCmdGroup != nullptr) && (*pguidCmdGroup != GUID_NULL))
        return OLECMDERR_E_UNKNOWNGROUP;

    if (nCmdexecopt == MSOCMDEXECOPT_SHOWHELP)
        return OLECMDERR_E_NOHELP;

    switch (nCmdID)
	{
	case OLECMDID_NEW:
	case OLECMDID_OPEN:
	case OLECMDID_SAVE:
	case OLECMDID_SAVEAS:
	case OLECMDID_PRINT:
	case OLECMDID_PAGESETUP:
	case OLECMDID_PROPERTIES:
	case OLECMDID_PRINTPREVIEW:
		/*
		Ignore command
		PostMessage(pThis->m_hwnd, DSO_WM_ASYNCH_OLECOMMAND, nCmdID, nCmdexecopt);
		*/
		return S_OK;
	}

	return OLECMDERR_E_NOTSUPPORTED;
}

////////////////////////////////////////////////////////////////////////
//
// MSOObject::XServiceProvider -- IServiceProvider Implementation
//
//   STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);
//
IMPLEMENT_INTERFACE_UNKNOWN(MSOObject, ServiceProvider)

STDMETHODIMP MSOObject::XServiceProvider::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
	METHOD_PROLOGUE(MSOObject, ServiceProvider);
	ODS("MSOObject::XServiceProvider::QueryService\n");

	if (pThis->m_psiteCtl) // Forward to host control/container...
		return pThis->m_psiteCtl->QueryService(guidService, riid, ppv);

	return E_NOINTERFACE;
}

////////////////////////////////////////////////////////////////////////
//
// MSOObject::XAuthenticate -- IAuthenticate Implementation
//
//   STDMETHODIMP Authenticate(HWND *phwnd, LPWSTR *pszUsername, LPWSTR *pszPassword);
//
IMPLEMENT_INTERFACE_UNKNOWN(MSOObject, Authenticate)

STDMETHODIMP MSOObject::XAuthenticate::Authenticate(HWND *phwnd, LPWSTR *pszUsername, LPWSTR *pszPassword)
{
	METHOD_PROLOGUE(MSOObject, Authenticate);
	ODS("MSOObject::XAuthenticate::Authenticate\n");
	if (phwnd) *phwnd = ((pThis->m_pwszUsername) ? (HWND)INVALID_HANDLE_VALUE : pThis->m_hwndCtl);
	if (pszUsername) *pszUsername = DsoConvertToLPOLESTR(pThis->m_pwszUsername);
	if (pszPassword) *pszPassword = DsoConvertToLPOLESTR(pThis->m_pwszPassword);
	return S_OK;
}

////////////////////////////////////////////////////////////////////////
//
// MSOObject::XContinueCallback -- IContinueCallback Implementation
//
//   STDMETHODIMP FContinue(void);
//   STDMETHODIMP FContinuePrinting(LONG cPagesPrinted, LONG nCurrentPage, LPOLESTR pwszPrintStatus);
//
IMPLEMENT_INTERFACE_UNKNOWN(MSOObject, ContinueCallback)

STDMETHODIMP MSOObject::XContinueCallback::FContinue(void)
{ // We don't support asynchronous cancel of printing, but if you wanted to add
	// such functionality, this is where you could do so...
	return S_OK;
}

STDMETHODIMP MSOObject::XContinueCallback::FContinuePrinting(LONG cPagesPrinted, LONG nCurrentPage, LPOLESTR pwszPrintStatus)
{
    Q_UNUSED(cPagesPrinted)
    Q_UNUSED(nCurrentPage)
    Q_UNUSED(pwszPrintStatus)

    TRACE3("MSOObject::XContinueCallback::FContinuePrinting(%d, %d, %S)\n", cPagesPrinted, nCurrentPage, pwszPrintStatus);
	return S_OK;
}

////////////////////////////////////////////////////////////////////////
//
// MSOObject::XPreviewCallback -- IPreviewCallback Implementation
//
//   STDMETHODIMP Notify(DWORD wStatus, LONG nLastPage, LPOLESTR pwszPreviewStatus);
//
IMPLEMENT_INTERFACE_UNKNOWN(MSOObject, PreviewCallback)

STDMETHODIMP MSOObject::XPreviewCallback::Notify(DWORD wStatus, LONG nLastPage, LPOLESTR pwszPreviewStatus)
{
    Q_UNUSED(nLastPage)
    Q_UNUSED(pwszPreviewStatus)

    METHOD_PROLOGUE(MSOObject, PreviewCallback);
	TRACE3("MSOObject::XPreviewCallback::Notify(%d, %d, %S)\n", wStatus, nLastPage, pwszPreviewStatus);

	// The only notification we act on is when the preview is done...
	if ((wStatus == NOTIFY_FORCECLOSEPREVIEW) ||
		(wStatus == NOTIFY_FINISHED) ||
		(wStatus == NOTIFY_UNABLETOPREVIEW))
	{
		/*pThis->ExitPrintPreview(FALSE);*/
	}

	return S_OK;
}

////////////////////////////////////////////////////////////////////////
//
// MSOObject::HrGetDataFromObject
//
//  Function designed to give indirect access to the content in an existing
//  loaded object, in a clipboard format specified in vtType. In other words,
//  a call to HrGetDataFromObject("Rich Text Format", [out] array) will get the
//  content of the current document in RTF format and return it as a byte array
//  vector in VB6-safe safearray. This array can be converted to a string for 
//  display or saved to database (etc.) without saving to disk. 
//
//  For this to work, the object embedded must support IDataObject and the clip
//  format that you request.
//
STDMETHODIMP MSOObject::HrGetDataFromObject(VARIANT *pvtType, VARIANT *pvtOutput)
{
    HRESULT hr;
    IDataObject* pdo = nullptr;
    LPWSTR pwszTypeName;
    LPSTR pszFormatName;
    SAFEARRAY* psa;
    VOID HUGEP* prawdata;
    FORMATETC ftc;
    STGMEDIUM stgm;
    LONG cfType;

    if ((pvtType == nullptr) || PARAM_IS_MISSING(pvtType) || (pvtOutput == nullptr) || PARAM_IS_MISSING(pvtOutput))
        return E_INVALIDARG;

    VariantClear(pvtOutput);

    // We take the name and find the right clipformat for the data type...
    pwszTypeName = LPWSTR_FROM_VARIANT(*pvtType);
	if ((pwszTypeName) != 0 && (pszFormatName = DsoConvertToMBCS(pwszTypeName)) != 0)
	{
        cfType = RegisterClipboardFormat(pszFormatName);
        DsoMemFree(pszFormatName);
    } else
        cfType = LONG_FROM_VARIANT(*pvtType, 0);
    CHECK_NULL_RETURN(cfType, E_INVALIDARG);

    // We must be able to get IDataObject for the transfer to work...
    if ((m_pole == nullptr)
        || (FAILED(m_pole->GetClipboardData(0, &pdo)) && FAILED(m_pole->QueryInterface(IID_IDataObject, (void**)&pdo))))
        return OLE_E_CANT_GETMONIKER;

    assert(pdo);
    CHECK_NULL_RETURN(pdo, E_UNEXPECTED);

    // We are going to ask for HGLOBAL data format only. This is majority
    // of the non-binary formats, which should be sufficient here...
    memset(&ftc, 0, sizeof(ftc));
    ftc.cfFormat = (WORD)cfType;
    ftc.dwAspect = DVASPECT_CONTENT;
	ftc.lindex = -1; ftc.tymed = TYMED_HGLOBAL;

	memset(&stgm, 0, sizeof(stgm));
	stgm.tymed = TYMED_HGLOBAL;

	// Ask the object for the data...
	if (SUCCEEDED(hr = pdo->QueryGetData(&ftc)) && 
		SUCCEEDED(hr = pdo->GetData(&ftc, &stgm)))
	{
		ULONG ulSize;
		if ((stgm.tymed == TYMED_HGLOBAL)!=0 && (stgm.hGlobal)!=0 &&
			(ulSize = GlobalSize(stgm.hGlobal))!=0)
		{
			LPVOID lpv = GlobalLock(stgm.hGlobal);
			if (lpv)
			{
				// We will return data as safearray vector of VB6 Byte type...
				psa = SafeArrayCreateVector(VT_UI1, 1, ulSize);
                if (psa) {
                    pvtOutput->vt = VT_ARRAY | VT_UI1;
                    pvtOutput->parray = psa;
                    prawdata = nullptr;

                    if (SUCCEEDED(SafeArrayAccessData(psa, &prawdata))) {
                        memcpy(prawdata, lpv, ulSize);
                        SafeArrayUnaccessData(psa);
                    }
                } else
                    hr = E_OUTOFMEMORY;

                GlobalUnlock(stgm.hGlobal);
            }
			else hr = E_ACCESSDENIED;
		}
		else hr = E_FAIL;

		ReleaseStgMedium(&stgm);
	}

	pdo->Release();
	return hr;
}

////////////////////////////////////////////////////////////////////////
//
// MSOObject::HrSetDataInObject
//
//   Function to take format type and either string or VB6-safe byte array
//   as data and set it into the current object (that is, it will replace the
//   current document content with this content). The fMbcsString flag determines
//   if BSTR passed should be converted to MBCS format before being set. For example,
//   if you previously got data as RTF (MBCS), then converted to Unicode BSTR and
//   passed it back, it would need to be converted back to MBCS for the set to work.
//   If the string data is binary, pass FALSE. If data is array, the param is ignored.
//
//   The function sets the data using IDataObject::SetData. Note that some objects 
//   will allow you to get a format but not set it, so this may not work for a previously
//   acquired data type. It is up to the host DocObject server, so test carefully before
//   depending on this functionality.
//
STDMETHODIMP MSOObject::HrSetDataInObject(VARIANT *pvtType, VARIANT *pvtInput, BOOL fMbcsString)
{
    HRESULT hr;
    IDataObject* pdo = nullptr;
    LPWSTR pwszTypeName;
    LPSTR pszFormatName;
    SAFEARRAY* psa = 0;
    VOID HUGEP* prawdata;
	FORMATETC ftc;
	STGMEDIUM stgm;
	LONG cfType;
    ULONG ulSize = 0;
    BOOL fIsArrayData = FALSE;
    BOOL fCleanupString = FALSE;

    if ((pvtType == nullptr) || PARAM_IS_MISSING(pvtType) || (pvtInput == nullptr) || PARAM_IS_MISSING(pvtInput))
        return E_INVALIDARG;

    // Find the clipboard format for the given data type...
    pwszTypeName = LPWSTR_FROM_VARIANT(*pvtType);
    if ((pwszTypeName)!=0 && (pszFormatName = DsoConvertToMBCS(pwszTypeName))!=0)
	{
        cfType = RegisterClipboardFormat(pszFormatName);
        DsoMemFree(pszFormatName);
    } else
        cfType = LONG_FROM_VARIANT(*pvtType, 0);
    CHECK_NULL_RETURN(cfType, E_INVALIDARG);

    // Depending on the type of data passed, and if we need to do any Unicode-to-MBCS
    // conversion for the caller, set up the data size and data pointer we will use
    // to create the global memory object we'll pass to the doc object server...
    prawdata = LPWSTR_FROM_VARIANT(*pvtInput);
    if (prawdata) {
        if (fMbcsString) {
            prawdata = (void*)DsoConvertToMBCS((BSTR)prawdata);
            CHECK_NULL_RETURN(prawdata, E_OUTOFMEMORY);
            fCleanupString = TRUE;
            ulSize = lstrlen((LPSTR)prawdata);
        } else
            ulSize = SysStringByteLen((BSTR)prawdata);
    } else {
        psa = PSARRAY_FROM_VARIANT(*pvtInput);
		if (psa)
		{
			LONG lb, ub, elSize;

			if ((SafeArrayGetDim(psa) > 1) ||
				FAILED(SafeArrayGetLBound(psa, 1, &lb)) || (lb < 0) ||
				FAILED(SafeArrayGetUBound(psa, 1, &ub)) || (ub < lb) ||
				((elSize = SafeArrayGetElemsize(psa)) < 1))
				return E_INVALIDARG;

			ulSize = (((ub + 1) - lb) * elSize);
			fIsArrayData = TRUE;
            if (FAILED(SafeArrayAccessData(psa, &prawdata)))
                return E_ACCESSDENIED;
        }

        CHECK_NULL_RETURN(prawdata, E_INVALIDARG);
    }

    // We must have a server and it must support IDataObject...
    if ((m_pole) && SUCCEEDED(m_pole->QueryInterface(IID_IDataObject, (void**)&pdo)) && (pdo)) {
        memset(&ftc, 0, sizeof(ftc));
		ftc.cfFormat = (WORD)cfType;
		ftc.dwAspect = DVASPECT_CONTENT;
		ftc.lindex = -1; ftc.tymed = TYMED_HGLOBAL;

		memset(&stgm, 0, sizeof(stgm));
		stgm.tymed = TYMED_HGLOBAL;
		stgm.hGlobal = GlobalAlloc(GPTR, ulSize);
		if (stgm.hGlobal)
		{
			LPVOID lpv = GlobalLock(stgm.hGlobal);
			if ((lpv) && (ulSize))
			{
				// Copy the data into transfer object...
				memcpy(lpv, prawdata, ulSize);
				GlobalUnlock(stgm.hGlobal);

				// Do the actual SetData call to transfer the data...
				hr = pdo->SetData(&ftc, &stgm, TRUE);
			} 
			else hr = E_UNEXPECTED;

			if (FAILED(hr))
				ReleaseStgMedium(&stgm);
		}
		else hr = E_OUTOFMEMORY;

		pdo->Release();
    } else
        hr = OLE_E_CANT_GETMONIKER;

    if (fIsArrayData)
		SafeArrayUnaccessData(psa);

	if (fCleanupString)
		DsoMemFree(prawdata);

	return hr;
}

////////////////////////////////////////////////////////////////////////
// MSOObject::FrameWindowProc
//
//  Site window procedure. Not much to do here except forward focus.
//
STDMETHODIMP_(LRESULT) MSOObject::FrameWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    MSOObject* pbndr = (MSOObject*)
#ifdef _WIN64
        GetWindowLongPtr(hwnd, GWLP_USERDATA);
#else
        GetWindowLong(hwnd, GWL_USERDATA);
#endif

    if (pbndr)
	{
		switch (msg)
		{
		case WM_PAINT:
			{
            PAINTSTRUCT ps;
            RECT rc;
            GetClientRect(hwnd, &rc);
            BeginPaint(hwnd, &ps);
            pbndr->OnDraw(DVASPECT_CONTENT, ps.hdc, (RECT*)&rc, nullptr, nullptr, TRUE);
            EndPaint(hwnd, &ps);
        }
            return 0;

        case WM_NCDESTROY:
#ifdef _WIN64
            SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
#else
            SetWindowLong(hwnd, GWL_USERDATA, 0);
#endif
            pbndr->m_hwnd = nullptr;
            break;

        case WM_SETFOCUS:
            if (pbndr->m_hwndUIActiveObj)
                SetFocus(pbndr->m_hwndUIActiveObj);
			return 0;

		case WM_SYSCOMMAND:
			if ((wParam & 0xFFF0) == SC_KEYMENU)
			{
				if ((pbndr->m_psiteCtl) && (pbndr->m_psiteCtl->SysMenuCommand((UINT)lParam) == S_OK))
					return 0;
			}
			break;

		case WM_ERASEBKGND:
			return 1;
		}
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

LPDISPATCH MSOObject::GetMSODocumentIDispatch()
{
    IOleInPlaceActiveObject* pOleInPlace = GetActiveObject();

    if (pOleInPlace == nullptr)
        return nullptr;

    IDispatch* pDisp = nullptr;
    HRESULT hr = pOleInPlace->QueryInterface(IID_IDispatch, (void**)&pDisp);

    if (FAILED(hr))
        return nullptr;

    return pDisp;
}

#ifdef Q_OS_WIN
#pragma warning(default : 4311)
#pragma warning(default : 4302)
#endif
