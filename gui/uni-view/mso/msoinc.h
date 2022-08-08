#pragma once

#include <docobj.h>
#include <tchar.h>

#pragma warning(push)
#pragma warning(disable: 4146)
#pragma warning(disable: 4298)
#pragma warning(disable: 4278)
#pragma warning(disable: 4298)
#pragma warning(disable: 4003)
#pragma warning(disable: 4336)

////////////////////////////////////////////////////////
//
// ќб¤зательно нужен установленный офис (минимум 2007) с Word, Excel, Outlook
//

// MSADDNDR.DLL - Microsoft AddInDesigner
//#import "libid:AC0714F2-3D04-11D1-AE7D-00A0C90F26F4" raw_interfaces_only raw_native_types no_namespace named_guids 
//auto_rename no_namespace named_guids

#define MT_BUILD

#ifdef MT_BUILD
#include "office\MSO.tlh"
using namespace Office;

// VBE6EXT.OLB - Visual Basic for Office support
#include "office\VBE6EXT.tlh"
//#include "office\VBE6EXT.tli"
using namespace VBIDE;

// EXCEL.EXE - Microsoft Excel type library
#include "office\EXCEL.tlh"
//#include "office\EXCEL.tli"

// MSWORD.OLB - Microsoft Word type library
#include "office\MSWORD.tlh"

// MSOUTL.OLB - Microsoft Outlook type library
#include "office\MSOUTL.tlh"
//#include "office\MSOUTL.tli"
#else // MT_BUILD()
#ifndef USE_DIRECT_MSO_PATH
// MSO.DLL - Primary Microsoft Office library
#import "libid:2DF8D04C-5BFA-101B-BDE5-00AA0044DE52" raw_interfaces_only rename_namespace("Office")
using namespace Office;

// VBE6EXT.OLB - Visual Basic for Office support
#import "libid:{0002e157-0000-0000-c000-000000000046}" rename_namespace("VBIDE")
using namespace VBIDE;

// EXCEL.EXE - Microsoft Excel type library
#import "libid:{00020813-0000-0000-c000-000000000046}" \
	raw_native_types named_guids auto_search  \
	rename ("RGB", "RGBEx") \
	rename("DialogBox", "DialogBoxEx") \
	exclude("IPicture") exclude("IFont") rename_namespace("Excel")

// MSWORD.OLB - Microsoft Word type library
#import "libid:{00020905-0000-0000-c000-000000000046}" auto_rename raw_interfaces_only raw_native_types rename_namespace("Word")

// MSOUTL.OLB - Microsoft Outlook type library
#import "libid:{00062fff-0000-0000-c000-000000000046}" \
	rename_namespace("Outlook")\
	exclude("_IRecipientControl", "_DRecipientControl")\
	rename("GetOrganizer", "GetOrganizerAddressEntry")
#else // USE_DIRECT_MSO_PATH
// MSO.DLL - Primary Microsoft Office library
#import "C:\Program Files (x86)\Common Files\microsoft shared\OFFICE15\MSO.DLL" raw_interfaces_only rename_namespace("Office")
using namespace Office;

// VBE6EXT.OLB - Visual Basic for Office support
#import "C:\Program Files (x86)\Common Files\microsoft shared\VBA\VBA6\VBE6EXT.OLB"  rename_namespace("VBIDE")
using namespace VBIDE;

// EXCEL.EXE - Microsoft Excel type library
#import "C:\Program Files (x86)\Microsoft Office\Office15\EXCEL.EXE" \
	raw_native_types named_guids auto_search  \
	rename ("RGB", "RGBEx") \
	rename("DialogBox", "DialogBoxEx") \
	exclude("IPicture") exclude("IFont") rename_namespace("Excel")

// MSWORD.OLB - Microsoft Word type library
#import "C:\Program Files (x86)\Microsoft Office\Office15\MSWORD.OLB" auto_rename raw_interfaces_only raw_native_types rename_namespace("Word")

// MSOUTL.OLB - Microsoft Outlook type library
#import "C:\Program Files (x86)\Microsoft Office\Office15\MSOUTL.OLB" \
	rename_namespace("Outlook")\
	exclude("_IRecipientControl", "_DRecipientControl")\
	rename("GetOrganizer", "GetOrganizerAddressEntry")
#endif // USE_DIRECT_MSO_PATH
#endif //MT_BUILD()

#pragma warning(pop) // restore warning state

#ifdef _FOR_FILECONVERTER
#else //_FOR_FILECONVERTER
#endif //_FOR_FILECONVERTER
