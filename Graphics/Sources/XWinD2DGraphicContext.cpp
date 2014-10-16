/*
* This file is part of Wakanda software, licensed by 4D under
*  (i) the GNU General Public License version 3 (GNU GPL v3), or
*  (ii) the Affero General Public License version 3 (AGPL v3) or
*  (iii) a commercial license.
* This file remains the exclusive property of 4D and/or its licensors
* and is protected by national and international legislations.
* In any event, Licensee's compliance with the terms and conditions
* of the applicable license constitutes a prerequisite to any use of this file.
* Except as otherwise expressly stated in the applicable license,
* such license does not include any other license or rights on this file,
* 4D's and/or its licensors' trademarks and/or other proprietary rights.
* Consequently, no title, copyright or other proprietary rights
* other than those specified in the applicable license is granted.
*/
#include "VGraphicsPrecompiled.h"
#include <iostream>

#if ENABLE_D2D

#include "V4DPictureIncludeBase.h"
#include "XWinD2DGraphicContext.h"
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include "XWinStyledTextBox.h"
#include "VRect.h"
#include "VRegion.h"
#include "VIcon.h"
#include "VPolygon.h"
#include "VPattern.h"
#include "VBezier.h"
#include "VGraphicImage.h"
#include "VGraphicFilter.h"
#include <stack>
#include "VFont.h"


//set to 1 to enable D2D logging
#if VERSIONDEBUG
#define D2D_LOG_ENABLE 0
#else
#define D2D_LOG_ENABLE 0
#endif

#define SHADOW_ALPHA	255 /*60*/
#define SHADOW_R		128 /*0*/
#define SHADOW_G		128 /*0*/
#define SHADOW_B		128 /*0*/

// Class constants
const uLONG		kDEBUG_REVEAL_DELAY	= 75;

#define VRECT_RGN_INFINITE VRect(0, 0, kMAX_GDI_RGN_SIZE, kMAX_GDI_RGN_SIZE)

bool													VWinD2DGraphicContext::sDesktopCompositionEnabled=false;
HMODULE													VWinD2DGraphicContext::sDwmApi=NULL;
VWinD2DGraphicContext::DwmIsCompositionEnabledProc		VWinD2DGraphicContext::fDwmIsCompositionEnabledProc=NULL;

bool VWinD2DGraphicContext::sD2DAvailable = false;

bool VWinD2DGraphicContext::sD2DEnabled = false;

/** use hardware if present or only software */
bool VWinD2DGraphicContext::sUseHardwareIfPresent = false;

/** hardware enabling status */
bool VWinD2DGraphicContext::sHardwareEnabled = true;

Gdiplus::Bitmap	*VWinD2DGraphicContext::fGdiplusSharedBmp = NULL;
GReal VWinD2DGraphicContext::fGdiplusSharedBmpWidth = 0;
GReal VWinD2DGraphicContext::fGdiplusSharedBmpHeight = 0;
bool VWinD2DGraphicContext::fGdiplusSharedBmpInUse = false;

#if D2D_FACTORY_MULTI_THREADED
CComPtr<ID2D1Factory> VWinD2DGraphicContext::fFactory;
#else
std::map<VTaskID, CComPtr<ID2D1Factory>> VWinD2DGraphicContext::fFactory;
#endif
VCriticalSection VWinD2DGraphicContext::fMutexFactory;

VCriticalSection VWinD2DGraphicContext::fMutexResources[];

VWinD2DGraphicContext::VectorOfLayer VWinD2DGraphicContext::fVectorOfLayer[];

std::map<HICON, Gdiplus::Bitmap*> VWinD2DGraphicContext::fCachedIcons;
std::map<HICON, CComPtr<ID2D1Bitmap>> VWinD2DGraphicContext::fCachedIconsD2D[];

VWinD2DGraphicContext::MapOfBitmap VWinD2DGraphicContext::fCachedBmp[];
VWinD2DGraphicContext::MapOfBitmap VWinD2DGraphicContext::fCachedBmpTransparentNoAlpha[];

#if VERSIONDEBUG
unsigned long gD2DCachedBitmapCount[2] = {0,0};
#endif

CComPtr<ID2D1DCRenderTarget> VWinD2DGraphicContext::fRscRenderTarget[];

VWinD2DGraphicContext::MapOfSharedGC VWinD2DGraphicContext::fMapOfSharedGC[];

/** DirectWrite factory */
CComPtr<IDWriteFactory> VWinD2DGraphicContext::fDWriteFactory;
VCriticalSection VWinD2DGraphicContext::fMutexDWriteFactory;

// Static

void	VWinD2DGraphicContext::DesktopCompositionModeChanged()
{
	if(sDwmApi && fDwmIsCompositionEnabledProc)
	{
		BOOL b;
		(*fDwmIsCompositionEnabledProc)(&b);
		sDesktopCompositionEnabled= (b==1);
	}
}

Boolean VWinD2DGraphicContext::Init( bool inUseHardwareIfPresent)
{
	sUseHardwareIfPresent = inUseHardwareIfPresent;
	sHardwareEnabled = sUseHardwareIfPresent;

	//return false;

	if (!VSystem::IsVista())
		return false; //D2D is available only with Vista/Win7

	//try to load desktop window manager
	HMODULE sDwmApi = LoadLibraryW(L"dwmapi.dll");
	xbox_assert(sDwmApi);
	
	if(sDwmApi)
	{
		DesktopCompositionModeChanged();// init desktop composition mode
	}
	
	//check if d2d1 dll is present
	HMODULE hModule = LoadLibraryW(L"d2d1.dll");
	xbox_assert(hModule);
	if(!hModule)
	{
		//oups: d2d1 dll is not found but it should be available on Vista+
		//		on Win7+, please check if DirectX runtime is properly installed 
		//		on Vista, SP2 and Platform Update for Vista should be installed
		return false;
	}

	//create D2D factory
#if D2D_FACTORY_MULTI_THREADED
	{
	VTaskLock lock(&fMutexFactory);
	sD2DAvailable = SUCCEEDED(::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &fFactory));
	}
#else
	sD2DAvailable = _CreateSingleThreadedFactory();
#endif

	//create DWrite factory (should always succeed if D2D factory is successfully created)
	if (sD2DAvailable)
	{
		VTaskLock lock(&fMutexDWriteFactory);
		HRESULT hr = DWriteCreateFactory(
				DWRITE_FACTORY_TYPE_ISOLATED,
				__uuidof(IDWriteFactory),
				reinterpret_cast<IUnknown**>(&fDWriteFactory)
				);
		xbox_assert(SUCCEEDED(hr));
	}

	{
#if D2D_GUI_SINGLE_THREADED
	VTaskLock lock(&(fMutexResources[D2D_CACHE_RESOURCE_HARDWARE]));
#endif
	_InitSharedResources();
	}

	if (!sD2DAvailable)
	{
		//release DWrite factory
		{
		VTaskLock lock(&fMutexDWriteFactory);
		fDWriteFactory = (IDWriteFactory *)NULL;
		}

		//release D2D factory
		{
		VTaskLock lock(&fMutexFactory);
	#if D2D_FACTORY_MULTI_THREADED
		if (((ID2D1Factory *)fFactory) != NULL)
			fFactory = (ID2D1Factory *)NULL;
	#else
		MapFactory::iterator it = fFactory.begin();
		for (;it != fFactory.end(); it++)
			(*it).second = (ID2D1Factory *)NULL;
		fFactory.clear();
	#endif
		}
	}

	sD2DEnabled = sD2DAvailable;
	return sD2DAvailable;
}

#if !D2D_FACTORY_MULTI_THREADED
bool VWinD2DGraphicContext::_CreateSingleThreadedFactory()
{
	VTaskID taskID = VTask::GetCurrentID();

	VTaskLock lock(&fMutexFactory);

	MapFactory::const_iterator it = fFactory.find( taskID);
	if (it != fFactory.end())
		return true;

	ID2D1Factory* factory = NULL;
	bool ok = SUCCEEDED(::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory));
	if (ok)
	{
		fFactory[taskID] = factory;
		factory->Release();
	}
	return ok;
}
#endif

void VWinD2DGraphicContext::_InitSharedResources()
{
	if (!sD2DAvailable)
		return;

	//create hardware render target for hardware shared resources
	{
		D2D_GUI_RESOURCE_MUTEX_LOCK(D2D_CACHE_RESOURCE_HARDWARE)

		if (sUseHardwareIfPresent && fRscRenderTarget[D2D_CACHE_RESOURCE_HARDWARE] == NULL)
		{
			//try to create a hardware render target
			D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
				D2D1_RENDER_TARGET_TYPE_HARDWARE,
				D2D1::PixelFormat(
					DXGI_FORMAT_B8G8R8A8_UNORM,
					D2D1_ALPHA_MODE_IGNORE), 
				96.0f,
				96.0f,
				D2D_RENDER_TARGET_USE_GDI_COMPATIBLE_DC ? D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE : D2D1_RENDER_TARGET_USAGE_NONE,
				D2D1_FEATURE_LEVEL_10
				);
			fMutexFactory.Lock();
			if (FAILED(fFactory->CreateDCRenderTarget(&props, &fRscRenderTarget[D2D_CACHE_RESOURCE_HARDWARE])))
			{
				sUseHardwareIfPresent = false;
				if (!VSystem::IsSeven())
				{
					//if hardware is unavailable on OS lower then Seven,
					//disable Direct2D because WARP fast software implementation is available on Seven only
					//(& reference software driver is too slow on Vista)
					sD2DAvailable = false;
					return;
				}
			}
			fMutexFactory.Unlock();
		}
	}
	//create software render target for sofware shared resources
	{
		D2D_GUI_RESOURCE_MUTEX_LOCK(D2D_CACHE_RESOURCE_SOFTWARE)

		if (fRscRenderTarget[D2D_CACHE_RESOURCE_SOFTWARE] == NULL)
		{
			//try to create a software render target
			D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
				D2D1_RENDER_TARGET_TYPE_SOFTWARE,
				D2D1::PixelFormat(
					DXGI_FORMAT_B8G8R8A8_UNORM,
					D2D1_ALPHA_MODE_IGNORE), 
				96.0f,
				96.0f,
				D2D_RENDER_TARGET_USE_GDI_COMPATIBLE_DC ? D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE : D2D1_RENDER_TARGET_USAGE_NONE,
				D2D1_FEATURE_LEVEL_DEFAULT
				);
			fMutexFactory.Lock();
			if (FAILED(fFactory->CreateDCRenderTarget(&props, &fRscRenderTarget[D2D_CACHE_RESOURCE_SOFTWARE])))
			{
				xbox_assert(false); //should always succeed
				sD2DAvailable = false;
			}
			fMutexFactory.Unlock();
		}
	}
}


void VWinD2DGraphicContext::DeInit()
{
	VTaskLock lock(&fMutexFactory);

	if(sDwmApi)
	{
		::FreeLibrary(sDwmApi);
		fDwmIsCompositionEnabledProc=NULL;
	}
	
	//release DirectWrite factory
	{
	VTaskLock lock(&fMutexDWriteFactory);
	fDWriteFactory = (IDWriteFactory *)NULL;
	}

	//release D2D factory
	{
#if D2D_FACTORY_MULTI_THREADED
	if (((ID2D1Factory *)fFactory) != NULL)
		fFactory = (ID2D1Factory *)NULL;
#else
	MapFactory::iterator it = fFactory.begin();
	for (;it != fFactory.end(); it++)
		(*it).second = (ID2D1Factory *)NULL;
	fFactory.clear();
#endif
	}
	//dipose shared resources

	//dipose software shared resources
	if (fGdiplusSharedBmp)
	{
		VTaskLock lock(&(fMutexResources[D2D_CACHE_RESOURCE_SOFTWARE]));
		xbox_assert(!fGdiplusSharedBmpInUse);
		delete fGdiplusSharedBmp;
		fGdiplusSharedBmp = NULL;
	}

	fCachedIcons.clear();
	{
	VTaskLock lock(&(fMutexResources[D2D_CACHE_RESOURCE_SOFTWARE]));

	MapOfSharedGC::iterator it;
	for (it = fMapOfSharedGC[D2D_CACHE_RESOURCE_SOFTWARE].begin(); it != fMapOfSharedGC[D2D_CACHE_RESOURCE_SOFTWARE].end(); it++)
		it->second->Release();
	fMapOfSharedGC[D2D_CACHE_RESOURCE_SOFTWARE].clear();

	fVectorOfLayer[D2D_CACHE_RESOURCE_SOFTWARE].clear();
	fCachedIconsD2D[D2D_CACHE_RESOURCE_SOFTWARE].clear();
	fRscRenderTarget[D2D_CACHE_RESOURCE_SOFTWARE] = (ID2D1DCRenderTarget *)NULL;
	fCachedBmp[D2D_CACHE_RESOURCE_SOFTWARE].clear();
	fCachedBmpTransparentNoAlpha[D2D_CACHE_RESOURCE_SOFTWARE].clear();
#if VERSIONDEBUG
#if D2D_LOG_ENABLE
	gD2DCachedBitmapCount[D2D_CACHE_RESOURCE_SOFTWARE] = 0;
	xbox::DebugMsg( "VWinD2DGraphicContext: software cached D2D bitmaps = 0\n");
#endif
#endif
	}

	//dispose hardware shared resources
	{
	VTaskLock lock(&(fMutexResources[D2D_CACHE_RESOURCE_HARDWARE]));

	MapOfSharedGC::iterator it;
	for (it = fMapOfSharedGC[D2D_CACHE_RESOURCE_HARDWARE].begin(); it != fMapOfSharedGC[D2D_CACHE_RESOURCE_HARDWARE].end(); it++)
		it->second->Release();
	fMapOfSharedGC[D2D_CACHE_RESOURCE_HARDWARE].clear();

	fVectorOfLayer[D2D_CACHE_RESOURCE_HARDWARE].clear();
	fCachedIconsD2D[D2D_CACHE_RESOURCE_HARDWARE].clear();
	fRscRenderTarget[D2D_CACHE_RESOURCE_HARDWARE] = (ID2D1DCRenderTarget *)NULL;
	fCachedBmp[D2D_CACHE_RESOURCE_HARDWARE].clear();
	fCachedBmpTransparentNoAlpha[D2D_CACHE_RESOURCE_HARDWARE].clear();
#if VERSIONDEBUG
#if D2D_LOG_ENABLE
	gD2DCachedBitmapCount[D2D_CACHE_RESOURCE_HARDWARE] = 0;
	xbox::DebugMsg( "VWinD2DGraphicContext: hardware cached D2D bitmaps = 0\n");
#endif
#endif
	}

	sD2DAvailable = false;
	sD2DEnabled = false;
}

/** enable/disable Direct2D implementation at runtime
@remarks
	if set to false, IsD2DAvailable() will return false even if D2D is available on the platform
	if set to true, IsD2DAvailable() will return true if D2D is available on the platform
*/
void VWinD2DGraphicContext::D2DEnable( bool inD2DEnable)
{
	bool enabled = sD2DEnabled;
	sD2DEnabled = sD2DAvailable && inD2DEnable;
	if (!sD2DEnabled && sD2DEnabled != enabled)
	{
		DisposeSharedResources( D2D_CACHE_RESOURCE_HARDWARE);
		DisposeSharedResources( D2D_CACHE_RESOURCE_SOFTWARE);
	}
}


/** create a shared graphic context binded initially to the specified native context ref 
@param inUserHandle
	user handle used to identify the shared graphic context
@param inContextRef
	native context ref
	CAUTION: on Windows, Direct2D cannot draw directly to a printer HDC so while printing & if D2DIsEnabled()
			 you should either call CreateBitmapGraphicContext to draw in a bitmap graphic context
			 & then print the bitmap with DrawBitmapToHDC or use explicitly another graphic context like GDIPlus or GDI
			 (use the desired graphic context constructor for that)
@param inBounds (used only by D2D impl - see VWinD2DGraphicContext)
	context area bounds 
@param inPageScale (used only by D2D impl - see remarks below)
	desired page scaling 
	(default is 1.0f)
@remarks
	if Direct2D is not available, method will behave the same than VGraphicContext::Create (param inUserHandle is ignored)
	so this method is mandatory only for Direct2D impl (but can be used safely for other impls so you can use same code in any platform)

	this method should be used if you want to re-use the same Direct2D graphic context with different GDI contexts:
	it creates a single Direct2D graphic context mapped to the passed user handle:
	- the first time you call this method, it allocates the graphic context, bind it to the input HDC & store it to a shared internal table (indexed by the user handle)
	- if you call this method again with the same user handle & with a different HDC or bounds, it rebinds the render target to the new HDC & bounds
	  which is more faster than to recreate the render target especially if new bounds are less or equal to the actual bounds

	caution: to release the returned graphic context, call as usual Release method of the graphic context
	
	if you want to remove the returned graphic context from the shared internal table too, call also RemoveShared 
	(actually you do not need to call this method because shared graphic contexts are auto-released by VGraphicContext::DeInit
	 & when needed for allocating video memory)

	For instance, in 4D this method should be called preferably than the previous one in order to avoid to create render target at each frame;

	if IsD2DImpl() return true, 

		all coordinates are expressed in device independent units (DIP) 
		where 1 DIP = 1 pixel if render target DPI = 96 so in order to avoid DPI scaling from 96 to DPI
		we force DPI equal to 96 by default (so 1 DIP = 1 Pixel by default).
		(It is because on Windows, 4D app is not dpi-aware and we must keep 
		this limitation for backward compatibility with previous impl)

		render target DPI is computed from inPageScale using the following formula:
		render target DPI = 96.0f*inPageScale 
		(yes render target DPI is a floating point value)

		bounds are expressed in DIP (so with DPI = 96): 
		D2D internally manages page scaling by scaling bounds & coordinates by DPI/96; 
*/
VGraphicContext* VWinD2DGraphicContext::CreateShared(sLONG inUserHandle, ContextRef inContextRef, const VRect& inBounds, const GReal inPageScale, const bool inTransparent, const bool inSoftwareOnly)
{
	if (!IsAvailable())
		return NULL;

	VTaskLock protect(&fMutexFactory);

	VGraphicContext *gc = NULL;
	
	BOOL typeOfResource;
	bool doNotCacheIt = false;

	if (!inSoftwareOnly && IsHardwareEnabled())
	{
		//search in hardware table

		D2D_GUI_RESOURCE_MUTEX_LOCK(D2D_CACHE_RESOURCE_HARDWARE)

		MapOfSharedGC::const_iterator it = fMapOfSharedGC[D2D_CACHE_RESOURCE_HARDWARE].find( inUserHandle);
		if (it != fMapOfSharedGC[D2D_CACHE_RESOURCE_HARDWARE].end())
		{
			typeOfResource = D2D_CACHE_RESOURCE_HARDWARE;
			gc = it->second;

			VWinD2DGraphicContext *gcD2D = static_cast<VWinD2DGraphicContext *>(gc);
			xbox_assert(gcD2D);
			if (!gcD2D->fIsShared)
			{
				xbox_assert(gcD2D->fUseCount == 0 && gcD2D->fLockCount == 0);
				gc->Retain();
				gcD2D->fIsShared = true; //mark as shared now
			}
			else
			{
				//shared gc already in use: it is not recommended to draw over a D2D context already opened with the same user handle
#if D2D_LOG_ENABLE 
				xbox::DebugMsg( "VWinD2DGraphicContext: failed to open hardware shared RT with user handle = %d: handle already in use\n", inUserHandle);
#endif
				gc = NULL;
				doNotCacheIt = true;
			}
		}
	}
	else 
	{
		//search in software table

		D2D_GUI_RESOURCE_MUTEX_LOCK(D2D_CACHE_RESOURCE_SOFTWARE)

		MapOfSharedGC::const_iterator it = fMapOfSharedGC[D2D_CACHE_RESOURCE_SOFTWARE].find( inUserHandle);
		if (it != fMapOfSharedGC[D2D_CACHE_RESOURCE_SOFTWARE].end())
		{
			typeOfResource = D2D_CACHE_RESOURCE_SOFTWARE;
			gc = it->second;

			VWinD2DGraphicContext *gcD2D = static_cast<VWinD2DGraphicContext *>(gc);
			xbox_assert(gcD2D);
			if (!gcD2D->fIsShared)
			{
				xbox_assert(gcD2D->fUseCount == 0 && gcD2D->fLockCount == 0);
				gc->Retain();
				gcD2D->fIsShared = true; //mark as shared now
				gc->SetSoftwareOnly(inSoftwareOnly);
			}
			else
			{
				//shared gc already in use: it is not recommended to draw over a D2D context already opened with the same user handle
#if D2D_LOG_ENABLE 
				xbox::DebugMsg( "VWinD2DGraphicContext: failed to open software shared RT with user handle = %d: handle already in use\n", inUserHandle);
#endif
				gc = NULL;
				doNotCacheIt = true;
			}
		}
	}

	if (gc)
	{
		//bind existing gc to new HDC & bounds
		if (gc->IsTransparent() != inTransparent)
			gc->SetTransparent( inTransparent);
		gc->BindContext( inContextRef, inBounds, true);

#if D2D_LOG_ENABLE 
		if (gc->IsHardware())
			xbox::DebugMsg( "VWinD2DGraphicContext: retain hardware shared RT with user handle = %d\n", inUserHandle);
		else
			xbox::DebugMsg( "VWinD2DGraphicContext: retain software shared RT with user handle = %d\n", inUserHandle);
#endif
	}
	else
	{
		//create new gc
		VWinD2DGraphicContext *gcD2D = new VWinD2DGraphicContext( inContextRef, inBounds, inPageScale, inTransparent);
		xbox_assert(gcD2D);

		gcD2D->SetSoftwareOnly(inSoftwareOnly);
		gcD2D->_InitDeviceDependentObjects(); //ensure rt is created before storing in internal resource domain table
											  //(in order to know if it is software or hardware resource)
		gc = static_cast<VGraphicContext *>(gcD2D);

		if (!doNotCacheIt)
		{
			D2D_GUI_RESOURCE_MUTEX_LOCK(gcD2D->fIsRTHardware);

			MapOfSharedGC::const_iterator it = fMapOfSharedGC[gcD2D->fIsRTHardware].find( inUserHandle);
			if (it != fMapOfSharedGC[gcD2D->fIsRTHardware].end())
			{
				//can happen if we failed to create hardware render target & we create software render target
				gcD2D = static_cast<VWinD2DGraphicContext *>(it->second);
				xbox_assert(gcD2D);
				if (!gcD2D->fIsShared)
				{
					gc->Release(); //release just created gc: we will use the existing one
					gcD2D->Retain();
					gcD2D->fIsShared = true; //mark as shared now
					gcD2D->SetSoftwareOnly(inSoftwareOnly);

					//bind existing gc to new HDC & bounds
					if (gcD2D->IsTransparent() != inTransparent)
						gcD2D->SetTransparent( inTransparent);
					gcD2D->BindContext( inContextRef, inBounds, false);

#if D2D_LOG_ENABLE
					if (gcD2D->fIsRTHardware)
						xbox::DebugMsg( "VWinD2DGraphicContext: retain hardware shared RT with user handle = %d\n", inUserHandle);
					else
						xbox::DebugMsg( "VWinD2DGraphicContext: retain software shared RT with user handle = %d\n", inUserHandle);
#endif

					return it->second;
				}
				else
				{
					//shared gc already in use: it is not recommended to draw over a D2D context already opened with the same user handle
					gc->Release();
#if D2D_LOG_ENABLE 
					if (gcD2D->fIsRTHardware)
						xbox::DebugMsg( "VWinD2DGraphicContext: failed to open hardware shared RT with user handle = %d: handle already in use\n", inUserHandle);
					else
						xbox::DebugMsg( "VWinD2DGraphicContext: failed to open software shared RT with user handle = %d: handle already in use\n", inUserHandle);
#endif
					return NULL;
				}
			}
			else
			{
				//add new shared gc to the resource table
				fMapOfSharedGC[gcD2D->fIsRTHardware][inUserHandle] = gc;
				gc->Retain();
				gcD2D->fIsShared = true; //mark as shared now
				gcD2D->fUserHandle = inUserHandle;
#if D2D_LOG_ENABLE 
				if (gcD2D->fIsRTHardware)
					xbox::DebugMsg( "VWinD2DGraphicContext: retain hardware shared RT with user handle = %d\n", inUserHandle);
				else
					xbox::DebugMsg( "VWinD2DGraphicContext: retain software shared RT with user handle = %d\n", inUserHandle);
#endif
			}
		}
	}
	return gc;
}


/** derived from IRefCountable */
sLONG VWinD2DGraphicContext::Release(const char* DebugInfo) const
{
	VTaskLock protect(&fMutexFactory);

	if (fIsShared && GetRefCount() == 2)
	{
		xbox_assert(fUseCount == 0 && fLockCount == 0);

#if D2D_LOG_ENABLE 
		if (fIsRTHardware)
			xbox::DebugMsg( "VWinD2DGraphicContext: release hardware shared RT with user handle = %d\n", fUserHandle);
		else
			xbox::DebugMsg( "VWinD2DGraphicContext: release software shared RT with user handle = %d\n", fUserHandle);
#endif
		fIsShared = false; //last release of a shared D2D context (after Release, refcount = 1 -> context is only owned by the internal shared table & not used)
	}
	return IRefCountable::Release( DebugInfo);
}


/** release the shared graphic context binded to the specified user handle	
@see
	CreateShared
*/
void VWinD2DGraphicContext::RemoveShared( sLONG inUserHandle)
{
	if (!IsAvailable())
		return;

	//search first in hardware table
	{
		D2D_GUI_RESOURCE_MUTEX_LOCK(D2D_CACHE_RESOURCE_HARDWARE)

		MapOfSharedGC::iterator it = fMapOfSharedGC[D2D_CACHE_RESOURCE_HARDWARE].find( inUserHandle);
		if (it != fMapOfSharedGC[D2D_CACHE_RESOURCE_HARDWARE].end())
		{
			it->second->Release();
			fMapOfSharedGC[D2D_CACHE_RESOURCE_HARDWARE].erase( it);
		}
	}
	//then search in software table
	{
		D2D_GUI_RESOURCE_MUTEX_LOCK(D2D_CACHE_RESOURCE_SOFTWARE)

		MapOfSharedGC::iterator it = fMapOfSharedGC[D2D_CACHE_RESOURCE_SOFTWARE].find( inUserHandle);
		if (it != fMapOfSharedGC[D2D_CACHE_RESOURCE_SOFTWARE].end())
		{
			it->second->Release();
			fMapOfSharedGC[D2D_CACHE_RESOURCE_SOFTWARE].erase( it);
		}
	}
}



/** create ID2D1DCRenderTarget render target binded to the specified HDC 
@param inHDC
	gdi device context ref
@param inBounds
	bounds (DIP unit: coordinate space equal to gdi coordinate space - see remarks below)
@param inPageScale
	desired page scaling for render target (see remarks below)
	(default is 1.0f)
@remarks
	caller can call BindHDC later to bind or rebind new dc 

	all coordinates are expressed in device independent units (DIP) 
	where 1 DIP = 1 pixel if render target DPI = 96 so in order to avoid DPI scaling from 96 to DPI
	we force DPI equal to 96 by default (so 1 DIP = 1 Pixel by default).
	(It is because on Windows, 4D app is not dpi-aware and we must keep 
	this limitation for backward compatibility with previous impl)

	render target DPI is computed from inPageScale using the following formula:
	render target DPI = 96.0f*inPageScale 
	(yes render target DPI is a floating point value)

	bounds are expressed in DIP (so with DPI = 96): 
	D2D internally manages page scaling by scaling bounds & coordinates by DPI/96; 
	while printing, just pass bounds like rendering on screen (gdi bounds)
	and set inPageScale equal to printer DPI/72 so that render target DPI = 96.0f*printer DPI/72
	(see usage in SVG component for an example)
*/
VWinD2DGraphicContext::VWinD2DGraphicContext(HDC inContext, const VRect& inBounds, const GReal inPageScale, const bool inTransparent):VGraphicContext()
{
#if !D2D_FACTORY_MULTI_THREADED
	_CreateSingleThreadedFactory();
#endif
	_Init();
	fHDC = inContext;
	fHDCBounds = inBounds;
	fHDCBounds.NormalizeToInt();
	fHDCBindRectDirty = true;
	fHDCBindDCDirty = true;
	fDPI = 96.0f*inPageScale;
	fIsTransparent = inTransparent;
}


/** change transparency status
@remarks
	render target can be rebuilded if transparency status is changed
*/
void VWinD2DGraphicContext::SetTransparent( bool inTransparent)
{
	xbox_assert(fUseCount <= 0); //this method should not be called while drawing
	if (fUseCount)
		return;

	if (fIsTransparent == inTransparent)
		return;
	fIsTransparent = inTransparent;

	//we must recreate render target because alpha mode has changed
	_DisposeDeviceDependentObjects(false);
}

/** bind new HDC to the current DC render target
@remarks
	do nothing and return false if current render target is not a ID2D1DCRenderTarget
*/
bool VWinD2DGraphicContext::BindHDC( HDC inHDC, const VRect& inBounds, bool inResetContext, bool inBindAlways) const
{
	xbox_assert(fUseCount <= 0);
	if (fUseCount > 0)
		return false;

	VRect bounds(inBounds);
	bounds.NormalizeToInt();

	if (fHDC != inHDC || bounds != fHDCBounds || inBindAlways)
	{
		if (inBindAlways)
			fHDCBindDCDirty = true;
		else
		{
			fHDCBindRectDirty = fHDCBindRectDirty || bounds != fHDCBounds;
			fHDCBindDCDirty = fHDCBindDCDirty || fHDC != inHDC;
		}
		fHDC = inHDC;
		fHDCBounds = bounds;
	}

	if (inResetContext)
	{
		//reset graphic context settings & resources
		ReleaseRefCountable(&fTextFont);
		if (fTextFontMetrics)
			delete fTextFontMetrics;
		fTextFontMetrics = NULL;

		fWhiteSpaceWidth = 0.0f;

		fGlobalAlpha = 1.0;

		fShadowHOffset = 0.0;
		fShadowVOffset = 0.0;
		fShadowBlurness = 0.0;

		fDrawingMode = DM_NORMAL;
		fIsHighQualityAntialiased = true;
		fTextRenderingMode = fHighQualityTextRenderingMode = TRM_NORMAL;
		fTextMeasuringMode = TMM_NORMAL;

		fStrokeWidth = 1.0f;
		fStrokeStyleProps = D2D1::StrokeStyleProperties();
		if (fStrokeDashPattern)
			delete [] fStrokeDashPattern;
		fStrokeDashPattern = NULL;
		fStrokeDashCount = 0;
		fStrokeDashCountInit = 0;
		fStrokeDashPatternInit.clear();
		fIsStrokeDotPattern = false;
		fIsStrokeDashPatternUnitEqualToStrokeWidth = false;

		fStrokeBrush = (ID2D1Brush *)NULL;
		fFillBrush = (ID2D1Brush *)NULL;
		fTextBrush = (ID2D1Brush *)NULL;

		fStrokeBrushSolid = (ID2D1SolidColorBrush *)NULL;
		fFillBrushSolid = (ID2D1SolidColorBrush *)NULL;
		fTextBrushSolid = (ID2D1SolidColorBrush *)NULL;

		fStrokeBrushLinearGradient = (ID2D1LinearGradientBrush *)NULL;
		fStrokeBrushRadialGradient = (ID2D1RadialGradientBrush *)NULL;
		fFillBrushLinearGradient = (ID2D1LinearGradientBrush *)NULL;
		fFillBrushRadialGradient = (ID2D1RadialGradientBrush *)NULL;

		fStrokeStyle = (ID2D1StrokeStyle *)NULL;
		fMutexFactory.Lock();
		HRESULT hr = GetFactory()->CreateStrokeStyle( &fStrokeStyleProps, NULL, 0, &fStrokeStyle);
		xbox_assert(SUCCEEDED(hr) && fStrokeStyle);
		fMutexFactory.Unlock();
	}
	return true;
}

VWinD2DGraphicContext::VWinD2DGraphicContext (ID2D1RenderTarget* inRT, const VPoint& inOrigViewport, uBOOL inOwnRT, const bool inTransparent):VGraphicContext()
{
#if !D2D_FACTORY_MULTI_THREADED
	_CreateSingleThreadedFactory();
#endif
	xbox_assert( inRT);
	_Init();
	fOwnRT = inOwnRT;
	fRenderTarget = inRT;
	//init hdc bounds to viewport origin and render target DIP bounds
	fHDCBounds.SetX( inOrigViewport.GetX());
	fHDCBounds.SetY( inOrigViewport.GetY());
	D2D1_SIZE_F size = inRT->GetSize();//must be size in DIP
	fHDCBounds.SetWidth ( size.width); 
	fHDCBounds.SetHeight( size.height);
	fDPI = GetDpiX();
	if (inOwnRT)
		inRT->Release();
	fIsTransparent = inTransparent;
}

/** create ID2D1HwndRenderTarget render target binded to the specified HWND 
@remarks
	ID2D1HwndRenderTarget render target is natively double-buffered so
	VWinD2DGraphicContext should be created at window creation 
	and destroyed only when window is destroyed 

	all coordinates are expressed in device independent units (DIP) 
	where 1 DIP = 1 pixel if render target DPI = 96 so in order to avoid DPI scaling from 96 to DPI
	we force DPI equal to 96 by default (so 1 DIP = 1 Pixel by default).
	(It is because on Windows, 4D app is not dpi-aware and we must keep 
	this limitation for backward compatibility with previous impl)

	render target DPI is computed from inPageScale using the following formula:
	render target DPI = 96.0f*inPageScale 
	(yes render target DPI is a floating point value)

	caller should call WndResize if window is resized
	(otherwise render target content is scaled to fit in window bounds)
*/
VWinD2DGraphicContext::VWinD2DGraphicContext(HWND inWindow, const GReal inPageScale, const bool inTransparent):VGraphicContext()
{
#if !D2D_FACTORY_MULTI_THREADED
	_CreateSingleThreadedFactory();
#endif
	_Init();
	fHwnd = inWindow;
	fDPI = 96.0f*inPageScale;
	fIsTransparent = inTransparent;
}

/** resize window render target 
@remarks
	should be called whenever window is resized to prevent render target scaling

	do nothing if current render target is not a ID2D1HwndRenderTarget
*/
void VWinD2DGraphicContext::WndResize()
{
	if (fUseCount > 0)
		return;

	if (fHwnd)
	{
		RECT rect;
		::GetClientRect( fHwnd, &rect);
		VRect bounds(rect);
		if (fHwndRenderTarget && 
			((fHDCBounds.GetWidth() != bounds.GetWidth()) || (fHDCBounds.GetHeight() != bounds.GetHeight())))
			fHwndRenderTarget->Resize( D2D1::SizeU(bounds.GetWidth(), bounds.GetHeight()));
		{
			fHDCBounds.FromRect( bounds);
			fHDCBindRectDirty = true;
		}
	}
}

/** create WIC render target using provided WIC bitmap as source
@remarks
	WIC Bitmap is retained until graphic context destruction

	inOrigViewport is expressed in DIP

	this render target is useful for software offscreen rendering
	(as it does not use hardware, render target content can never be lost: it can be used for permanent offscreen rendering)
*/
VWinD2DGraphicContext::VWinD2DGraphicContext( IWICBitmap *inWICBitmap, const GReal inPageScale, const VPoint& inOrigViewport)
{
#if !D2D_FACTORY_MULTI_THREADED
	_CreateSingleThreadedFactory();
#endif
	xbox_assert( inWICBitmap);
	_Init();
	fIsRTSoftwareOnly = true;

	fOwnRT = true;
	fDPI = 96.0f*inPageScale;

	//init hdc bounds to viewport origin and WIC bounds
	fHDCBounds.SetX( inOrigViewport.GetX());
	fHDCBounds.SetY( inOrigViewport.GetY());
	UINT iwidth, iheight;
	GReal width,height;
	inWICBitmap->GetSize( &iwidth, &iheight);
	if (inPageScale != 1.0f)
	{
		//convert to DIP size
		width  = iwidth / inPageScale;
		height = iheight / inPageScale;
	}
	else
	{
		width = iwidth;
		height = iheight;
	}
	fHDCBounds.SetWidth ( width);
	fHDCBounds.SetHeight( height);

	fWICBitmap = inWICBitmap;

	//determine if render target needs to be transparent
	WICPixelFormatGUID pf;
	fWICBitmap->GetPixelFormat( &pf);
	if (VPictureCodec_WIC::HasPixelFormatAlpha( pf))
		fIsTransparent = true;
	else
		fIsTransparent = false;
}

/** create WIC render target from scratch
@remarks
	create a WIC render target bound to a internal WIC Bitmap using GUID_WICPixelFormat32bppPBGRA as pixel format
	(in order to be compatible with ID2D1Bitmap)

	width and height are expressed in DIP 
	(so bitmap size in pixel will be width*inPageScale x height*inPageScale)

	inOrigViewport is expressed in DIP too

	this render target is useful for software offscreen rendering
	(as it does not use hardware, render target content can never be lost: it can be used for permanent offscreen rendering)
*/
VWinD2DGraphicContext::VWinD2DGraphicContext( sLONG inWidth, sLONG inHeight, const GReal inPageScale, const VPoint& inOrigViewport, const bool inTransparent, const bool inTransparentCompatibleGDI)
{
	if (inWidth == 0)
		inWidth = 1;
	if (inHeight == 0)
		inHeight = 1;
	xbox_assert(inPageScale >= 1.0f);

#if !D2D_FACTORY_MULTI_THREADED
	_CreateSingleThreadedFactory();
#endif
	_Init();
	fIsRTSoftwareOnly = true;

	fOwnRT = true;
	fDPI = 96.0f*inPageScale;

	//init hdc bounds (DIP bounds)
	fViewportOffset = inOrigViewport;
	fHDCBounds.SetX( 0.0f);
	fHDCBounds.SetY( 0.0f);
	fHDCBounds.SetWidth ( inWidth);
	fHDCBounds.SetHeight( inHeight);

	//convert dip size to pixel
	UINT width = inWidth;
	UINT height = inHeight;
	if (inPageScale != 1.0f)
	{
		GReal rwidth = inWidth*inPageScale;
		GReal rheight = inHeight*inPageScale;

		GReal offset = 0.0f;
		if (rwidth - floor(rwidth) != 0.0f)
			offset = 1.0f;
		else if (rheight - floor(rheight) != 0.0f)
			offset = 1.0f;
		width = (UINT)(rwidth+offset);
		height = (UINT)(rheight+offset);
	}
	fIsTransparent = inTransparent;

#if D2D_GDI_USE_GDIPLUS_BITMAP_FOR_TRANSPARENCY
	//create lazy Gdiplus bitmap, gc & hdc which will be used by rt to direct access the bitmap
	//(we use Gdiplus bitmap here in order to allow direct access to bitmap HDC while drawing with GDI 
	// - it spares background copy from rt to GDI context if we want to draw with GDI on transparent)
	if (fIsTransparent && inTransparentCompatibleGDI)
	{
		{
			VTaskLock lock(&(fMutexResources[D2D_CACHE_RESOURCE_SOFTWARE]));
			if (!fGdiplusSharedBmpInUse && fGdiplusSharedBmp)
			{
				//use shared bitmap if not in use yet
				bool doClear = true;
				if (width > fGdiplusSharedBmpWidth || height > fGdiplusSharedBmpHeight)
				{
					delete fGdiplusSharedBmp;
					fGdiplusSharedBmp = new Gdiplus::Bitmap(	width,
																height,
																PixelFormat32bppPARGB);
					fGdiplusSharedBmpWidth = width;
					fGdiplusSharedBmpHeight = height;
					doClear = false;
				}
				fGdiplusSharedBmpInUse = true;
				fGdiplusBmp = fGdiplusSharedBmp;
				fGdiplusGC = new Gdiplus::Graphics( static_cast<Gdiplus::Image *>(fGdiplusBmp));
				fGdiplusGC->SetPixelOffsetMode( Gdiplus::PixelOffsetModeHalf);
				xbox_assert(fGdiplusGC);
				if (doClear)
				{
					//clear bitmap
					Gdiplus::SolidBrush *clearColor = new Gdiplus::SolidBrush(Gdiplus::Color(0,0,0,0));
					fGdiplusGC->SetCompositingMode( Gdiplus::CompositingModeSourceCopy);
					fGdiplusGC->FillRectangle( clearColor, Gdiplus::RectF( 0.0f, 0.0f, width, height));
					fGdiplusGC->SetCompositingMode( Gdiplus::CompositingModeSourceOver);
					delete clearColor;
				}
			}
		}
		if (!fGdiplusBmp)
		{
			//allocate new bitmap
			fGdiplusBmp = new Gdiplus::Bitmap(	width,
												height,
												PixelFormat32bppPARGB);
			xbox_assert(fGdiplusBmp);
			{
				VTaskLock lock(&(fMutexResources[D2D_CACHE_RESOURCE_SOFTWARE]));
				if (!fGdiplusSharedBmpInUse && !fGdiplusSharedBmp)
				{
					fGdiplusSharedBmp = fGdiplusBmp;
					fGdiplusSharedBmpWidth = width;
					fGdiplusSharedBmpHeight = height;
					fGdiplusSharedBmpInUse = true;
				}
			}
			fGdiplusGC = new Gdiplus::Graphics( static_cast<Gdiplus::Image *>(fGdiplusBmp));
			xbox_assert(fGdiplusGC);
		}
		fHDC = fGdiplusGC->GetHDC();
		return;
	}
#endif
	//retain factory instance (use com smart pointer for auto-release)
	CComPtr<IWICImagingFactory> factory;
	HRESULT result = factory.CoCreateInstance(CLSID_WICImagingFactory);
	xbox_assert(SUCCEEDED(result)); //always succeed if Direct2D is supported

	factory->CreateBitmap(  width, height,
							fIsTransparent ? GUID_WICPixelFormat32bppPBGRA : GUID_WICPixelFormat32bppBGR, //use pixel format compatible with D2D
							WICBitmapCacheOnDemand,
							&fWICBitmap);
	fWICBitmap->SetResolution( 96.0f, 96.0f); //set WIC resolution to default: it is not used by D2D
}

/** create a D2D bitmap compatible with the specified render target from the current render target content
@remarks
	call this method outside BeginUsingContext/EndUsingContext
	otherwise this method will fail and return NULL

	with this method, you can draw content from this render target to another one

	if current render target is a WIC bitmap render target, D2D bitmap is build directly from the internal WIC bitmap
*/
ID2D1Bitmap *VWinD2DGraphicContext::_CreateBitmap(ID2D1RenderTarget* inRT, const VRect *inBoundsTransformed) const 
{
	FlushGDI(); 
	VIndex oldUseCount = fUseCount;
	bool oldLockType = fLockParentContext;
	StParentContextNoDraw hdcNoDraw(this);

	if (fUseCount > 0)
	{
		//we need to finish current drawing session if we need to access to the WIC bitmap
		fLockParentContext = true; //ensure render target is presented now
		_Lock(); 
	}
	if (!testAssert(fUseCount == 0)) //failed to lock (probably because inside a layer)
	{
		//restore drawing session
		_Unlock();
		fLockParentContext = oldLockType;
		return NULL;
	}

	D2D1_SIZE_U sizePixel = fRenderTarget->GetPixelSize();
	VRect bounds;
	if (inBoundsTransformed)
	{
		bounds.FromRect(*inBoundsTransformed);
		if (fDPI != 96.0f)
		{
			//convert to pixel coords
			bounds.SetX(bounds.GetX()*fDPI/96.0f);
			bounds.SetY(bounds.GetY()*fDPI/96.0f);
			bounds.SetWidth(bounds.GetWidth()*fDPI/96.0f);
			bounds.SetHeight(bounds.GetHeight()*fDPI/96.0f);
		}
		bounds.NormalizeToInt();
		if (bounds == VRect(0,0,sizePixel.width,sizePixel.height))
			inBoundsTransformed = NULL;
		else
		{
			bounds.Intersect(VRect(0,0,sizePixel.width,sizePixel.height));
			sizePixel.width = bounds.GetWidth();
			sizePixel.height = bounds.GetHeight();
		}
	}
	else
		bounds.SetCoords(0, 0, sizePixel.width, sizePixel.height);

	ID2D1Bitmap *bitmap = NULL;
	if (fGdiplusBmp != NULL)
	{
		//Gdiplus bitmap

		if (!inBoundsTransformed)
		{
			VTaskLock lock(&(fMutexResources[D2D_CACHE_RESOURCE_SOFTWARE]));
			if (fGdiplusSharedBmpInUse && fGdiplusBmp == fGdiplusSharedBmp && (sizePixel.width != fGdiplusSharedBmpWidth || sizePixel.height != fGdiplusSharedBmpHeight))
				inBoundsTransformed = &bounds;
		}

		Gdiplus::PixelFormat pf;
		pf = fGdiplusBmp->GetPixelFormat();
		if (pf != PixelFormat32bppPARGB
			&&
			pf != PixelFormat32bppRGB) //ensure format is compatible with D2D  
		{
			if (oldUseCount > 0)
			{
				//restore drawing session
				_Unlock();
				fLockParentContext = oldLockType;
			}
			return NULL;
		}
		//unlock Gdiplus bmp: release Gdiplus graphic context
		xbox_assert(fGdiplusGC);
		fGdiplusGC->ReleaseHDC( fHDC);
		delete fGdiplusGC;
		fGdiplusGC = NULL;

		if (inBoundsTransformed)
		{
			xbox_assert(fGdiplusBmp->GetPixelFormat() == PixelFormat32bppPARGB);
			int stride = bounds.GetWidth()*4; //PARGB 
			VPtr buffer = VMemory::NewPtr( stride*bounds.GetHeight(),'bbuf');
			xbox_assert(buffer);

			Gdiplus::BitmapData bmData;
			bmData.PixelFormat	= fGdiplusBmp->GetPixelFormat();
			bmData.Scan0		= (void *)buffer;
			bmData.Stride		= (INT)stride;
			bmData.Width		= (UINT)bounds.GetWidth();
			bmData.Height		= (UINT)bounds.GetHeight();
			Gdiplus::Rect rect(bounds.GetLeft(), bounds.GetTop(), bounds.GetWidth(), bounds.GetHeight());
			Gdiplus::Status status = fGdiplusBmp->LockBits(	&rect,
															Gdiplus::ImageLockModeRead | Gdiplus::ImageLockModeUserInputBuf,
															fGdiplusBmp->GetPixelFormat(),
															&bmData);
			if (status == Gdiplus::Ok)
			{
				if (testAssert(SUCCEEDED(inRT->CreateBitmap(	sizePixel,
																D2D1::BitmapProperties(
																	D2D1::PixelFormat(
																		DXGI_FORMAT_B8G8R8A8_UNORM,
																		D2D1_ALPHA_MODE_PREMULTIPLIED), 
																	96.0f,		
																	96.0f),
																&bitmap))))
				{
					D2D1_RECT_U srcRectPixel = D2D1::RectU( 0, 0, bounds.GetWidth(), bounds.GetHeight()); 
					if (FAILED(bitmap->CopyFromMemory( &srcRectPixel, bmData.Scan0, stride)))
					{
						xbox_assert(false);
						bitmap->Release();
						bitmap = NULL;
					}
				}
			}
			fGdiplusBmp->UnlockBits(&bmData);
			VMemory::DisposePtr( buffer);
		}
		else
			bitmap = VPictureCodec_WIC::DecodeD2D( inRT, fGdiplusBmp);

		//restore Gdiplus context & rebind rt to the GDI context
		xbox_assert(fGdiplusGC == NULL);
		fGdiplusGC = new Gdiplus::Graphics( static_cast<Gdiplus::Image *>(fGdiplusBmp));
		xbox_assert(fGdiplusGC);
		HDC hdc = fGdiplusGC->GetHDC();
		BindHDC( hdc, fHDCBounds, false);
	}
	else if (fWICBitmap != NULL && inBoundsTransformed == NULL)
	{
		//WIC bitmap render target

		WICPixelFormatGUID pf;
		fWICBitmap->GetPixelFormat( &pf);
		if (pf != GUID_WICPixelFormat32bppPBGRA
			&&
			pf != GUID_WICPixelFormat32bppBGR) //ensure format is compatible with D2D  
		{
			if (oldUseCount > 0)
			{
				//restore drawing session
				_Unlock();
				fLockParentContext = oldLockType;
			}
			return NULL;
		}

		FLOAT dpiX, dpiY;
		fRenderTarget->GetDpi( &dpiX, &dpiY);
		HRESULT hr = inRT->CreateBitmapFromWicBitmap(
						fWICBitmap, 
						&bitmap);
		xbox_assert(SUCCEEDED(hr));
	}
	else
	{
		//DC or HWND render target

		if (fRenderTarget == NULL)
			_InitDeviceDependentObjects();
		if (fRenderTarget == NULL)
		{
			if (oldUseCount > 0)
			{
				//restore drawing session
				_Unlock();
				fLockParentContext = oldLockType;
			}
			return NULL;
		}
		if (testAssert(SUCCEEDED(inRT->CreateBitmap(	sizePixel,
														D2D1::BitmapProperties(
																fRenderTarget->GetPixelFormat(),
																96.0f,		
																96.0f),
														&bitmap))))
		{
			D2D1_POINT_2U destPosPixel = D2D1::Point2U();
			D2D1_RECT_U srcRectPixel = D2D1::RectU( bounds.GetLeft(), bounds.GetTop(), bounds.GetLeft()+bounds.GetWidth(), bounds.GetTop()+bounds.GetHeight()); 
			if (FAILED(bitmap->CopyFromRenderTarget( &destPosPixel, fRenderTarget, &srcRectPixel)))
			{
				xbox_assert(false);
				bitmap->Release();
				bitmap = NULL;
			}
		}
	}

	if (oldUseCount > 0)
	{
		//restore drawing session
		_Unlock();
		fLockParentContext = oldLockType;
	}

	return bitmap;
}

/** draw the attached bitmap to a GDI device context 
@remarks
	D2D does not support drawing directly to a printer HDC
	so for printing, you sould render first to a bitmap graphic context 
	and then call this method to draw bitmap to the printer HDC

	does nothing if current graphic context is not a bitmap graphic context
*/
void VWinD2DGraphicContext::DrawBitmapToHDC( HDC hdc, const VRect& inDestBounds, const Real inPageScale, const VRect* inSrcBounds)
{
	FlushGDI(); 
	VIndex oldUseCount = fUseCount;
	bool oldLockType = fLockParentContext;
	StParentContextNoDraw hdcNoDraw(this);

	if (fUseCount > 0)
	{
		//we need to finish current drawing session if we need to access to the WIC bitmap
		fLockParentContext = true; //ensure render target is presented now
		_Lock(); 
	}
	if (!testAssert(fUseCount == 0)) //failed to lock (probably because inside a layer)
	{
		//restore drawing session
		_Unlock();
		fLockParentContext = oldLockType;
		return;
	}

	//here we use a Gdiplus gc to draw the WIC bitmap
	//(because Gdiplus can draw in printer dc)

	Gdiplus::Bitmap *bmp = fGdiplusBmp ? fGdiplusBmp : VPictureCodec_WIC::FromWIC( fWICBitmap);
	if (!bmp)
	{
		if (oldUseCount > 0)
		{
			//restore drawing session
			_Unlock();
			fLockParentContext = oldLockType;
		}
		return;
	}
	if (bmp == fGdiplusBmp)
	{
		//unlock Gdiplus bmp: release Gdiplus graphic context
		xbox_assert(fGdiplusGC);
		fGdiplusGC->ReleaseHDC( fHDC);
		delete fGdiplusGC;
		fGdiplusGC = NULL;
	}

	Gdiplus::Graphics gc( hdc);
	gc.SetPixelOffsetMode( Gdiplus::PixelOffsetModeHalf);
	gc.SetPageScale( inPageScale);
	gc.SetPageUnit( Gdiplus::UnitPixel);
	Gdiplus::Matrix ctmNoTrans;
	gc.GetTransform( &ctmNoTrans);
	GReal x = ctmNoTrans.OffsetX();
	GReal y = ctmNoTrans.OffsetY();
	ctmNoTrans.Translate( -x, -y, Gdiplus::MatrixOrderAppend);

	Gdiplus::InterpolationMode intMode = gc.GetInterpolationMode();

	if (ctmNoTrans.IsIdentity()
		&&
		(
		(!inSrcBounds) 
		|| 
		(
		inSrcBounds->GetWidth() == inDestBounds.GetWidth()
		&&
		inSrcBounds->GetHeight() == inDestBounds.GetHeight()
		)
		))
		gc.SetInterpolationMode( Gdiplus::InterpolationModeNearestNeighbor);
	else
		gc.SetInterpolationMode( Gdiplus::InterpolationModeHighQualityBicubic);

	if (!inSrcBounds && fGdiplusBmp)
	{
		VTaskLock lock(&(fMutexResources[D2D_CACHE_RESOURCE_SOFTWARE]));
		if (fGdiplusSharedBmpInUse && fGdiplusBmp == fGdiplusSharedBmp && (fHDCBounds.GetWidth() != fGdiplusSharedBmpWidth || fHDCBounds.GetHeight() != fGdiplusSharedBmpHeight))
			inSrcBounds = &fHDCBounds;
	}

	if (inSrcBounds)
	{
		VRect srcBounds;
		srcBounds.FromRect(*inSrcBounds);
		if (fDPI != 96.0f)
		{
			//convert to WIC/GDIPlus bitmap coords
			srcBounds.SetX(srcBounds.GetX()*fDPI/96.0f);
			srcBounds.SetY(srcBounds.GetY()*fDPI/96.0f);
			srcBounds.SetWidth(srcBounds.GetWidth()*fDPI/96.0f);
			srcBounds.SetHeight(srcBounds.GetHeight()*fDPI/96.0f);
		}
		Gdiplus::RectF destRect(inDestBounds.GetX(), 
								inDestBounds.GetY(),
								inDestBounds.GetWidth(), 
								inDestBounds.GetHeight());
		Gdiplus::Status status = gc.DrawImage(	static_cast<Gdiplus::Image *>(bmp),
												destRect,
												srcBounds.GetX(), 
												srcBounds.GetY(),
												srcBounds.GetWidth(), 
												srcBounds.GetHeight(),
												Gdiplus::UnitPixel);
	}
	else
		Gdiplus::Status status = gc.DrawImage(	static_cast<Gdiplus::Image *>(bmp), 
												inDestBounds.GetX(), 
												inDestBounds.GetY(),
												inDestBounds.GetWidth(), 
												inDestBounds.GetHeight()
												);
	if (bmp == fGdiplusBmp)
	{
		//restore Gdiplus context
		xbox_assert(fGdiplusGC == NULL);
		fGdiplusGC = new Gdiplus::Graphics( static_cast<Gdiplus::Image *>(fGdiplusBmp));
		xbox_assert(fGdiplusGC);
		HDC hdc = fGdiplusGC->GetHDC();
		BindHDC( hdc, fHDCBounds, false);
	}
	else
		delete bmp;

	gc.SetInterpolationMode( intMode);

	if (oldUseCount > 0)
	{
		//restore drawing session
		_Unlock();
		fLockParentContext = oldLockType;
	}
}

VWinD2DGraphicContext::VWinD2DGraphicContext(const VWinD2DGraphicContext& inOriginal) : VGraphicContext(inOriginal)
{
#if !D2D_FACTORY_MULTI_THREADED
	_CreateSingleThreadedFactory();
#endif
	_Init();
	assert(false);
	if(inOriginal.fHDC)
		fHDC = inOriginal.fHDC;
	else if(inOriginal.fHwnd)
		fHwnd = inOriginal.fHwnd;
	fDPI = inOriginal.fDPI;
	fHDCBounds = inOriginal.fHDCBounds;
	fHDCBindRectDirty = true;
	fHDCBindDCDirty = true;
	fWICBitmap = inOriginal.fWICBitmap;
	fIsTransparent = inOriginal.fIsTransparent;
	fIsRTSoftwareOnly = inOriginal.fIsRTSoftwareOnly;
	if (inOriginal.fGdiplusBmp)
	{
		//release inOriginal Gdiplus gc
		xbox_assert(inOriginal.fUseCount == 0);
		inOriginal.fGdiplusGC->ReleaseHDC( inOriginal.fHDC);
		delete inOriginal.fGdiplusGC; //unlock original bitmap before cloning

		//clone inOriginal Gdiplus bmp & init Gdiplus gc
		fGdiplusBmp = inOriginal.fGdiplusBmp->Clone(0,0,inOriginal.fGdiplusBmp->GetWidth(), inOriginal.fGdiplusBmp->GetHeight(), inOriginal.fGdiplusBmp->GetPixelFormat());
		xbox_assert(fGdiplusBmp);
		fGdiplusGC = new Gdiplus::Graphics( static_cast<Gdiplus::Image *>(fGdiplusBmp));
		xbox_assert(fGdiplusGC);
		fHDC = fGdiplusGC->GetHDC();
		fViewportOffset = inOriginal.fViewportOffset;

		//restore inOriginal Gdiplus gc
		inOriginal.fGdiplusGC = new Gdiplus::Graphics( static_cast<Gdiplus::Image *>(inOriginal.fGdiplusBmp));
		xbox_assert(inOriginal.fGdiplusGC);
		HDC hdc = inOriginal.fGdiplusGC->GetHDC();
		inOriginal.BindHDC( hdc, fHDCBounds, false);
	}
}


VWinD2DGraphicContext::~VWinD2DGraphicContext()
{
	_Dispose();
}



GReal VWinD2DGraphicContext::GetDpiX() const
{
	if (fRenderTarget == NULL)
		return fDPI;
	FLOAT dpiX, dpiY;
	fRenderTarget->GetDpi( &dpiX, &dpiY);
	return dpiX;
}

GReal VWinD2DGraphicContext::GetDpiY() const 
{
	if (fRenderTarget == NULL)
		return fDPI;
	FLOAT dpiX, dpiY;
	fRenderTarget->GetDpi( &dpiX, &dpiY);
	return dpiY;
}



ContextRef	VWinD2DGraphicContext::BeginUsingParentContext()const
{
	bool wantsLockParentContext = fLockParentContext && (!fLayerOffScreenCount) && (fHDC || fHwnd); //do not allow locking rt parent context if we are 
																									//inside a layer or if it is a WIC bitmap context
#if D2D_GDI_USE_GDIPLUS_BITMAP_FOR_TRANSPARENCY
	bool doLockForGDI = !fParentContextNoDraw && fUseCount > 0 && fIsTransparent && !wantsLockParentContext && !fLayerOffScreenCount; 
	if (doLockForGDI)
	{
		if (fHDC || fHwnd)
		{
			//here better to lock directly rt parent hdc than to prepare surface for GDI
			//so flush current drawing context & lock rt parent dc 
			wantsLockParentContext = true;
		}
	}
#endif

	if (wantsLockParentContext && !fGDI_CurIsParentDC)
		//caller wants to use render target parent hdc & not a gdi-compatible context over render target
		//so flush current gdi-compatible context if any
		FlushGDI();

	if (fGDI_HDC)
	{
		if (testAssert(fGDI_HDC_FromBeginUsingParentContext))
		{
			//context created from BeginUsingParentContext is reentrant (for keep alive as long as possible): deal with reentrance
			xbox_assert(fHDCOpenCount >= 0 && fHDCOpenCount != 1);
			fHDCOpenCount++;
			if (fHDCOpenCount == 1)
				fHDCOpenCount++; //simulates call to _GetHDC
			return fGDI_HDC;
		}
		else
		{
			//context created from _GetHDC only is not reentrant so flush current GDI context 
			FlushGDI();
		}
	}

	if (fGDI_IsReleasing)
	{
		xbox_assert(fHDCOpenCount == 0);
		fHDCOpenCount++;

		//we are releasing current GDI hdc: for reentrance we return parent context hdc
		//(should work because the only case of reentrance if while we get GDI metrics from a not truetype font:
		// as it is only for metrics & parent context metrics are equal to GDI gc over render target metrics it is fine)
		if (fHDC)
			return fHDC;
		else if (fHwnd) //HWND render target 
			return ::GetDC(fHwnd);
		else
			//bitmap context: as metrics are same as screen GDI metrics (for printing we never use a D2D context)
			//return screen compatible hdc
			return ::CreateCompatibleDC( NULL);
	}

	fGDI_HDC_FromBeginUsingParentContext = true;

	xbox_assert(fHDCOpenCount == 0);

	HDC hdc = NULL;
	fHDCOpenCount++;
	xbox_assert(fHDCOpenCount == 1);

	bool inheritContext = fUseCount > 0;

	fHDCFont = NULL;
	fHDCSaveFont = NULL;
	fHDCSavedClip = NULL;
	fHDCNeedRestoreClip = false;
	fHDCTransform = NULL;

	if (fTextFont != NULL)
		fHDCFont = fTextFont->GetFontRef();

	bool bSelectD2DClipping = false;
	VRect clipRectD2D;
	VRegion *clipRegionD2D = NULL;
	VAffineTransform ctm;
	
	//lock parent HDC only if not in a offscreen layer
	bool lockParentContext = fLockParentContext;
	fLockParentContext = wantsLockParentContext || !inheritContext;

#if D2D_RENDER_TARGET_USE_GDI_COMPATIBLE_DC
	if (!fLockParentContext)
	{
		hdc = _GetHDC(); //if fUseCount > 0, will flush current batch of primitives and lock gdi surface 
		xbox_assert(fHDCOpenCount == 2);
		if (!hdc)
		{
			fLockParentContext = lockParentContext;
			fHDCOpenCount = 0;
			fGDI_HDC_FromBeginUsingParentContext = false;
			return NULL;
		}
		ctm = fLockSaveTransform;
		if (inheritContext)
		{
			bSelectD2DClipping = true;

			//get clipping bounds in render target user space
			clipRectD2D = fLockSaveClipBounds;
			clipRectD2D.NormalizeToInt();

			clipRegionD2D = fLockSaveClipRegion;
		}
	}
	else
	{
#endif
		_GetTransformToScreen(ctm); //get user to screen transform because GetTransform returns only current layer transform
		if (inheritContext)
		{
			bSelectD2DClipping = true;
			//get clipping bounds in render target user space
			_GetClipBoundingBox( clipRectD2D);

			//transform clipping bounds to GDI coordinate space
			clipRectD2D = ctm.TransformRect(clipRectD2D);
			clipRectD2D.NormalizeToInt(); //set to pixel bounds (because HRGN uses integer coordinates)

			//transform clip region to GDI coord space
			if (fClipRegionCur)
			{
				clipRegionD2D = new VRegion(fClipRegionCur);
				VPoint origOffset = ctm.TransformPoint( VPoint(0,0));
				clipRegionD2D->SetPosBy(origOffset.GetX(), origOffset.GetY());
			}
		}
		else
			//outside BeginUsingContext/EndUsingContext, transform is identify because SetTransform is valid only inside context
			ctm.MakeIdentity();
		hdc = _GetHDC(); //if fUseCount > 0, will flush current batch of primitives and lock gdi surface 
		if (!hdc)
		{
			fLockParentContext = lockParentContext;
			fHDCOpenCount = 0;
			fGDI_HDC_FromBeginUsingParentContext = false;
			return NULL;
		}
#if D2D_RENDER_TARGET_USE_GDI_COMPATIBLE_DC
	}
#endif
	
	if (inheritContext)
	{
		::GetViewportOrgEx(hdc, &fHDCSaveViewportOrg);
		xbox_assert(fLockParentContext || (fHDCSaveViewportOrg.x == 0 && fHDCSaveViewportOrg.y == 0));
		if (bSelectD2DClipping)
		{
#if VERSIONDEBUG
			RECT	bounds;
			int error2 = ::GetClipBox(hdc, &bounds);
#endif
			fHDCSavedClip=::CreateRectRgn(0,0,0,0);
			int error = ::GetClipRgn(hdc,fHDCSavedClip);
			if(error <= 0)
			{
#if VERSIONDEBUG
				if (error == -1)
				{
					DWORD lastError = ::GetLastError();
					std::cerr<<"VWinD2DGraphicContext::BeginUsingParentContext(): GetClipRgn error = "<<lastError<<"\n";
				}
#endif
				DeleteObject(fHDCSavedClip);
				fHDCSavedClip=NULL;
			}	
			fHDCNeedRestoreClip = true;

			//apply D2D clipping
			HRGN hdcclip = NULL;
			if (clipRegionD2D)
			{
				//complex clipping region:

				hdcclip = ::CreateRectRgn(0, 0, 0, 0);
				::CopyRgn( hdcclip, (RgnRef)(*clipRegionD2D));
				if (fLockParentContext)
					delete clipRegionD2D;		
			}
			else
				//simple rect clipping
				hdcclip = ::CreateRectRgn( clipRectD2D.GetX(), clipRectD2D.GetY(), clipRectD2D.GetRight(), clipRectD2D.GetBottom());
			
			if (fHDCSaveViewportOrg.x || fHDCSaveViewportOrg.y)
				::OffsetRgn( hdcclip, fHDCSaveViewportOrg.x, fHDCSaveViewportOrg.y);

			if (fHDCSavedClip)
				::CombineRgn( hdcclip, fHDCSavedClip, hdcclip, RGN_AND);
			::SelectClipRgn(hdc,hdcclip);
			if(hdcclip)
				::DeleteObject(hdcclip);
		}

		if(fHDCFont)
			fHDCSaveFont = (HFONT)::SelectObject(hdc, fHDCFont);
		
		//set new GDI viewport in order GDI clipping to be consistent with local user space (because GDI graphic context clipping takes only account of viewport origin)
		if (fLockParentContext)
			//if we lock render target parent context, we need to add fHDCBounds origin to ctm
			ctm.SetTranslation( ctm.GetTranslateX()+fHDCBounds.GetX(), ctm.GetTranslateY()+fHDCBounds.GetY());

		int mapMode = ::GetMapMode( hdc);
		xbox_assert(mapMode == MM_TEXT);

		//if(!ctm.IsIdentity())
			fHDCTransform = new HDC_TransformSetter(hdc, ctm);
		//else
		//	fHDCTransform = NULL;

		_RevealCurClipping();
	}
	else if(fHDCFont) //if fUseCount <= 0, we still need to inherit gc font
		fHDCSaveFont = (HFONT)::SelectObject(hdc, fHDCFont);

	//ensure GetTransform is consistent with GDI world transform
	D2D1_MATRIX_3X2_F mat;
	ctm.ToNativeMatrix( (D2D_MATRIX_REF)&mat);
	if (fRenderTarget)
		fRenderTarget->SetTransform( mat); 

#if VERSIONDEBUG
	//if (!fLockParentContext && inheritContext && ::GetAsyncKeyState(VK_LSHIFT) & 0x8000)
	//{
	//	RECT	bounds;
	//	::GetClipBox(hdc, &bounds);
	//	bounds.left -= fHDCSaveViewportOrg.x;
	//	bounds.top -= fHDCSaveViewportOrg.y;
	//	
	//	HBRUSH	brush = ::CreateSolidBrush(RGB(0, 0, 255));
	//	::FrameRect(hdc, &bounds, brush);
	//	
	//	::DeleteObject(brush);
	//}
#endif

	fLockParentContext = lockParentContext;

	return hdc;
};

void	VWinD2DGraphicContext::EndUsingParentContext(ContextRef inContextRef)const
{
	if (fGDI_HDC)
	{
		xbox_assert( fGDI_HDC == inContextRef);
		if (testAssert(fGDI_HDC_FromBeginUsingParentContext))
		{
			//context created from BeginUsingParentContext is reentrant so keep GDI dc alive as long as possible...
			fHDCOpenCount--;
			xbox_assert(fHDCOpenCount >= 1);
			if (fHDCOpenCount == 1)
				fHDCOpenCount = 0; //simulates call to _ReleaseHDC
			return;
		}
		else
		{
			//normally we should never fall here but just in case...
			_ReleaseHDC( inContextRef, false);
			return;
		}
	}
	if (fHDCOpenCount == 0)
		return;

	if (fGDI_IsReleasing)
	{
		xbox_assert(fHDCOpenCount == 1);
		fHDCOpenCount--;

		if (!fHDC)
		{
			if (fHwnd) //HWND render target 
				::ReleaseDC( fHwnd, inContextRef);
			else
				//bitmap context
				::DeleteDC( inContextRef);
		}
		return;
	}

	xbox_assert(fHDCOpenCount == 2);
	fHDCOpenCount--;

	if (fHDCTransform)
		delete fHDCTransform;

	if (inContextRef && fLockCount > 0)
		::SetViewportOrgEx(inContextRef, fHDCSaveViewportOrg.x, fHDCSaveViewportOrg.y, NULL);

	if (fHDCNeedRestoreClip)
	{
		::SelectClipRgn(inContextRef,fHDCSavedClip);
		if(fHDCSavedClip)
		{
			::DeleteObject(fHDCSavedClip);
			fHDCSavedClip = NULL;
		}
	}

	if (fHDCSaveFont)
		::SelectObject(inContextRef, fHDCSaveFont);

	fGDI_HDC_FromBeginUsingParentContext = false;

	_ReleaseHDC( inContextRef); //unlock gdi surface and resume D2D drawing if appropriate
};

HDC VWinD2DGraphicContext::_GetHDC() const
{
	if (fGDI_HDC)
	{
		xbox_assert(fHDCOpenCount >= 0);
		fHDCOpenCount++;
		return fGDI_HDC;
	}
	xbox_assert(fLockCount == 0);

	if (fGDI_IsReleasing)
	{
		xbox_assert(fHDCOpenCount == 0);
		fHDCOpenCount++;

		//we are releasing current GDI hdc: for reentrance we return parent context hdc
		//(should work because the only case of reentrance if while we get GDI metrics from a not truetype font:
		// as it is only for metrics & parent context metrics are equal to GDI gc over render target metrics it is fine)
		if (fHDC)
			return fHDC;
		else if (fHwnd) //HWND render target 
			return ::GetDC(fHwnd);
		else
			//bitmap context: as metrics are same as screen GDI metrics (for printing we never use a D2D context)
			//return screen compatible hdc
			return ::CreateCompatibleDC( NULL);
	}
	fGDI_CurIsParentDC = false;
	xbox_assert(fGDI_SaveClipCounter == 0);
	xbox_assert(fGDI_SaveContextCounter == 0);

	_Lock();
	xbox_assert(fLockSaveUseCount <= 0 || fLockCount == 1);

	HDC result = NULL;
	fHDCOpenCount++;
	xbox_assert(fHDCOpenCount == 1 || fHDCOpenCount == 2);
	
	if (fUseCount <= 0)
	{
		fGDI_CurIsParentDC = true;
#if D2D_GDI_USE_GDIPLUS_BITMAP_FOR_TRANSPARENCY
		//xbox_assert(!doPrepareTransparencyForGDI);
#endif
		if (fHDC)
			//DC render target: return associated hdc
			result = fHDC;
		else if (fHwnd) //HWND render target 
		{
			result = ::GetDC(fHwnd);
			if (!result)
			{
				_Unlock();
				return result;
			}
		}
		else
			return ::CreateCompatibleDC(NULL); //only usable for metrics
		#if VERSIONDEBUG
		if (result && fLockCount > 0)
			fDebugLockGDIParentCounter++;
		#endif
	}

	if (fUseCount > 0 && fRenderTarget != NULL)
	{
		//try to get HDC from render target if render target is gdi-compatible
#if D2D_GDI_USE_GDIPLUS_BITMAP_FOR_TRANSPARENCY
		/*
		if (doPrepareTransparencyForGDI && hdcGDIPlus) 
		{
			//while drawing in a bitmap render target or transparent surface,
			//we use GDIPlus as a workaround for D2D bug
			//(GDI texts are not drawed at all otherwise)
			//(only if we are called from BeginUsingParentContext ie fHDCOpenCount >= 2)

			if (fGDI_TransparentGdiplusGC)
				delete fGDI_TransparentGdiplusGC;
			fGDI_TransparentGdiplusGC = NULL;
			if (fGDI_TransparentBmp)
				delete fGDI_TransparentBmp;
			fGDI_TransparentBmp = NULL;

			//create GDIPlus PARGB surface
			fGDI_TransparentBmp = bmpGDIPlus;
			fGDI_TransparentGdiplusGC = gcGDIPlus;
			result = hdcGDIPlus;

			//update locked ctm & clipping so GDI world transform & clipping is properly initialized later in BeginUsingParentContext
			
			//backup first current locked transform & transformed clipping
			fGDI_TransparentBackupTransform = fLockSaveTransform;
			fGDI_TransparentBackupClipBounds = fLockSaveClipBounds;
			fGDI_TransparentBackupClipRegion = fLockSaveClipRegion ? new VRegion(fLockSaveClipRegion) : NULL;

			//now convert locked ctm & clipping to GDIPlus surface actual user space

			//get untransformed clip bounds
			fLockSaveClipBounds = fLockSaveTransform.Inverse().TransformRect(fLockSaveClipBounds);

			//set new locked transformed
			fLockSaveTransform.Translate( -fGDI_CurBoundsClipTransparent.GetX(), -fGDI_CurBoundsClipTransparent.GetY(), VAffineTransform::MatrixOrderAppend);
			if (pageScale != 1.0f)
				fLockSaveTransform.Scale( pageScale, pageScale, VAffineTransform::MatrixOrderAppend);

			//set new locked transformed clip bounds
			fLockSaveClipBounds.SetPosBy( -fGDI_CurBoundsClipTransparent.GetX(), -fGDI_CurBoundsClipTransparent.GetY());
			if (fLockSaveClipRegion)
			{
				fLockSaveClipRegion->SetPosBy( -fGDI_CurBoundsClipTransparent.GetX(), -fGDI_CurBoundsClipTransparent.GetY());
				if (pageScale != 1.0f)
					fLockSaveClipRegion->SetSizeBy( pageScale, pageScale);
			}
		}
		if (!result)
		{
		*/
#endif
			CComPtr<ID2D1GdiInteropRenderTarget> rtGDI;
			fRenderTarget->QueryInterface( &rtGDI);

			rtGDI->GetDC( D2D1_DC_INITIALIZE_MODE_COPY, &result);

			#if VERSIONDEBUG
			if (result && fLockCount > 0)
				fDebugLockGDIInteropCounter++;
			#endif
#if D2D_GDI_USE_GDIPLUS_BITMAP_FOR_TRANSPARENCY
		//}
#endif
	}
	if (!result)
		_Unlock();
	if (result && fLockCount > 0)
	{
		xbox_assert(fLockSaveUseCount > 0);

		//on default, GDI context inherits current text font & color, stroke solid color, fill solid color & stroke width
		//(GDI cannot apply gradients patterns)
		//if fast GDI is enabled:
		//  if caller fills later a shape like line(s) or simple rect with a solid opaque color (without dashes for stroke), GDI context will be used
		//  otherwise D2D context will be used
		//	Also legacy texts are painted with GDI 
		//  It is recommended for instance to group simple shape primitives & legacy texts in a drawing block with fast GDI mode enabled
		//  (to reduce GDI over D2D contexts instances & so speed up rendering - as switching from D2D to GDI & from GDI to D2D has a high performance hit)
		//if fast GDI is disabled:
		//	if caller fills later any shape, D2D will be used but GDI
		//	(if fast GDI is disabled, actually only legacy texts are painted with GDI - and RichTextEdit)

		VColor textColor;
		GetTextColor(textColor);

		fGDI_HDC = result;
		fGDI_GC = new VWinGDIGraphicContext(fGDI_HDC);
		fGDI_GC->BeginUsingContext();
		
		//backup text font & color (as they can be updated without triggering a FlushGDI to keep alive GDI gc while drawing text)
		xbox_assert(fGDI_TextFont.size() == 0);

		fGDI_TextFont.push_back( RetainRefCountable(fTextFont));
		fGDI_TextColor.push_back( textColor);
		if (fGDI_Fast)
		{
			fGDI_StrokeWidth.push_back( fStrokeWidth);
			fGDI_StrokePattern.push_back(NULL);
			fGDI_HasSolidStrokeColor.push_back(false);
			fGDI_HasSolidStrokeColorInherit.push_back(fStrokeBrush.IsEqualObject(fStrokeBrushSolid));
			fGDI_StrokeColor.push_back(VColor());
			fGDI_HasStrokeCustomDashesInherit.push_back( fStrokeDashCount != 0);

			fGDI_HasSolidFillColor.push_back(false);
			fGDI_HasSolidFillColorInherit.push_back(fFillBrush.IsEqualObject(fFillBrushSolid));
			fGDI_FillColor.push_back(VColor());
			fGDI_FillPattern.push_back(NULL);
		}

		fGDI_GC->SetTextDrawingMode(DM_NORMAL); 

		//inherit text font, brush & drawing mode
		if (fTextFont && fHDCOpenCount == 1) //if 2, it is done yet by BeginUsingParentContext
			fGDI_GC->SetFont( fTextFont);
		fGDI_GC->SetTextColor(textColor);

		fGDI_GC->SetLineColor( VCOLOR_FROM_D2D_COLOR(fStrokeBrushSolid->GetColor()));
		fGDI_GC->SetFillColor( VCOLOR_FROM_D2D_COLOR(fFillBrushSolid->GetColor()));
		fGDI_GC->SetLineWidth( fStrokeWidth);
	}
	else
		fGDI_HDC = NULL;

	xbox_assert(fGDI_SaveClipCounter == 0);
	xbox_assert(fGDI_SaveContextCounter == 0);
	return result;
}

void VWinD2DGraphicContext::_ReleaseHDC(HDC inDC, bool inDeferRelease) const
{
	if (fGDI_HDC && inDeferRelease)
	{
		xbox_assert(fHDCOpenCount >= 1);
		fHDCOpenCount--;
		return;
	}
	if (fHDCOpenCount == 0)
		return;

	if (fGDI_IsReleasing)
	{
		xbox_assert(fHDCOpenCount == 1);
		fHDCOpenCount--;

		if (!fHDC)
		{
			if (fHwnd) //HWND render target 
				::ReleaseDC( fHwnd, inDC);
			else
				//bitmap context
				::DeleteDC( inDC);
		}
		return;
	}

	fHDCOpenCount--;
	xbox_assert(fHDCOpenCount == 0);

	if (!inDC)
	{
		xbox_assert( fGDI_HDC == NULL);
		return;
	}

	fGDI_IsReleasing = true;

	if (fGDI_HDC)
	{
		xbox_assert( fGDI_HDC == inDC);

		//restore context & clipping on the stack
		//(important to ensure GDI context is properly restored...)
		for (int i = fGDI_Restore.size(); i > 0; i--)
		{
			if (fGDI_Restore[i-1] == eSave_Clipping)
				fGDI_GC->RestoreClip();
			else if (fGDI_Restore[i-1] == eSave_Context)
				fGDI_GC->RestoreContext();
		}

		fGDI_HDC = NULL;
		fGDI_GC->EndUsingContext();
		fGDI_GC->Release();
		fGDI_GC = NULL;
	}

	if (!fGDI_CurIsParentDC && fRenderTarget != NULL)
	{
#if D2D_GDI_USE_GDIPLUS_BITMAP_FOR_TRANSPARENCY
		/*
		if (fGDI_TransparentGdiplusGC)
		{
			//context is attached to a GDIPlus bmp

			xbox_assert(fLayerOffScreenCount > 0 || fIsTransparent);

			//while drawing in a bitmap render target,
			//we use GDIPlus as a workaround for D2D bug
			//(only if we are called from EndUsingParentContext)

			//restore first current locked transform & transformed clipping
			fLockSaveTransform = fGDI_TransparentBackupTransform;
			fLockSaveClipBounds = fGDI_TransparentBackupClipBounds;
			if (fLockSaveClipRegion)
				delete fLockSaveClipRegion;
			fLockSaveClipRegion = fGDI_TransparentBackupClipRegion;
			fGDI_TransparentBackupClipRegion = NULL;

			//release GDI hdc
			fGDI_TransparentGdiplusGC->ReleaseHDC( inDC);

			//delete GDIPlus graphic context
			delete fGDI_TransparentGdiplusGC;
			fGDI_TransparentGdiplusGC = NULL;

			//convert GDIPlus surface to D2D bitmap
			//TODO: this is a big perf issue because need to be done per frame 
			//		(perhaps should cache the gdiplus bmp memory & reuse it & also use shared D2D bitmap with CopyFromMemory instead of re-allocating D2D bitmap
			//		 but this does not reduce the system memory->video memory overhead)
			ID2D1Bitmap *bmp = VPictureCodec_WIC::DecodeD2D( fRenderTarget, fGDI_TransparentBmp);

			//release GDIPlus bitmap
			delete fGDI_TransparentBmp;
			fGDI_TransparentBmp = NULL;

			if (bmp)
			{
				//finally composite D2D surface to actual render target
				VAffineTransform ctm;
				_GetTransform(ctm);
				fRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
				fRenderTarget->DrawBitmap(  bmp, 
											D2D_RECT(fGDI_CurBoundsClipTransparent),
											1.0f,
											D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
				_SetTransform(ctm);
				bmp->Release();
			}
		}
		else
		{
		*/
#endif
			//gdi-compatible render target: release temporary dc

			CComPtr<ID2D1GdiInteropRenderTarget> rtGDI;
			fRenderTarget->QueryInterface( &rtGDI);

			//ensure rect is relative to rt origin
			if (::GetGraphicsMode(inDC) == GM_ADVANCED)
				::ModifyWorldTransform( inDC, NULL, MWT_IDENTITY);

			RECT rect;
			rect.left = floor(fLockSaveClipBounds.GetLeft());
			rect.top = floor(fLockSaveClipBounds.GetTop());
			rect.right = ceil(fLockSaveClipBounds.GetRight());
			rect.bottom = ceil(fLockSaveClipBounds.GetBottom());
			rtGDI->ReleaseDC( &rect);
#if D2D_GDI_USE_GDIPLUS_BITMAP_FOR_TRANSPARENCY
		//}
#endif
	}
	if (fGDI_CurIsParentDC)
	{
		xbox_assert( fUseCount <= 0);

		if (fHwnd)
			::ReleaseDC(fHwnd, inDC);
		else if (!fHDC)
			::DeleteDC( inDC);
		fGDI_CurIsParentDC = false;
	}
	_Unlock();

	//if there was some GDI clipping or context in the stack when we released GDI HDC, 
	//we need to apply it on D2D context to ensure clipping & context is properly inherited
	//(because there is still some RestoreContext or RestoreClip waiting)
	if (fLockSaveUseCount > 0 && !fUnlockNoRestoreDrawingContext)
	{
		if testAssert(0 < fGDI_TextFont.size())
		{
			if (fGDI_TextFont[0])
				_SetFont( fGDI_TextFont[0]);
			_SetTextColor( fGDI_TextColor[0]);
			if (fGDI_Fast)
			{
				if (fGDI_HasSolidStrokeColor[0])
					_SetLineColor( fGDI_StrokeColor[0]);
				if (fGDI_StrokePattern[0])
					_SetLinePattern(fGDI_StrokePattern[0]);
				_SetLineWidth( fGDI_StrokeWidth[0]);
				if (fGDI_HasSolidFillColor[0])
					_SetFillColor( fGDI_FillColor[0]);
				if (fGDI_FillPattern[0])
					_SetFillPattern(fGDI_FillPattern[0]);
			}
		}
		std::vector<VRect>::const_iterator itClip = fGDI_ClipRect.begin();
		int saveContextCount = 1;
		for (int i = 0; i < fGDI_Restore.size(); i++)
		{
			if (fGDI_Restore[i] == eSave_Clipping)
				_SaveClip();
			else if (fGDI_Restore[i] == ePush_Clipping)
			{
				VRect bounds = *itClip;
				//transform GDI bounds in rt initial coord space
				VAffineTransform ctm;
				_GetTransform(ctm);
				_SetTransform(VAffineTransform());
				_ClipRect( bounds);
				_SetTransform(ctm);
				itClip++;
			}
			else //ePush_Context
			{
				_SaveContext();
				if (saveContextCount < fGDI_TextFont.size())
				{
					if (fGDI_TextFont[saveContextCount])
						_SetFont( fGDI_TextFont[saveContextCount]);
					_SetTextColor( fGDI_TextColor[saveContextCount]);
					if (fGDI_Fast)
					{
						if (fGDI_HasSolidStrokeColor[saveContextCount])
							_SetLineColor(	fGDI_StrokeColor[saveContextCount]);
						if (fGDI_StrokePattern[saveContextCount])
							_SetLinePattern(fGDI_StrokePattern[saveContextCount]);
						_SetLineWidth(	fGDI_StrokeWidth[saveContextCount]);
						if (fGDI_HasSolidFillColor[saveContextCount])
							_SetFillColor(	fGDI_FillColor[saveContextCount]);
						if (fGDI_FillPattern[saveContextCount])
							_SetFillPattern(fGDI_FillPattern[saveContextCount]);
					}
				}
				saveContextCount++;
			}
		}
	}
	std::vector<VFont *>::iterator it = fGDI_TextFont.begin();
	for (;it != fGDI_TextFont.end(); it++)
	{
		if (*it)
			(*it)->Release();
	}
	std::vector<const VPattern *>::iterator itPattern = fGDI_StrokePattern.begin();
	for (;itPattern != fGDI_StrokePattern.end(); itPattern++)
	{
		if (*itPattern)
			(*itPattern)->Release();
	}
	itPattern = fGDI_FillPattern.begin();
	for (;itPattern != fGDI_FillPattern.end(); itPattern++)
	{
		if (*itPattern)
			(*itPattern)->Release();
	}

	fGDI_PushedClipCounter = 0;
	fGDI_SaveClipCounter = 0;
	fGDI_SaveContextCounter = 0;

	fGDI_LastPushedClipCounter.clear();
	fGDI_ClipRect.clear();
	fGDI_Restore.clear();
	fGDI_TextFont.clear();
	fGDI_TextColor.clear();
	if (fGDI_Fast)
	{
		fGDI_HasSolidStrokeColor.clear();
		fGDI_HasSolidStrokeColorInherit.clear();
		fGDI_StrokeColor.clear();
		fGDI_StrokePattern.clear();
		fGDI_StrokeWidth.clear();
		fGDI_HasStrokeCustomDashesInherit.clear();
		
		fGDI_HasSolidFillColor.clear();
		fGDI_FillColor.clear();
		fGDI_FillPattern.clear();
		fGDI_HasSolidFillColorInherit.clear();
	}
	fGDI_IsReleasing = false;
}

void VWinD2DGraphicContext::_Init()
{
	fIsShared = false;
	fUserHandle = -1;
	fUseCount = 0;
#if VERSIONDEBUG
	fTaskUseCount = 0;
#endif
	fLockCount = 0;
	fHDCOpenCount=0;
	fGlobalAlphaCount = 0;

	fHwnd = NULL;
	fHDC = NULL;
	fGDI_HDC = NULL;
	fGDI_Fast = false;
	fGDI_QDCompatible = false;
	fGDI_HDC_FromBeginUsingParentContext = false;
	fGDI_GC = NULL;
	fGDI_SaveClipCounter = 0;
	fGDI_SaveContextCounter = 0;
	fGDI_PushedClipCounter = 0;
	fGDI_LastPushedClipCounter.reserve(16);
	fGDI_ClipRect.reserve(16);
	fGDI_Restore.reserve(16);
	fGDI_IsReleasing = false;
	fGDI_CurIsPreparedForTransparent = false;
	fGDI_CurIsParentDC = false;
	fLockParentContext = D2D_RENDER_TARGET_USE_GDI_COMPATIBLE_DC ? false : true;
	fParentContextNoDraw = false;
	fUnlockNoRestoreDrawingContext = false;
	fHDCBindRectDirty = true;
	fHDCBindDCDirty = true;

	fOwnRT=true;
	fDPI = 96.0f;
	fIsRTHardware = FALSE;
	fIsRTSoftwareOnly = false;

	fTextFont = NULL;
	fTextFontMetrics = NULL;
	fWhiteSpaceWidth = 0.0f;

	fGlobalAlpha = 1.0;

	fShadowHOffset = 0.0;
	fShadowVOffset = 0.0;
	fShadowBlurness = 0.0;

	fDrawingMode = DM_NORMAL;
	fTextRenderingMode = fHighQualityTextRenderingMode = TRM_NORMAL;
	fTextMeasuringMode = TMM_NORMAL;

	fStrokeWidth = 1.0f;
	fStrokeStyleProps = D2D1::StrokeStyleProperties();
	fStrokeDashPattern = NULL;
	fStrokeDashCount = 0;
	fStrokeDashCountInit = 0;
	fIsStrokeDotPattern = false;
	fIsStrokeDashPatternUnitEqualToStrokeWidth = false;

	fMutexFactory.Lock();
	HRESULT hr = GetFactory()->CreateStrokeStyle( &fStrokeStyleProps, NULL, 0, &fStrokeStyle);
	xbox_assert(SUCCEEDED(hr) && fStrokeStyle);
	fMutexFactory.Unlock();

	fBrushPos.SetPos(0,0);

	fHasClipOverride = false;

	fLayerOffScreenCount = 0;

#if D2D_GDI_USE_GDIPLUS_BITMAP_FOR_TRANSPARENCY
	fGdiplusBmp = NULL;
	fGdiplusGC = NULL;
	//fGDI_TransparentBmp = NULL;
	//fGDI_TransparentGdiplusGC = NULL;
	//fGDI_TransparentHDC = NULL;
#endif
	fBackingStoreGdiplusBmp = NULL;

	fClipRegionInit = NULL;
	fClipRegionCur = NULL;
	fLockSaveClipRegion = NULL;
}



void VWinD2DGraphicContext::_Dispose()
{
#if VERSIONDEBUG
	xbox_assert(fTaskUseCount == 0);
#endif
	xbox_assert(fUseCount == 0);

	xbox_assert(fGDI_HDC == NULL);
	xbox_assert(fLayerOffScreenCount == 0);

	xbox_assert(fClipRegionInit == NULL);
	xbox_assert(fClipRegionCur == NULL);
	xbox_assert(fLockSaveClipRegion == NULL);

	if (fBackingStoreGdiplusBmp)
		delete fBackingStoreGdiplusBmp;
	fBackingStoreGdiplusBmp = NULL;

#if D2D_GDI_USE_GDIPLUS_BITMAP_FOR_TRANSPARENCY
	if (fGdiplusBmp)
	{
		//release Gdiplus gc & bmp
		xbox_assert(fGdiplusGC && fHDC);
		fGdiplusGC->ReleaseHDC( fHDC);
		delete fGdiplusGC;
		fGdiplusGC = NULL;
		{
			VTaskLock lock(&(fMutexResources[D2D_CACHE_RESOURCE_SOFTWARE]));
			if (fGdiplusSharedBmpInUse && fGdiplusBmp == fGdiplusSharedBmp)
			{
				fGdiplusBmp = NULL;
				fGdiplusSharedBmpInUse = false;
			}
		}
		if (fGdiplusBmp)
		{
			delete fGdiplusBmp;
			fGdiplusBmp = NULL;
		}
	}
	//xbox_assert(fGDI_TransparentGdiplusGC == NULL);
	//xbox_assert(fGDI_TransparentBmp == NULL);
	//if (fGDI_TransparentGdiplusGC)
	//	delete fGDI_TransparentGdiplusGC;
	//fGDI_TransparentGdiplusGC = NULL;
	//if (fGDI_TransparentBmp)
	//	delete fGDI_TransparentBmp;
	//fGDI_TransparentBmp = NULL;
#endif

#if D2D_GUI_SINGLE_THREADED
	{
	VTaskLock lock(&(fMutexResources[fIsRTHardware]));
#endif
	_DisposeDeviceDependentObjects(false);
#if D2D_GUI_SINGLE_THREADED
	}
#endif

	if (fStrokeDashPattern)
		delete [] fStrokeDashPattern;
	fStrokeDashPattern = NULL;
	fStrokeDashCount = 0;
	fStrokeDashCountInit = 0;
	fStrokeDashPatternInit.clear();
	fStrokeStyle = (ID2D1StrokeStyle *)NULL;

	fWICBitmap = (IWICBitmap *)NULL;
}

void VWinD2DGraphicContext::_InitDefaultFont() const
{
	StParentContextNoDraw hdcNoDraw(this);

	if (fTextFont == NULL)
	{
		HDC hdc = _GetHDC();

		LOGFONTW logFont;
		memset( &logFont, 0, sizeof(LOGFONTW));
		HGDIOBJ	fontObj = ::GetCurrentObject(hdc, OBJ_FONT);
		if (fontObj)
		{
			::GetObjectW(fontObj, sizeof(LOGFONTW), (void *)&logFont);
			_ReleaseHDC(hdc);

			VFontFace face = 0;
			if(logFont.lfItalic)
				face |= KFS_ITALIC;
			if(logFont.lfUnderline)
				face |= KFS_UNDERLINE;
			if(logFont.lfStrikeOut)
				face |= KFS_STRIKEOUT;
			if(logFont.lfWeight == FW_BOLD)
				face |= KFS_BOLD;

			GReal sizePoint = Abs(logFont.lfHeight);  //as we assume 4D form 72 dpi 1pt = 1px

			fTextFont = VFont::RetainFont( VString(logFont.lfFaceName), face, sizePoint, 72, false);
		}
		else
		{
			_ReleaseHDC(hdc);

			fTextFont = VFont::RetainStdFont(STDF_CAPTION);
			//VString familyName;
			//VFont::GetGenericFontFamilyName( GENERIC_FONT_SERIF, familyName);
			//fTextFont = VFont::RetainFont( familyName, KFS_NORMAL, 12, false);
		}
		if (fTextFontMetrics)
			delete fTextFontMetrics;
		fTextFontMetrics = new VFontMetrics( fTextFont, static_cast<const VGraphicContext *>(this));
	}
}

void VWinD2DGraphicContext::_InitDeviceDependentObjects() const
{
	//create render target
	xbox_assert(fUseCount == 0);

	if (fRenderTarget == NULL)
	{
		xbox_assert( fGDI_HDC == NULL);

		fOwnRT = true;

		if (fHDC)
		{
			//create a DC render target
			xbox_assert(fDCRenderTarget == NULL && fDPI == 96.0f);
			
			fIsRTHardware = FALSE;
			if (sUseHardwareIfPresent && sHardwareEnabled && (!fIsRTSoftwareOnly))
			{
				//try to create a hardware render target
				D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
					D2D1_RENDER_TARGET_TYPE_HARDWARE,
					D2D1::PixelFormat(
						DXGI_FORMAT_B8G8R8A8_UNORM,
						IsTransparent() ? D2D1_ALPHA_MODE_PREMULTIPLIED : D2D1_ALPHA_MODE_IGNORE), 
					fDPI,
					fDPI,
					D2D_RENDER_TARGET_USE_GDI_COMPATIBLE_DC ? D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE : D2D1_RENDER_TARGET_USAGE_NONE,
					D2D1_FEATURE_LEVEL_10
					);
				fMutexFactory.Lock();
				bool ok = SUCCEEDED(GetFactory()->CreateDCRenderTarget(&props, &fDCRenderTarget));
				if (!ok)
				{
					//perhaps video memory is full: dispose shared resources & try again
					_DisposeSharedResources(false);
					ok = SUCCEEDED(GetFactory()->CreateDCRenderTarget(&props, &fDCRenderTarget));
				}
				fMutexFactory.Unlock();
				if (ok)
				{
#if VERSIONDEBUG
					D2D1_PIXEL_FORMAT pf = fDCRenderTarget->GetPixelFormat();
					if (IsTransparent())
						xbox_assert(pf.alphaMode == D2D1_ALPHA_MODE_PREMULTIPLIED);
#endif
					fIsRTHardware = TRUE;

					HRESULT hr = fDCRenderTarget->QueryInterface(__uuidof(ID2D1RenderTarget), (void **)&fRenderTarget);
					xbox_assert(SUCCEEDED(hr));
					fHDCBindRectDirty = true;
					fHDCBindDCDirty = true;
				}
			}
			if (fDCRenderTarget == NULL)
			{
				//try to create a sofware render target
				D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
					D2D1_RENDER_TARGET_TYPE_SOFTWARE,
					D2D1::PixelFormat(
						DXGI_FORMAT_B8G8R8A8_UNORM,
						IsTransparent() ? D2D1_ALPHA_MODE_PREMULTIPLIED : D2D1_ALPHA_MODE_IGNORE), 
					fDPI,
					fDPI,
					D2D_RENDER_TARGET_USE_GDI_COMPATIBLE_DC ? D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE : D2D1_RENDER_TARGET_USAGE_NONE,
					D2D1_FEATURE_LEVEL_DEFAULT
					);
				fMutexFactory.Lock();
				bool ok = SUCCEEDED(GetFactory()->CreateDCRenderTarget(&props, &fDCRenderTarget));
				fMutexFactory.Unlock();
				xbox_assert(ok); //should always succeed

				HRESULT hr = fDCRenderTarget->QueryInterface(__uuidof(ID2D1RenderTarget), (void **)&fRenderTarget);
				fHDCBindRectDirty = true;
				fHDCBindDCDirty = true;
			}
			if (fRenderTarget && fIsShared)
			{
				//update shared tables if gc is a shared gc & if resource domain has changed
				bool  domainMightHaveChanged = true;
				{
					//check presence of gc in shared resource domain
					D2D_GUI_RESOURCE_MUTEX_LOCK(fIsRTHardware)

					MapOfSharedGC::const_iterator it = fMapOfSharedGC[fIsRTHardware].begin();
					for (;it != fMapOfSharedGC[fIsRTHardware].end(); it++)
					{
						if (it->second == (VGraphicContext *)(static_cast<const VGraphicContext *>(this))) //dirty but no choice here 
																										   //(no matter because we reinterpret the pointer & do not modify the class itself) 
						{
							//shared gc is in the correct resource domain: we do not need to do nothing
							domainMightHaveChanged = false;
							break;
						}
					}
				}
				if (domainMightHaveChanged)
				{
					//shared gc domain has changed so check presence in the other domain & update tables if appropriate

					BOOL domainOther = fIsRTHardware ? FALSE : TRUE;

					D2D_GUI_RESOURCE_MUTEX_LOCK(domainOther)

					MapOfSharedGC::iterator it = fMapOfSharedGC[domainOther].begin();
					for (;it != fMapOfSharedGC[domainOther].end(); it++)
					{
						if (it->second == (VGraphicContext *)static_cast<const VGraphicContext *>(this))
						{
							//shared gc is in another resource domain: move it to the new resource domain
							D2D_GUI_RESOURCE_MUTEX_LOCK(fIsRTHardware)

							sLONG handle = it->first;
							fMapOfSharedGC[domainOther].erase(it);
							if (fMapOfSharedGC[fIsRTHardware][handle])
								fMapOfSharedGC[fIsRTHardware][handle]->Release();
							fMapOfSharedGC[fIsRTHardware][handle] = (VGraphicContext *)(static_cast<const VGraphicContext *>(this)); //dirty but no choice here 
																																	 //(no matter because we reinterpret the pointer & do not modify the class itself) 
							break;
						}
					}
				}
			}
		}
		else if(fHwnd)
		{
			//create a HWND render target
			//(HWND render target is natively double-buffered so we do not need
			// to manage offscreen drawing as with Gdiplus impl)

			//while rendering on screen, we MUST use 96dpi in order to avoid dpi scaling:
			//(we need 1 DIP = 1 Pixel in order gdi coordinate space to be equal initially to D2D coordinate space)
			xbox_assert( fDPI == 96.0f);
			
			RECT rect;
			::GetClientRect( fHwnd, &rect);
			D2D1_SIZE_U size = D2D1::SizeU(rect.right-rect.left, rect.bottom-rect.top);
			fHDCBounds.FromRectRef( rect);

			//create a HWND render target
			xbox_assert(fHwndRenderTarget == NULL);

			D2D1_RENDER_TARGET_PROPERTIES props;
			D2D1_HWND_RENDER_TARGET_PROPERTIES propsHwnd = D2D1::HwndRenderTargetProperties(fHwnd, size, D2D1_PRESENT_OPTIONS_IMMEDIATELY|D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS);
			fIsRTHardware = FALSE;
			if (sUseHardwareIfPresent && sHardwareEnabled && (!fIsRTSoftwareOnly))
			{
				//try to create a hardware render target
				D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
					D2D1_RENDER_TARGET_TYPE_HARDWARE,
					D2D1::PixelFormat(
						DXGI_FORMAT_B8G8R8A8_UNORM,
						IsTransparent() ? D2D1_ALPHA_MODE_PREMULTIPLIED : D2D1_ALPHA_MODE_IGNORE), 
					fDPI,
					fDPI,
					D2D_RENDER_TARGET_USE_GDI_COMPATIBLE_DC ? D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE : D2D1_RENDER_TARGET_USAGE_NONE,
					D2D1_FEATURE_LEVEL_10
					);
				fMutexFactory.Lock();
				bool ok = SUCCEEDED(GetFactory()->CreateHwndRenderTarget(
							props,
							propsHwnd,
							&fHwndRenderTarget
							));
				if (!ok)
				{
					//perhaps video memory is full: dispose shared resources & try again
					_DisposeSharedResources(false);
					ok = SUCCEEDED(GetFactory()->CreateHwndRenderTarget(
								props,
								propsHwnd,
								&fHwndRenderTarget
								));
				}
				fMutexFactory.Unlock();
				if (ok)
					fIsRTHardware = TRUE;
			}
			if (fHwndRenderTarget == NULL)
			{
				//try to create a sofware render target
				D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
					D2D1_RENDER_TARGET_TYPE_SOFTWARE,
					D2D1::PixelFormat(
						DXGI_FORMAT_B8G8R8A8_UNORM,
						IsTransparent() ? D2D1_ALPHA_MODE_PREMULTIPLIED : D2D1_ALPHA_MODE_IGNORE), 
					fDPI,
					fDPI,
					D2D_RENDER_TARGET_USE_GDI_COMPATIBLE_DC ? D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE : D2D1_RENDER_TARGET_USAGE_NONE,
					D2D1_FEATURE_LEVEL_DEFAULT
					);
				fMutexFactory.Lock();
				bool ok = SUCCEEDED(GetFactory()->CreateHwndRenderTarget(
							props,
							propsHwnd,
							&fHwndRenderTarget
							));
				fMutexFactory.Unlock();
				xbox_assert(ok); //should always succeed
			}

			if (fHwndRenderTarget != NULL)
			{
				HRESULT hr = fHwndRenderTarget->QueryInterface(__uuidof(ID2D1RenderTarget), (void **)&fRenderTarget);
				xbox_assert(SUCCEEDED(hr));
			}
		}
		else
		{
			//create a WIC bitmap render target (software rendering only)
			if (fWICBitmap == NULL)
				xbox_assert(false); 
			else
			{
				//create WIC bitmap render target (gdi-compatible)

				fIsRTHardware = FALSE; //WIC bitmap render target is always software

				D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
				rtProps.type = D2D1_RENDER_TARGET_TYPE_SOFTWARE;
				rtProps.pixelFormat = D2D1::PixelFormat(
						DXGI_FORMAT_B8G8R8A8_UNORM,
						IsTransparent() ? D2D1_ALPHA_MODE_PREMULTIPLIED : D2D1_ALPHA_MODE_IGNORE); 
				rtProps.dpiX = rtProps.dpiY = fDPI;
				rtProps.usage = D2D_RENDER_TARGET_USE_GDI_COMPATIBLE_DC ? D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE : D2D1_RENDER_TARGET_USAGE_NONE;

				//create render target
				fMutexFactory.Lock();
				HRESULT result = GetFactory()->CreateWicBitmapRenderTarget(
									fWICBitmap,
									rtProps,
									&fRenderTarget);
				fMutexFactory.Unlock();
			}
		}
	}

	if (fRenderTarget != NULL)
	{
		//create default brushes
		if (fStrokeBrush == NULL)
			if (SUCCEEDED(fRenderTarget->CreateSolidColorBrush( D2D1::ColorF( 0, 0, 0, fGlobalAlpha), &fStrokeBrushSolid)))
			{
				HRESULT hr = fStrokeBrushSolid->QueryInterface(__uuidof(ID2D1Brush), (void **)&fStrokeBrush);
				xbox_assert(SUCCEEDED(hr));
			}
		if (fFillBrush == NULL)
			if (SUCCEEDED(fRenderTarget->CreateSolidColorBrush( D2D1::ColorF( 0, 0, 0, fGlobalAlpha), &fFillBrushSolid)))
			{
				HRESULT hr = fFillBrushSolid->QueryInterface(__uuidof(ID2D1Brush), (void **)&fFillBrush);
				xbox_assert(SUCCEEDED(hr));
			}
		if (fTextBrush == NULL)
		{
			if (fUseShapeBrushForText)
				fTextBrush = fFillBrush;
			else
				if (SUCCEEDED(fRenderTarget->CreateSolidColorBrush( D2D1::ColorF( 0, 0, 0, fGlobalAlpha), &fTextBrushSolid)))
				{
					HRESULT hr = fTextBrushSolid->QueryInterface(__uuidof(ID2D1Brush), (void **)&fTextBrush);
					xbox_assert(SUCCEEDED(hr));
				}
		}

		//create font
		_InitDefaultFont();
	}

	fTextRect = D2D1::RectF(0.0f,0.0f,DWRITE_TEXT_LAYOUT_MAX_WIDTH,DWRITE_TEXT_LAYOUT_MAX_HEIGHT);
}

void VWinD2DGraphicContext::DisposeSharedResources(int inHardware)
{
	VTaskLock lock(&(fMutexResources[inHardware]));

	//dispose shared layers
	fVectorOfLayer[inHardware].clear();

	//dispose shared render targets
	MapOfSharedGC::iterator it;
	for (it = fMapOfSharedGC[inHardware].begin(); it != fMapOfSharedGC[inHardware].end(); it++)
		it->second->Release();
	fMapOfSharedGC[inHardware].clear();

	//dispose cache of D2D icons
	fCachedIconsD2D[inHardware].clear();

	//dispose cache of bitmaps
	fCachedBmp[inHardware].clear();
	fCachedBmpTransparentNoAlpha[inHardware].clear();
#if VERSIONDEBUG
#if D2D_LOG_ENABLE
	gD2DCachedBitmapCount[inHardware] = 0;
	if (inHardware)
		xbox::DebugMsg( "VWinD2DGraphicContext: hardware cached D2D bitmaps = 0\n");
	else
		xbox::DebugMsg( "VWinD2DGraphicContext: software cached D2D bitmaps = 0\n");
#endif
#endif
}


void VWinD2DGraphicContext::_DisposeSharedResources(bool inDisposeResourceRT) const
{
	for (int typeRsc = D2D_CACHE_RESOURCE_SOFTWARE;  typeRsc <= D2D_CACHE_RESOURCE_HARDWARE; typeRsc++)
	{
		//dispose shared resources but the resource domain render target
		DisposeSharedResources( typeRsc);

		//dispose resource domain render target
		if (inDisposeResourceRT)
		{
			VTaskLock lock(&(fMutexResources[typeRsc]));
			fRscRenderTarget[typeRsc] = (ID2D1DCRenderTarget *)NULL;
		}
	}
}

void VWinD2DGraphicContext::_DisposeDeviceDependentObjects(bool inDisposeSharedResources) const
{
	xbox_assert(fGDI_HDC == NULL);

	//clear gradient stop collection cache
	fMapOfGradientStopCollection.clear();
	fQueueOfGradientStopCollectionID.clear();

	//clear clipping stack
	while (!fStackLayerClip.empty())
		fStackLayerClip.pop_back();
	while (!fLockSaveStackLayerClip.empty())
		fLockSaveStackLayerClip.pop_back();

	//clear context stack
	fDrawingContext.Clear();

	fStrokeBrush = (ID2D1Brush *)NULL;
	fFillBrush = (ID2D1Brush *)NULL;
	fTextBrush = (ID2D1Brush *)NULL;

	fStrokeBrushSolid = (ID2D1SolidColorBrush *)NULL;
	fFillBrushSolid = (ID2D1SolidColorBrush *)NULL;
	fTextBrushSolid = (ID2D1SolidColorBrush *)NULL;

	fStrokeBrushLinearGradient = (ID2D1LinearGradientBrush *)NULL;
	fStrokeBrushRadialGradient = (ID2D1RadialGradientBrush *)NULL;
	fFillBrushLinearGradient = (ID2D1LinearGradientBrush *)NULL;
	fFillBrushRadialGradient = (ID2D1RadialGradientBrush *)NULL;

	if (fTextFont != NULL)
		fTextFont->Release();
	fTextFont = NULL;
	if (fTextFontMetrics)
		delete fTextFontMetrics;
	fTextFontMetrics = NULL;
	fWhiteSpaceWidth = 0.0f;

	fOwnRT = true;
	fRenderTarget = (ID2D1RenderTarget *)NULL;
	fDCRenderTarget = (ID2D1DCRenderTarget *)NULL;
	fHwndRenderTarget = (ID2D1HwndRenderTarget *)NULL;

	if (inDisposeSharedResources)
	{
		//dispose shared resources & rebuild resource render target
		_DisposeSharedResources(true);
		_InitSharedResources();
	}
}

void VWinD2DGraphicContext::GetTransform(VAffineTransform &outTransform)
{
	_GetTransform(outTransform);
}

void VWinD2DGraphicContext::_GetTransform(VAffineTransform &outTransform) const
{
	if (fRenderTarget == NULL || fUseCount <= 0)
	{
		outTransform.MakeIdentity();
		outTransform.SetTranslation(-fViewportOffset.GetX(), -fViewportOffset.GetY());
		return;
	}
	//else if (fUseCount > 0 && fLockCount > 0 && fGDI_HDC)
	//{
	//	xbox_assert(fGDI_GC);
	//	/*
	//	if (fGDI_TransparentGdiplusGC)
	//		outTransform = fGDI_TransparentBackupTransform;
	//	else
	//	*/
	//		outTransform = fLockSaveTransform;
	//	return;
	//}
	D2D1_MATRIX_3X2_F mat;
	fRenderTarget->GetTransform( &mat);
	outTransform.FromNativeTransform( (D2D_MATRIX_REF)&mat);
}


void VWinD2DGraphicContext::SetTransform(const VAffineTransform &inTransform)
{
	StUseContext_NoRetain	context(this);
	FlushGDI();

	D2D1_MATRIX_3X2_F mat;
	inTransform.ToNativeMatrix( (D2D_MATRIX_REF)&mat);
	fRenderTarget->SetTransform( mat); 
#if VERSIONDEBUG
	//VAffineTransform ctm;
	//_GetTransform( ctm);
	//xbox_assert(ctm == inTransform);
#endif
}

void VWinD2DGraphicContext::_SetTransform(const VAffineTransform &inTransform) const
{
	if (fRenderTarget == NULL)
		return;

	FlushGDI();

	D2D1_MATRIX_3X2_F mat;
	inTransform.ToNativeMatrix( (D2D_MATRIX_REF)&mat);
	fRenderTarget->SetTransform( mat); 
#if VERSIONDEBUG
	//VAffineTransform ctm;
	//_GetTransform( ctm);
	//xbox_assert(ctm == inTransform);
#endif
}


void VWinD2DGraphicContext::ConcatTransform(const VAffineTransform &inTransform)
{
	StUseContext_NoRetain	context(this);
	FlushGDI();

	VAffineTransform ctm;
	GetTransform( ctm);
	ctm.Multiply( inTransform, VAffineTransform::MatrixOrderPrepend);

	D2D1_MATRIX_3X2_F mat;
	ctm.ToNativeMatrix( (D2D_MATRIX_REF)&mat);
	fRenderTarget->SetTransform(mat); 
}
void VWinD2DGraphicContext::RevertTransform(const VAffineTransform &inTransform)
{
	ConcatTransform(inTransform.Inverse());
}
void VWinD2DGraphicContext::ResetTransform()
{
	StUseContext_NoRetain	context(this);
	FlushGDI();
	if (fRenderTarget)
		fRenderTarget->SetTransform( D2D1::IdentityMatrix()); 
}

void VWinD2DGraphicContext::BeginUsingContext(bool inNoDraw)
{
#if VERSIONDEBUG
	//::DebugMsg("VWinD2DGraphicContext::BeginUsingContext: current task ID = %x\n", VTask::GetCurrentID());
#endif

	if (!inNoDraw && (fGdiplusBmp || fWICBitmap))
	{
		//we are about to write on the bitmap: ensure backing store is released
		fBackingStoreWICBmp = (IWICBitmap *)NULL;
		if (fBackingStoreGdiplusBmp)
		{
			delete fBackingStoreGdiplusBmp;
			fBackingStoreGdiplusBmp = NULL;
		}
	}

	StParentContextDraw hdcDraw(this, !inNoDraw);

	if (fLockCount > 0)
	{
		if (fUseCount > 0)
		{
			fUseCount++;
			fLockSaveUseCount = fUseCount;
		}
		else 
			fLockSaveUseCount++;
		return;
	}

	fUseCount++;

	if (fUseCount == 1)
	{
#if VERSIONDEBUG
		fDebugLockGDIInteropCounter = 0;
		fDebugLockGDIParentCounter = 0;
#endif
		xbox_assert(fGDI_HDC == NULL);

		xbox_assert(fClipRegionInit == NULL);
		xbox_assert(fClipRegionCur == NULL);
		xbox_assert(fLockSaveClipRegion == NULL);
		
		//build device dependent resources if not done yet
		fUseCount = 0;

		if (fRenderTarget != NULL)
		{
			//reset render target if hardware status has changed
			if (fIsRTHardware)
			{
				if ((!IsHardwareEnabled()) || fIsRTSoftwareOnly)
					_DisposeDeviceDependentObjects();
			}
			else
			{
				if (IsHardwareEnabled() && (!fIsRTSoftwareOnly))
					_DisposeDeviceDependentObjects();
			}
		}

		_InitDeviceDependentObjects();

		if (inNoDraw && (fHDC || fHwnd)) //for bitmap context (fHDC = NULL && fHwnd == NULL), we need always to begin context in order to be able to get GDI context from bitmap later
			return;

#if VERSIONDEBUG
		fTaskUseCount++;
		xbox_assert(fTaskUseCount == 1);
#endif

		//begin draw
		if (fRenderTarget != NULL)
		{
			//set initial clipping bounds
			fClipBoundsInit = fHDCBounds;
			if (fHDC || fHwnd)
			{
				//inherit gdi clipping bounds
				VRect boundsGDI;
				if (fHDC)
					//get HDC clipping
					GetClipBoundingBox( fHDC, boundsGDI, true);
				else
					//get HWND clipping
					GetClipBoundingBox( fHwnd, boundsGDI, true);
				
				//transform GDI bounds (in pixels) to render target coordinate space (in DIP)
				if (fDPI != 96.0f)
				{
					//convert to DIP bounds
					boundsGDI.SetX( boundsGDI.GetX() * 96.0f / fDPI);
					boundsGDI.SetY( boundsGDI.GetY() * 96.0f / fDPI);
					boundsGDI.SetWidth( boundsGDI.GetWidth() * 96.0f / fDPI);
					boundsGDI.SetHeight( boundsGDI.GetHeight() * 96.0f / fDPI);
					boundsGDI.NormalizeToInt();
				}
				fClipBoundsInit.Intersect( boundsGDI);
			}
			if (fHDC)
			{
				/* 
				if (fDPI == 96.0f)
				{
					intersect HDC bounds with initial clipping
					if (fHDCBounds != fClipBoundsInit)
					{
						fHDCBounds = fClipBoundsInit;
						fHDCBindRectDirty = fHDCBindRectDirty || fHDCBounds != fHDCBindRectLast;
					}
				}
				*/
				if (fDCRenderTarget != NULL && (fHDCBindDCDirty || fHDCBindRectDirty))
				{
					//bind to new HDC or bounds

					if (fDPI != 96.0f)
					{
						//transform bounds in order to take account DPI scaling
						//(page scaling)
						GReal DIPToPixel = fDPI/96.0f;
						VRect bounds;
						bounds.SetX( floor(fHDCBounds.GetX()*DIPToPixel+0.5f));
						bounds.SetY( floor(fHDCBounds.GetY()*DIPToPixel+0.5f));
						bounds.SetWidth( floor(fHDCBounds.GetWidth()*DIPToPixel+0.5f));
						bounds.SetHeight( floor(fHDCBounds.GetHeight()*DIPToPixel+0.5f));
						RECT rect;
						bounds.ToRectRef( rect);
						HRESULT hr = fDCRenderTarget->BindDC( fHDC, &rect);
						if (FAILED(hr))
						{
							//try to free all shared memory
							_DisposeDeviceDependentObjects(true);
							_InitDeviceDependentObjects();
							xbox_assert(fRenderTarget != NULL);
							HRESULT hr = fDCRenderTarget->BindDC( fHDC, &rect);
							xbox_assert(SUCCEEDED(hr));
						}
					}
					else
					{
						RECT rect;
						fHDCBounds.ToRectRef( rect);
						HRESULT hr = fDCRenderTarget->BindDC( fHDC, &rect);
						if (FAILED(hr))
						{
							//try to free all shared memory
							_DisposeDeviceDependentObjects(true);
							_InitDeviceDependentObjects();
							xbox_assert(fRenderTarget != NULL);
							HRESULT hr = fDCRenderTarget->BindDC( fHDC, &rect);
							xbox_assert(SUCCEEDED(hr));
						}
					}
					fHDCBindRectLast = fHDCBounds;
					fHDCBindRectDirty = false;
					fHDCBindDCDirty = false;
				}
			}
			else if (fHwnd)
			{
				//resize render target if window has been resized
				RECT rect;
				::GetClientRect( fHwnd, &rect);
				GReal width = rect.right-rect.left;
				GReal height = rect.bottom-rect.top;
				if (width != fHDCBounds.GetWidth() || height != fHDCBounds.GetHeight())
				{
					xbox_assert(fHwndRenderTarget != NULL);
					D2D1_SIZE_U size;
					size.width = (UINT32)width;
					size.height = (UINT32)height;
					fHwndRenderTarget->Resize( size);
				}
			}

			//transform to render target coordinate space
			fClipBoundsInit.SetPosBy( -fHDCBounds.GetX(), -fHDCBounds.GetY());
			if (fClipRegionInit)
				fClipRegionInit->SetPosBy( -fHDCBounds.GetX(), -fHDCBounds.GetY());
			fClipBoundsCur = fClipBoundsInit;
			if (fClipRegionInit)
				fClipRegionCur = new VRegion( fClipRegionInit);

			fUseCount = 1;

			fMutexDraw.Lock();
			fRenderTarget->BeginDraw();

			if (fIsHighQualityAntialiased)
				EnableAntiAliasing();
			else
				DisableAntiAliasing();

			fTextRenderingMode = ~fHighQualityTextRenderingMode;
			SetTextRenderingMode( fHighQualityTextRenderingMode);

			//set initial transform 
			VAffineTransform ctm;
			ctm.SetTranslation( -fViewportOffset.GetX(), -fViewportOffset.GetY()); //if bitmap context = viewport offset, otherwise should be (0,0)
			ctm.SetTranslation( -fHDCBounds.GetX(), -fHDCBounds.GetY()); //if bitmap context should be equal to 0,0 (bitmap origin == rt origin), otherwise should be equal to parent dc viewport offset
			SetTransform( ctm);

			fHasClipOverride = false;
			if (fHDC || fHwnd)
				PushClipInit();

#if VERSIONDEBUG
			#if D2D_LOG_ENABLE
			xbox::DebugMsg("VWinD2DGraphicContext: begin using D2D context: ");
			VString type;
			type =	VString("") + 
					((fGdiplusBmp || fWICBitmap != (IWICBitmap *)NULL) ? "Bitmap context - " : (fHDC ? "HDC context - " : "HWND context - ")) +
					(fIsRTHardware ? "hardware - " : "software - ") +
					(fIsTransparent ? "transparent" : "opaque") +
					"\n";
			xbox::DebugMsg(type);
			#endif
#endif
		}
	}
}


/** set to true to speed up GDI over D2D render context rendering 
@remarks
	that mode optimizes GDI over D2D rendering so that GDI over D2D context is preserved much longer
	but it means that some rendering is done by GDI but D2D (like pictures or drawing simple lines)

	note that clearing that mode forces a GDI flush

	that mode is disabled on default
	that mode does nothing in context but D2D
*/
void VWinD2DGraphicContext::EnableFastGDIOverD2D( bool inEnable)
{
	if (inEnable)
	{
		if (!fGDI_Fast)
		{
			if (fUseCount <= 0)
				return;
			FlushGDI();
			fGDI_Fast = true;
			//open GDI context so it is still preserved on EndUsingParentContext
			//(also save current context)
			HDC hdc = BeginUsingParentContext();
			EndUsingParentContext( hdc);
		}
	}
	else 
		if (fGDI_Fast)
		{
			FlushGDI(); //flush GDI & restore context 
			fGDI_Fast = false;
		}
}



/** flush actual GDI context (created from render target) */
void VWinD2DGraphicContext::FlushGDI() const
{
	if (fGDI_HDC)
	{
		xbox_assert(fLockCount > 0);
		xbox_assert(fGDI_GC);

		//restore context & clipping on the stack
		//(important to ensure GDI context is properly restored...)
		for (int i = fGDI_Restore.size(); i > 0; i--)
		{
			if (fGDI_Restore[i-1] == eSave_Clipping)
				fGDI_GC->RestoreClip();
			else if (fGDI_Restore[i-1] == eSave_Context)
				fGDI_GC->RestoreContext();
		}

		//release render target GDI dc 
		HDC hdc = fGDI_HDC;
		fGDI_HDC = NULL; //reset to ensure EndUsingParentContext or _ReleaseHDC will actually release GDI dc
		fGDI_GC->EndUsingContext();
		fGDI_GC->Release();
		fGDI_GC = NULL;
		xbox_assert(fHDCOpenCount == 0); //if assert has failed here, it is because you have called graphic method from this gc inside BeginUsingParentContext/EndUsingParentContext: 
										 //this is not allowed because you cannot access this context while a overlay GDI context is locked:
									     // so we assert it but we deal with it gracefully to avoid crashing in release: but you should fix the calling code to avoid this assert in debug
										 // and to avoid perf issues
		if (fGDI_HDC_FromBeginUsingParentContext)
		{
			fHDCOpenCount = 2;
			EndUsingParentContext( hdc);
		}
		else
		{
			fHDCOpenCount = 1;
			_ReleaseHDC( hdc, false);
		}
	}
}

void VWinD2DGraphicContext::EndUsingContext()
{
	if (fLockCount > 0)
	{
		if (fUseCount <= 0)
		{
			fLockSaveUseCount--;
			if (fLockSaveUseCount > 0)
				return;
			fUseCount = fLockSaveUseCount = 0;
		}
		else
		{
			fUseCount--;
			fLockSaveUseCount = fUseCount;
			if (fUseCount > 0)
				return;
			else
				fUseCount = fLockSaveUseCount = 1;
		}
	}
	fUnlockNoRestoreDrawingContext = fUseCount <= 1;
	FlushGDI();
	fUnlockNoRestoreDrawingContext = false;
	xbox_assert(fLockCount == 0);

	if (fUseCount <= 0)
		return;
	
	fUseCount--;
	if (fUseCount == 0)
	{
		// end draw
		if (fRenderTarget != NULL)
		{
			//pop all clipping layers
			fUseCount++;
			while (!fStackLayerClip.empty())
				RestoreClip();
			xbox_assert(fLayerOffScreenCount == 0);

			SetTransform( VAffineTransform());
			fUseCount = 0;
			fLockCount = 0;

			HRESULT hr = fRenderTarget->EndDraw();

			if (hr == D2DERR_RECREATE_TARGET || hr == E_HANDLE) //if display has changed error might be E_HANDLE
			{
#if D2D_LOG_ENABLE
				xbox::DebugMsg("VWinD2DGraphicContext: driver or display has changed -> dispose & rebuild device dependent resources \n");
#endif
				//can happen only for hardware render target 
				//if hardware or driver has changed 
				//so dispose ALL device dependent resources 
				//(they will be rebuild on next BeginUsingContext)
				_DisposeDeviceDependentObjects(true);
			}
			else if (FAILED(hr))
			{
#if VERSIONDEBUG
				//VString error( DXGetErrorStringW(hr));
				
				xbox_assert(false);
#endif
#if D2D_LOG_ENABLE
				xbox::DebugMsg("VWinD2DGraphicContext: fatal or unknown error -> fallback to software mode & rebuild all resources\n");
#endif
				//fatal or unknown error: dispose ALL device dependent resources & fallback to software mode (secure) only
				VTaskLock lock(&fMutexFactory);
				sUseHardwareIfPresent = false;
				sHardwareEnabled = false;
				_DisposeDeviceDependentObjects(true);
			}

			fMutexDraw.Unlock();
		}

		if (fClipRegionInit)
		{
			delete fClipRegionInit;
			fClipRegionInit = NULL;
		}
		if (fClipRegionCur)
		{
			delete fClipRegionCur;
			fClipRegionCur = NULL;
		}
		if (fLockSaveClipRegion)
		{
			delete fLockSaveClipRegion;
			fLockSaveClipRegion = NULL;
		}
		xbox_assert(fDrawingContext.IsEmpty());

#if VERSIONDEBUG
		fTaskUseCount--;
		xbox_assert(fTaskUseCount == 0);
		#if D2D_LOG_ENABLE
		xbox::DebugMsg("VWinD2DGraphicContext: end using D2D context:\n");
		xbox::DebugMsg("\tcount of GDI interop context opened = %d\n\tcount of GDI parent context opened = %d\n", fDebugLockGDIInteropCounter, fDebugLockGDIParentCounter);
		{
		VTaskLock lock(&(fMutexResources[D2D_CACHE_RESOURCE_HARDWARE]));
		xbox::DebugMsg("\tcount of shared hardware bitmaps remaining = %d\n", gD2DCachedBitmapCount[D2D_CACHE_RESOURCE_HARDWARE]);
		}
		{
		VTaskLock lock(&(fMutexResources[D2D_CACHE_RESOURCE_SOFTWARE]));
		xbox::DebugMsg("\tcount of shared software bitmaps remaining = %d\n", gD2DCachedBitmapCount[D2D_CACHE_RESOURCE_SOFTWARE]);
		}
		#endif
#endif
	}
}

void VWinD2DGraphicContext::Flush()
{
	if (fUseCount <= 0)
		return;
	FlushGDI();

	if (fRenderTarget != NULL)
	{
		//here we do not use ID21RenderTarget::Flush because it would pop all outstanding layers & clipping:
		//we use _Lock() & _Unlock() because _Unlock will restore layers & clipping
		bool oldLockType = fLockParentContext;
		fLockParentContext = true; //ensure render target is presented now
		StParentContextNoDraw hdcNoDraw(this);
		_Lock(); 
		_Unlock();
		fLockParentContext = oldLockType;
	}
}

/** flush all drawing commands and allow temporary read/write access to render target gdi-compatible dc
	or to attached hdc or hwnd
@remarks
	this is useful if you want to access to a render target between a BeginUsingContext/EndUsingContext
	call _Unlock to resume drawing

	do nothing if outside BeginUsingContext/EndUsingContext
*/
void VWinD2DGraphicContext::_Lock() const
{
#if VERSIONDEBUG
	if (fRenderTarget == NULL)
		xbox_assert(fUseCount == 0);  
#endif

	if (fGDI_HDC)
	{
		xbox_assert(fLockCount > 0);
		fLockCount++;
		return;
	}

	if (fLockCount > 0)
	{
		fLockCount++;
		return;
	}

	if (fUseCount <= 0)
	{
		fLockSaveTransform.MakeIdentity();
		fLockSaveHasClipOverride = false;
		fLockSaveClipBounds = fHDCBounds;
		xbox_assert(fClipRegionCur == NULL);
		fLockSaveClipRegion = NULL;
		fLockSaveUseCount = 0;
		return;
	}

	if (fGDI_Fast)
		_SaveContext();

	fLockCount++;

	//lock parent HDC only if not in a offscreen layer
	bool lockParentContext = fLockParentContext && (!fLayerOffScreenCount);

	//end draw

	//backup transform
	_GetTransform( fLockSaveTransform);

	//pop all clipping layers & backup it
	fLockSaveHasClipOverride = fHasClipOverride;
	fLockSaveClipBounds = fClipBoundsCur;
	fLockSaveClipRegion = fClipRegionCur ? new VRegion( fClipRegionCur) : NULL;
	xbox_assert(fLockSaveStackLayerClip.empty());
	while (!fStackLayerClip.empty())
	{
		if (!lockParentContext)
		{
			//if we want to draw directly over render target content
			//ensure we do not close last pushed offscreen layer
			//because we want to get access to the current layer render target
			_RestoreClipOverride();
			if (fStackLayerClip.empty())
				break;
			if (fStackLayerClip.back().IsImageOffScreen())
				break;
		}

		VImageOffScreenRef offscreen = _RestoreClip();
		if (offscreen)
			offscreen->Release();
	}
	
	//backup context use count
	fLockSaveUseCount = fUseCount;

	if (lockParentContext)
	{
		//flush current drawing & present render target in order to allow drawing in parent dc
		fRenderTarget->SetTransform( D2D1::IdentityMatrix()); 
		fUseCount = 0;
		HRESULT hr = fRenderTarget->EndDraw();
		if (!testAssert(SUCCEEDED((hr))))
		{
#if VERSIONDEBUG
			//VString error( DXGetErrorStringW(hr));
#endif

			fLockSaveUseCount = 0;
			while (!fLockSaveStackLayerClip.empty())
				fLockSaveStackLayerClip.pop_back();
		}
		fMutexDraw.Unlock();
	}
	else
	{
		//flush current drawing but do not present now render target:
		//if we need to draw with GDI, we will draw in render target dc if render target is GDI-compatible
		/* //useless to flush now 
		HRESULT hr = fRenderTarget->Flush();
		if (!testAssert(SUCCEEDED((hr))))
		{
#if VERSIONDEBUG
			//VString error( DXGetErrorStringW(hr));
#endif
			//pop all layers & release offscreen layers if any
			while (!fStackLayerClip.empty())
			{
				VImageOffScreenRef offscreen = _RestoreClip();
				if (offscreen)
					offscreen->Release();
			}
			
			//present render target
			fRenderTarget->EndDraw();
			fUseCount = 0;
			fLockSaveUseCount = 0;
			while (!fLockSaveStackLayerClip.empty())
				fLockSaveStackLayerClip.pop_back();
		}
		*/
	}
}

/** resume drawing after a _Lock() */
void	VWinD2DGraphicContext::_Unlock() const
{
	if (fLockCount <= 0)
		return;

	fLockCount--;
	if (fLockCount == 0)
	{
		if (fGDI_HDC)
		{
			fLockCount = 1;
			return;
		}

		if (fLockSaveUseCount <= 0)
			return;

		//resume drawing
		if (fRenderTarget != NULL)
		{
			if (fUnlockNoRestoreDrawingContext)
			{
				xbox_assert(fUseCount > 0);
				xbox_assert(fUseCount == fLockSaveUseCount);

				while (!fLockSaveStackLayerClip.empty())
					fLockSaveStackLayerClip.pop_back();
				fHasClipOverride = false;

				if (fLockSaveClipRegion)
				{
					delete fLockSaveClipRegion;
					fLockSaveClipRegion = NULL;
				}

				if (fGDI_Fast)
					//we need to restore context as it was saved in _Lock()
					_RestoreContext();
				return;
			}

			if (fUseCount <= 0)
			{
				if (fDCRenderTarget != NULL && (fHDCBindDCDirty || fHDCBindRectDirty))
				{
					//bind to new HDC or bounds

					if (fDPI != 96.0f)
					{
						//transform bounds in order to take account DPI scaling
						//(page scaling)
						GReal DIPToPixel = fDPI/96.0f;
						VRect bounds;
						bounds.SetX( floor(fHDCBounds.GetX()*DIPToPixel+0.5f));
						bounds.SetY( floor(fHDCBounds.GetY()*DIPToPixel+0.5f));
						bounds.SetWidth( floor(fHDCBounds.GetWidth()*DIPToPixel+0.5f));
						bounds.SetHeight( floor(fHDCBounds.GetHeight()*DIPToPixel+0.5f));
						RECT rect;
						bounds.ToRectRef( rect);
						HRESULT hr = fDCRenderTarget->BindDC( fHDC, &rect);
						if (FAILED(hr))
						{
							//try to free all shared memory
							_DisposeDeviceDependentObjects(true);
							_InitDeviceDependentObjects();
							xbox_assert(fRenderTarget != NULL);
							HRESULT hr = fDCRenderTarget->BindDC( fHDC, &rect);
							xbox_assert(SUCCEEDED(hr));
						}
					}
					else
					{
						RECT rect;
						fHDCBounds.ToRectRef( rect);
						HRESULT hr = fDCRenderTarget->BindDC( fHDC, &rect);
						if (FAILED(hr))
						{
							//try to free all shared memory
							_DisposeDeviceDependentObjects(true);
							_InitDeviceDependentObjects();
							xbox_assert(fRenderTarget != NULL);
							HRESULT hr = fDCRenderTarget->BindDC( fHDC, &rect);
							xbox_assert(SUCCEEDED(hr));
						}
					}
					fHDCBindRectLast = fHDCBounds;
					fHDCBindRectDirty = false;
					fHDCBindDCDirty = false;
				}

				fMutexDraw.Lock();
				fRenderTarget->BeginDraw();
				fClipBoundsCur = fClipBoundsInit;
				if (fClipRegionCur)
					delete fClipRegionCur;
				if (fClipRegionInit)
					fClipRegionCur = new VRegion( fClipRegionInit);
				else
					fClipRegionCur = NULL;
			}
			//restore use count 
			fUseCount = fLockSaveUseCount;
		
			//restore layers & clipping
			fHasClipOverride = false;
			while (!fLockSaveStackLayerClip.empty())
			{
				const VLayerClipElem& layerClip = fLockSaveStackLayerClip.back();

				if (layerClip.IsImageOffScreen())
					xbox_assert(false);
				else 
				{
					fStackLayerClip.push_back( layerClip);

					if (layerClip.IsLayer())
					{
						if (layerClip.GetGeometry())
						{
							//push geometry layer
							_SetTransform( layerClip.GetTransform());
							
							fRenderTarget->PushLayer(
								D2D1::LayerParameters(	D2D1::InfiniteRect(),
														layerClip.GetGeometry(),
														D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
														D2D1::IdentityMatrix(),
														layerClip.GetOpacity(),
														NULL,
														IsTransparent() ? D2D1_LAYER_OPTIONS_NONE : D2D1_LAYER_OPTIONS_INITIALIZE_FOR_CLEARTYPE),
								layerClip.GetLayer()
								);
						}
						else
						{
							//push opacity layer
							_SetTransform( VAffineTransform()); 
							
							fRenderTarget->PushLayer(
								D2D1::LayerParameters(	D2D_RECT(layerClip.GetBounds()), 
														NULL,
														D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
														D2D1::IdentityMatrix(),
														layerClip.GetOpacity(),
														NULL,
														IsTransparent() ? D2D1_LAYER_OPTIONS_NONE : D2D1_LAYER_OPTIONS_INITIALIZE_FOR_CLEARTYPE),
								layerClip.GetLayer()
								);
						}
					}
					else if (!layerClip.IsDummy())
					{
						//push axis-aligned rect clipping
						_SetTransform( VAffineTransform()); 
						fRenderTarget->PushAxisAlignedClip( D2D_RECT(layerClip.GetBounds()), 
															D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
					}
				}
				fLockSaveStackLayerClip.pop_back();
			}
			fHasClipOverride = fLockSaveHasClipOverride;
			fClipBoundsCur = fLockSaveClipBounds;
			if (fClipRegionCur)
				delete fClipRegionCur;
			fClipRegionCur = fLockSaveClipRegion ? fLockSaveClipRegion : NULL;
			fLockSaveClipRegion = NULL;

			//restore transform
			_SetTransform( fLockSaveTransform);

			if (fGDI_Fast)
				_RestoreContext();
		}
		else
		{
			while (!fLockSaveStackLayerClip.empty())
				fLockSaveStackLayerClip.pop_back();
			fHasClipOverride = false;

			if (fLockSaveClipRegion)
				delete fLockSaveClipRegion;
			fLockSaveClipRegion = NULL;
		}
	}
}




#pragma mark-


void VWinD2DGraphicContext::_SetFillColor(const VColor& inColor, VBrushFlags* ioFlags) const
{
	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	if (fGDI_Fast && fGDI_HDC)
	{
		xbox_assert(fLockCount > 0 && fGDI_GC);
		fGDI_GC->SetFillColor( inColor);

		fGDI_HasSolidFillColor.back() = true;
		fGDI_HasSolidFillColorInherit.back() = true;
		fGDI_FillColor.back() = inColor;
		return;
	}
	
	FlushGDI();

	fFillBrush = (ID2D1Brush *)NULL;
	fFillBrushLinearGradient = (ID2D1LinearGradientBrush *)NULL;
	fFillBrushRadialGradient = (ID2D1RadialGradientBrush *)NULL;

	if (fFillBrushSolid != NULL)
		fFillBrushSolid->SetColor( D2D_COLOR( inColor));
	else
	{
		if (FAILED(fRenderTarget->CreateSolidColorBrush( D2D_COLOR( inColor), &fFillBrushSolid)))
			return;
	}
	HRESULT hr = fFillBrushSolid->QueryInterface(__uuidof(ID2D1Brush), (void **)&fFillBrush);
	xbox_assert(SUCCEEDED(hr));
	fFillBrush->SetOpacity( fGlobalAlpha);

	if (ioFlags)
		ioFlags->fFillBrushChanged = 1;

	if (fUseShapeBrushForText)
	{
		fTextBrush = fFillBrush;
		if (ioFlags)
			ioFlags->fTextBrushChanged = 1;
	}
}

/** create gradient stop collection from the specified gradient pattern */
ID2D1GradientStopCollection *VWinD2DGraphicContext::_CreateGradientStopCollection( const VGradientPattern *p) const
{
	//create stop collection
	int numStop = p->GetGradientStops().size();
	if (numStop < 2)
		numStop = 2;
	VString sID;
	bool isLUT =  (p->GetGradientStyle() == GRAD_STYLE_LUT 
				   ||
				   p->GetGradientStyle() == GRAD_STYLE_LUT_FAST)
				   &&
				   p->GetGradientStops().size() >= 2;

	if (numStop <= D2D_GRADIENT_STOP_COLLECTION_CACHE_MAX_STOP_COUNT)
	{
		//try to re-use gradient stop collection from cache

		//build hash id from stops (one unicode character per color component & offset + one unicode character for extend mode)
		sID.EnsureSize( numStop*(4+1)+1);

		sID.AppendUniChar( p->GetWrapMode()+1);

		if (isLUT)
		{
			GradientStopVector::const_iterator it = p->GetGradientStops().begin();
			UniChar index = 1;
			for (;it != p->GetGradientStops().end(); it++)
			{
				sID.AppendUniChar( (index++)+(*it).second.GetRed());
				sID.AppendUniChar( (index++)+(*it).second.GetGreen());
				sID.AppendUniChar( (index++)+(*it).second.GetBlue());
				sID.AppendUniChar( (index++)+(*it).first*255);
				sID.AppendUniChar( (index++)+(*it).second.GetAlpha());
			}
		}
		else
		{
			UniChar index = 1;
			sID.AppendUniChar( (index++)+p->GetStartColor().GetRed());
			sID.AppendUniChar( (index++)+p->GetStartColor().GetGreen());
			sID.AppendUniChar( (index++)+p->GetStartColor().GetBlue());
			sID.AppendUniChar( (index++)+0);
			sID.AppendUniChar( (index++)+p->GetStartColor().GetAlpha());

			sID.AppendUniChar( (index++)+p->GetEndColor().GetRed());
			sID.AppendUniChar( (index++)+p->GetEndColor().GetGreen());
			sID.AppendUniChar( (index++)+p->GetEndColor().GetBlue());
			sID.AppendUniChar( (index++)+255);
			sID.AppendUniChar( (index++)+p->GetEndColor().GetAlpha());
		}

		MapOfGradientStopCollection::const_iterator it = fMapOfGradientStopCollection.find(sID);
		if (it != fMapOfGradientStopCollection.end())
		{
			//gradient stop collection is already in cache: retain it and return it
			ID2D1GradientStopCollection* rtGradientStops = it->second;
			rtGradientStops->AddRef();
			return rtGradientStops;
		}
	}

	//build new gradient stop collection
	D2D1_GRADIENT_STOP *gradientStops = numStop <= 8 ? fTempStops : new D2D1_GRADIENT_STOP [numStop];

	if (isLUT)
	{
		GradientStopVector::const_iterator it = p->GetGradientStops().begin();
		int index = 0;
		for (;it != p->GetGradientStops().end(); it++, index++)
		{
			gradientStops[index].color = D2D_COLOR( (*it).second);
			gradientStops[index].position = (*it).first;
		}
	}
	else
	{
		gradientStops[0].color = D2D_COLOR( p->GetStartColor());
		gradientStops[0].position = 0.0f;

		gradientStops[1].color = D2D_COLOR( p->GetEndColor());
		gradientStops[1].position = 1.0f;
	}

	//determine wrap mode
	D2D1_EXTEND_MODE extendMode;
	switch (p->GetWrapMode())
	{
	case ePatternWrapClamp:
		extendMode = D2D1_EXTEND_MODE_CLAMP; 
		break;
	case ePatternWrapTile:
		extendMode = D2D1_EXTEND_MODE_WRAP; 
		break;
	case ePatternWrapFlipX:
	case ePatternWrapFlipY:
	case ePatternWrapFlipXY:
		extendMode = D2D1_EXTEND_MODE_MIRROR; 
		break;
	default:
		assert(false);
		break;
	}

	ID2D1GradientStopCollection* rtGradientStops = NULL;
	if (FAILED(fRenderTarget->CreateGradientStopCollection(
				gradientStops,
				numStop,
				D2D1_GAMMA_2_2,
				extendMode,
				&rtGradientStops
				)))
	{
		if (numStop > 8)
			delete [] gradientStops;
		return NULL;
	}
	if (numStop > 8)
		delete [] gradientStops;

	//add gradient stop collection to internal texture cache
	if (!sID.IsEmpty())
	{
		if (fMapOfGradientStopCollection.size() >= D2D_GRADIENT_STOP_COLLECTION_CACHE_MAX)
		{
			//cache is full: free some textures to allocate space
			VString sIDFree;
			for (int i = 0; i < D2D_GRADIENT_STOP_COLLECTION_CACHE_FREE_COUNT; i++)
			{
				sIDFree = fQueueOfGradientStopCollectionID.front();
				MapOfGradientStopCollection::iterator it = fMapOfGradientStopCollection.find( sIDFree);
				if (it != fMapOfGradientStopCollection.end())
					fMapOfGradientStopCollection.erase( it);
				fQueueOfGradientStopCollectionID.pop_front();
			}
		}
		//add gradient stop collection to internal cache
		fQueueOfGradientStopCollectionID.push_back( sID);
		fMapOfGradientStopCollection[sID] = rtGradientStops;
	}

	return rtGradientStops;
}

//create linear gradient brush from the specified input pattern
void VWinD2DGraphicContext::_CreateLinearGradientBrush( const VPattern* inPattern, CComPtr<ID2D1Brush> &ioBrush, CComPtr<ID2D1LinearGradientBrush> &ioBrushLinear, CComPtr<ID2D1RadialGradientBrush> &ioBrushRadial) const
{
	const VAxialGradientPattern *p = static_cast<const VAxialGradientPattern*>(inPattern); 

	ioBrushLinear = (ID2D1LinearGradientBrush *)NULL;
	ioBrushRadial = (ID2D1RadialGradientBrush *)NULL;

	ID2D1GradientStopCollection* rtGradientStops = _CreateGradientStopCollection( static_cast<const VGradientPattern *>(p));
	if (rtGradientStops == NULL)
		return;

	//create linear gradient

	D2D1_MATRIX_3X2_F brushTransform;
	if (p->GetTransform().IsIdentity())
		brushTransform = D2D1::Matrix3x2F::Identity();
	else
		p->GetTransform().ToNativeMatrix( (D2D_MATRIX_REF)&brushTransform);

	if (SUCCEEDED(fRenderTarget->CreateLinearGradientBrush(
			D2D1::LinearGradientBrushProperties(
				D2D1::Point2F(p->GetStartPoint().GetX(), p->GetStartPoint().GetY()),
				D2D1::Point2F(p->GetEndPoint().GetX(), p->GetEndPoint().GetY())),
			D2D1::BrushProperties( fGlobalAlpha, brushTransform),
		    rtGradientStops,
			&ioBrushLinear)))
	{
		ioBrush = (ID2D1Brush *)NULL;
		HRESULT hr = ioBrushLinear->QueryInterface(__uuidof(ID2D1Brush), (void **)&ioBrush);
		xbox_assert(SUCCEEDED(hr));
	}
	rtGradientStops->Release();
}


//create radial gradient brush from the specified input pattern
void VWinD2DGraphicContext::_CreateRadialGradientBrush( const VPattern* inPattern, CComPtr<ID2D1Brush> &ioBrush, CComPtr<ID2D1LinearGradientBrush> &ioBrushLinear, CComPtr<ID2D1RadialGradientBrush> &ioBrushRadial) const
{
	const VRadialGradientPattern *p = static_cast<const VRadialGradientPattern*>(inPattern); 

	ioBrushLinear = (ID2D1LinearGradientBrush *)NULL;
	ioBrushRadial = (ID2D1RadialGradientBrush *)NULL;
	
	ID2D1GradientStopCollection* rtGradientStops = _CreateGradientStopCollection( static_cast<const VGradientPattern *>(p));
	if (rtGradientStops == NULL)
		return;

	//create radial gradient
	D2D1_POINT_2F center = D2D1::Point2F( p->GetEndPoint().GetX(), p->GetEndPoint().GetY());
	D2D1_POINT_2F gradientOriginOffset = D2D1::Point2F( p->GetStartPoint().GetX()-p->GetEndPoint().GetX(), 
														p->GetStartPoint().GetY()-p->GetEndPoint().GetY());

	D2D1_MATRIX_3X2_F brushTransform;
	if (p->GetTransform().IsIdentity())
		brushTransform = D2D1::Matrix3x2F::Identity();
	else
		p->GetTransform().ToNativeMatrix( (D2D_MATRIX_REF)&brushTransform);

	if (SUCCEEDED(fRenderTarget->CreateRadialGradientBrush(
			D2D1::RadialGradientBrushProperties(
				center,
				gradientOriginOffset,
				p->GetEndRadiusX(),
				p->GetEndRadiusY()
				),
			D2D1::BrushProperties( fGlobalAlpha, brushTransform),
			rtGradientStops,
			&ioBrushRadial)))
	{
		ioBrush = (ID2D1Brush *)NULL;
		HRESULT hr = ioBrushRadial->QueryInterface(__uuidof(ID2D1Brush), (void **)&ioBrush);
		xbox_assert(SUCCEEDED(hr));
	}
	rtGradientStops->Release();
}


void VWinD2DGraphicContext::_SetFillPattern(const VPattern* inPattern, VBrushFlags* ioFlags) const
{
	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	if (inPattern == NULL || !inPattern->IsGradient())
	{
		if (fGDI_Fast && fGDI_HDC)
		{
			xbox_assert(fLockCount > 0 && fGDI_GC);
			fGDI_GC->SetFillPattern( inPattern);

			if (inPattern == NULL)
			{
				//ensure pattern will be restored to solid color when restoring later D2D context
				VPattern *pattern = VPattern::RetainStdPattern( PAT_FILL_PLAIN);
				if (testAssert(pattern))
					CopyRefCountable(&(fGDI_FillPattern.back()), const_cast<const VPattern *>(pattern));
				ReleaseRefCountable(&pattern);
			}
			else
				CopyRefCountable(&(fGDI_FillPattern.back()), inPattern);

			fGDI_HasSolidFillColorInherit.back() = true;
			return;
		}

		FlushGDI();

		fFillBrushLinearGradient = (ID2D1LinearGradientBrush *)NULL;
		fFillBrushRadialGradient = (ID2D1RadialGradientBrush *)NULL;
		fFillBrush = (ID2D1Brush *)NULL;
		if (fFillBrushSolid != NULL)
		{
			HRESULT hr = fFillBrushSolid->QueryInterface(__uuidof(ID2D1Brush), (void **)&fFillBrush);
			xbox_assert(SUCCEEDED(hr));
		}
		if (ioFlags)
			ioFlags->fFillBrushChanged = 1;
		if (fUseShapeBrushForText)
		{
			fTextBrush = fFillBrush;
			if (ioFlags)
				ioFlags->fTextBrushChanged = 1;
		}
	}
	else
	{ 
		FlushGDI();
		if (inPattern->GetKind() == 'axeP')
		{
			_CreateLinearGradientBrush( inPattern, fFillBrush, fFillBrushLinearGradient, fFillBrushRadialGradient);
			if (ioFlags)
				ioFlags->fFillBrushChanged = 1;
			if (fUseShapeBrushForText)
			{
				fTextBrush = fFillBrush;
				if (ioFlags)
					ioFlags->fTextBrushChanged = 1;
			}
		}
		else if (inPattern->GetKind() == 'radP')
		{
			_CreateRadialGradientBrush( inPattern, fFillBrush, fFillBrushLinearGradient, fFillBrushRadialGradient);
			if (ioFlags)
				ioFlags->fFillBrushChanged = 1;
			if (fUseShapeBrushForText)
			{
				fTextBrush = fFillBrush;
				if (ioFlags)
					ioFlags->fTextBrushChanged = 1;
			}
		}
	}
}


void VWinD2DGraphicContext::_SetLineColor(const VColor& inColor, VBrushFlags* ioFlags) const
{
	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	if (fGDI_Fast && fGDI_HDC)
	{
		xbox_assert(fLockCount > 0 && fGDI_GC);
		fGDI_GC->SetLineColor( inColor);

		fGDI_HasSolidStrokeColor.back() = true;
		fGDI_StrokeColor.back() = inColor;
		fGDI_HasSolidStrokeColorInherit.back() = true;

		return;
	}
	
	FlushGDI();

	fStrokeBrush = (ID2D1Brush *)NULL;
	fStrokeBrushLinearGradient = (ID2D1LinearGradientBrush *)NULL;
	fStrokeBrushRadialGradient = (ID2D1RadialGradientBrush *)NULL;

	xbox_assert(fStrokeBrushSolid != NULL);
	fStrokeBrushSolid->SetColor( D2D_COLOR( inColor));
	HRESULT hr = fStrokeBrushSolid->QueryInterface(__uuidof(ID2D1Brush), (void **)&fStrokeBrush);
	xbox_assert(SUCCEEDED(hr));

	fStrokeBrush->SetOpacity( fGlobalAlpha);

	if (ioFlags)
		ioFlags->fLineBrushChanged = 1;
}



void VWinD2DGraphicContext::_SetLinePattern(const VPattern* inPattern, VBrushFlags* ioFlags) const
{
	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	if (inPattern != NULL) 
	{
		if (inPattern->GetKind() == 'axeP')
		{
			FlushGDI();
			_CreateLinearGradientBrush( inPattern, fStrokeBrush, fStrokeBrushLinearGradient, fStrokeBrushRadialGradient);
			if (ioFlags)
				ioFlags->fLineBrushChanged = 1;
			return;
		}
		else if (inPattern->GetKind() == 'radP')
		{
			FlushGDI();
			_CreateRadialGradientBrush( inPattern, fStrokeBrush, fStrokeBrushLinearGradient, fStrokeBrushRadialGradient);
			if (ioFlags)
				ioFlags->fLineBrushChanged = 1;
			return;
		}
	}

	if (fGDI_Fast && fGDI_HDC)
	{
		xbox_assert(fLockCount > 0 && fGDI_GC);
		fGDI_GC->SetLinePattern( inPattern);

		if (inPattern == NULL || dynamic_cast<const VStandardPattern*>(inPattern) == NULL)
		{
			//reset to solid color & no dashes
			if (!fGDI_HasSolidStrokeColor.back())
			{
				fGDI_HasSolidStrokeColor.back() = true;
				//get inherited color
				fGDI_StrokeColor.back() = VCOLOR_FROM_D2D_COLOR(fStrokeBrushSolid->GetColor());
				for (int i = 0; i < fGDI_HasSolidStrokeColor.size(); i++)
				{
					if (fGDI_HasSolidStrokeColor[i])
						fGDI_StrokeColor.back() = fGDI_StrokeColor[i];
				}
			}
			fGDI_HasSolidStrokeColorInherit.back() = true;

			//ensure we reset pattern to plain when later D2D context is restored
			VPattern *pattern = VPattern::RetainStdPattern( PAT_LINE_PLAIN);
			if (testAssert(pattern))
				CopyRefCountable(&(fGDI_StrokePattern.back()), const_cast<const VPattern *>(pattern));
			ReleaseRefCountable(&pattern);
		}
		else
			CopyRefCountable(&(fGDI_StrokePattern.back()), inPattern);
		
		fGDI_HasStrokeCustomDashesInherit.back() = false;
		return;
	}
	
	FlushGDI();

	const VStandardPattern *stdp = dynamic_cast<const VStandardPattern*>(inPattern);

	D2D1_DASH_STYLE dashStyle = fStrokeStyle->GetDashStyle();
	D2D1_DASH_STYLE newDashStyle = dashStyle;
	
	if (stdp == NULL)
	{
		newDashStyle = D2D1_DASH_STYLE_SOLID;

		if (fStrokeBrush != fStrokeBrushSolid)
		{
			//reset to solid color
			fStrokeBrush = (ID2D1Brush *)NULL;
			fStrokeBrushLinearGradient = (ID2D1LinearGradientBrush *)NULL;
			fStrokeBrushRadialGradient = (ID2D1RadialGradientBrush *)NULL;

			xbox_assert(fStrokeBrushSolid != NULL);
			HRESULT hr = fStrokeBrushSolid->QueryInterface(__uuidof(ID2D1Brush), (void **)&fStrokeBrush);
			xbox_assert(SUCCEEDED(hr));

			fStrokeBrush->SetOpacity( fGlobalAlpha);

			if (ioFlags)
				ioFlags->fLineBrushChanged = 1;
		}
	}
	else
	{
		PatternStyle ps = stdp->GetPatternStyle();
		switch(ps)
		{
			case PAT_LINE_DOT_MEDIUM:
				newDashStyle = D2D1_DASH_STYLE_DASH;
				break;
			case PAT_LINE_DOT_MINI:
			case PAT_LINE_DOT_SMALL:
				newDashStyle = D2D1_DASH_STYLE_DOT;
				break;
			default:
				newDashStyle = D2D1_DASH_STYLE_SOLID;
				break;
		}
	}

	if (newDashStyle != dashStyle)
	{
		bool isDashDot = newDashStyle == D2D1_DASH_STYLE_DOT;
		if (isDashDot)
			newDashStyle = D2D1_DASH_STYLE_CUSTOM; //workaround for invisible D2D1_DASH_STYLE_DOT 
		
		fStrokeStyle = (ID2D1StrokeStyle *)NULL;
		fStrokeStyleProps.dashStyle = newDashStyle;
		if (fStrokeDashPattern)
			delete [] fStrokeDashPattern;
		fStrokeDashPattern = NULL;
		fStrokeDashCount = 0;
		fStrokeDashCountInit = 0;
		fStrokeDashPatternInit.clear();
		if (isDashDot)
		{
			fStrokeDashPatternInit.push_back( 1);
			fStrokeDashPatternInit.push_back( 1);
			fStrokeDashCountInit = fStrokeDashCount = 2;
			fStrokeDashPattern = new FLOAT[ 2];
			fStrokeDashPattern[0] = fStrokeDashPattern[1] = 1;
			fIsStrokeDotPattern = true;
		}
		else
			fIsStrokeDotPattern = false;

		fMutexFactory.Lock();

		HRESULT hr = GetFactory()->CreateStrokeStyle( 
				&fStrokeStyleProps,
				fStrokeDashCount ? fStrokeDashPattern : NULL, 
				fStrokeDashCount, 
				&fStrokeStyle);
		xbox_assert(SUCCEEDED(hr) && fStrokeStyle);
		fMutexFactory.Unlock();
		if (ioFlags)
			ioFlags->fLineBrushChanged = 1;
	}
}

/** force dash pattern unit to be equal to stroke width (on default it is equal to user unit) */
void VWinD2DGraphicContext::ShouldLineDashPatternUnitEqualToLineWidth( bool inValue)
{
	if (fIsStrokeDashPatternUnitEqualToStrokeWidth == inValue)
		return;

	FlushGDI();
	fIsStrokeDashPatternUnitEqualToStrokeWidth = inValue;
	if (fStrokeStyle->GetDashStyle() == D2D1_DASH_STYLE_CUSTOM && !fIsStrokeDotPattern)
		//for custom dash pattern, we need to rebuild dash pattern
		//because dash pattern unit is stroke width
		_SetLineDashPattern( fStrokeStyleProps.dashOffset, fStrokeDashPatternInit);
}


/** set line dash pattern
@param inDashOffset
	offset into the dash pattern to start the line (user units)
@param inDashPattern
	dash pattern (alternative paint and unpaint lengths in user units)
	for instance {3,2,4} will paint line on 3 user units, unpaint on 2 user units
						 and paint again on 4 user units and so on...
*/
void VWinD2DGraphicContext::_SetLineDashPattern(GReal inDashOffset, const VLineDashPattern& inDashPattern, VBrushFlags* ioFlags) const
{
	if (fGDI_Fast && fGDI_HDC)
	{
		xbox_assert(fLockCount > 0 && fGDI_GC);
		if (inDashPattern.size() < 2)
		{
			//clear dash pattern

			VPattern *pattern = VPattern::RetainStdPattern( PAT_LINE_PLAIN);
			if (testAssert(pattern))
				CopyRefCountable(&(fGDI_StrokePattern.back()), const_cast<const VPattern *>(pattern));
			ReleaseRefCountable(&pattern);

			fGDI_HasStrokeCustomDashesInherit.back() = false;
			return;
		}
	}

	FlushGDI();
	D2D1_DASH_STYLE dashStyle = fStrokeStyle->GetDashStyle();

	if (inDashPattern.size() < 2)
	{
		//clear dash pattern
		if (dashStyle != D2D1_DASH_STYLE_SOLID)
		{
			fStrokeStyle = (ID2D1StrokeStyle *)NULL;
			fStrokeStyleProps.dashStyle = D2D1_DASH_STYLE_SOLID;
			if (fStrokeDashPattern)
				delete [] fStrokeDashPattern;
			fStrokeDashPattern = NULL;
			fStrokeDashCount = 0;
			fStrokeDashCountInit = 0;
			fStrokeDashPatternInit.clear();
			fMutexFactory.Lock();
			HRESULT hr = GetFactory()->CreateStrokeStyle( 
					&fStrokeStyleProps,
					fStrokeDashCount ? fStrokeDashPattern : NULL, 
					fStrokeDashCount, 
					&fStrokeStyle);
			xbox_assert(SUCCEEDED(hr) && fStrokeStyle);
			fMutexFactory.Unlock();
			if (ioFlags)
				ioFlags->fLineBrushChanged = 1;
			fIsStrokeDotPattern = false;
		}
	}
	else
	{
		//build dash pattern
		if (&fStrokeDashPatternInit != &inDashPattern)
			fStrokeDashPatternInit = inDashPattern;
		else if (fIsStrokeDotPattern)
			return;

		if (inDashPattern.size() > fStrokeDashCountInit)
		{
			fStrokeDashCountInit = inDashPattern.size();
			if (fStrokeDashPattern)
				delete [] fStrokeDashPattern;
			fStrokeDashPattern = new FLOAT[ fStrokeDashCountInit];
		}
		xbox_assert(!fStrokeDashCountInit || fStrokeDashPattern);
		fStrokeDashCount = inDashPattern.size();

		VLineDashPattern::const_iterator it = inDashPattern.begin();
		FLOAT *patternValue = fStrokeDashPattern;

		if (fIsStrokeDashPatternUnitEqualToStrokeWidth)
		{
			for (;it != inDashPattern.end(); it++, patternValue++)
			{
				*patternValue = (FLOAT)(*it); //D2D pattern unit is equal to pen width 
															 
				//workaround for D2D dash pattern bug
				if (*patternValue == 0.0f)
					*patternValue = 0.0000001f;
			}
		}
		else
			for (;it != inDashPattern.end(); it++, patternValue++)
			{
				*patternValue = (FLOAT)(*it) / (fStrokeWidth == 0.0f ? 0.0001f : fStrokeWidth); //D2D pattern unit is equal to pen width 
															 //so as we use absolute user units divide first by pen width	
				//workaround for D2D dash pattern bug
				if (*patternValue == 0.0f)
					*patternValue = 0.0000001f;
			}

		//set dash pattern
		fStrokeStyle = (ID2D1StrokeStyle *)NULL;
		fStrokeStyleProps.dashStyle = D2D1_DASH_STYLE_CUSTOM;

		fStrokeStyleProps.dashOffset = inDashOffset;
		fMutexFactory.Lock();
		HRESULT hr = GetFactory()->CreateStrokeStyle( 
				&fStrokeStyleProps,
				fStrokeDashCount ? fStrokeDashPattern : NULL, 
				fStrokeDashCount, 
				&fStrokeStyle);
		fMutexFactory.Unlock();
		xbox_assert(SUCCEEDED(hr) && fStrokeStyle);
		if (ioFlags)
			ioFlags->fLineBrushChanged = 1;
		fIsStrokeDotPattern = false;
	}
}

/** set fill rule */
void VWinD2DGraphicContext::SetFillRule( FillRuleType inFillRule)
{
	FlushGDI();
	fFillRule = inFillRule;
}


GReal VWinD2DGraphicContext::GetLineWidth () const 
{
	if (fGDI_Fast && fGDI_HDC)
	{
		xbox_assert(fLockCount > 0 && fGDI_GC);
		return fGDI_GC->GetLineWidth();
	}

	return fStrokeWidth; 
}

void VWinD2DGraphicContext::_SetLineWidth(GReal inWidth, VBrushFlags* ioFlags) const
{
	if (fGDI_Fast && fGDI_HDC)
	{
		xbox_assert(fLockCount > 0 && fGDI_GC);
		fGDI_GC->SetLineWidth( inWidth);

		fGDI_StrokeWidth.back() = inWidth;
		return;
	}

	FlushGDI();
	if (fStrokeWidth != inWidth)
	{
		fStrokeWidth = inWidth;

		if (fStrokeStyle->GetDashStyle() == D2D1_DASH_STYLE_CUSTOM && !fIsStrokeDotPattern && !fIsStrokeDashPatternUnitEqualToStrokeWidth)
			//for custom dash pattern, we need to rebuild dash pattern
			//because dash pattern unit is stroke width
			_SetLineDashPattern( fStrokeStyleProps.dashOffset, fStrokeDashPatternInit);

		if (ioFlags)
			ioFlags->fLineSizeChanged = 1;
	}
}


void VWinD2DGraphicContext::SetLineCap (CapStyle inCapStyle, VBrushFlags *ioFlags)
{
	if (fGDI_Fast && fGDI_HDC)
	{
		xbox_assert(fLockCount > 0 && fGDI_GC);
		return;
	}

	FlushGDI();

	D2D1_CAP_STYLE capStyle;
	switch (inCapStyle)
	{
		case CS_BUTT:
			capStyle = D2D1_CAP_STYLE_FLAT;
			break;
			
		case CS_ROUND:
			capStyle = D2D1_CAP_STYLE_ROUND;
			break;
			
		case CS_SQUARE:
			capStyle = D2D1_CAP_STYLE_SQUARE;
			break;
		
		default:
			assert(false);
			break;
	}

	if (fStrokeStyle->GetStartCap() == capStyle)
		return;

	fStrokeStyle = (ID2D1StrokeStyle *)NULL;
	fStrokeStyleProps.startCap = capStyle;
	fStrokeStyleProps.endCap = capStyle;
	fStrokeStyleProps.dashCap = capStyle;
	fMutexFactory.Lock();
	HRESULT hr = GetFactory()->CreateStrokeStyle( 
			&fStrokeStyleProps,
			fStrokeDashCount ? fStrokeDashPattern : NULL, 
			fStrokeDashCount, 
			&fStrokeStyle);
	fMutexFactory.Unlock();
	xbox_assert(SUCCEEDED(hr) && fStrokeStyle);
	if (ioFlags)
		ioFlags->fLineBrushChanged = 1;
}

void VWinD2DGraphicContext::SetLineJoin(JoinStyle inJoinStyle, VBrushFlags* ioFlags)
{
	if (fGDI_Fast && fGDI_HDC)
	{
		xbox_assert(fLockCount > 0 && fGDI_GC);
		return;
	}

	FlushGDI();

	D2D1_LINE_JOIN lineJoin;
	switch (inJoinStyle)
	{
		case JS_MITER:
			lineJoin = D2D1_LINE_JOIN_MITER;
			break;
			
		case JS_ROUND:
			lineJoin = D2D1_LINE_JOIN_ROUND;
			break;
			
		case JS_BEVEL:
			lineJoin = D2D1_LINE_JOIN_BEVEL;
			break;
		
		default:
			assert(false);
			break;
	}
	if (fStrokeStyle->GetLineJoin() == lineJoin)
		return;

	fStrokeStyle = (ID2D1StrokeStyle *)NULL;
	fStrokeStyleProps.lineJoin = lineJoin;
	fMutexFactory.Lock();
	HRESULT hr = GetFactory()->CreateStrokeStyle( 
			&fStrokeStyleProps,
			fStrokeDashCount ? fStrokeDashPattern : NULL, 
			fStrokeDashCount, 
			&fStrokeStyle);
	fMutexFactory.Unlock();
	xbox_assert(SUCCEEDED(hr) && fStrokeStyle);
	if (ioFlags)
		ioFlags->fLineBrushChanged = 1;
}

void VWinD2DGraphicContext::SetLineMiterLimit( GReal inMiterLimit)
{
	if (fGDI_Fast && fGDI_HDC)
	{
		xbox_assert(fLockCount > 0 && fGDI_GC);
		return;
	}

	FlushGDI();

	if (fStrokeStyle->GetMiterLimit() == inMiterLimit)
		return;

	fStrokeStyle = (ID2D1StrokeStyle *)NULL;
	fStrokeStyleProps.miterLimit = inMiterLimit;
	fMutexFactory.Lock();
	HRESULT hr = GetFactory()->CreateStrokeStyle( 
			&fStrokeStyleProps,
			fStrokeDashCount ? fStrokeDashPattern : NULL, 
			fStrokeDashCount, 
			&fStrokeStyle);
	fMutexFactory.Unlock();
	xbox_assert(SUCCEEDED(hr) && fStrokeStyle);
}


void VWinD2DGraphicContext::SetLineStyle(CapStyle inCapStyle, JoinStyle inJoinStyle, VBrushFlags* ioFlags)
{
	SetLineCap( inCapStyle);
	SetLineJoin( inJoinStyle);
}



void VWinD2DGraphicContext::_SetTextColor(const VColor& inColor, VBrushFlags* ioFlags) const
{
	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	if (fUseShapeBrushForText)
	{
		_SetFillColor( inColor, ioFlags);
		return;
	}

	if (fGDI_HDC)
	{
		xbox_assert(fLockCount > 0 && fGDI_GC);
		fGDI_GC->SetTextColor( inColor);

		//we must update font & text color to restore later on FlushGDI
		//otherwise the FlushGDI context restore will restore font & text from before first BeginUsingParentContext
		//(because text color & font update do not trigger a FlushGDI in order to keep alive GDI context as long as possible for texts)
		fGDI_TextColor.back() = inColor;
		return;
	}

	fTextBrush = (ID2D1Brush *)NULL;

	if (fTextBrushSolid != NULL)
		fTextBrushSolid->SetColor( D2D_COLOR( inColor));
	else
	{
		if (FAILED(fRenderTarget->CreateSolidColorBrush( D2D_COLOR( inColor), &fTextBrushSolid)))
			return;
	}
	HRESULT hr = fTextBrushSolid->QueryInterface(__uuidof(ID2D1Brush), (void **)&fTextBrush);
	xbox_assert(SUCCEEDED(hr));
	fTextBrush->SetOpacity( fGlobalAlpha);

	if (ioFlags)
		ioFlags->fTextBrushChanged = 1;
}

void VWinD2DGraphicContext::GetTextColor (VColor& outColor) const
{
	if (fGDI_HDC)
	{
		xbox_assert(fLockCount > 0 && fGDI_GC);
		fGDI_GC->GetTextColor( outColor);
		return;
	}

	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	if (fUseShapeBrushForText)
	{
		if (fFillBrushSolid != NULL)
		{
			D2D1_COLOR_F colorNative = fFillBrushSolid->GetColor();
			outColor.SetColor( (uBYTE)(colorNative.r*255.0f), (uBYTE)(colorNative.g*255.0f), (uBYTE)(colorNative.b*255.0f), (uBYTE)(colorNative.a*255.0f));
		}
		return;
	}

	if (fTextBrushSolid != NULL)
	{
		D2D1_COLOR_F colorNative = fTextBrushSolid->GetColor();
		outColor.SetColor( (uBYTE)(colorNative.r*255.0f), (uBYTE)(colorNative.g*255.0f), (uBYTE)(colorNative.b*255.0f), (uBYTE)(colorNative.a*255.0f));
	}
}


void VWinD2DGraphicContext::SetBackColor(const VColor& inColor, VBrushFlags* ioFlags)
{
}


DrawingMode VWinD2DGraphicContext::SetDrawingMode(DrawingMode inMode, VBrushFlags* ioFlags)
{
	if (fGDI_Fast && fGDI_HDC)
	{
		xbox_assert(fLockCount > 0 && fGDI_GC);
		return fGDI_GC->SetDrawingMode( inMode);
	}
	DrawingMode previousMode = fDrawingMode;
	if (GetMaxPerfFlag())
		fDrawingMode = DM_NORMAL;
	else
		fDrawingMode = inMode;
	return previousMode;
}

void VWinD2DGraphicContext::GetBrushPos (VPoint& outHwndPos) const
{
}

GReal VWinD2DGraphicContext::SetTransparency(GReal inAlpha)
{
	if (fGlobalAlpha == inAlpha)
		return fGlobalAlpha;

	if (fGDI_Fast && fGDI_HDC)
	{
		xbox_assert(fLockCount > 0 && fGDI_GC);
		if (inAlpha == 1.0f)
			return fGlobalAlpha;
	}

	FlushGDI();
	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	GReal result = fGlobalAlpha;
	fGlobalAlpha = inAlpha;
	fFillBrush->SetOpacity( inAlpha);
	fStrokeBrush->SetOpacity( inAlpha);
	fTextBrush->SetOpacity( inAlpha);
	return result;
}


void VWinD2DGraphicContext::SetShadowValue(GReal inHOffset, GReal inVOffset, GReal inBlurness)
{
	if (fGDI_Fast && fGDI_HDC)
	{
		xbox_assert(fLockCount > 0 && fGDI_GC);
		fGDI_GC->SetShadowValue( inHOffset, inVOffset, inBlurness);
		return;
	}
	FlushGDI();
	fShadowHOffset = inHOffset;
	fShadowVOffset = inVOffset;
	fShadowBlurness = inBlurness;
}


void VWinD2DGraphicContext::EnableAntiAliasing()
{
	//StUseContext_NoRetain	context(this);
	//FlushGDI();

	fIsHighQualityAntialiased = true;
	if (fUseCount > 0)
	{
		if (GetMaxPerfFlag())
			fRenderTarget->SetAntialiasMode( D2D1_ANTIALIAS_MODE_ALIASED);
		else
			fRenderTarget->SetAntialiasMode( D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	}
}


void VWinD2DGraphicContext::DisableAntiAliasing()
{
	//StUseContext_NoRetain	context(this);
	//FlushGDI();

	fIsHighQualityAntialiased = false;
	if (fUseCount > 0)
		fRenderTarget->SetAntialiasMode( D2D1_ANTIALIAS_MODE_ALIASED);
}


bool VWinD2DGraphicContext::IsAntiAliasingAvailable()
{
	return true;
}
TextMeasuringMode VWinD2DGraphicContext::SetTextMeasuringMode(TextMeasuringMode inMode)
{
	TextMeasuringMode previous = fTextMeasuringMode;
	fTextMeasuringMode = inMode;
	return previous;
}

TextRenderingMode VWinD2DGraphicContext::SetTextRenderingMode( TextRenderingMode inMode)
{
	//caution: please DO NOT add tUseContext_NoRetain	context(this) here (!)
	//StUseContext_NoRetain	context(this);

	//if (fRenderTarget == NULL)
	//	_InitDeviceDependentObjects();
	//FlushGDI();

	TextRenderingMode previousMode = fTextRenderingMode;

	if (inMode & TRM_LEGACY_ON)
		inMode &= ~TRM_LEGACY_OFF; //TRM_LEGACY_ON & TRM_LEGACY_OFF are mutually exclusive 

	fHighQualityTextRenderingMode = inMode;
	if (fMaxPerfFlag)
		inMode = (inMode & (TRM_CONSTANT_SPACING|TRM_LEGACY_ON|TRM_LEGACY_OFF)) | TRM_WITHOUT_ANTIALIASING;

	if (fTextRenderingMode == inMode)
		return previousMode;

	if ((previousMode & (TRM_CONSTANT_SPACING|TRM_LEGACY_ON|TRM_LEGACY_OFF)) != (inMode & (TRM_CONSTANT_SPACING|TRM_LEGACY_ON|TRM_LEGACY_OFF)))
		//if legacy mode has changed, we need to flush current GDI context if any
		FlushGDI();

	fTextRenderingMode = inMode;
	if (fRenderTarget && fUseCount > 0)
	{
		D2D1_TEXT_ANTIALIAS_MODE textAntialias;

		if (inMode & TRM_WITH_ANTIALIASING_CLEARTYPE)
			textAntialias = IsTransparent() || fWICBitmap || fLayerOffScreenCount ? D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE : D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE;
		else if (inMode & TRM_WITH_ANTIALIASING_NORMAL)
			textAntialias = D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE;
		else if (inMode & TRM_WITHOUT_ANTIALIASING)
			textAntialias = D2D1_TEXT_ANTIALIAS_MODE_ALIASED;
		else
			textAntialias = IsTransparent() || fWICBitmap || fLayerOffScreenCount ? D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE : D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE;

		fRenderTarget->SetTextAntialiasMode( textAntialias);
	}

	StParentContextNoDraw hdcNoDraw(this);
	if (fTextFontMetrics)
		//ensure we update font metrics if legacy mode has changed 
		fTextFontMetrics->UseLegacyMetrics(fTextFontMetrics->GetDesiredUseLegacyMetrics());

	return previousMode;
}

TextRenderingMode  VWinD2DGraphicContext::GetTextRenderingMode() const
{
	//caution: please DO NOT add tUseContext_NoRetain	context(this) here (!)
	return fTextRenderingMode;
}


void VWinD2DGraphicContext::_SetFont( VFont *inFont, GReal inScale) const
{
	if (inFont == fTextFont && inScale == 1.0f)
	{
		if (fGDI_HDC)
		{
			xbox_assert(fUseCount > 0 && fLockCount > 0 && fGDI_GC);
			VFont *font = fGDI_GC->RetainFont();
			if (font != inFont)
				fGDI_GC->SetFont( inFont, 1.0f);
			ReleaseRefCountable(&font);

			//we must update font & text color to restore later on FlushGDI
			//otherwise the FlushGDI will restore font & text saved before first BeginUsingParentContext
			//(because text color & font update do not trigger a FlushGDI in order to keep alive GDI context as long as possible for texts)
			if (fGDI_TextFont.back() != inFont)
			{
				VFont *font = fGDI_TextFont.back();
				ReleaseRefCountable(&font);
				fGDI_TextFont.back() = RetainRefCountable(inFont);
			}
		}
		return;
	}
	StParentContextNoDraw hdcNoDraw(this);

	if (inScale != 1.0f)
	{
		if (fTextFont)
			fTextFont->Release();
		fTextFont = inFont->DeriveFontSize( inFont->GetSize() * inScale);
	}
	else
		CopyRefCountable( &fTextFont, inFont);

	if (fGDI_HDC)
	{
		xbox_assert(fUseCount > 0 && fLockCount > 0 && fGDI_GC);
		fGDI_GC->SetFont( fTextFont, 1.0f);

		//we must update font & text color to restore later on FlushGDI
		//otherwise the FlushGDI will restore font & text saved before first BeginUsingParentContext
		//(because text color & font update do not trigger a FlushGDI in order to keep alive GDI context as long as possible for texts)
		if (fGDI_TextFont.back() != fTextFont)
		{
			VFont *font = fGDI_TextFont.back();
			ReleaseRefCountable(&font);
			fGDI_TextFont.back() = RetainRefCountable(fTextFont);
		}
	}

	fWhiteSpaceWidth = 0.0f;

	if (fTextFontMetrics)
		delete fTextFontMetrics;
	fTextFontMetrics = new VFontMetrics( fTextFont, static_cast<const VGraphicContext *>(this));
}


/** determine current white space width 
@remarks
	used for custom kerning
*/
void	VWinD2DGraphicContext::_ComputeWhiteSpaceWidth() const
{
	if (fWhiteSpaceWidth != 0.0f)
		return;
	GReal kerning = fCharKerning + fCharSpacing;
	if (kerning == 0.0f)
		return;

	//precompute white space width 
	VRect bounds;
	GetTextBoundsTypographic(" ", bounds);
	fWhiteSpaceWidth = bounds.GetWidth();
}


VFont*	VWinD2DGraphicContext::RetainFont()
{
	if (fTextFont == NULL)
		_InitDefaultFont();

	if (fTextFont)
		fTextFont->Retain();
	return fTextFont;
}


void VWinD2DGraphicContext::SetTextPosTo(const VPoint& inHwndPos)
{
	fTextRect.left = inHwndPos.GetX();
	fTextRect.top = inHwndPos.GetY();
}


void VWinD2DGraphicContext::SetTextPosBy(GReal inHoriz, GReal inVert)
{
	fTextRect.left += inHoriz;
	fTextRect.top += inVert;
}


void VWinD2DGraphicContext::SetTextAngle(GReal inAngle)
{
}

void VWinD2DGraphicContext::GetTextPos (VPoint& outHwndPos) const
{
	outHwndPos.SetX(fTextRect.left);
	outHwndPos.SetY(fTextRect.top);
}


DrawingMode VWinD2DGraphicContext::SetTextDrawingMode(DrawingMode inMode, VBrushFlags* /*ioFlags*/)
{
	return DM_PLAIN;
}


void VWinD2DGraphicContext::SetSpaceKerning(GReal inSpaceExtra)
{
	if (fCharSpacing == inSpaceExtra)
		return;
	fCharSpacing = inSpaceExtra;
	fWhiteSpaceWidth = 0.0f;
}


void VWinD2DGraphicContext::SetCharKerning(GReal inSpaceExtra)
{
	if (fCharKerning == inSpaceExtra)
		return;
	fCharKerning = inSpaceExtra;
	fWhiteSpaceWidth = 0.0f;
}


uBYTE VWinD2DGraphicContext::SetAlphaBlend(uBYTE inAlphaBlend)
{
	return 0;
}


void VWinD2DGraphicContext::SetPixelForeColor(const VColor& inColor)
{
}


void VWinD2DGraphicContext::SetPixelBackColor(const VColor& inColor)
{
}



#pragma mark-

GReal VWinD2DGraphicContext::GetTextWidth(const VString& inString, bool inRTL) const
{
	if (inString.IsEmpty())
		return 0.0f;
	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	return fTextFontMetrics->GetTextWidth( inString, inRTL);
}

void VWinD2DGraphicContext::GetTextBounds( const VString& inString, VRect& oRect) const
{
	GetTextBoundsTypographic( inString, oRect, TLM_NORMAL);
}



//get text bounds 
//
//@param inString
//	text string
//@param oRect
//	text bounds (out param)
//@param inLayoutMode
//  layout mode
//
//@remark
//	this method return the typographic text line bounds 
//@note
//	this method is used by SVG component
void VWinD2DGraphicContext::GetTextBoundsTypographic( const VString& inString, VRect& oRect, TextLayoutMode inLayoutMode) const
{
	if (inString.IsEmpty())
	{
		oRect.SetEmpty();
		return;
	}

	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!fTextFont->IsTrueTypeFont())))
	{
		StParentContextNoDraw hdcNoDraw(this);
		oRect.SetEmpty();
		_GetLegacyTextBoxSize(inString, oRect, inLayoutMode);
		return;
	}

	if (inLayoutMode & TLM_VERTICAL)
	{
		VGraphicContext	*gc = BeginUsingGdiplus();
		if (gc)
		{
			gc->GetTextBoundsTypographic( inString, oRect, inLayoutMode);
			EndUsingGdiplus();
			return;
		}
	}

	IDWriteTextLayout *textLayout = _GetTextBoundsTypographic( inString, oRect, inLayoutMode);
	if (textLayout)
		textLayout->Release();
}


IDWriteTextLayout *VWinD2DGraphicContext::_GetTextBoundsTypographic( const VString& inString, VRect& oRect, TextLayoutMode inLayoutMode) const
{
	if (inString.IsEmpty())
	{
		oRect.SetEmpty();
		return NULL;
	}
	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	DWRITE_TEXT_METRICS textMetrics;
	IDWriteTextLayout *textLayout = NULL;
	{
		VTaskLock lock(&fMutexDWriteFactory);
		textLayout = fTextFont->GetImpl().CreateTextLayout( fRenderTarget, inString, (!(fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)), DWRITE_TEXT_LAYOUT_MAX_WIDTH, DWRITE_TEXT_LAYOUT_MAX_HEIGHT, AL_LEFT, AL_TOP, inLayoutMode);
		if (!textLayout)
		{
			oRect.SetEmpty();
			return NULL;
		}
		textLayout->GetMetrics( &textMetrics);
	}

	GReal kerning = fCharKerning+fCharSpacing;
	GReal width = 0.0f;
	GReal height = textMetrics.height;
	if (kerning 
		&&
		inString.GetLength() > 1
		&&
		(!(inLayoutMode & TLM_RIGHT_TO_LEFT))
		)
	{
		//measure string width taking account custom kerning
		//(for now work only with left to right or top to bottom text)
		
		_ComputeWhiteSpaceWidth();
		const UniChar *c = inString.GetCPointer();
		while (*c)
		{
			width  += ((*c) == 0x20 ? fWhiteSpaceWidth : GetCharWidth( *c));
			c++;
		}
		width += (inString.GetLength()-1)*kerning;
	}
	else
		width = textMetrics.widthIncludingTrailingWhitespace;

	oRect.SetCoords( 0.0f, 0.0f, width, height);

	//inflate with overhang metrics
	if (!(inLayoutMode & TLM_D2D_IGNORE_OVERHANGS))
	{
		DWRITE_OVERHANG_METRICS textOverhangMetrics;
		textLayout->GetOverhangMetrics( &textOverhangMetrics);

		if (textOverhangMetrics.left > 0.0f)
			oRect.SetSizeBy( textOverhangMetrics.left, 0.0f);
		if (textOverhangMetrics.right > 0.0f)
			oRect.SetSizeBy( textOverhangMetrics.right, 0.0f);
		if (textOverhangMetrics.top > 0.0f)
			oRect.SetSizeBy( 0.0f, textOverhangMetrics.top);
		if (textOverhangMetrics.bottom > 0.0f)
			oRect.SetSizeBy( 0.0f, textOverhangMetrics.bottom);
	}

	//deal with math discrepancies: ensure last line of text is not clipped (so if height is 40.99 we convert it to 50)
	height = oRect.GetHeight();
	if ((ceil(height)-height) <= 0.01)
		oRect.SetHeight(ceil(height));
	return textLayout;
}


GReal VWinD2DGraphicContext::GetCharWidth(UniChar inChar) const
{
	VStr4	charString(inChar);
	return GetTextWidth(charString);
}


GReal VWinD2DGraphicContext::GetTextHeight(bool inIncludeExternalLeading) const
{
	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	return inIncludeExternalLeading ? fTextFontMetrics->GetLineHeight() : fTextFontMetrics->GetTextHeight();
}
sLONG VWinD2DGraphicContext::GetNormalizedTextHeight(bool inIncludeExternalLeading) const
{
	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	return inIncludeExternalLeading ? fTextFontMetrics->GetNormalizedLineHeight() : fTextFontMetrics->GetNormalizedTextHeight();
}



#pragma mark-

void VWinD2DGraphicContext::NormalizeContext()
{
}


void VWinD2DGraphicContext::_SaveContext() const
{
	if (fGDI_HDC)
	{
		fGDI_GC->SaveContext();
		fGDI_SaveContextCounter++;
		fGDI_Restore.push_back(eSave_Context);
		//we push current font & text color to restore later in D2D parent context because SetTextColor & SetFont do not trigger a FlushGDI to keep alive GDI gc as long as possible
		fGDI_TextFont.push_back( RetainRefCountable(fGDI_TextFont.back()));
		fGDI_TextColor.push_back( fGDI_TextColor.back());
		if (fGDI_Fast)
		{
			fGDI_HasSolidStrokeColor.push_back(false);
			fGDI_HasSolidStrokeColorInherit.push_back( fGDI_HasSolidStrokeColorInherit.back());
			fGDI_StrokeColor.push_back( fGDI_StrokeColor.back());
			fGDI_StrokePattern.push_back(NULL);
			fGDI_StrokeWidth.push_back( fGDI_StrokeWidth.back());
			fGDI_HasStrokeCustomDashesInherit.push_back( fGDI_HasStrokeCustomDashesInherit.back());

			fGDI_HasSolidFillColor.push_back(false);
			fGDI_HasSolidFillColorInherit.push_back( fGDI_HasSolidFillColorInherit.back());
			fGDI_FillColor.push_back( fGDI_FillColor.back());
			fGDI_FillPattern.push_back(NULL);
		}
		return;
	}

	//StUseContext_NoRetain	context(this);
	if (fUseCount <= 0)
		return;

	fDrawingContext.Save( this);
}


void VWinD2DGraphicContext::_RestoreContext() const
{
	if (fGDI_HDC)
	{
		if (fGDI_SaveContextCounter > 0)
		{
			xbox_assert(fGDI_Restore.back() == eSave_Context);
			fGDI_Restore.pop_back();
			
			fGDI_TextFont.pop_back();
			fGDI_TextColor.pop_back();
			if (fGDI_Fast)
			{
				fGDI_HasSolidStrokeColor.pop_back();
				fGDI_HasSolidStrokeColorInherit.pop_back();
				fGDI_StrokeColor.pop_back();
				fGDI_StrokeWidth.pop_back();
				fGDI_HasStrokeCustomDashesInherit.pop_back();

				fGDI_HasSolidFillColor.pop_back();
				fGDI_HasSolidFillColorInherit.pop_back();
				fGDI_FillColor.pop_back();
				if (fGDI_StrokePattern.back())
					fGDI_StrokePattern.back()->Release();
				fGDI_StrokePattern.pop_back();
				if (fGDI_FillPattern.back())
					fGDI_FillPattern.back()->Release();
				fGDI_FillPattern.pop_back();
			}

			fGDI_SaveContextCounter--;
			fGDI_GC->RestoreContext();
			return;
		}
		else
			FlushGDI();
	}

	if (fUseCount <= 0)
		return;
	//StUseContext_NoRetain	context(this);

	fDrawingContext.Restore( this);
}


#pragma mark-

void VWinD2DGraphicContext::DrawTextBox(const VString& inText, AlignStyle inHAlign, AlignStyle inVAlign, const VRect& inHwndBounds, TextLayoutMode inLayoutMode)
{
	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();
	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!fTextFont->IsTrueTypeFont())))
	{
		_DrawLegacyTextBox(inText, inHAlign, inVAlign, inHwndBounds, inLayoutMode);
		return;
	}

	if (inLayoutMode & TLM_VERTICAL)
	{
		VGraphicContext	*gc = BeginUsingGdiplus();
		if (gc)
		{
			gc->DrawTextBox( inText, inHAlign, inVAlign, inHwndBounds, inLayoutMode);
			EndUsingGdiplus();
			return;
		}
	}

	DrawStyledText( inText, NULL, inHAlign, inVAlign, inHwndBounds, inLayoutMode);
}



//return text box bounds
//@remark:
// if input bounds width is not null, use it as max line wrapping width (or max text height in vertical layout mode)
// if input bounds height is not null, use it as max text height (or max line wrapping width in vertical layout mode)
void VWinD2DGraphicContext::GetTextBoxBounds( const VString& inText, VRect& ioHwndBounds, TextLayoutMode inLayoutMode)
{
	if (inText.IsEmpty())
	{
		ioHwndBounds.SetWidth( 0.0f);
		ioHwndBounds.SetHeight( 0.0f);
		return;
	}

	//for metrics, we need only RT to be builded: we do not need be between BeginUsingContext/EndUsingContext
	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!fTextFont->IsTrueTypeFont())))
	{
		StParentContextNoDraw hdcNoDraw(this);
		_GetLegacyTextBoxSize(inText, ioHwndBounds, inLayoutMode);
		return;
	}

	if (inLayoutMode & TLM_VERTICAL)
	{
		VGraphicContext	*gc = BeginUsingGdiplus();
		if (gc)
		{
			gc->GetTextBoxBounds( inText, ioHwndBounds, inLayoutMode);
			EndUsingGdiplus();
			return;
		}
	}

	GReal width, height;
	width = ioHwndBounds.GetWidth();
	height = ioHwndBounds.GetHeight();

	VTaskLock lock(&fMutexDWriteFactory);
		
	IDWriteTextLayout *textLayout = fTextFont->GetImpl().CreateTextLayout( 
					fRenderTarget,
					inText, 
					(!(fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)),
					width != 0.0f ? width : DWRITE_TEXT_LAYOUT_MAX_WIDTH, 
					height != 0.0f ? height : DWRITE_TEXT_LAYOUT_MAX_HEIGHT, 
					AL_LEFT, AL_TOP, inLayoutMode);
	if (!textLayout)
	{
		ioHwndBounds.SetWidth( 0.0f);
		ioHwndBounds.SetHeight( 0.0f);
		return;
	}

	DWRITE_TEXT_METRICS textMetrics;
	textLayout->GetMetrics( &textMetrics);
	width = textMetrics.widthIncludingTrailingWhitespace;
	height = textMetrics.height;

	ioHwndBounds.SetWidth( width);
	ioHwndBounds.SetHeight( height);

	//inflate with overhang metrics
	if (!(inLayoutMode & TLM_D2D_IGNORE_OVERHANGS))
	{
		DWRITE_OVERHANG_METRICS textOverhangMetrics;
		textLayout->GetOverhangMetrics( &textOverhangMetrics);

		if (textOverhangMetrics.left > 0.0f)
			ioHwndBounds.SetSizeBy( textOverhangMetrics.left, 0.0f);
		if (textOverhangMetrics.right > 0.0f)
			ioHwndBounds.SetSizeBy( textOverhangMetrics.right, 0.0f);
		if (textOverhangMetrics.top > 0.0f)
			ioHwndBounds.SetSizeBy( 0.0f, textOverhangMetrics.top);
		if (textOverhangMetrics.bottom > 0.0f)
			ioHwndBounds.SetSizeBy( 0.0f, textOverhangMetrics.bottom);
	}

	//deal with math discrepancies: ensure last line of text is not clipped (so if height is 40.99 we convert it to 50)
	height = ioHwndBounds.GetHeight();
	if ((ceil(height)-height) <= 0.01)
		ioHwndBounds.SetHeight(ceil(height));

	textLayout->Release();
}

/** draw single line text using custom kerning */
void VWinD2DGraphicContext::_DrawTextWithCustomKerning(const VString& inString, const VPoint& inPos, GReal inOffsetBaseline)
{
	GReal kerning = fCharKerning+fCharSpacing;

	std::vector<GReal> vPos;
	fTextFontMetrics->MeasureText( inString, vPos);

	_ComputeWhiteSpaceWidth();
	VStr4 sChar(" ");
	const UniChar *c = inString.GetCPointer();
	VPoint pos;
	sLONG index = 0;
	GReal kerningTotal = 0.0f;
	std::vector<GReal>::const_iterator itPos = vPos.begin();
	pos.SetX( inPos.GetX());
	pos.SetY( inPos.GetY()-inOffsetBaseline);
	while (*c)
	{
		if (*c != 0x20)
		{
			sChar.SetUniChar(1, *c);
			fRenderTarget->DrawText(
				sChar.GetCPointer(), 1, fTextFont->GetImpl().GetDWriteTextFormat(), 
				D2D1::RectF( pos.GetX(), pos.GetY(), pos.GetX()+DWRITE_TEXT_LAYOUT_MAX_WIDTH, pos.GetY()+DWRITE_TEXT_LAYOUT_MAX_HEIGHT),
				fTextBrush,
				D2D1_DRAW_TEXT_OPTIONS_NONE
				);
		}
		
		c++;
		index++;
		if (*c)
		{
			kerningTotal += kerning;
			pos.SetX( inPos.GetX()+(*itPos)+kerningTotal);		
		}
		itPos++;
	}
	return;
}


void VWinD2DGraphicContext::DrawText(const VString& inString, TextLayoutMode inLayoutMode, bool inRTLAnchorRight)
{
	if (inString.IsEmpty())
		return;

	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!fTextFont->IsTrueTypeFont())))
	{
		//legacy GDI drawing (D2D cannot draw TrueType fonts)
		DrawText( inString, VPoint(fTextRect.left, fTextRect.top), inLayoutMode, inRTLAnchorRight);
		return;
	}
		
	DrawText( inString, VPoint(fTextRect.left, fTextRect.top), inLayoutMode, inRTLAnchorRight);
	if (inLayoutMode & TLM_TEXT_POS_UPDATE_NOT_NEEDED)
		return;
	VRect bounds;
	GetTextBoundsTypographic( inString, bounds, inLayoutMode);
	if (inRTLAnchorRight && (inLayoutMode & TLM_RIGHT_TO_LEFT))
		fTextRect.left -= bounds.GetWidth();
	else
		fTextRect.left += bounds.GetWidth();
}


//draw text at the specified position
//
//@param inString
//	text string
//@param inLayoutMode
//  layout mode (here only TLM_VERTICAL and TLM_RIGHT_TO_LEFT are used)
//
//@remark: this method does not make any layout formatting 
//		   text is drawed on a single line from the specified starting coordinate
//		   (which is relative to the first glyph horizontal or vertical baseline)
//		   useful when you want to position exactly a text at a specified coordinate
//		   without taking account extra spacing due to layout formatting (which is implementation variable)
//@note: this method is used by SVG component
void VWinD2DGraphicContext::DrawText(const VString& inString, const VPoint& inPos, TextLayoutMode inLayoutMode, bool inRTLAnchorRight)
{
	if (inString.IsEmpty())
		return;

	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!fTextFont->IsTrueTypeFont())))
	{
		//legacy GDI drawing (D2D cannot draw TrueType fonts)

		bool oldLockParentContext = fLockParentContext;
		fLockParentContext = false; 
		HDC hdc = BeginUsingParentContext();
		if (hdc)
		{
			if (fGDI_GC)
			{
				fGDI_GC->SetTextPosTo( inPos);
				fGDI_GC->DrawText( inString, inLayoutMode, inRTLAnchorRight);
			}
			else
			{
				VWinGDIGraphicContext *gcGDI = new VWinGDIGraphicContext( hdc);
				gcGDI->SetTextPosTo( inPos);
				gcGDI->DrawText( inString, inLayoutMode, inRTLAnchorRight);
				gcGDI->Release();
			}
		}
		EndUsingParentContext(hdc);
		fLockParentContext = oldLockParentContext;
		return;
	}

	StUseContext_NoRetain	context(this);

	if (inLayoutMode & TLM_VERTICAL)
	{
		VGraphicContext	*gc = BeginUsingGdiplus();
		if (gc)
		{
			gc->DrawText( inString, inPos, inLayoutMode, inRTLAnchorRight);
			EndUsingGdiplus();
			return;
		}
	}

	FlushGDI();

	VTaskLock lock(&fMutexDWriteFactory);

	bool isAnchorRight = inRTLAnchorRight && (inLayoutMode & TLM_RIGHT_TO_LEFT) != 0;

	IDWriteTextLayout *textLayout = NULL;
	textLayout = fTextFont->GetImpl().CreateTextLayout( fRenderTarget, inString, (!(fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)), DWRITE_TEXT_LAYOUT_MAX_WIDTH, DWRITE_TEXT_LAYOUT_MAX_HEIGHT, AL_LEFT, AL_TOP, inLayoutMode);
	if (!textLayout)
		return;

	GReal offsetX = 0.0f;
	if (isAnchorRight)
	{
		DWRITE_TEXT_METRICS textMetrics;
		textLayout->GetMetrics(&textMetrics);
		offsetX -= textMetrics.width; //as RTL text is aligned to the left (AL_LEFT), 
									  //we subtract without trailing spaces as trailing spaces are beyond the text box left edge 
									  //(trailing spaces are visually on left for RTL text)
									  //and start pos x is snapped to the text box left edge
	}

	//determine y offset to baseline (optimization: as here text is singleline & monostyle only, baseline offset = external leading+ascent)
	//xbox_assert(fTextFontMetrics->GetLeading() == 0); //should always be 0 for Direct2D
	GReal baseline = fTextFontMetrics->GetAscent()+fTextFontMetrics->GetLeading();
	UINT32 lineCount = 0;
	DWRITE_LINE_METRICS lineMetrics[8];
	HRESULT hr = textLayout->GetLineMetrics( &(lineMetrics[0]), 8, &lineCount);  
	if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
	{
		DWRITE_LINE_METRICS *pLineMetrics = new DWRITE_LINE_METRICS [lineCount];
		if (FAILED(textLayout->GetLineMetrics( pLineMetrics, lineCount, &lineCount)))
		{
			delete [] pLineMetrics;
			textLayout->Release();
			return;
		}
		baseline = pLineMetrics->baseline;
		delete [] pLineMetrics;
	}
	else if (FAILED(hr))
	{
		textLayout->Release();
		return;
	}
	else
		baseline = lineMetrics[0].baseline;

	//adjust with overhang metrics
	VPoint pos( inPos);

	if (!(inLayoutMode & TLM_D2D_IGNORE_OVERHANGS))
	{
		DWRITE_OVERHANG_METRICS textOverhangMetrics;
		textLayout->GetOverhangMetrics( &textOverhangMetrics);

		pos.SetPosBy( textOverhangMetrics.left > 0 ? textOverhangMetrics.left : 0.0f, textOverhangMetrics.top > 0 ? textOverhangMetrics.top : 0.0f);
	}

	//eventually apply custom kerning
	sLONG length = inString.GetLength();
	GReal kerning = fCharKerning+fCharSpacing;
	if (kerning 
		&&
		(!(inLayoutMode & TLM_RIGHT_TO_LEFT))
		&& 
		length > 1)
	{
		//draw string with custom char kerning
		//(for now work only with left to right or top to bottom text)
		textLayout->Release();
		_DrawTextWithCustomKerning( inString, pos, baseline);
		return;
	}
	
	//draw full text
	fRenderTarget->DrawTextLayout(	D2D1::Point2F( pos.GetX()+offsetX, pos.GetY()-baseline),
									textLayout,
									fTextBrush,
									D2D1_DRAW_TEXT_OPTIONS_NONE);

	textLayout->Release();
}



/** draw text layout 
@remarks
	text layout origin is set to inTopLeft
*/
void VWinD2DGraphicContext::_DrawTextLayout( VTextLayout *inTextLayout, const VPoint& inTopLeft)
{
	if (inTextLayout->fTextLength == 0 && inTextLayout->fPlaceHolder.IsEmpty())
		return;

	inTextLayout->BeginUsingContext(this);
	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!inTextLayout->fUseFontTrueTypeOnly)))
	{
		//fallback to legacy text rendering
		VRect bounds;
		inTextLayout->GetLayoutBounds( NULL, bounds, inTopLeft);
		
		VGraphicContext::_DrawTextLayout( inTextLayout, inTopLeft);

		inTextLayout->EndUsingContext();
		return;
	}
	if (!testAssert(inTextLayout->fLayoutIsValid))
	{
		inTextLayout->EndUsingContext();
		return;
	}
	//flush derived GDI context if any
	FlushGDI();

	xbox_assert(inTextLayout->fLayoutD2D);
	if (!inTextLayout->_BeginDrawLayer( inTopLeft))
	{
		//text has been refreshed from layer: we do not need to redraw text content
		inTextLayout->_EndDrawLayer();
		inTextLayout->EndUsingContext();
		return;
	}

	//redraw layer text content or draw without layer

	GReal inflateX = 0.0f;
	if (fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)
	{
		//D2D1_DRAW_TEXT_OPTIONS_CLIP seems to be quite aggressive if antialiasing is disabled (sometimes text leftmost or rightmost pixel is cropped...)
		//as a workaround we need to slightly increase clipping horizontal bounds (1px) in device space (not in user space because we must take account scaling from user to device space (!))
		VAffineTransform ctm;
		GetTransform( ctm);
		if (ctm.GetScaleX())
			inflateX = 1.0f/ctm.GetScaleX();
	}

	if (inTextLayout->fCurOverhang.left > 0.0f || 
		inTextLayout->fCurOverhang.right > 0.0f || 
		inTextLayout->fCurOverhang.top > 0.0f || 
		inTextLayout->fCurOverhang.bottom > 0.0f || 
		inflateX != 0.0f)
	{
		//glyphs extend outside the layout box so clip including glyph extend
		VPoint pos(inTopLeft);
		pos.SetPosBy(inTextLayout->fMargin.left+inTextLayout->fCurOverhang.left, inTextLayout->fMargin.top+inTextLayout->fCurOverhang.top);
	
		VRect boundsOverhang(	inTopLeft.GetX()+inTextLayout->fMargin.left-inflateX, 
								inTopLeft.GetY()+inTextLayout->fMargin.top, 
								inTextLayout->_GetLayoutWidthMinusMargin()+inflateX*2, 
								inTextLayout->_GetLayoutHeightMinusMargin());
		SaveClip();
		ClipRect(boundsOverhang);
		fRenderTarget->DrawTextLayout( D2D_POINT(pos),
									   inTextLayout->fLayoutD2D,
									   fTextBrush,
									   D2D1_DRAW_TEXT_OPTIONS_NONE);
		RestoreClip();
	}
	else
	{
		VPoint pos(inTopLeft);
		pos.SetPosBy(inTextLayout->fMargin.left, inTextLayout->fMargin.top);
		fRenderTarget->DrawTextLayout( D2D_POINT(pos),
									   inTextLayout->fLayoutD2D,
									   fTextBrush,
									   D2D1_DRAW_TEXT_OPTIONS_CLIP);
	}
	inTextLayout->_EndDrawLayer();
	inTextLayout->EndUsingContext();
}
	
/** return text layout bounds 
	@remark:
		text layout origin is set to inTopLeft
		on output, outBounds contains text layout bounds including any glyph overhangs

		on input, max layout width is determined by inTextLayout->GetMaxWidth() & max layout height by inTextLayout->GetMaxHeight()
		(if max width or max height is equal to 0.0f, it is assumed to be infinite)

		if text is empty and if inReturnCharBoundsForEmptyText == true, returned bounds will be set to a single char bounds 
		(useful while editing in order to compute initial text bounds which should not be empty in order to draw caret)
*/
void VWinD2DGraphicContext::_GetTextLayoutBounds( VTextLayout *inTextLayout, VRect& outBounds, const VPoint& inTopLeft, bool inReturnCharBoundsForEmptyText)
{
	if (inTextLayout->fTextLength == 0 && !inReturnCharBoundsForEmptyText)
	{
		outBounds.SetCoordsTo( inTopLeft.GetX(), inTopLeft.GetY(), 0.0f, 0.0f);
		return;
	}

	inTextLayout->BeginUsingContext(this, true);
	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!inTextLayout->fUseFontTrueTypeOnly)))
	{
		//fallback to legacy text rendering
		StParentContextNoDraw hdcNoDraw(this);
		VGraphicContext::_GetTextLayoutBounds( inTextLayout, outBounds, inTopLeft, inReturnCharBoundsForEmptyText);
		inTextLayout->EndUsingContext();
		return;
	}
	if (!testAssert(inTextLayout->fLayoutIsValid))
	{
		inTextLayout->EndUsingContext();
		return;
	}

	outBounds.SetCoordsTo( inTopLeft.GetX(), inTopLeft.GetY(), 0.0f, 0.0f);
	if (testAssert(inTextLayout->fLayoutIsValid))
	{
		outBounds.SetWidth( inTextLayout->fCurLayoutWidth);
		outBounds.SetHeight( inTextLayout->fCurLayoutHeight);
	}
	inTextLayout->EndUsingContext();
}


/** return text layout run bounds from the specified range 
@remarks
	text layout origin is set to inTopLeft
*/
void VWinD2DGraphicContext::_GetTextLayoutRunBoundsFromRange( VTextLayout *inTextLayout, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft, sLONG inStart, sLONG inEnd)
{
	if (inTextLayout->fTextLength == 0)
	{
		outRunBounds.clear();
		return;
	}

	inTextLayout->BeginUsingContext(this, true);
	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!inTextLayout->fUseFontTrueTypeOnly)))
	{
		StParentContextNoDraw hdcNoDraw(this);
		VGraphicContext::_GetTextLayoutRunBoundsFromRange(inTextLayout, outRunBounds, inTopLeft, inStart, inEnd);
		inTextLayout->EndUsingContext();
		return;
	}

	if (!testAssert(inTextLayout->fLayoutIsValid))
	{
		inTextLayout->EndUsingContext();
		return;
	}

	xbox_assert(inTextLayout->fLayoutD2D);
	
	if (inEnd < 0)
		inEnd = inTextLayout->fTextLength;

	VPoint pos(inTopLeft);
	pos.SetPosBy(inTextLayout->fMargin.left+inTextLayout->fCurOverhang.left, inTextLayout->fMargin.top+inTextLayout->fCurOverhang.top);
	_GetRangeBounds( inTextLayout->fLayoutD2D, inStart, inEnd-inStart, pos, outRunBounds);

	inTextLayout->EndUsingContext();
}


/** return the caret position & height of text at the specified text position in the text layout
@remarks
	text layout origin is set to inTopLeft

	text position is 0-based

	caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)
	if text layout is drawed at inTopLeft

	by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
	but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
*/
void VWinD2DGraphicContext::_GetTextLayoutCaretMetricsFromCharIndex( VTextLayout *inTextLayout, const VPoint& inTopLeft, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading, const bool inCaretUseCharMetrics)
{
	inTextLayout->BeginUsingContext(this, true);
	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!inTextLayout->fUseFontTrueTypeOnly)))
	{
		StParentContextNoDraw hdcNoDraw(this);
		VGraphicContext::_GetTextLayoutCaretMetricsFromCharIndex(inTextLayout, inTopLeft, inCharIndex, outCaretPos, outTextHeight, inCaretLeading, inCaretUseCharMetrics);
		inTextLayout->EndUsingContext();
		return;
	}

	if (!testAssert(inTextLayout->fLayoutIsValid))
	{
		inTextLayout->EndUsingContext();
		outCaretPos = inTopLeft;
		outTextHeight = 0.0f;
		return;
	}

	xbox_assert(inTextLayout->fLayoutD2D);
	
	VPoint pos(inTopLeft);
	pos.SetPosBy(inTextLayout->fMargin.left+inTextLayout->fCurOverhang.left, inTextLayout->fMargin.top+inTextLayout->fCurOverhang.top);

	bool leading = inCaretLeading;
	VIndex charIndex = inCharIndex;
	if (inTextLayout->fTextLength == 0)
	{
		//for a empty text, adjust with dummy character
		charIndex = 0;
		leading = true;
	}

	FLOAT x,y;
	DWRITE_HIT_TEST_METRICS metrics;
	inTextLayout->fLayoutD2D->HitTestTextPosition(  charIndex, leading ? FALSE : TRUE, &x, &y, &metrics);

	if (inCaretUseCharMetrics)
	{
		//get metrics from character pos

		outCaretPos.SetX( x+pos.GetX());
		outCaretPos.SetY( y+pos.GetY());
		outTextHeight = metrics.height;
	}
	else
	{
		//get metrics from the line containing the character

		outCaretPos.SetX( x+pos.GetX());
		VRect lineBounds;
		_GetLineBoundsFromCharIndex(inTextLayout->fLayoutD2D, charIndex, lineBounds);
		outCaretPos.SetY( lineBounds.GetY()+pos.GetY());
		outTextHeight = lineBounds.GetHeight();
	}
	inTextLayout->EndUsingContext();
}


/** return the text position at the specified coordinates
@remarks
	text layout origin is set to inTopLeft (in gc user space)
	the hit coordinates are defined by inPos param (in gc user space)

	output text position is 0-based

	return true if input position is inside character bounds
	if method returns false, it returns the closest character index to the input position
*/
bool VWinD2DGraphicContext::_GetTextLayoutCharIndexFromPos( VTextLayout *inTextLayout, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex)
{
	if (inTextLayout->fTextLength == 0)
	{
		outCharIndex = 0;
		return false;
	}

	inTextLayout->BeginUsingContext(this, true);
	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!inTextLayout->fUseFontTrueTypeOnly)))
	{
		StParentContextNoDraw hdcNoDraw(this);
		bool inside = VGraphicContext::_GetTextLayoutCharIndexFromPos(inTextLayout, inTopLeft, inPos, outCharIndex);
		inTextLayout->EndUsingContext();
		return inside;
	}

	if (!testAssert(inTextLayout->fLayoutIsValid))
	{
		inTextLayout->EndUsingContext();
		outCharIndex = 0;
		return false;
	}

	xbox_assert(inTextLayout->fLayoutD2D);
	
	VPoint pos(inTopLeft);
	pos.SetPosBy(inTextLayout->fMargin.left+inTextLayout->fCurOverhang.left, inTextLayout->fMargin.top+inTextLayout->fCurOverhang.top);

	BOOL isTrailing = FALSE;
	BOOL isInside = FALSE;
	DWRITE_HIT_TEST_METRICS metrics;
	inTextLayout->fLayoutD2D->HitTestPoint( inPos.GetX()-pos.GetX(), inPos.GetY()-pos.GetY(), &isTrailing, &isInside, &metrics);

	outCharIndex = metrics.textPosition;
	if (isTrailing)
		outCharIndex++;
	if (outCharIndex > inTextLayout->fTextLength)
		outCharIndex = inTextLayout->fTextLength;

	inTextLayout->EndUsingContext();
	return isInside != FALSE;
}


/* update text layout
@remarks
	build text layout according to layout settings & current graphic context settings

	this method is called only from VTextLayout::_UpdateTextLayout
*/
void VWinD2DGraphicContext::_UpdateTextLayout( VTextLayout *inTextLayout)
{
	xbox_assert( inTextLayout->fGC == this);

	if (!testAssert(fRenderTarget != NULL))
		return; //fRenderTarget should have been initialized yet (by VTextLayout::BeginUsingContext)

	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!inTextLayout->fUseFontTrueTypeOnly)))
	{
		//fallback to legacy text rendering
		StParentContextNoDraw hdcNoDraw(this);
		VGraphicContext::_UpdateTextLayout( inTextLayout);
		return;
	}


	inTextLayout->fLayoutBoundsDirty = false;

	//release previous impl
	if (inTextLayout->fLayoutD2D)
	{
		inTextLayout->fLayoutD2D->Release();
		inTextLayout->fLayoutD2D = NULL;
	}
	ReleaseRefCountable(&(inTextLayout->fTextBox));

	VTreeTextStyle *styles = inTextLayout->_RetainFullTreeTextStyle();

	//compute DWrite layout & determine actual bounds
	VRect bounds(0.0f, 0.0f, inTextLayout->fMaxWidth, inTextLayout->fMaxHeight);
	VRect boundsWithOverhangs;
	DWRITE_TEXT_METRICS textMetrics;
	DWRITE_OVERHANG_METRICS textOverhangMetrics;
	_GetStyledTextBoxBounds(inTextLayout->GetText(), styles, 
							inTextLayout->fHAlign, inTextLayout->fVAlign, inTextLayout->fLayoutMode, bounds, 
							&(inTextLayout->fLayoutD2D), 
							inTextLayout->fDPI, false, true, 
							(void *)&textMetrics,
							(void *)&textOverhangMetrics);

	inTextLayout->_ReleaseFullTreeTextStyle( styles);

	if (inTextLayout->fLayoutD2D)
	{
		//store text layout metrics (including overhangs)
		inTextLayout->fCurLayoutWidth = bounds.GetWidth();
		inTextLayout->fCurLayoutHeight = bounds.GetHeight();
		
		if (inTextLayout->fMaxHeight != 0.0f && inTextLayout->fCurLayoutHeight > inTextLayout->fMaxHeight)
			inTextLayout->fCurLayoutHeight = std::floor(inTextLayout->fMaxHeight); //might be ~0 otherwise it is always rounded yet
		if (inTextLayout->fMaxWidth != 0.0f && inTextLayout->fCurLayoutWidth > inTextLayout->fMaxWidth)
			inTextLayout->fCurLayoutWidth = std::floor(inTextLayout->fMaxWidth); //might be ~0 otherwise it is always rounded yet
		
		//store text layout overhang metrics
		inTextLayout->fCurOverhang.left = textOverhangMetrics.left > 0.0f ? textOverhangMetrics.left : 0.0f;
		inTextLayout->fCurOverhang.right = textOverhangMetrics.right > 0.0f ? textOverhangMetrics.right : 0.0f;
		inTextLayout->fCurOverhang.top = textOverhangMetrics.top > 0.0f ? textOverhangMetrics.top : 0.0f;
		inTextLayout->fCurOverhang.bottom = textOverhangMetrics.bottom > 0.0f ? textOverhangMetrics.bottom : 0.0f;

		//store formatted text metrics
		inTextLayout->fCurWidth = std::ceil(textMetrics.widthIncludingTrailingWhitespace+inTextLayout->fCurOverhang.left+inTextLayout->fCurOverhang.right);
		inTextLayout->fCurHeight = std::ceil(textMetrics.height+inTextLayout->fCurOverhang.top+inTextLayout->fCurOverhang.bottom);

		if (inTextLayout->fMargin.left != 0.0f || inTextLayout->fMargin.right != 0.0f)
		{
			inTextLayout->fCurWidth += inTextLayout->fMargin.left+inTextLayout->fMargin.right;
			inTextLayout->fCurLayoutWidth += inTextLayout->fMargin.left+inTextLayout->fMargin.right;
		}
		if (inTextLayout->fMargin.top != 0.0f || inTextLayout->fMargin.bottom != 0.0f)
		{
			inTextLayout->fCurHeight += inTextLayout->fMargin.top+inTextLayout->fMargin.bottom;
			inTextLayout->fCurLayoutHeight += inTextLayout->fMargin.top+inTextLayout->fMargin.bottom;
		}
	}
}

void VWinD2DGraphicContext::DrawStyledText( const VString& inText, VTreeTextStyle *inStyles, AlignStyle inHAlign, AlignStyle inVAlign, const VRect& inHwndBounds, TextLayoutMode inLayoutMode, const GReal inRefDocDPI)
{
	if (inText.IsEmpty())
		return;

	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();
	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!UseFontTrueTypeOnly( inStyles))))
	{
		_DrawLegacyStyledText(inText, inStyles, inHAlign, inVAlign, inHwndBounds, inLayoutMode, inRefDocDPI);
		return;
	}

	StUseContext_NoRetain	context(this);
	FlushGDI();

	VTaskLock lock(&fMutexDWriteFactory);
	IDWriteTextLayout *textLayout = NULL;
	VFont *fontScaled = NULL;
	if (inRefDocDPI != 72.0f)
	{
		fontScaled = VFont::RetainFont( fTextFont->GetName(), fTextFont->GetFace(), fTextFont->GetPixelSize(), inRefDocDPI);
		xbox_assert(fontScaled);
		textLayout = fontScaled->GetImpl().CreateTextLayout( fRenderTarget, inText, (!(fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)), inHwndBounds.GetWidth(), inHwndBounds.GetHeight(), inHAlign, inVAlign, inLayoutMode, inStyles, inRefDocDPI);
		ReleaseRefCountable(&fontScaled);
	}
	else
		textLayout = fTextFont->GetImpl().CreateTextLayout( fRenderTarget, inText, (!(fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)), inHwndBounds.GetWidth(), inHwndBounds.GetHeight(), inHAlign, inVAlign, inLayoutMode, inStyles, inRefDocDPI);
	if (!textLayout)
		return;

	DWRITE_OVERHANG_METRICS textOverhangMetrics;
	if (!(inLayoutMode & TLM_D2D_IGNORE_OVERHANGS))
		textLayout->GetOverhangMetrics( &textOverhangMetrics);
	else
		textOverhangMetrics.left = textOverhangMetrics.right = textOverhangMetrics.top = textOverhangMetrics.bottom = 0.0f;

	VPoint pos( inHwndBounds.GetTopLeft());
	pos.SetPosBy( textOverhangMetrics.left > 0 ? textOverhangMetrics.left : 0.0f, textOverhangMetrics.top > 0 ? textOverhangMetrics.top : 0.0f);

	if (textOverhangMetrics.left > 0.0f || 
		textOverhangMetrics.right > 0.0f || 
		textOverhangMetrics.top > 0.0f || 
		textOverhangMetrics.bottom > 0.0f || 
		(fTextRenderingMode & TRM_WITHOUT_ANTIALIASING))
	{
		//glyphs extend outside the layout box so clip including glyph extend
		//(but not vertically)
		VRect bounds(inHwndBounds);
		GReal x = bounds.GetX();
		GReal width = bounds.GetWidth();
		if (textOverhangMetrics.left > 0)
			width += textOverhangMetrics.left;
		if (textOverhangMetrics.right > 0)
			width += textOverhangMetrics.right;
		if (fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)
		{
			//D2D1_DRAW_TEXT_OPTIONS_CLIP seems to be quite aggressive if antialiasing is disabled (sometimes text leftmost or rightmost pixel is cropped...)
			//as a workaround we need to slightly increase clipping horizontal bounds (1px) in device space 
			VAffineTransform ctm;
			GetTransform( ctm);
			if (ctm.GetScaleX())
			{
				GReal inflate = 1.0f/ctm.GetScaleX();
				x -= inflate;
				width += inflate*2;
			}
		}
		bounds.SetX( x);
		bounds.SetWidth( width);
		SaveClip();
		ClipRect(bounds);
		fRenderTarget->DrawTextLayout( D2D_POINT(pos),
									   textLayout,
									   fTextBrush,
									   D2D1_DRAW_TEXT_OPTIONS_NONE);
		RestoreClip();
	}
	else
		fRenderTarget->DrawTextLayout( D2D_POINT(pos),
									   textLayout,
									   fTextBrush,
									   D2D1_DRAW_TEXT_OPTIONS_CLIP);
	textLayout->Release();
	
}

void  VWinD2DGraphicContext::_GetStyledTextBoxBounds( const VString& inText, VTreeTextStyle *inStyles, AlignStyle inHAlign, AlignStyle inVAlign, TextLayoutMode inLayoutMode, 
													 VRect& ioBounds, IDWriteTextLayout **outLayout, 
													 const GReal inRefDocDPI, bool inEnableLegacy, bool inReturnLayoutDesignBounds, void *inTextMetrics, void *inTextOverhangMetrics, bool inForceMonoStyle)
{
	if (inText.IsEmpty() && !inTextMetrics && !inTextOverhangMetrics)
	{
		ioBounds.SetWidth(0.0f);
		ioBounds.SetHeight(0.0f);
		if (outLayout)
			*outLayout = NULL;
		return;
	}

	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	if (inEnableLegacy)
		if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!UseFontTrueTypeOnly( inStyles))))
		{
			StParentContextNoDraw hdcNoDraw(this);
			_GetLegacyStyledTextBoxBounds(inText, inStyles, ioBounds, inRefDocDPI, inForceMonoStyle);
			if (outLayout)
				*outLayout = NULL;
			return;
		}

	VTaskLock lock(&fMutexDWriteFactory);
		
	IDWriteTextLayout *textLayout = NULL;
	VFont *fontScaled = NULL;
	if (inRefDocDPI != 72.0f)
	{
		fontScaled = VFont::RetainFont( fTextFont->GetName(), fTextFont->GetFace(), fTextFont->GetPixelSize(), inRefDocDPI);
		xbox_assert(fontScaled);
		textLayout = fontScaled->GetImpl().CreateTextLayout( 
						fRenderTarget,
						inText, 
						(!(fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)),
						ioBounds.GetWidth() != 0.0f ? ioBounds.GetWidth() : DWRITE_TEXT_LAYOUT_MAX_WIDTH, 
						ioBounds.GetHeight() != 0.0f ? ioBounds.GetHeight() : DWRITE_TEXT_LAYOUT_MAX_HEIGHT, 
						inHAlign, inVAlign, inLayoutMode,
						inStyles,
						inRefDocDPI,
						!inReturnLayoutDesignBounds);
		ReleaseRefCountable(&fontScaled);
	}
	else
		textLayout = fTextFont->GetImpl().CreateTextLayout( 
						fRenderTarget,
						inText, 
						(!(fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)),
						ioBounds.GetWidth() != 0.0f ? ioBounds.GetWidth() : DWRITE_TEXT_LAYOUT_MAX_WIDTH, 
						ioBounds.GetHeight() != 0.0f ? ioBounds.GetHeight() : DWRITE_TEXT_LAYOUT_MAX_HEIGHT, 
						inHAlign, inVAlign, inLayoutMode,
						inStyles,
						inRefDocDPI,
						!inReturnLayoutDesignBounds);
	if (!textLayout)
	{
		ioBounds.SetWidth(0.0f);
		ioBounds.SetHeight(0.0f);
		if (outLayout)
			*outLayout = NULL;
		return;
	}

	DWRITE_TEXT_METRICS textMetricsLocal;
	DWRITE_TEXT_METRICS *textMetrics;
	if (inTextMetrics)
		textMetrics = (DWRITE_TEXT_METRICS *)inTextMetrics;
	else
		textMetrics = &textMetricsLocal;

	textLayout->GetMetrics( textMetrics);

	if (inReturnLayoutDesignBounds)
	{
		//return text layout bounds
		if (ioBounds.GetWidth() == 0.0f || ioBounds.GetHeight() == 0.0f)
		{
			//if layout bounds are not fixed by caller, determine them & update DWrite layout
			//(this is necessary to avoid to align formatted text on DWRITE_TEXT_LAYOUT_MAX_WIDTH or DWRITE_TEXT_LAYOUT_MAX_HEIGHT in right or center or default alignment mode
			// if no max width or height is defined)
			GReal maxWidth = 0.0f, maxHeight = 0.0f;
			if (ioBounds.GetWidth() == 0.0f)
				//set max width to computed formatted text width
				textLayout->SetMaxWidth( maxWidth = textMetrics->widthIncludingTrailingWhitespace);
			if (ioBounds.GetHeight() == 0.0f)
				//set max height to computed formatted text height
				textLayout->SetMaxHeight( maxHeight = textMetrics->height);
			//get new metrics
			textLayout->GetMetrics( textMetrics);
			xbox_assert(ioBounds.GetWidth() != 0.0f || textMetrics->layoutWidth <= maxWidth);
			xbox_assert(ioBounds.GetHeight() != 0.0f || textMetrics->layoutHeight <= maxHeight);
		}
		ioBounds.SetWidth( textMetrics->layoutWidth);
		ioBounds.SetHeight( textMetrics->layoutHeight);
	}
	else
	{
		//return formatted text bounds (can be smaller than layout bounds depending on alignment & max width or height)
		ioBounds.SetWidth( textMetrics->widthIncludingTrailingWhitespace);
		ioBounds.SetHeight( textMetrics->height);
	}

	//inflate with overhang metrics

	if (!(inLayoutMode & TLM_D2D_IGNORE_OVERHANGS))
	{
		DWRITE_OVERHANG_METRICS textOverhangMetricsLocal;
		DWRITE_OVERHANG_METRICS *textOverhangMetrics = inTextOverhangMetrics ? (DWRITE_OVERHANG_METRICS *)inTextOverhangMetrics : &textOverhangMetricsLocal;

		textLayout->GetOverhangMetrics( textOverhangMetrics);

		if (textOverhangMetrics->left > 0.0f)
			ioBounds.SetSizeBy( textOverhangMetrics->left, 0.0f);
		if (textOverhangMetrics->right > 0.0f)
			ioBounds.SetSizeBy( textOverhangMetrics->right, 0.0f);
		if (textOverhangMetrics->top > 0.0f)
			ioBounds.SetSizeBy( 0.0f, textOverhangMetrics->top);
		if (textOverhangMetrics->bottom > 0.0f)
			ioBounds.SetSizeBy( 0.0f, textOverhangMetrics->bottom);
	}
	else if (inTextOverhangMetrics)
	{
		DWRITE_OVERHANG_METRICS *textOverhangMetrics = (DWRITE_OVERHANG_METRICS *)inTextOverhangMetrics;
		textOverhangMetrics->left = textOverhangMetrics->right = textOverhangMetrics->top = textOverhangMetrics->bottom = 0.0f;
	}

	ioBounds.SetWidth( std::ceil(ioBounds.GetWidth()));
	ioBounds.SetHeight( std::ceil(ioBounds.GetHeight()));

	//deal with math discrepancies: ensure last line of text is not clipped (so if height is 40.99 we convert it to 50)
	//GReal height = ioBounds.GetHeight();
	//if ((ceil(height)-height) <= 0.01)
	//	ioBounds.SetHeight(ceil(height));
	
	if (outLayout)
		*outLayout = textLayout;
	else
		textLayout->Release();
}

//return styled text box bounds
//@remark:
// if input bounds width is not null, use it as max line wrapping width (or max text height in vertical layout mode)
// if input bounds height is not null, use it as max text height (or max line wrapping width in vertical layout mode)
void VWinD2DGraphicContext::GetStyledTextBoxBounds( const VString& inText, VTreeTextStyle *inStyles, VRect& ioBounds, const GReal inRefDocDPI, bool inForceMonoStyle)
{
	_GetStyledTextBoxBounds( inText, inStyles, AL_LEFT, AL_TOP, TLM_NORMAL, ioBounds, NULL, inRefDocDPI, true, false, NULL, NULL, inForceMonoStyle);
}


/** return styled text box run bounds from the specified range
@remarks
	used only by some impl like Direct2D or CoreText in order to draw text run background
*/
void VWinD2DGraphicContext::GetStyledTextBoxRunBoundsFromRange( const VString& inText, VTreeTextStyle *inStyles, const VRect& inBounds, std::vector<VRect>& outRunBounds, sLONG inStart, sLONG inEnd, AlignStyle inHAlign, AlignStyle inVAlign, TextLayoutMode inMode, const GReal inRefDocDPI)
{
	if (inText.IsEmpty())
		return;

	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!UseFontTrueTypeOnly( inStyles))))
	{
		StParentContextNoDraw hdcNoDraw(this);
		_GetLegacyStyledTextBoxRunBoundsFromRange(inText, inStyles, inBounds, outRunBounds, inStart, inEnd, inHAlign, inVAlign, inMode, inRefDocDPI);
		return;
	}

	VTaskLock lock(&fMutexDWriteFactory);
	IDWriteTextLayout *textLayout = NULL;
	VFont *fontScaled = NULL;
	if (inRefDocDPI != 72.0f)
	{
		fontScaled = VFont::RetainFont( fTextFont->GetName(), fTextFont->GetFace(), fTextFont->GetPixelSize(), inRefDocDPI);
		xbox_assert(fontScaled);
		textLayout = fontScaled->GetImpl().CreateTextLayout( fRenderTarget, inText, (!(fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)), inBounds.GetWidth(), inBounds.GetHeight(), inHAlign, inVAlign, inMode, inStyles, inRefDocDPI);
		ReleaseRefCountable(&fontScaled);
	}
	else
		textLayout = fTextFont->GetImpl().CreateTextLayout( fRenderTarget, inText, (!(fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)), inBounds.GetWidth(), inBounds.GetHeight(), inHAlign, inVAlign, inMode, inStyles, inRefDocDPI);
	if (!textLayout)
		return;

	if (inEnd < 0)
		inEnd = inText.GetLength();

	VPoint pos( inBounds.GetTopLeft());
	if (!(inMode & TLM_D2D_IGNORE_OVERHANGS))
	{
		DWRITE_OVERHANG_METRICS textOverhangMetrics;
		textLayout->GetOverhangMetrics( &textOverhangMetrics);

		pos.SetPosBy( textOverhangMetrics.left > 0 ? textOverhangMetrics.left : 0.0f, textOverhangMetrics.top > 0 ? textOverhangMetrics.top : 0.0f);
	}

	_GetRangeBounds( textLayout, inStart, inEnd-inStart, pos, outRunBounds);
}


/** for the specified styled text box, return the text position at the specified coordinates
@remarks
	output text position is 0-based

	input coordinates are in graphic context user space

	return true if input position is inside character bounds
	if method returns false, it returns the closest character index to the input position
*/
bool VWinD2DGraphicContext::GetStyledTextBoxCharIndexFromCoord( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VPoint& inPos, VIndex& outCharIndex, AlignStyle inHAlign, AlignStyle inVAlign, TextLayoutMode inLayoutMode, const GReal inRefDocDPI)
{
	if (inText.IsEmpty())
	{
		outCharIndex = 0;
		return false;
	}

	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!UseFontTrueTypeOnly( inStyles))))
	{
		StParentContextNoDraw hdcNoDraw(this);
		return _GetLegacyStyledTextBoxCharIndexFromCoord( inText, inStyles, inHwndBounds, inPos, outCharIndex, inHAlign, inVAlign, inLayoutMode, inRefDocDPI);
	}
	VTaskLock lock(&fMutexDWriteFactory);
	IDWriteTextLayout *textLayout = NULL;
	VFont *fontScaled = NULL;
	if (inRefDocDPI != 72.0f)
	{
		fontScaled = VFont::RetainFont( fTextFont->GetName(), fTextFont->GetFace(), fTextFont->GetPixelSize(), inRefDocDPI);
		xbox_assert(fontScaled);
		textLayout = fontScaled->GetImpl().CreateTextLayout( fRenderTarget, inText, (!(fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)), inHwndBounds.GetWidth(), inHwndBounds.GetHeight(), inHAlign, inVAlign, inLayoutMode, inStyles, inRefDocDPI);
		ReleaseRefCountable(&fontScaled);
	}
	else
		textLayout = fTextFont->GetImpl().CreateTextLayout( fRenderTarget, inText, (!(fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)), inHwndBounds.GetWidth(), inHwndBounds.GetHeight(), inHAlign, inVAlign, inLayoutMode, inStyles, inRefDocDPI);
	if (!textLayout)
	{
		outCharIndex = 0;
		return false;
	}

	VPoint pos( inHwndBounds.GetTopLeft());
	if (!(inLayoutMode & TLM_D2D_IGNORE_OVERHANGS))
	{
		DWRITE_OVERHANG_METRICS textOverhangMetrics;
		textLayout->GetOverhangMetrics( &textOverhangMetrics);

		pos.SetPosBy( textOverhangMetrics.left > 0 ? textOverhangMetrics.left : 0.0f, textOverhangMetrics.top > 0 ? textOverhangMetrics.top : 0.0f);
	}

	BOOL isTrailing = FALSE;
	BOOL isInside = FALSE;
	DWRITE_HIT_TEST_METRICS metrics;
	textLayout->HitTestPoint( inPos.GetX()-pos.GetX(), inPos.GetY()-pos.GetY(), &isTrailing, &isInside, &metrics);
	textLayout->Release();

	outCharIndex = metrics.textPosition;
	if (isTrailing)
	{
		outCharIndex++;
		if (outCharIndex > inText.GetLength())
			outCharIndex = inText.GetLength();
	}
	return isInside != FALSE;
}

DWRITE_LINE_METRICS *VWinD2DGraphicContext::_GetLineMetrics( IDWriteTextLayout *inLayout, UINT32& outLineCount, bool& doDeleteLineMetrics)
{
	outLineCount = 0;
	static DWRITE_LINE_METRICS WinD2DLineMetrics[8];
	DWRITE_LINE_METRICS *pLineMetrics = &(WinD2DLineMetrics[0]);
	doDeleteLineMetrics = false;
	HRESULT hr = inLayout->GetLineMetrics( pLineMetrics, 8, &outLineCount); 
	if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
	{
		pLineMetrics = new DWRITE_LINE_METRICS [outLineCount];
		if (FAILED(inLayout->GetLineMetrics( pLineMetrics, outLineCount, &outLineCount)))
		{
			delete [] pLineMetrics;
			return NULL;
		}
		doDeleteLineMetrics = true;
		return pLineMetrics;
	}
	else if (FAILED(hr))
		return NULL;
	return pLineMetrics;
}

void VWinD2DGraphicContext::_GetRangeBounds( IDWriteTextLayout *inLayout, const VIndex inStart, const uLONG inLength, const VPoint& inOffset, std::vector<VRect>& outRangeBounds)
{
	DWRITE_HIT_TEST_METRICS hitTestMetrics[8];
	DWRITE_HIT_TEST_METRICS *pHitTestMetrics = &(hitTestMetrics[0]);
	bool doDeleteMetrics = false;
	UINT32 hitTestMetricsCount = 0;
	HRESULT hr = inLayout->HitTestTextRange( inStart, inLength, 0.0f, 0.0f, pHitTestMetrics, 8, &hitTestMetricsCount); 
	if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
	{
		pHitTestMetrics = new DWRITE_HIT_TEST_METRICS [hitTestMetricsCount];
		if (FAILED(inLayout->HitTestTextRange( inStart, inLength, 0.0f, 0.0f, pHitTestMetrics, hitTestMetricsCount, &hitTestMetricsCount)))
		{
			delete [] pHitTestMetrics;
			return;
		}
		doDeleteMetrics = true;
	}
	else if (FAILED(hr))
		return;

	DWRITE_HIT_TEST_METRICS *curMetrics = pHitTestMetrics;
	for (int i = 0; i < hitTestMetricsCount; i++, curMetrics++)
		outRangeBounds.push_back(VRect( curMetrics->left+inOffset.GetX(), curMetrics->top+inOffset.GetY(), curMetrics->width, curMetrics->height));

	if (doDeleteMetrics)
		delete [] pHitTestMetrics;
}


void VWinD2DGraphicContext::_GetRangeBounds( IDWriteTextLayout *inLayout, const VIndex inStart, const uLONG inLength, VRect& outRangeBounds)
{
	DWRITE_HIT_TEST_METRICS hitTestMetrics[8];
	DWRITE_HIT_TEST_METRICS *pHitTestMetrics = &(hitTestMetrics[0]);
	bool doDeleteMetrics = false;
	UINT32 hitTestMetricsCount = 0;
	HRESULT hr = inLayout->HitTestTextRange( inStart, inLength, 0.0f, 0.0f, pHitTestMetrics, 8, &hitTestMetricsCount); 
	if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
	{
		pHitTestMetrics = new DWRITE_HIT_TEST_METRICS [hitTestMetricsCount];
		if (FAILED(inLayout->HitTestTextRange( inStart, inLength, 0.0f, 0.0f, pHitTestMetrics, hitTestMetricsCount, &hitTestMetricsCount)))
		{
			delete [] pHitTestMetrics;
			outRangeBounds.SetEmpty();
			return;
		}
		doDeleteMetrics = true;
	}
	else if (FAILED(hr))
	{
		outRangeBounds.SetEmpty();
		return;
	}

	DWRITE_HIT_TEST_METRICS *curMetrics = pHitTestMetrics;
	outRangeBounds = VRect( curMetrics->left, curMetrics->top, curMetrics->width, curMetrics->height);
	curMetrics++;
	for (int i = 1; i < hitTestMetricsCount; i++, curMetrics++)
	{
		VRect bounds( curMetrics->left, curMetrics->top, curMetrics->width, curMetrics->height);
		outRangeBounds.Union( bounds);
	}

	if (doDeleteMetrics)
		delete [] pHitTestMetrics;
}

void VWinD2DGraphicContext::_GetLineBoundsFromCharIndex( IDWriteTextLayout *inLayout, const VIndex inCharIndex, VRect& outLineBounds)
{
	UINT32 lineCount = 0;
	bool doDeleteLineMetrics = false;
	DWRITE_LINE_METRICS *pLineMetrics = _GetLineMetrics( inLayout, lineCount, doDeleteLineMetrics);
	if (!pLineMetrics)
	{
		outLineBounds.SetEmpty();
		return;
	}

	DWRITE_LINE_METRICS *curLineMetrics = pLineMetrics;
	int pos = 0;
	for (int i = 0; i < lineCount; i++, curLineMetrics++)
	{
		int lineLength = curLineMetrics->length;
		xbox_assert(lineLength >= curLineMetrics->trailingWhitespaceLength);

		if (inCharIndex >= pos && (inCharIndex < pos+lineLength 
			|| 
			(lineLength == 0 && inCharIndex == pos)
			||
			((i == lineCount-1) && inCharIndex >= pos+lineLength)))
		{
			_GetRangeBounds( inLayout, pos, lineLength, outLineBounds);
			break;
		}

		pos += lineLength;
	}

	if (doDeleteLineMetrics)
		delete [] pLineMetrics;
}


/** for the specified styled text box, return the caret position & height of text at the specified text position
@remarks
	text position is 0-based

	caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)
*/
void VWinD2DGraphicContext::GetStyledTextBoxCaretMetricsFromCharIndex( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading, const bool inCaretUseCharMetrics, AlignStyle inHAlign, AlignStyle inVAlign, TextLayoutMode inLayoutMode, const GReal inRefDocDPI)
{
	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	if ((fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!UseFontTrueTypeOnly( inStyles))))
	{
		StParentContextNoDraw hdcNoDraw(this);
		return _GetLegacyStyledTextBoxCaretMetricsFromCharIndex( inText, inStyles, inHwndBounds, inCharIndex, outCaretPos, outTextHeight, inCaretLeading, inCaretUseCharMetrics, inHAlign, inVAlign, inLayoutMode, inRefDocDPI);
	}
	VTaskLock lock(&fMutexDWriteFactory);

	IDWriteTextLayout *textLayout = NULL;
	bool leading = inCaretLeading;
	VIndex charIndex = inCharIndex;
	if (inText.IsEmpty())
	{
		//for a empty text, ensure we return a valid caret
		VFont *fontScaled = NULL;
		if (inRefDocDPI != 72.0f)
		{
			fontScaled = VFont::RetainFont( fTextFont->GetName(), fTextFont->GetFace(), fTextFont->GetPixelSize(), inRefDocDPI);
			xbox_assert(fontScaled);
			textLayout = fontScaled->GetImpl().CreateTextLayout( fRenderTarget, VString("x"), (!(fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)), inHwndBounds.GetWidth(), inHwndBounds.GetHeight(), inHAlign, inVAlign, inLayoutMode, inStyles, inRefDocDPI);
			ReleaseRefCountable(&fontScaled);
		}
		else
			textLayout = fTextFont->GetImpl().CreateTextLayout( fRenderTarget, VString("x"), (!(fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)), inHwndBounds.GetWidth(), inHwndBounds.GetHeight(), inHAlign, inVAlign, inLayoutMode, inStyles, inRefDocDPI);
		charIndex = 0;
		leading = true;
	}
	else
	{
		VFont *fontScaled = NULL;
		if (inRefDocDPI != 72.0f)
		{
			fontScaled = VFont::RetainFont( fTextFont->GetName(), fTextFont->GetFace(), fTextFont->GetPixelSize(), inRefDocDPI);
			xbox_assert(fontScaled);
			textLayout = fontScaled->GetImpl().CreateTextLayout( fRenderTarget, inText, (!(fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)), inHwndBounds.GetWidth(), inHwndBounds.GetHeight(), inHAlign, inVAlign, inLayoutMode, inStyles, inRefDocDPI);
			ReleaseRefCountable(&fontScaled);
		}
		else
			textLayout = fTextFont->GetImpl().CreateTextLayout( fRenderTarget, inText, (!(fTextRenderingMode & TRM_WITHOUT_ANTIALIASING)), inHwndBounds.GetWidth(), inHwndBounds.GetHeight(), inHAlign, inVAlign, inLayoutMode, inStyles, inRefDocDPI);
	}
	if (!textLayout)
		return;

	FLOAT x,y;
	DWRITE_HIT_TEST_METRICS metrics;
	textLayout->HitTestTextPosition(  charIndex, leading ? FALSE : TRUE, &x, &y, &metrics);

	VPoint pos( inHwndBounds.GetTopLeft());
	if (!(inLayoutMode & TLM_D2D_IGNORE_OVERHANGS))
	{
		DWRITE_OVERHANG_METRICS textOverhangMetrics;
		textLayout->GetOverhangMetrics( &textOverhangMetrics);

		pos.SetPosBy( textOverhangMetrics.left > 0 ? textOverhangMetrics.left : 0.0f, textOverhangMetrics.top > 0 ? textOverhangMetrics.top : 0.0f);
	}

	if (inCaretUseCharMetrics)
	{
		//get metrics from character pos

		textLayout->Release();
		outCaretPos.SetX( x+pos.GetX());
		outCaretPos.SetY( y+pos.GetY());
		outTextHeight = metrics.height;
	}
	else
	{
		//get metrics from the line containing the character

		outCaretPos.SetX( x+pos.GetX());
		VRect lineBounds;
		_GetLineBoundsFromCharIndex(textLayout, charIndex, lineBounds);
		outCaretPos.SetY( lineBounds.GetY()+pos.GetY());
		outTextHeight = lineBounds.GetHeight();

		textLayout->Release();
	}
}

/** extract text lines & styles from a wrappable text using the specified passed max width
@remarks 
	layout mode is assumed to be equal to default = TLM_NORMAL
*/
void VWinD2DGraphicContext::GetTextBoxLines( const VString& inText, const GReal inMaxWidth, std::vector<VString>& outTextLines, VTreeTextStyle *inStyles, std::vector<VTreeTextStyle *> *outTextLinesStyles, const GReal inRefDocDPI, const bool inNoBreakWord, bool *outLinesOverflow)  
{
	if (fRenderTarget == NULL)
		_InitDeviceDependentObjects();

	if (inNoBreakWord || (fTextRenderingMode & TRM_LEGACY_ON) || ((!(fTextRenderingMode & TRM_LEGACY_OFF)) && (!UseFontTrueTypeOnly( inStyles))))
	{
		//DWrite breaks words if max width is too small so fallback on generic method
		StParentContextNoDraw hdcNoDraw(this);
		VGraphicContext::GetTextBoxLines( inText, inMaxWidth, outTextLines, inStyles, outTextLinesStyles, inRefDocDPI, inNoBreakWord, outLinesOverflow);
		return;
	}

	if (outLinesOverflow)
		*outLinesOverflow = false;

	outTextLines.clear();
	if (outTextLinesStyles)
		outTextLinesStyles->clear();

	IDWriteTextLayout *textLayout = NULL;
	VRect bounds(0.0f, 0.0f, inMaxWidth == 0.0f ? 0.0000001f : inMaxWidth, DWRITE_TEXT_LAYOUT_MAX_HEIGHT);
	_GetStyledTextBoxBounds( inText, inStyles, AL_LEFT, AL_TOP, TLM_NORMAL, bounds, &textLayout, inRefDocDPI);
	if (!textLayout)
		return;	

	//get line metrics
	UINT32 lineCount = 0;
	bool doDeleteLineMetrics = false;
	DWRITE_LINE_METRICS *pLineMetrics = _GetLineMetrics( textLayout, lineCount, doDeleteLineMetrics);
	if (!pLineMetrics)
	{
		textLayout->Release();
		return;
	}

	//get text lines
	DWRITE_LINE_METRICS *curLineMetrics = pLineMetrics;
	int pos = 0;
	for (int i = 0; i < lineCount; i++, curLineMetrics++)
	{
		xbox_assert(curLineMetrics->length >= curLineMetrics->trailingWhitespaceLength);
		VString textLine;
		int lineLength = curLineMetrics->length-curLineMetrics->trailingWhitespaceLength;

		//get line text
		inText.GetSubString( pos+1, lineLength, textLine);
		outTextLines.push_back(textLine);

		//get line styles
		if (outTextLinesStyles)
		{
			VTreeTextStyle *styles = NULL;
			if (inStyles && lineLength > 0)
				styles = inStyles->CreateTreeTextStyleOnRange( pos, pos+lineLength);
			outTextLinesStyles->push_back( styles);
		}

		pos += curLineMetrics->length;
	}

	if (doDeleteLineMetrics)
		delete [] pLineMetrics;

	textLayout->Release();

#if VERSIONDEBUG
	//for (int i = 0; i < outTextLines.size(); i++)
	//{
	//	xbox::DebugMsg( outTextLines[i]+"\n");
	//}
#endif
}


#pragma mark-

void VWinD2DGraphicContext::FrameRect(const VRect& _inHwndBounds)
{
	StUseContext_NoRetain	context(this);

	if (fGDI_Fast && fGDI_HDC && fDrawingMode != DM_SHADOW && fGlobalAlpha == 1.0f 
		&& 
		!fGDI_HasStrokeCustomDashesInherit.back() && fGDI_HasSolidStrokeColorInherit.back())
	{
		fGDI_GC->FrameRect( _inHwndBounds);
		return;
	}

	FlushGDI();

	VRect inHwndBounds(_inHwndBounds);
	if ((fShapeCrispEdgesEnabled || fGDI_Fast) && !fGDI_QDCompatible)
		CEAdjustRectInTransformedSpace( inHwndBounds);

	if (fDrawingMode == DM_SHADOW)
	{
		VRect bounds = inHwndBounds;
		bounds.SetPosBy( fShadowHOffset, fShadowVOffset);
		CComPtr<ID2D1SolidColorBrush> brushShadow;
		if (SUCCEEDED(fRenderTarget->CreateSolidColorBrush( D2D1::ColorF( SHADOW_R/255.0f, SHADOW_G/255.0f, SHADOW_B/255.0f, SHADOW_ALPHA/255.0f), &brushShadow)))
		{
			brushShadow->SetOpacity( fGlobalAlpha);
			fRenderTarget->DrawRectangle( D2D_RECT(bounds), brushShadow, fStrokeWidth, fStrokeStyle);
		}
	}
	fRenderTarget->DrawRectangle( D2D_RECT( inHwndBounds), fStrokeBrush, fStrokeWidth, fStrokeStyle);
}


void VWinD2DGraphicContext::FrameOval(const VRect& _inHwndBounds)
{
	StUseContext_NoRetain	context(this);
	FlushGDI();
	VRect inHwndBounds(_inHwndBounds);

	if ((fShapeCrispEdgesEnabled || fGDI_Fast) && !fGDI_QDCompatible)
		CEAdjustRectInTransformedSpace( inHwndBounds);

	GReal radiusX = inHwndBounds.GetWidth()*0.5f;
	GReal radiusY = inHwndBounds.GetHeight()*0.5f;
	D2D1_POINT_2F center = D2D1::Point2F( inHwndBounds.GetLeft()+radiusX, inHwndBounds.GetTop()+radiusY);
								
	if (fDrawingMode == DM_SHADOW)
	{
		D2D1_POINT_2F centerShadow = center;
		centerShadow.x += fShadowHOffset;
		centerShadow.y += fShadowVOffset;
		CComPtr<ID2D1SolidColorBrush> brushShadow;
		if (SUCCEEDED(fRenderTarget->CreateSolidColorBrush( D2D1::ColorF( SHADOW_R/255.0f, SHADOW_G/255.0f, SHADOW_B/255.0f, SHADOW_ALPHA/255.0f), &brushShadow)))
		{
			brushShadow->SetOpacity( fGlobalAlpha);
			fRenderTarget->DrawEllipse( D2D1::Ellipse( centerShadow, radiusX, radiusY), brushShadow, fStrokeWidth, fStrokeStyle); 
		}
	}
	fRenderTarget->DrawEllipse( D2D1::Ellipse( center, radiusX, radiusY), fStrokeBrush, fStrokeWidth, fStrokeStyle); 
}


void VWinD2DGraphicContext::FrameRegion(const VRegion& inHwndRegion)
{
	VRect bounds = inHwndRegion.GetBounds();
	FrameRect( bounds);
}

/** build path from polygon */
VGraphicPath *VWinD2DGraphicContext::_BuildPathFromPolygon(const VPolygon& inPolygon, bool inFillOnly)
{
	VGraphicPath *path = NULL;
	sLONG pc = inPolygon.GetPointCount();
	if (pc >= 1)
	{
		path = new VGraphicPath();
		path->Begin();

		VPoint start, cur;
		VPoint curpoint;
		bool crispEdges = (fShapeCrispEdgesEnabled || fGDI_Fast) && !fGDI_QDCompatible;
		for(VIndex n = 0; n < pc; n++) 
		{
			inPolygon.GetNthPoint(n+1, cur); 
			if (crispEdges)
				CEAdjustPointInTransformedSpace( cur, inFillOnly);
			
			if (n == 0)
			{
				start = cur;
				path->BeginSubPathAt( cur);
			}
			else
				path->AddLineTo( cur);
		}
		if (cur != start)
			//ensure polygon is closed
			path->CloseSubPath();
		path->End();
	}
	return path;
}

void VWinD2DGraphicContext::FramePolygon(const VPolygon& inHwndPolygon)
{
	VGraphicPath *pathNative = _BuildPathFromPolygon( inHwndPolygon);
	if (!pathNative)
		return;
	FlushGDI();
	if (fShapeCrispEdgesEnabled)
	{
		//crispEdges has already been applied by _BuildPathFromPolygon
		fShapeCrispEdgesEnabled = false;
		FramePath( *pathNative);
		fShapeCrispEdgesEnabled = true;
	}
	else
		FramePath( *pathNative);
	delete pathNative;
}


void VWinD2DGraphicContext::FrameBezier(const VGraphicPath& inHwndBezier)
{
	FramePath( inHwndBezier);
}

void VWinD2DGraphicContext::FramePath(const VGraphicPath& inHwndPath)
{
	if (!inHwndPath.IsBuilded())
	{
		//caller should have called VGraphicPath::End() method to end populating path
		//(necessary by D2D impl in order to finish building path and set it to immutable)
		xbox_assert(false);
		return;
	}

	StUseContext_NoRetain	context(this);
	FlushGDI();
	StUsePath thePath( static_cast<VGraphicContext *>(this), &inHwndPath);

	if (fDrawingMode == DM_SHADOW)
	{
		VAffineTransform ctm;
		GetTransform( ctm);
		VAffineTransform ctmOffset;
		ctmOffset.SetTranslation( fShadowHOffset, fShadowVOffset);
		ctmOffset.Multiply( ctm, VAffineTransform::MatrixOrderAppend);
		SetTransform(ctmOffset);
		
		CComPtr<ID2D1SolidColorBrush> brushShadow;
		if (SUCCEEDED(fRenderTarget->CreateSolidColorBrush( D2D1::ColorF( SHADOW_R/255.0f, SHADOW_G/255.0f, SHADOW_B/255.0f, SHADOW_ALPHA/255.0f), &brushShadow)))
		{
			brushShadow->SetOpacity( fGlobalAlpha);
			fRenderTarget->DrawGeometry( thePath.Get()->GetImplPathD2D(), (ID2D1SolidColorBrush *)brushShadow, fStrokeWidth, fStrokeStyle);
		}
		SetTransform( ctm);
	}
	fRenderTarget->DrawGeometry( thePath.Get()->GetImplPathD2D(), fStrokeBrush, fStrokeWidth, fStrokeStyle);
}



void VWinD2DGraphicContext::FillRect(const VRect& _inHwndBounds)
{
	StUseContext_NoRetain	context(this);

	if (fGDI_Fast && fGDI_HDC && fDrawingMode != DM_SHADOW && fGlobalAlpha == 1.0f 
		&& 
		fGDI_HasSolidFillColorInherit.back())
	{
		fGDI_GC->FillRect( _inHwndBounds);
		return;
	}

	FlushGDI();

	VRect inHwndBounds(_inHwndBounds);
	if ((fShapeCrispEdgesEnabled || fGDI_Fast) && !fGDI_QDCompatible)
		CEAdjustRectInTransformedSpace( inHwndBounds, true);

	if (fDrawingMode == DM_SHADOW)
	{
		VRect bounds = inHwndBounds;
		bounds.SetPosBy( fShadowHOffset, fShadowVOffset);

		CComPtr<ID2D1SolidColorBrush> brushShadow;
		if (SUCCEEDED(fRenderTarget->CreateSolidColorBrush( D2D1::ColorF( SHADOW_R/255.0f, SHADOW_G/255.0f, SHADOW_B/255.0f, SHADOW_ALPHA/255.0f), &brushShadow)))
		{
			brushShadow->SetOpacity( fGlobalAlpha);
			fRenderTarget->FillRectangle( D2D_RECT( bounds), brushShadow);
		}
	}
	fRenderTarget->FillRectangle( D2D_RECT( inHwndBounds), fFillBrush);
}


void VWinD2DGraphicContext::FillOval(const VRect& _inHwndBounds)
{
	StUseContext_NoRetain	context(this);
	FlushGDI();
	VRect inHwndBounds(_inHwndBounds);

	if ((fShapeCrispEdgesEnabled || fGDI_Fast) && !fGDI_QDCompatible)
		CEAdjustRectInTransformedSpace( inHwndBounds, true);

	GReal radiusX = inHwndBounds.GetWidth()*0.5f;
	GReal radiusY = inHwndBounds.GetHeight()*0.5f;
	D2D1_POINT_2F center = D2D1::Point2F( inHwndBounds.GetLeft()+radiusX, inHwndBounds.GetTop()+radiusY);
								
	if (fDrawingMode == DM_SHADOW)
	{
		D2D1_POINT_2F centerShadow = center;
		centerShadow.x += fShadowHOffset;
		centerShadow.y += fShadowVOffset;
		CComPtr<ID2D1SolidColorBrush> brushShadow;
		if (SUCCEEDED(fRenderTarget->CreateSolidColorBrush( D2D1::ColorF( SHADOW_R/255.0f, SHADOW_G/255.0f, SHADOW_B/255.0f, SHADOW_ALPHA/255.0f), &brushShadow)))
		{
			brushShadow->SetOpacity( fGlobalAlpha);
			fRenderTarget->FillEllipse( D2D1::Ellipse( centerShadow, radiusX, radiusY), brushShadow); 
		}
	}
	fRenderTarget->FillEllipse( D2D1::Ellipse( center, radiusX, radiusY), fFillBrush); 
}


void VWinD2DGraphicContext::FillRegion(const VRegion& inHwndRegion)
{
	VRect bounds = inHwndRegion.GetBounds();
	FillRect( bounds);
}


void VWinD2DGraphicContext::FillPolygon(const VPolygon& inHwndPolygon)
{
	VGraphicPath *pathNative = _BuildPathFromPolygon( inHwndPolygon, true);
	if (!pathNative)
		return;
	FlushGDI();
	if (fShapeCrispEdgesEnabled)
	{
		//crispEdges has already been applied by _BuildPathFromPolygon
		fShapeCrispEdgesEnabled = false;
		FillPath( *pathNative);
		fShapeCrispEdgesEnabled = true;
	}
	else
		FillPath( *pathNative);
	delete pathNative;
}


void VWinD2DGraphicContext::FillBezier(VGraphicPath& inHwndBezier)
{
	FillPath( inHwndBezier);
}

void VWinD2DGraphicContext::FillPath(VGraphicPath& inHwndPath)
{
	if (!inHwndPath.IsBuilded())
	{
		//caller should have called VGraphicPath::End() method to end populating path
		//(necessary by D2D impl in order to finish building path and set it to immutable)
		xbox_assert(false);
		return;
	}

	StUseContext_NoRetain	context(this);
	FlushGDI();
	StUsePath thePath( static_cast<VGraphicContext *>(this), &inHwndPath, true);

	if (fDrawingMode == DM_SHADOW)
	{
		VAffineTransform ctm;
		GetTransform( ctm);
		VAffineTransform ctmOffset;
		ctmOffset.SetTranslation( fShadowHOffset, fShadowVOffset);
		ctmOffset.Multiply( ctm, VAffineTransform::MatrixOrderAppend);
		SetTransform(ctmOffset);
		
		CComPtr<ID2D1SolidColorBrush> brushShadow;
		if (SUCCEEDED(fRenderTarget->CreateSolidColorBrush( D2D1::ColorF( SHADOW_R/255.0f, SHADOW_G/255.0f, SHADOW_B/255.0f, SHADOW_ALPHA/255.0f), &brushShadow)))
		{
			brushShadow->SetOpacity( fGlobalAlpha);
			fRenderTarget->FillGeometry( thePath.Get()->GetImplPathD2D(), (ID2D1SolidColorBrush *)brushShadow);
		}
		SetTransform( ctm);
	}
	fRenderTarget->FillGeometry( thePath.Get()->GetImplPathD2D(), fFillBrush);
}


void VWinD2DGraphicContext::_BuildRoundRectPath(const VRect _inBounds,GReal inWidth,GReal inHeight,VGraphicPath& outPath, bool inFillOnly)
{
	VRect inBounds(_inBounds);
	if ((fShapeCrispEdgesEnabled || fGDI_Fast) && !fGDI_QDCompatible)
		CEAdjustRectInTransformedSpace(inBounds, inFillOnly);

	bool isCrispEdgesEnabled = fShapeCrispEdgesEnabled;
	fShapeCrispEdgesEnabled = false;
	VGraphicContext::_BuildRoundRectPath( inBounds, inWidth, inHeight, outPath, inFillOnly);
	fShapeCrispEdgesEnabled = isCrispEdgesEnabled;
}


void VWinD2DGraphicContext::DrawRect(const VRect& inHwndBounds)
{
	StUseContext_NoRetain	context(this);
	if (fGDI_Fast && fGDI_HDC && fDrawingMode != DM_SHADOW && fGlobalAlpha == 1.0f
		&& 
		!fGDI_HasStrokeCustomDashesInherit.back() && fGDI_HasSolidStrokeColorInherit.back() && fGDI_HasSolidFillColorInherit.back())
	{
		fGDI_GC->DrawRect( inHwndBounds);
		return;
	}

	FlushGDI();
	if (fShapeCrispEdgesEnabled && !fGDI_Fast && !fGDI_QDCompatible)
	{
		VRect bounds( inHwndBounds);
		CEAdjustRectInTransformedSpace( bounds);

		fShapeCrispEdgesEnabled = false;
		FillRect(bounds);
		FrameRect(bounds);
		fShapeCrispEdgesEnabled = true;
	}
	else
	{
		FillRect(inHwndBounds);
		FrameRect(inHwndBounds);
	}
}


void VWinD2DGraphicContext::DrawOval(const VRect& inHwndBounds)
{
	if (fShapeCrispEdgesEnabled && !fGDI_Fast && !fGDI_QDCompatible)
	{
		StUseContext_NoRetain	context(this);
		FlushGDI();

		VRect bounds( inHwndBounds);
		CEAdjustRectInTransformedSpace( bounds);

		fShapeCrispEdgesEnabled = false;
		FillOval(bounds);
		FrameOval(bounds);
		fShapeCrispEdgesEnabled = true;
	}
	else
	{
		FillOval(inHwndBounds);
		FrameOval(inHwndBounds);
	}
}


void VWinD2DGraphicContext::DrawRegion(const VRegion& inHwndRegion)
{
	FillRegion(inHwndRegion);
	FrameRegion(inHwndRegion); 
}


void VWinD2DGraphicContext::DrawPolygon(const VPolygon& inHwndPolygon)
{
	VGraphicPath *pathNative = _BuildPathFromPolygon( inHwndPolygon);
	if (!pathNative)
		return;
	FlushGDI();
	if (fShapeCrispEdgesEnabled)
	{
		//crispEdges has already been applied by _BuildPathFromPolygon
		fShapeCrispEdgesEnabled = false;
		FillPath( *pathNative);
		FramePath( *pathNative);
		fShapeCrispEdgesEnabled = true;
	}
	else
	{
		FillPath( *pathNative);
		FramePath( *pathNative);
	}
	delete pathNative;
}


void VWinD2DGraphicContext::DrawBezier(VGraphicPath& inHwndBezier)
{
	DrawPath( inHwndBezier);
}

void VWinD2DGraphicContext::DrawPath(VGraphicPath& inHwndPath)
{
	if (fShapeCrispEdgesEnabled)
	{
		StUseContext_NoRetain	context(this);
		FlushGDI();

		StUsePath thePath( static_cast<VGraphicContext *>(this), &inHwndPath);
		fShapeCrispEdgesEnabled = false;
		FillPath(*(thePath.GetModifiable()));
		FramePath(*(thePath.Get()));
		fShapeCrispEdgesEnabled = true;
	}
	else
	{
		FillPath(inHwndPath);
		FramePath(inHwndPath);
	}
}

void VWinD2DGraphicContext::EraseRect(const VRect& inHwndBounds)
{
	xbox_assert(false);	// Not supported
}


void VWinD2DGraphicContext::EraseRegion(const VRegion& inHwndRegion)
{
	xbox_assert(false);	// Not supported
}


void VWinD2DGraphicContext::InvertRect(const VRect& inHwndBounds)
{
	if (fDPI != 96.0f)
		//only possible while rendering on screen (ie 1 DIP = 1 Pixel)
		return;
	
	RECT r = inHwndBounds;
	HDC hdc = _GetHDC();
	if (hdc)
		::InvertRect(hdc, &r);
	_ReleaseHDC(hdc);
}


void VWinD2DGraphicContext::InvertRegion(const VRegion& inHwndRegion)
{
	if (fDPI != 96.0f)
		//only possible while rendering on screen (ie 1 DIP = 1 Pixel)
		return;

	HDC hdc = _GetHDC();
	::InvertRgn(hdc, inHwndRegion);
	_ReleaseHDC(hdc);
}


void VWinD2DGraphicContext::DrawLine(const VPoint& inHwndStart, const VPoint& inHwndEnd)
{
	StUseContext_NoRetain	context(this);

	if (fGDI_Fast && fGDI_HDC && fDrawingMode != DM_SHADOW && fGlobalAlpha == 1.0f 
		&& 
		!fGDI_HasStrokeCustomDashesInherit.back() && fGDI_HasSolidStrokeColorInherit.back())
	{
		fGDI_GC->DrawLine( inHwndStart, inHwndEnd);
		return;
	}
	FlushGDI();

	D2D1_POINT_2F start;
	D2D1_POINT_2F end; 

	if ((fShapeCrispEdgesEnabled || fGDI_Fast) && !fGDI_QDCompatible)
	{
		VPoint _start = inHwndStart;
		VPoint _end = inHwndEnd;
		CEAdjustPointInTransformedSpace( _start);
		CEAdjustPointInTransformedSpace( _end);
		start = D2D1::Point2F( _start.GetX(), _start.GetY());
		end	= D2D1::Point2F( _end.GetX(), _end.GetY());
	}
	else
	{
		start = D2D1::Point2F( inHwndStart.GetX(), inHwndStart.GetY());
		end	= D2D1::Point2F( inHwndEnd.GetX(), inHwndEnd.GetY());
	}

	if (fDrawingMode == DM_SHADOW)
	{
		D2D1_POINT_2F startShadow = start;
		D2D1_POINT_2F endShadow = end;
		startShadow.x += fShadowHOffset;
		startShadow.y += fShadowVOffset;
		endShadow.x += fShadowHOffset;
		endShadow.y += fShadowVOffset;

		CComPtr<ID2D1SolidColorBrush> brushShadow;
		if (SUCCEEDED(fRenderTarget->CreateSolidColorBrush( D2D1::ColorF( SHADOW_R/255.0f, SHADOW_G/255.0f, SHADOW_B/255.0f, SHADOW_ALPHA/255.0f), &brushShadow)))
		{
			brushShadow->SetOpacity( fGlobalAlpha);
			fRenderTarget->DrawLine( startShadow, endShadow, brushShadow, fStrokeWidth, fStrokeStyle);
		}
	}
	fRenderTarget->DrawLine( start, end, fStrokeBrush, fStrokeWidth, fStrokeStyle);
}

void VWinD2DGraphicContext::DrawLine(const GReal inHwndStartX,const GReal inHwndStartY, const GReal inHwndEndX,const GReal inHwndEndY)
{
	DrawLine( VPoint(inHwndStartX, inHwndStartY), VPoint(inHwndEndX, inHwndEndY));
}

void VWinD2DGraphicContext::DrawLines(const GReal* inCoords, sLONG inCoordCount)
{
	if (inCoordCount<4)
		return;

	StUseContext_NoRetain	context(this);
	if (fGDI_Fast && fGDI_HDC && fDrawingMode != DM_SHADOW && fGlobalAlpha == 1.0f 
		&& 
		!fGDI_HasStrokeCustomDashesInherit.back() && fGDI_HasSolidStrokeColorInherit.back())
	{
		fGDI_GC->DrawLines( inCoords, inCoordCount);
		return;
	}

	FlushGDI();

	if (fShapeCrispEdgesEnabled && !fGDI_Fast && !fGDI_QDCompatible)
	{
		fShapeCrispEdgesEnabled = false;

		VPoint last( inCoords[0], inCoords[1]);
		CEAdjustPointInTransformedSpace( last);

		for(VIndex n=2; n<inCoordCount; n+=2)
		{
			VPoint cur( inCoords[n], inCoords[n+1]);
			CEAdjustPointInTransformedSpace( cur);

			DrawLine( last, cur);

			last = cur;
		}

		fShapeCrispEdgesEnabled = true;
		return;
	}

	GReal lastX = inCoords[0];
	GReal lastY = inCoords[1];
	for(VIndex n = 2; n < inCoordCount; n += 2)
	{
		DrawLine(lastX, lastY, inCoords[n], inCoords[n+1]);
		lastX = inCoords[n];
		lastY = inCoords[n+1];
	}
}


void VWinD2DGraphicContext::DrawLineTo(const VPoint& inHwndEnd)
{
	DrawLine(fBrushPos.GetX(),fBrushPos.GetY(),inHwndEnd.GetX(),inHwndEnd.GetY());
	fBrushPos=inHwndEnd;
}


void VWinD2DGraphicContext::DrawLineBy(GReal inWidth, GReal inHeight)
{
	DrawLine(fBrushPos.GetX(),fBrushPos.GetY(),fBrushPos.GetX()+inWidth,fBrushPos.GetY()+inHeight);
	fBrushPos.SetPosBy( inWidth,inHeight);
}


void VWinD2DGraphicContext::MoveBrushTo(const VPoint& inHwndPos)
{	
	fBrushPos = inHwndPos;
}


void VWinD2DGraphicContext::MoveBrushBy(GReal inWidth, GReal inHeight)
{	
	fBrushPos.SetPosBy(inWidth,inHeight);
}


void VWinD2DGraphicContext::DrawIcon(const VIcon& inIcon, const VRect& inHwndBounds, PaintStatus /*inState*/, GReal /*inScale*/)
{
	StUseContext_NoRetain	context(this);

	if (fGDI_Fast && fGDI_HDC && fGlobalAlpha == 1.0f)
	{
		fGDI_GC->DrawIcon( inIcon, inHwndBounds);
		return;
	}

	FlushGDI();

	Gdiplus::Bitmap *bitmap = fCachedIcons[inIcon];
	if (bitmap == NULL)
	{
		bitmap = new Gdiplus::Bitmap(inIcon);
		fCachedIcons[inIcon] = bitmap;
		// **************************************
		// LP: Quick and dirty alpha icon : BEGIN
		// **************************************
		Gdiplus::BitmapData* bitmapData = new Gdiplus::BitmapData();
		VRect vr = inIcon.GetBounds();
		Gdiplus::Rect rect(vr.GetLeft(), vr.GetTop(), vr.GetWidth(), vr.GetHeight());
		bitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, bitmapData);

		UINT* pixels = (UINT*)bitmapData->Scan0;

		UINT p=0;
		UINT m = 0x00ffffff;
		UINT i = 0;
		UINT h = inIcon.GetHeight();
		UINT w = inIcon.GetWidth();
		UINT Z = bitmapData->Stride / 4;
		for(UINT row = 0; row < h; row++)
		{
			for(UINT col = 0; col < w; col++)
			{
				i = row * Z + col;
				p = pixels[i] & m;
				if (p == m)
					pixels[i] = p;
			}
		}
		bitmap->UnlockBits(bitmapData);
		delete bitmapData;
		// **************************************
		// LP: Quick and dirty alpha icon :   END
		// **************************************
	}
	ID2D1Bitmap *bitmapNative = NULL;
	{
		//test if D2D bitmap is in the cache belonging to the same resource domain as current render target
		D2D_GUI_RESOURCE_MUTEX_LOCK(fIsRTHardware)
		
		bitmapNative = (ID2D1Bitmap *)fCachedIconsD2D[fIsRTHardware][inIcon];
		if (!bitmapNative)
		{
			//create new D2D bitmap & cache it
			bitmapNative = (ID2D1Bitmap *)VPictureCodec_WIC::DecodeD2D(fRscRenderTarget[fIsRTHardware], bitmap);
			if (!bitmapNative)
			{
				//dispose all shared resources & try again
				_DisposeSharedResources(false);
				bitmapNative = (ID2D1Bitmap *)VPictureCodec_WIC::DecodeD2D(fRscRenderTarget[fIsRTHardware], bitmap);
			}
			if (bitmapNative)
			{
				fCachedIconsD2D[fIsRTHardware][inIcon] = bitmapNative;
				bitmapNative->Release();
			}
		}
		if (bitmapNative)
		{
			//bitmap is in the cache: create shared D2D bitmap 
			//(unlike layers we cannot re-use directly ID2D1Bitmap belonging to the same resource domain
			// but we need to create a shared ID2D1Bitmap - which will point the same texture in video or software memory)
			ID2D1Bitmap *bitmapNativeSrc = bitmapNative;
			bitmapNative = NULL;
			if (FAILED(fRenderTarget->CreateSharedBitmap( __uuidof(ID2D1Bitmap), (void *)bitmapNativeSrc, NULL, &bitmapNative)))
				return;
		}
	}
	if (bitmapNative)
	{
		fRenderTarget->DrawBitmap( bitmapNative, D2D_RECT(inHwndBounds), fGlobalAlpha, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
		bitmapNative->Release();
	}
}

void VWinD2DGraphicContext::DrawPicture (const VPicture& inPicture,const VRect& inHwndBounds,VPictureDrawSettings *inSet)
{
	StUseContext_NoRetain	context(this);

	if (fGDI_Fast && fGDI_HDC && fGlobalAlpha == 1.0f)
	{
		fGDI_GC->DrawPicture( inPicture, inHwndBounds, inSet);
		return;
	}

	FlushGDI();

	VPictureDrawSettings settings( inSet);
	if (fGlobalAlpha != 1.0f && (!fGlobalAlphaCount))
		settings.SetAlphaBlend( settings.GetAlpha()*fGlobalAlpha);
	fGlobalAlphaCount++;
	inPicture.Draw(static_cast<VGraphicContext *>(this), inHwndBounds, &settings);
	fGlobalAlphaCount--;
}



//draw picture data
void VWinD2DGraphicContext::DrawPictureData (const VPictureData *inPictureData, const VRect& inHwndBounds,VPictureDrawSettings *inSet)
{
	xbox_assert(inPictureData);
	
	StUseContext_NoRetain	context(this);

	if (fGDI_Fast && fGDI_HDC && fGlobalAlpha == 1.0f)
	{
		fGDI_GC->DrawPictureData( inPictureData, inHwndBounds, inSet);
		return;
	}

	FlushGDI();

	VPictureDrawSettings settings( inSet);
	if (fGlobalAlpha != 1.0f && (!fGlobalAlphaCount))
		settings.SetAlphaBlend( settings.GetAlpha()*fGlobalAlpha);
	
	//save current tranformation matrix
	VAffineTransform ctm;
	GetTransform(ctm);
	
	//better to call this method in order to avoid to create new VWinD2DGraphicContext instance
	//(so we can share same render target)
	fGlobalAlphaCount++;
	inPictureData->DrawInD2DGraphicContext( this, inHwndBounds, &settings);
	fGlobalAlphaCount--;

	//restore current transformation matrix
	SetTransform(ctm);
}


void VWinD2DGraphicContext::DrawFileBitmap(const VFileBitmap *inFileBitmap, const VRect& inHwndBounds)
{
	StUseContext_NoRetain	context(this);

	FlushGDI();

	//create D2D bitmap from gdiplus bitmap
	ID2D1Bitmap* bmp = VPictureCodec_WIC::DecodeD2D( fRenderTarget, inFileBitmap->fBitmap);
	xbox_assert(bmp);
	if (!bmp)
		return;

	//draw D2D bitmap
	D2D1_RECT_F rect = D2D_RECT(inHwndBounds);
	fRenderTarget->DrawBitmap( bmp, &rect, fGlobalAlpha);

	//release D2D bitmap
	bmp->Release();
}


/** create or retain D2D cached bitmap mapped with the Gdiplus bitmap 
@remarks
	this will create or retain a shared D2D bitmap using the frame 0 of the Gdiplus bitmap
*/
ID2D1Bitmap *VWinD2DGraphicContext::CreateOrRetainBitmap( Gdiplus::Bitmap *inBmp, const VColor *inTransparentColor)
{
	FlushGDI();
	if (fRenderTarget == NULL)
		return NULL;

	ID2D1Bitmap *bitmapNative = NULL;
	{
		//test if D2D bitmap is in the cache belonging to the same resource domain as current render target
		VTaskLock lock(&(fMutexResources[fIsRTHardware]));
		
		MapOfBitmap& mapBmp = inTransparentColor ? fCachedBmpTransparentNoAlpha[fIsRTHardware] : fCachedBmp[fIsRTHardware];

		bitmapNative = (ID2D1Bitmap *)mapBmp[inBmp];
		if (!bitmapNative)
		{
			Gdiplus::Bitmap *bmpSource = inBmp;
			if (inTransparentColor)
			{
				//convert not transparent to transparent Gdiplus bitmap
				bmpSource = new Gdiplus::Bitmap(inBmp->GetWidth(), inBmp->GetHeight(), PixelFormat32bppPARGB); //using PARGB here prevents VPictureCodec_WIC::DecodeD2D
																																//to convert to D2D compatible pixel format
				if (bmpSource)
				{
					Gdiplus::Graphics gc(static_cast<Gdiplus::Image *>(bmpSource));
					gc.SetPageUnit(Gdiplus::UnitPixel );
					gc.SetPixelOffsetMode( Gdiplus::PixelOffsetModeHalf);
					gc.SetInterpolationMode( Gdiplus::InterpolationModeNearestNeighbor);

					Gdiplus::RectF dstRect(0.0f,0.0f,inBmp->GetWidth(),inBmp->GetHeight());
					Gdiplus::Color color( (Gdiplus::ARGB)inTransparentColor->GetRGBAColor());
					Gdiplus::ImageAttributes attr;
					attr.SetColorKey(color, color, Gdiplus::ColorAdjustTypeDefault);
					gc.DrawImage(inBmp, dstRect, 0.0f, 0.0f, inBmp->GetWidth(), inBmp->GetHeight(), Gdiplus::UnitPixel , &attr);
					gc.Flush();
				}
				else
					bmpSource = inBmp;
			}


			//create new D2D bitmap & cache it
			bitmapNative = (ID2D1Bitmap *)VPictureCodec_WIC::DecodeD2D(fRscRenderTarget[fIsRTHardware], bmpSource);
			if (!bitmapNative)
			{
				//dispose all shared resources & try again
				_DisposeSharedResources(false);
				bitmapNative = (ID2D1Bitmap *)VPictureCodec_WIC::DecodeD2D(fRscRenderTarget[fIsRTHardware], bmpSource);
			}
			if (bmpSource != inBmp)
				delete bmpSource;
			if (bitmapNative)
			{
				mapBmp[inBmp] = bitmapNative;
#if VERSIONDEBUG
#if D2D_LOG_ENABLE
				gD2DCachedBitmapCount[fIsRTHardware]++;
				//if (fIsRTHardware)
				//	xbox::DebugMsg( "VWinD2DGraphicContext: hardware cached D2D bitmaps = %d\n", gD2DCachedBitmapCount[fIsRTHardware]);
				//else
				//	xbox::DebugMsg( "VWinD2DGraphicContext: software cached D2D bitmaps = %d\n", gD2DCachedBitmapCount[fIsRTHardware]);
#endif
#endif
				bitmapNative->Release();
			}
		}
		if (bitmapNative)
		{
			//bitmap is in the cache: create shared D2D bitmap 
			//(unlike layers we cannot re-use directly ID2D1Bitmap belonging to the same resource domain
			// but we need to create a shared ID2D1Bitmap - which will point the same texture in video or software memory)
			ID2D1Bitmap *bitmapNativeSrc = bitmapNative;
			bitmapNative = NULL;
			if (FAILED(fRenderTarget->CreateSharedBitmap( __uuidof(ID2D1Bitmap), (void *)bitmapNativeSrc, NULL, &bitmapNative)))
				return NULL;
		}
	}
	return bitmapNative;
}

/** release cached D2D bitmap associated with the specified Gdiplus bitmap */
void VWinD2DGraphicContext::ReleaseBitmap( const Gdiplus::Bitmap *inBmp)
{
	if (inBmp == NULL)
		return;

	{
		VTaskLock lock(&(fMutexResources[D2D_CACHE_RESOURCE_HARDWARE]));

		MapOfBitmap::iterator it = fCachedBmp[D2D_CACHE_RESOURCE_HARDWARE].find(inBmp);
		if (it != fCachedBmp[D2D_CACHE_RESOURCE_HARDWARE].end())
		{
			fCachedBmp[D2D_CACHE_RESOURCE_HARDWARE].erase(it);
#if VERSIONDEBUG
#if D2D_LOG_ENABLE
			if (!testAssert(gD2DCachedBitmapCount[D2D_CACHE_RESOURCE_HARDWARE] > 0))
				xbox::DebugMsg( "VWinD2DGraphicContext::ReleaseBitmap: cached bitmap count is not consistent\n");
			else
			{
				gD2DCachedBitmapCount[D2D_CACHE_RESOURCE_HARDWARE]--;
			//	xbox::DebugMsg( "VWinD2DGraphicContext: hardware cached D2D bitmaps = %d\n", gD2DCachedBitmapCount[D2D_CACHE_RESOURCE_HARDWARE]);
			}
		}
		else
		{
			//xbox::DebugMsg( "ReleaseBitmap: tried to release bitmap which is not cached yet\n");
		}
#else
		}
#endif
#else
		}
#endif

		it = fCachedBmpTransparentNoAlpha[D2D_CACHE_RESOURCE_HARDWARE].find(inBmp);
		if (it != fCachedBmpTransparentNoAlpha[D2D_CACHE_RESOURCE_HARDWARE].end())
		{
			fCachedBmpTransparentNoAlpha[D2D_CACHE_RESOURCE_HARDWARE].erase(it);
#if VERSIONDEBUG
#if D2D_LOG_ENABLE
			if (!testAssert(gD2DCachedBitmapCount[D2D_CACHE_RESOURCE_HARDWARE] > 0))
				xbox::DebugMsg( "VWinD2DGraphicContext::ReleaseBitmap: cached bitmap count is not consistent\n");
			else
			{
				gD2DCachedBitmapCount[D2D_CACHE_RESOURCE_HARDWARE]--;
			//	xbox::DebugMsg( "VWinD2DGraphicContext: hardware cached D2D bitmaps = %d\n", gD2DCachedBitmapCount[D2D_CACHE_RESOURCE_HARDWARE]);
			}
		}
		else
		{
			//xbox::DebugMsg( "ReleaseBitmap: tried to release bitmap which is not cached yet\n");
		}
#else
		}
#endif
#else
		}
#endif
	}
	{
		VTaskLock lock(&(fMutexResources[D2D_CACHE_RESOURCE_SOFTWARE]));

		MapOfBitmap::iterator it = fCachedBmp[D2D_CACHE_RESOURCE_SOFTWARE].find(inBmp);
		if (it != fCachedBmp[D2D_CACHE_RESOURCE_SOFTWARE].end())
		{
			fCachedBmp[D2D_CACHE_RESOURCE_SOFTWARE].erase(it);
#if VERSIONDEBUG
#if D2D_LOG_ENABLE
			if (!testAssert(gD2DCachedBitmapCount[D2D_CACHE_RESOURCE_SOFTWARE] > 0))
				xbox::DebugMsg( "VWinD2DGraphicContext::ReleaseBitmap: cached bitmap count is not consistent\n");
			else
			{
				gD2DCachedBitmapCount[D2D_CACHE_RESOURCE_SOFTWARE]--;
			//	xbox::DebugMsg( "VWinD2DGraphicContext: software cached D2D bitmaps = %d\n", gD2DCachedBitmapCount[D2D_CACHE_RESOURCE_SOFTWARE]);
			}
		}
		else
		{
			//xbox::DebugMsg( "ReleaseBitmap: tried to release bitmap which is not cached yet\n");
		}
#else
		}
#endif
#else
		}
#endif

		it = fCachedBmpTransparentNoAlpha[D2D_CACHE_RESOURCE_SOFTWARE].find(inBmp);
		if (it != fCachedBmpTransparentNoAlpha[D2D_CACHE_RESOURCE_SOFTWARE].end())
		{
			fCachedBmpTransparentNoAlpha[D2D_CACHE_RESOURCE_SOFTWARE].erase(it);
#if VERSIONDEBUG
#if D2D_LOG_ENABLE
			if (!testAssert(gD2DCachedBitmapCount[D2D_CACHE_RESOURCE_SOFTWARE] > 0))
				xbox::DebugMsg( "VWinD2DGraphicContext::ReleaseBitmap: cached bitmap count is not consistent\n");
			else
			{
				gD2DCachedBitmapCount[D2D_CACHE_RESOURCE_SOFTWARE]--;
			//	xbox::DebugMsg( "VWinD2DGraphicContext: software cached D2D bitmaps = %d\n", gD2DCachedBitmapCount[D2D_CACHE_RESOURCE_SOFTWARE]);
			}
		}
		else
		{
			//xbox::DebugMsg( "ReleaseBitmap: tried to release bitmap which is not cached yet\n");
		}
#else
		}
#endif
#else
		}
#endif
	}
}


void VWinD2DGraphicContext::DrawPictureFile(const VFile& inFile, const VRect& inHwndBounds)
{
	//TODO ?
	xbox_assert(false);
}


#pragma mark-



/** axis-aligned rect clipping or layer */
VLayerClipElem::VLayerClipElem( const VAffineTransform& inCTM, const VRect& inBounds, const VRegion* inRegion, const VRect& inPrevClipBounds, const VRegion *inPrevClipRegion,
								ID2D1Layer* inLayer, ID2D1Geometry* inGeom, const GReal inOpacity, 
								bool inHasClipOverride)
{
	fCTM = inCTM;

	xbox_assert(!inRegion || inCTM.IsIdentity()); //ensure we pass region in transformed space
	fRegion = inRegion ? new VRegion( inRegion) : NULL; 
	fBounds = inRegion ? inBounds : inCTM.TransformRect(inBounds);
	fBounds.NormalizeToInt();

	fPrevClipRegion = inPrevClipRegion ? new VRegion( inPrevClipRegion) : NULL;
	fPrevClipBounds = inPrevClipBounds;

	fLayer = inLayer;
	fGeom = inGeom;
	fOpacity = inOpacity;
	fHasClipOverrideBackup = inHasClipOverride;

	fImageOffScreen = NULL;
	fOwnOffScreen = true;
}

/** offscreen layer */
VLayerClipElem::VLayerClipElem( const VAffineTransform& inCTM, const VRect& inBounds, const VRect& inPrevClipBounds, const VRegion *inPrevClipRegion,
								VImageOffScreen* inImageOffScreen, const GReal inOpacity, 
								bool inHasClipOverride, bool inOwnOffScreen)
{
	fCTM = inCTM;
	fBounds = inCTM.TransformRect(inBounds);
	fRegion = NULL;

	fPrevClipBounds = inPrevClipBounds;
	fPrevClipRegion = inPrevClipRegion ? new VRegion( inPrevClipRegion) : NULL;

	fOpacity = inOpacity;
	fHasClipOverrideBackup = inHasClipOverride;

	fImageOffScreen = NULL;
	CopyRefCountable(&fImageOffScreen, inImageOffScreen);
	fOwnOffScreen = inOwnOffScreen;
}



/** dummy constructor (used by SaveClip()) */
VLayerClipElem::VLayerClipElem( const VRect& inBounds, const VRect& inPrevClipBounds, const VRegion *inPrevClipRegion, bool inHasClipOverride)
{
	fBounds = inBounds;
	fRegion = NULL;

	fPrevClipBounds = inPrevClipBounds;
	fPrevClipRegion = inPrevClipRegion ? new VRegion( inPrevClipRegion) : NULL;

	fOpacity = D2D_LAYERCLIP_DUMMY;
	fHasClipOverrideBackup = inHasClipOverride;

	fImageOffScreen = NULL;
	fOwnOffScreen = true;
}


VLayerClipElem::~VLayerClipElem()
{
	if (fRegion)
		delete fRegion;
	if (fPrevClipRegion)
		delete fPrevClipRegion;
	fLayer = (ID2D1Layer *)NULL;
	ReleaseRefCountable(&fImageOffScreen);
	fGeom = (ID2D1Geometry *)NULL;
}

/** copy constructor */
VLayerClipElem::VLayerClipElem( const VLayerClipElem& inClip)
{
	fCTM = inClip.fCTM;
	fLayer = inClip.fLayer;
	fImageOffScreen = NULL;
	CopyRefCountable(&fImageOffScreen, inClip.fImageOffScreen);
	fOwnOffScreen = inClip.fOwnOffScreen;
	fGeom = inClip.fGeom;
	fOpacity = inClip.fOpacity;

	fBounds = inClip.GetBounds();
	fRegion = inClip.fRegion ? new VRegion( inClip.fRegion) : NULL;

	fPrevClipBounds = inClip.fPrevClipBounds;
	fPrevClipRegion = inClip.fPrevClipRegion ? new VRegion( inClip.fPrevClipRegion) : NULL;

	fHasClipOverrideBackup = inClip.fHasClipOverrideBackup;
}

VLayerClipElem& VLayerClipElem::operator = (const VLayerClipElem& inClip)
{
	fCTM = inClip.fCTM;
	fLayer = inClip.fLayer;
	CopyRefCountable(&fImageOffScreen, inClip.fImageOffScreen);
	fOwnOffScreen = inClip.fOwnOffScreen;
	fGeom = inClip.fGeom;
	fOpacity = inClip.fOpacity;

	if (fRegion)
		delete fRegion;
	if (fPrevClipRegion)
		delete fPrevClipRegion;

	fBounds = inClip.GetBounds();
	fRegion = inClip.fRegion ? new VRegion( inClip.fRegion) : NULL;

	fPrevClipBounds = inClip.fPrevClipBounds;
	fPrevClipRegion = inClip.fPrevClipRegion ? new VRegion( inClip.fPrevClipRegion) : NULL;

	fHasClipOverrideBackup = inClip.fHasClipOverrideBackup;
	return *this;
}

Boolean	VLayerClipElem::operator == (const VLayerClipElem& inClip) const
{
	return (fCTM == inClip.fCTM
			&&
			fLayer == inClip.fLayer 
			&&
			fImageOffScreen == inClip.fImageOffScreen 
			&&
			fOwnOffScreen == inClip.fOwnOffScreen
			&&
			fBounds == inClip.fBounds 
			&&
			fPrevClipBounds == inClip.fPrevClipBounds
			&&
			fGeom == inClip.fGeom
			&&
			fOpacity == inClip.fOpacity
			&&
			fHasClipOverrideBackup == inClip.fHasClipOverrideBackup
			);
}

void VWinD2DGraphicContext::_RestoreClipOverride() const
{
	//pop clipping set since last SaveClip or _BeginLayer (with Clip... methods)

	while (fHasClipOverride)
	{
		if (fStackLayerClip.empty())
		{
			fHasClipOverride = false;
			return;
		}

		if ((!fStackLayerClip.back().IsDummy()) && fRenderTarget != NULL && fUseCount > 0)
		{
			xbox_assert(!fStackLayerClip.back().IsImageOffScreen());
				
			if (fStackLayerClip.back().IsLayer())
			{
				fRenderTarget->PopLayer();
				if (fLockCount <= 0) 
					_FreeLayer( fStackLayerClip.back().RetainLayer());
			}
			else
				fRenderTarget->PopAxisAlignedClip();
		}

		if (fLockCount > 0)
			//backup layer & clipping
			fLockSaveStackLayerClip.push_back( fStackLayerClip.back());

		fHasClipOverride = fStackLayerClip.back().HasClipOverride();
		fClipBoundsCur = fStackLayerClip.back().GetPrevClipBounds();
		if (fClipRegionCur)
			delete fClipRegionCur;
		fClipRegionCur = fStackLayerClip.back().GetOwnedPrevClipRegion();

		fStackLayerClip.pop_back(); //this will release ID2D1Layer ref if any
	}
}

/** push initial clipping */
void VWinD2DGraphicContext::PushClipInit() const
{
	FlushGDI();

	StUseContext_NoRetain	context(this);
	if (fUseCount <= 0)
		return;

	//get current clipping (pre-transformed) bounds
	xbox_assert(fStackLayerClip.empty());

	if (fClipRegionCur)
	{
		//push new clipping layer
		VGraphicPath *path = fClipRegionCur->GetComplexPath();

		ID2D1Layer *layer = NULL;
		layer = _AllocateLayer();
		if (!layer)
			return;

		VAffineTransform ctm;
		_GetTransform( ctm);
		fRenderTarget->SetTransform( D2D1::IdentityMatrix()); 

		fRenderTarget->PushLayer(
			D2D1::LayerParameters( 
					D2D_RECT(fClipBoundsCur),
					path ? path->GetImplPathD2D() : NULL,
					D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
					D2D1::IdentityMatrix(),
					1.0f,
					NULL,
					IsTransparent() ? D2D1_LAYER_OPTIONS_NONE : D2D1_LAYER_OPTIONS_INITIALIZE_FOR_CLEARTYPE
					),
			layer
			);

		_SetTransform( ctm);

		//push clipping element 
		fStackLayerClip.push_back( VLayerClipElem( VAffineTransform(), fClipBoundsCur, fClipRegionCur, fClipBoundsCur, NULL, layer, path->GetImplPathD2D(), 1.0f, false));
		delete path;

		layer->Release();
	}
	else
	{
		//push axis aligned clipping 
		VAffineTransform ctm;
		_GetTransform( ctm);
		fRenderTarget->SetTransform( D2D1::IdentityMatrix()); 
		fRenderTarget->PushAxisAlignedClip( D2D_RECT(fClipBoundsCur), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		_SetTransform( ctm);

		//push on internal stack the transformed clipping
		fStackLayerClip.push_back( VLayerClipElem( VAffineTransform(), fClipBoundsCur, NULL, fClipBoundsCur, NULL, NULL, NULL, 1.0f, false));
	}

	//reset clip override status
	fHasClipOverride = false;
}

void VWinD2DGraphicContext::_SaveClip() const
{
	FlushGDI();

	//StUseContext_NoRetain	context(this);
	if (fUseCount <= 0)
		return;

	//push dummy VLayerClipElem object (in order to save clipping override status)
	fStackLayerClip.push_back( VLayerClipElem( fClipBoundsCur, fClipBoundsCur, fClipRegionCur, fHasClipOverride));

	//reset clip override status
	fHasClipOverride = false;
}


VImageOffScreenRef VWinD2DGraphicContext::_RestoreClip() const
{
	FlushGDI();

	//StUseContext_NoRetain	context(this);
	if (fUseCount <= 0)
		return NULL;

	if (fStackLayerClip.empty())
		return NULL;

	_RestoreClipOverride();
	if (fStackLayerClip.empty())
		return NULL;

	VImageOffScreenRef offscreen = NULL;

	if ((!fStackLayerClip.back().IsDummy()) && fRenderTarget != NULL && fUseCount > 0)
	{
		if (fStackLayerClip.back().IsImageOffScreen())
			offscreen = _EndLayerOffScreen();
		else if (fStackLayerClip.back().IsLayer())
		{
			fRenderTarget->PopLayer();
			if (fLockCount <= 0) 
				_FreeLayer( fStackLayerClip.back().RetainLayer());
		}
		else
			fRenderTarget->PopAxisAlignedClip();
	}

	fHasClipOverride = fStackLayerClip.back().HasClipOverride();
	fClipBoundsCur = fStackLayerClip.back().GetPrevClipBounds();
	if (fClipRegionCur)
		delete fClipRegionCur;
	fClipRegionCur = fStackLayerClip.back().GetOwnedPrevClipRegion();

	if (fLockCount > 0)
	{
		//backup layer & clipping
		if (fStackLayerClip.back().IsImageOffScreen())
		{
			//do not backup offscreen: replace with dummy clip element
			if (offscreen)
			{
				offscreen->Release();
				offscreen = NULL;
			}

			//update fLockSaveTransform: take account offscreen translation
			//(otherwise restored transform by _Unlock() would be wrong) 
			VPoint transLayerToParent = fStackLayerClip.back().GetBounds().GetTopLeft();
			fLockSaveTransform.SetTranslation( 
						fLockSaveTransform.GetTranslateX()+transLayerToParent.GetX(), 
						fLockSaveTransform.GetTranslateY()+transLayerToParent.GetY());
			fLockSaveClipBounds.SetPosBy( transLayerToParent.GetX(), transLayerToParent.GetY());
			if (fLockSaveClipRegion)
				fLockSaveClipRegion->SetPosBy( transLayerToParent.GetX(), transLayerToParent.GetY());

			fLockSaveStackLayerClip.push_back( VLayerClipElem(fStackLayerClip.back().GetBounds(), fClipBoundsCur, fClipRegionCur, fHasClipOverride));
		}
		else
			fLockSaveStackLayerClip.push_back( fStackLayerClip.back());
	}
	fStackLayerClip.pop_back(); //this will release ID2D1Layer ref if any
	return offscreen;
}

//add or intersect the specified clipping path with the current clipping path
void VWinD2DGraphicContext::_ClipPath (const VGraphicPath& inPath, const VRegion *inRegion, Boolean inAddToPrevious, Boolean inIntersectToPrevious) const
{
#if VERSIONDEBUG
	if (sDebugNoClipping)
		return;
#endif
	if (!inPath.GetImplPathD2D())
		return;
	//StUseContext_NoRetain	context(this);
	if (fUseCount <= 0 || fGDI_HDC)
		return;

	FlushGDI();

	//NDJQ: in order to be compliant with Quartz2D impl, only intersect with current path

	//push new clipping layer
	ID2D1Layer *layer = NULL;
	layer = _AllocateLayer();
	if (!layer)
		return;
#if VERSIONDEBUG
	VAffineTransform ctmParent;
	_GetTransform(ctmParent);
#endif
	fRenderTarget->PushLayer(
		D2D1::LayerParameters( 
				D2D1::InfiniteRect(), 
				inPath.GetImplPathD2D(),
				D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
				D2D1::IdentityMatrix(),
				1.0f,
				NULL,
				IsTransparent() ? D2D1_LAYER_OPTIONS_NONE : D2D1_LAYER_OPTIONS_INITIALIZE_FOR_CLEARTYPE
				),
        layer
        );
#if VERSIONDEBUG
	VAffineTransform ctmLayer;
	_GetTransform(ctmLayer);
	//ensure layer does not modify current transform
	xbox_assert(ctmLayer == ctmParent);
#endif

	//push clipping element 
	VAffineTransform ctm;
	_GetTransform( ctm);
	VRect boundsClip = ctm.TransformRect(inPath.GetBounds());
	fStackLayerClip.push_back( VLayerClipElem( VAffineTransform(), boundsClip, inRegion, fClipBoundsCur, fClipRegionCur, layer, inPath.GetImplPathD2D(), 1.0f, fHasClipOverride));
	fClipBoundsCur.Intersect( fStackLayerClip.back().GetBounds());
	if (fClipRegionCur || inRegion)
	{
		if (fClipRegionCur)
		{
			if (inRegion)
				fClipRegionCur->Intersect( *inRegion);
			else
			{
				VRect bounds = fStackLayerClip.back().GetBounds();
				bounds.NormalizeToInt();
				VRegion region( bounds);
				fClipRegionCur->Intersect( region);
			}
		}
		else
		{
			VRect bounds = fClipBoundsCur;
			bounds.NormalizeToInt();
			fClipRegionCur = new VRegion( bounds);
			fClipRegionCur->Intersect( *inRegion);
		}
	}
	fHasClipOverride = true;

	layer->Release();

	_RevealCurClipping();
}


/** add or intersect the specified clipping path outline with the current clipping path 
@remarks
	this will effectively create a clipping path from the path outline 
	(using current stroke brush) and merge it with current clipping path,
	making possible to constraint drawing to the path outline
*/
void VWinD2DGraphicContext::ClipPathOutline (const VGraphicPath& inPath, Boolean inAddToPrevious, Boolean inIntersectToPrevious)
{
#if VERSIONDEBUG
	if (sDebugNoClipping)
		return;
#endif
	VGraphicPath *pathOutline = inPath.Clone();
	pathOutline->Widen( fStrokeWidth, fStrokeStyle);
	pathOutline->Outline();

	ClipPath( *pathOutline);
}


void VWinD2DGraphicContext::_ClipRect(const VRect& inHwndBounds, Boolean inAddToPrevious, Boolean inIntersectToPrevious) const
{
#if VERSIONDEBUG
	if (sDebugNoClipping)
		return;
#endif

	VAffineTransform ctm;
	_GetTransform(ctm);

	if (fGDI_HDC && ctm.GetShearX() == 0.0f && ctm.GetShearY() == 0.0f)
	{
		//for GDI, clipping must be specified in GDI viewport space = rt transformed space
		VRect boundsTransformed(inHwndBounds);
		if (ctm.IsIdentity())
			boundsTransformed.FromRect(inHwndBounds);
		else
			boundsTransformed = ctm.TransformRect(inHwndBounds);
		boundsTransformed.NormalizeToInt();

		//as ClipRect adjusts clip box with viewport offset, we need to set clip region in transformed space
		{
		StGDIResetTransform resetCTM(fGDI_GC->GetParentPort());
		fGDI_GC->ClipRect( boundsTransformed, inAddToPrevious, inIntersectToPrevious);
		}

		//if (!fGDI_Restore.empty())
		{
			fGDI_Restore.push_back( ePush_Clipping);
			fGDI_ClipRect.push_back( boundsTransformed);
			fGDI_PushedClipCounter++;
		}
		return;
	}
	
	if (fUseCount <= 0)
		return;

	FlushGDI();

	//StUseContext_NoRetain	context(this);

	if (ctm.GetShearX() == 0.0f && ctm.GetShearY() == 0.0f)
	{
		//we can use axis-aligned clipping because there is no transform rotation

		VRect boundsTransformed = ctm.TransformRect(inHwndBounds);
		boundsTransformed.NormalizeToInt();
		boundsTransformed.Intersect(fClipBoundsCur);
		if (boundsTransformed == fClipBoundsCur)
			//do nothing
			return;
		else
		{
			//push axis aligned clipping 
			fRenderTarget->SetTransform( D2D1::IdentityMatrix());
			fRenderTarget->PushAxisAlignedClip( D2D_RECT(boundsTransformed), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
			_SetTransform( ctm);
				
			//update internal stack
			fStackLayerClip.push_back( VLayerClipElem( VAffineTransform(), boundsTransformed, NULL, fClipBoundsCur, fClipRegionCur, NULL, NULL, 1.0f, fHasClipOverride));
			fClipBoundsCur.Intersect( fStackLayerClip.back().GetBounds());
			if (fClipRegionCur)
			{
				VRect bounds = fStackLayerClip.back().GetBounds();
				VRegion region( bounds);
				fClipRegionCur->Intersect( region);
			}
		}
		//set clip override status
		fHasClipOverride = true;

		_RevealCurClipping();
	}
	else
	{
		//we MUST use path clipping because there is a transform rotation

		VGraphicPath *path = CreatePath();
		path->Begin();
		path->AddRect( inHwndBounds);
		path->End();
		_ClipPath2( *path, inAddToPrevious, inIntersectToPrevious);
		delete path;
	}
}


void VWinD2DGraphicContext::ClipRegion(const VRegion& inHwndRegion, Boolean inAddToPrevious, Boolean inIntersectToPrevious)
{
	if (fGDI_HDC)
	{
		//for GDI, clipping must be specified in GDI viewport space = rt transformed space
		VRegion region(inHwndRegion);

		VPoint translation;

		VAffineTransform ctm;
		_GetTransform(ctm);
		if (ctm.GetScaleX() == 1.0f && ctm.GetScaleY() == 1.0f
			&&
			ctm.GetShearX() == 0.0f && ctm.GetShearY() == 0.0f)
		{
			if (ctm.GetTranslateX() != 0.0f || ctm.GetTranslateY() != 0.0f)
				translation.SetPosBy( ctm.GetTranslateX(), ctm.GetTranslateY());
		}
		else
		{
			//use Gdiplus region in order to properly convert to device space from vectorial local user space
			Gdiplus::Region regionGDIPlus = inHwndRegion;
			Gdiplus::Matrix ctmNative;
			ctm.ToNativeMatrix( ctmNative);
			regionGDIPlus.Transform( &ctmNative);
			Gdiplus::Graphics gc(fGDI_HDC);
			HRGN hrgn = regionGDIPlus.GetHRGN(&gc);
			region.FromRegionRef( hrgn, true);
		}

		if (translation.GetX() || translation.GetY())
			region.SetPosBy( translation.GetX(), translation.GetY());

		//as ClipRegion adjusts region with viewport offset, we need to set clip region in transformed space
		{
		StGDIResetTransform resetCTM(fGDI_GC->GetParentPort());
		fGDI_GC->ClipRegion( region, inAddToPrevious, inIntersectToPrevious);
		}

		//if (!fGDI_Restore.empty())
		{
			VRect boundsTransformed(inHwndRegion.GetBounds());
			if (!ctm.IsIdentity())
			{
				boundsTransformed = ctm.TransformRect(boundsTransformed);
				boundsTransformed.NormalizeToInt();
			}
			fGDI_Restore.push_back( ePush_Clipping);
			fGDI_ClipRect.push_back( boundsTransformed);
			fGDI_PushedClipCounter++;
		}
		return;
	}

	if (fUseCount <= 0)
		return;

	VGraphicPath *pathComplex = inHwndRegion.GetComplexPath();
	if (pathComplex)
	{
		_ClipPath( *pathComplex, &inHwndRegion, inAddToPrevious, inIntersectToPrevious);
		delete pathComplex;

		//SetFillColor( VColor(0xff,0,0));
		//FillRect(inHwndRegion.GetBounds());
   }
   else
		ClipRect( inHwndRegion.GetBounds(), inAddToPrevious, inIntersectToPrevious);
}


void VWinD2DGraphicContext::GetClip(HDC inHDC, VRegion& outHwndRgn, bool inApplyOffsetViewport) 
{
	HRGN	clip = ::CreateRectRgn(0, 0, 0, 0);
	sLONG result = ::GetClipRgn(inHDC, clip);
	
	if (result == 0 || result == -1)
	{
		outHwndRgn.FromRect( VRECT_RGN_INFINITE);
		::DeleteObject(clip);
	}
	else
	{
		outHwndRgn.FromRegionRef(clip, true);

		if (!inApplyOffsetViewport)
			return;

		// In case of offscreen drawing, port origin may have been moved
		// As SelectClip isn't aware of port origin, we offset the region manually
		POINT	offset;
		::GetViewportOrgEx(inHDC, &offset);
		outHwndRgn.SetPosBy(-offset.x, -offset.y);
	}
}

void VWinD2DGraphicContext::GetClip (HWND inHWND, VRegion& outHwndRgn, bool inApplyOffsetViewport) 
{
	HDC	hdc = ::GetDC(inHWND);
	GetClip( hdc, outHwndRgn, inApplyOffsetViewport);
	::ReleaseDC(inHWND, hdc);
}

void VWinD2DGraphicContext::GetClipBoundingBox(HDC inHDC, VRect& outBounds, bool inApplyOffsetViewport) 
{
	RECT rect;
	sLONG result = ::GetClipBox(inHDC, &rect);
	
	if (result == ERROR || result == -1)
		outBounds.FromRect( VRECT_RGN_INFINITE);
	else
	{
		outBounds.FromRectRef( rect);
		if (!inApplyOffsetViewport)
			return;

		// In case of offscreen drawing, port origin may have been moved
		// As SelectClip isn't aware of port origin, we offset the bounds manually
		POINT	offset;
		::GetViewportOrgEx(inHDC, &offset);
		outBounds.SetPosBy(-offset.x, -offset.y);
	}
}

void VWinD2DGraphicContext::GetClipBoundingBox(HWND inHWND, VRect& outBounds, bool inApplyOffsetViewport) 
{
	HDC	hdc = ::GetDC(inHWND);
	GetClipBoundingBox( hdc, outBounds, inApplyOffsetViewport);
	::ReleaseDC(inHWND, hdc);
}


void VWinD2DGraphicContext::GetClip(VRegion& outHwndRgn) const
{
	if (fGDI_HDC)
	{
		//as GetClip adjusts region with viewport offset, we need to get clip region in transformed space
		{
		StGDIResetTransform resetCTM(fGDI_GC->GetParentPort());
		fGDI_GC->GetClip( outHwndRgn);
		}

		//for GDI, GetClip returns clipping specified in GDI viewport space:
		//we need to transform it to local user space

		VPoint translation;

		VAffineTransform ctm;
		_GetTransform(ctm);
		if (ctm.GetScaleX() == 1.0f && ctm.GetScaleY() == 1.0f
			&&
			ctm.GetShearX() == 0.0f && ctm.GetShearY() == 0.0f)
		{
			if (ctm.GetTranslateX() != 0.0f || ctm.GetTranslateY() != 0.0f)
				translation.SetPosBy( -ctm.GetTranslateX(), -ctm.GetTranslateY());
		}
		else
		{
			//use Gdiplus region in order to properly convert to device space from vectorial local user space
			Gdiplus::Region regionGDIPlus = outHwndRgn;
			Gdiplus::Matrix ctmNative;
			ctm.Inverse().ToNativeMatrix( ctmNative);
			regionGDIPlus.Transform( &ctmNative);
			Gdiplus::Graphics gc(fGDI_HDC);
			HRGN hrgn = regionGDIPlus.GetHRGN(&gc);
			outHwndRgn.FromRegionRef( hrgn, true);
			return;
		}

		if (translation.GetX() || translation.GetY())
			outHwndRgn.SetPosBy( translation.GetX(), translation.GetY());
		return;
	}

	if (fUseCount > 0)
	{
		//inside BeginUsingContext/EndUsingContext:
		//get render target clipping 

		//get transformed clipping bounds
		VRect bounds = fClipBoundsCur;

		//convert to current coordinate space
		VAffineTransform ctm;
		_GetTransform(ctm);
		bounds = ctm.Inverse().TransformRect(bounds);
		bounds.NormalizeToInt(); //set to pixel bounds (because HRGN uses integer coordinates)
		outHwndRgn.FromRect( bounds);
	}
	//if outside BeginUsingContext/EndUsingContext,
	//return GDI clipping
	else if (fHDC)
		//get HDC clipping
		GetClip( fHDC, outHwndRgn);
	else if (fHwnd)
		//get HWND clipping
		GetClip( fHwnd, outHwndRgn);
	else
	{
		//bitmap context: return bitmap bounds
		VRect bounds = fHDCBounds;
		bounds.NormalizeToInt(); //set to pixel bounds (because HRGN uses integer coordinates)
		outHwndRgn.FromRect( bounds);
	}
}


void VWinD2DGraphicContext::RevealClipping(ContextRef inContext)
{
#if VERSIONDEBUG
	if (sDebugRevealClipping)
	{
		if (!inContext)
			return;

		HRGN	clip = ::CreateRectRgn(0, 0, 0, 0);
		
		RECT	bounds;
		::GetClipRgn(inContext, clip);
		::GetRgnBox(clip, &bounds);
		
		HBRUSH	brush = ::CreateSolidBrush(RGB(210, 210, 0));
		::FrameRgn(inContext, clip, brush, 4, 4);
		
		::DeleteObject(brush);
		::DeleteObject(clip);
		::Sleep(kDEBUG_REVEAL_DELAY);
	}
#endif
}
void VWinD2DGraphicContext::_RevealCurClipping() const
{
#if VERSIONDEBUG
	if (sDebugRevealClipping)
	{
		if (fUseCount <= 0) //reveal GDI clipping
		{
			HDC hdc = _GetHDC();
			RevealClipping( hdc);
			_ReleaseHDC( hdc);
			return;
		}

		//inside BeginUsingContext/EndUsingContext:
		//reveal render target clipping 

		if (fGDI_HDC)
		{
			RevealClipping( fGDI_HDC);
			return;
		}

		StUseContext_NoRetain	context(this);

		VRect bounds;
		_GetClipBoundingBox( bounds);

		CComPtr<ID2D1SolidColorBrush> brush;
		if (SUCCEEDED(fRenderTarget->CreateSolidColorBrush( D2D1::ColorF( 0, 0, 255, 32), &brush)))
			fRenderTarget->FillRectangle( D2D_RECT( bounds), brush);

		//useless here because render target is presented only on EndDraw
		//(and flush renders only current batch of primitives on render target, not on screen)
		//::Sleep(kDEBUG_REVEAL_DELAY);
	}
#endif
}

void VWinD2DGraphicContext::RevealUpdate(HWND inWindow)
{
	//Please call this BEFORE BeginPaint
#if VERSIONDEBUG
	if (sDebugRevealUpdate)
	{
		HDC	curDC = ::GetDC(inWindow);
		
		RECT	bounds;
		HRGN	clip =::CreateRectRgn(0, 0, 0, 0);
		::GetUpdateRgn(inWindow, clip, false);
		::GetRgnBox(clip, &bounds);
		
		HBRUSH	brush = ::CreateSolidBrush(RGB(210, 210, 0));
		HRGN	saveClip = ::CreateRectRgn(0, 0, 0, 0);
		int restoreClip = ::GetClipRgn(curDC, saveClip);
		::SelectClipRgn(curDC, clip);
		
		::FillRgn(curDC, clip, brush);
		if (restoreClip)
			::SelectClipRgn(curDC, saveClip);
		else
			::SelectClipRgn(curDC, NULL);
		::DeleteObject(brush);
		::DeleteObject(saveClip);
		::DeleteObject(clip);
		
		::ReleaseDC(inWindow, curDC);
		::GdiFlush();
		
		::Sleep(kDEBUG_REVEAL_DELAY);
	}
#endif
}


void VWinD2DGraphicContext::RevealBlitting(ContextRef inContext, const RgnRef inHwndRegion)
{
}


void VWinD2DGraphicContext::RevealInval(ContextRef inContext, const RgnRef inHwndRegion)
{
}


/** retain WIC bitmap  
@remarks
	return NULL if graphic context is not a bitmap context or if context is GDI

	bitmap is still shared by the context so you should not modify it (!)
*/
IWICBitmap *VWinD2DGraphicContext::RetainWICBitmap() const
{
	IWICBitmap *bmp = (IWICBitmap *)fWICBitmap;
	if (!bmp)
	{
		if (fGdiplusBmp)
		{
			if (fBackingStoreWICBmp)
			{
				((IWICBitmap *)fBackingStoreWICBmp)->AddRef();
				return fBackingStoreWICBmp;
			}
			
			VTaskLock lock(&(fMutexResources[D2D_CACHE_RESOURCE_SOFTWARE]));
			if (fGdiplusSharedBmpInUse && fGdiplusBmp == fGdiplusSharedBmp && (fHDCBounds.GetWidth() != fGdiplusSharedBmpWidth || fHDCBounds.GetHeight() != fGdiplusSharedBmpHeight))
				fBackingStoreWICBmp = VPictureCodec_WIC::DecodeWIC( fGdiplusBmp, &fHDCBounds);
			else
				fBackingStoreWICBmp = VPictureCodec_WIC::DecodeWIC( fGdiplusBmp);

			if (fBackingStoreWICBmp)
				((IWICBitmap *)fBackingStoreWICBmp)->AddRef();
			return fBackingStoreWICBmp;
		}
		else
			return NULL;
	}
	bmp->AddRef();
	return bmp;
}


/** get Gdiplus bitmap  
@remarks
	return NULL if graphic context is not a bitmap context or if context is GDI

	bitmap is still owned by the context so you should not destroy it (!)
*/
Gdiplus::Bitmap *VWinD2DGraphicContext::GetGdiplusBitmap() const
{
	VTaskLock lock(&(fMutexResources[D2D_CACHE_RESOURCE_SOFTWARE]));
	if (fGdiplusBmp)
	{
		if (fBackingStoreGdiplusBmp)
			return fBackingStoreGdiplusBmp;

		if (!fGdiplusSharedBmpInUse || fGdiplusBmp != fGdiplusSharedBmp || (fHDCBounds.GetWidth() == fGdiplusSharedBmpWidth && fHDCBounds.GetHeight() == fGdiplusSharedBmpHeight))
			return fGdiplusBmp;
		fBackingStoreGdiplusBmp = fGdiplusBmp->Clone( 0.0f, 0.0f, fHDCBounds.GetWidth(), fHDCBounds.GetHeight(), fGdiplusBmp->GetPixelFormat());
		return fBackingStoreGdiplusBmp;
	}
	else if (fWICBitmap)
	{
		if (fBackingStoreGdiplusBmp)
			return fBackingStoreGdiplusBmp;
	
		fBackingStoreGdiplusBmp = VPictureCodec_WIC::FromWIC( fWICBitmap);
		return fBackingStoreGdiplusBmp;
	}
	return NULL;
}

#pragma mark-

void VWinD2DGraphicContext::_CopyContentTo(VGraphicContext* inDestContext, const VRect* inSrcBounds, const VRect* inDestBounds, TransferMode inMode, const VRegion* inMask) const
{
	if (!inDestContext)
		return;

	xbox_assert(fDPI == 96.0f); //not allowed while printing

	VRect srcBoundsTransformed;
	if (inSrcBounds)
	{
		VAffineTransform ctm;
		_GetTransform(ctm);
		if (!ctm.IsIdentity())
			srcBoundsTransformed = ctm.TransformRect( *inSrcBounds);
		else
			srcBoundsTransformed = *inSrcBounds;
	}
	if (!inSrcBounds && fGdiplusBmp)
	{
		VTaskLock lock(&(fMutexResources[D2D_CACHE_RESOURCE_SOFTWARE]));
		if (fGdiplusSharedBmpInUse && fGdiplusBmp == fGdiplusSharedBmp && (fHDCBounds.GetWidth() != fGdiplusSharedBmpWidth || fHDCBounds.GetHeight() != fGdiplusSharedBmpHeight))
		{
			inSrcBounds = &fHDCBounds;
			srcBoundsTransformed = fHDCBounds;
		}
	}

	StParentContextNoDraw hdcNoDraw(this);

	if (inDestContext->IsD2DImpl() /*&& inDestContext->IsHardware() == IsHardware()*/)
	{
		//optimization if dest & src are D2D & same resource domain
		VWinD2DGraphicContext *gcD2D = static_cast<VWinD2DGraphicContext *>(inDestContext);
		gcD2D->BeginUsingContext();
		gcD2D->FlushGDI();

		if (gcD2D->fRenderTarget)
		{
			//create D2D bitmap from source
			ID2D1Bitmap *bmp = _CreateBitmap( gcD2D->fRenderTarget, inSrcBounds ? &srcBoundsTransformed : NULL);
			if (bmp)
			{
				if (inMask)
				{
					//clip with mask 
					gcD2D->SaveClip();
					gcD2D->ClipRegion( *inMask);
				}


				if (inDestBounds)
				{
					D2D1_RECT_F destBounds = D2D1::RectF( inDestBounds->GetLeft(), inDestBounds->GetTop(), inDestBounds->GetLeft()+inDestBounds->GetWidth(), inDestBounds->GetTop()+inDestBounds->GetHeight()); 
					gcD2D->fRenderTarget->DrawBitmap( bmp, destBounds,1,D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
				}
				else
					gcD2D->fRenderTarget->DrawBitmap( bmp);

				if (inMask)
					gcD2D->RestoreClip();

				bmp->Release();
			}
		}
		gcD2D->EndUsingContext();
	}
	else if (fGdiplusBmp)
	{
		FlushGDI(); 
		VIndex oldUseCount = fUseCount;
		bool oldLockType = fLockParentContext;
		if (fUseCount > 0)
		{
			//we need to finish current drawing session if we need to access to the bitmap
			fLockParentContext = true; //ensure render target is presented now
			_Lock(); 
		}
		if (!testAssert(fUseCount == 0)) //failed to lock (probably because inside a layer)
		{
			//restore drawing session
			_Unlock();
			fLockParentContext = oldLockType;
			return;
		}

		//unlock Gdiplus bmp: release Gdiplus graphic context
		xbox_assert(fGdiplusGC);
		fGdiplusGC->ReleaseHDC( fHDC);
		delete fGdiplusGC;
		fGdiplusGC = NULL;

		VPictureData* pictData = NULL;
		{
		VTaskLock lock(&(fMutexResources[D2D_CACHE_RESOURCE_SOFTWARE]));
		VRect localBounds(0,0,fGdiplusBmp->GetWidth(), fGdiplusBmp->GetHeight());
		if (inSrcBounds && localBounds != srcBoundsTransformed)
		{
			VPictureData_GDIPlus *pictDataGdiplus =  new VPictureData_GDIPlus(fGdiplusBmp, TRUE); //to prevent a copy, we give ownership temporary to pictDataGdiplus
			VPictureData* newPictData = pictDataGdiplus->CreateSubPicture( srcBoundsTransformed);
			Gdiplus::Bitmap *bmp = pictDataGdiplus->GetAndOwnGDIPlus_Bitmap(); //remove ownership from pictDataGdiplus
			xbox_assert(bmp == fGdiplusBmp);
			pictDataGdiplus->Release();
			pictData = newPictData;
		}
		else
			pictData = static_cast<VPictureData *>(new VPictureData_GDIPlus(fGdiplusBmp, FALSE));
		}
		if (pictData)
		{
			VRect destBounds;
			if (inDestBounds)
				destBounds.FromRect(*inDestBounds);
			else 
				destBounds.SetCoords(0,0,pictData->GetWidth(),pictData->GetHeight());

			inDestContext->BeginUsingContext();
			inDestContext->DrawPictureData( pictData, destBounds);
			inDestContext->EndUsingContext();

			pictData->Release();
		}

		//restore Gdiplus context
		xbox_assert(fGdiplusGC == NULL);
		fGdiplusGC = new Gdiplus::Graphics( static_cast<Gdiplus::Image *>(fGdiplusBmp));
		xbox_assert(fGdiplusGC);
		HDC hdc = fGdiplusGC->GetHDC();
		BindHDC( hdc, fHDCBounds, false);

		if (oldUseCount > 0)
		{
			//restore drawing session
			_Unlock();
			fLockParentContext = oldLockType;
		}
	}
	else if (fWICBitmap)
	{
		FlushGDI(); 
		VIndex oldUseCount = fUseCount;
		bool oldLockType = fLockParentContext;
		if (fUseCount > 0)
		{
			//we need to finish current drawing session if we need to access to the WIC bitmap
			fLockParentContext = true; //ensure render target is presented now
			_Lock(); 
		}
		if (!testAssert(fUseCount == 0)) //failed to lock (probably because inside a layer)
		{
			//restore drawing session
			_Unlock();
			fLockParentContext = oldLockType;
			return;
		}

		Gdiplus::Bitmap *bmp = VPictureCodec_WIC::FromWIC( fWICBitmap);
		VPictureData* pictData = testAssert(bmp) ? static_cast<VPictureData *>(new VPictureData_GDIPlus(bmp, TRUE)) : NULL;
		if (pictData)
		{
			if (inSrcBounds)
			{
				VRect localBounds(0,0,bmp->GetWidth(), bmp->GetHeight());
				if (localBounds != srcBoundsTransformed)
				{
					VPictureData* newPictData = pictData->CreateSubPicture( srcBoundsTransformed);
					pictData->Release();
					pictData = newPictData;
				}
			}

			VRect destBounds;
			if (inDestBounds)
				destBounds.FromRect(*inDestBounds);
			else 
				destBounds.SetCoords(0,0,pictData->GetWidth(),pictData->GetHeight());

			inDestContext->BeginUsingContext();
			inDestContext->DrawPictureData( pictData, destBounds);
			inDestContext->EndUsingContext();

			pictData->Release();
		}

		if (oldUseCount > 0)
		{
			//restore drawing session
			_Unlock();
			fLockParentContext = oldLockType;
		}
	}
}

VPictureData* VWinD2DGraphicContext::CopyContentToPictureData()
{
	if (fLayerOffScreenCount > 0)
		return NULL;

	xbox_assert(fDPI == 96.0f); //not allowed while printing

	if (fGdiplusBmp)
	{
		FlushGDI(); 
		VIndex oldUseCount = fUseCount;
		bool oldLockType = fLockParentContext;
		StParentContextNoDraw hdcNoDraw(this);
		if (fUseCount > 0)
		{
			//we need to finish current drawing session if we need to access to the Gdiplus bitmap
			fLockParentContext = true; //ensure render target is presented now
			_Lock(); 
		}
		if (!testAssert(fUseCount == 0)) //failed to lock (probably because inside a layer)
		{
			//restore drawing session
			_Unlock();
			fLockParentContext = oldLockType;
			return NULL;
		}
		//unlock Gdiplus bmp: release Gdiplus graphic context
		xbox_assert(fGdiplusGC);
		fGdiplusGC->ReleaseHDC( fHDC);
		delete fGdiplusGC;
		fGdiplusGC = NULL;

		VPictureData* pictData = NULL;
		VTaskLock lock(&(fMutexResources[D2D_CACHE_RESOURCE_SOFTWARE]));
		if (fGdiplusSharedBmpInUse && fGdiplusBmp == fGdiplusSharedBmp && (fHDCBounds.GetWidth() != fGdiplusSharedBmpWidth || fHDCBounds.GetHeight() != fGdiplusSharedBmpHeight))
		{
			VPictureData_GDIPlus *pictDataGdiplus =  new VPictureData_GDIPlus(fGdiplusBmp, TRUE); //to prevent a copy, we give ownership temporary to pictDataGdiplus
			VPictureData* newPictData = pictDataGdiplus->CreateSubPicture( fHDCBounds);
			Gdiplus::Bitmap *bmp = pictDataGdiplus->GetAndOwnGDIPlus_Bitmap(); //remove ownership from pictDataGdiplus
			xbox_assert(bmp == fGdiplusBmp);
			pictDataGdiplus->Release();
			pictData = newPictData;
		}
		else
			pictData = static_cast<VPictureData *>(new VPictureData_GDIPlus(fGdiplusBmp, FALSE));

		//restore Gdiplus context
		xbox_assert(fGdiplusGC == NULL);
		fGdiplusGC = new Gdiplus::Graphics( static_cast<Gdiplus::Image *>(fGdiplusBmp));
		xbox_assert(fGdiplusGC);
		HDC hdc = fGdiplusGC->GetHDC();
		BindHDC( hdc, fHDCBounds, false);

		if (oldUseCount > 0)
		{
			//restore drawing session
			_Unlock();
			fLockParentContext = oldLockType;
		}

		return pictData;
	}
	else if (fWICBitmap != NULL)
	{
		FlushGDI(); 
		VIndex oldUseCount = fUseCount;
		bool oldLockType = fLockParentContext;
		StParentContextNoDraw hdcNoDraw(this);
		if (fUseCount > 0)
		{
			//we need to finish current drawing session if we need to access to the WIC bitmap
			fLockParentContext = true; //ensure render target is presented now
			_Lock(); 
		}
		if (!testAssert(fUseCount == 0)) //failed to lock (probably because inside a layer)
		{
			//restore drawing session
			_Unlock();
			fLockParentContext = oldLockType;
			return NULL;
		}

		Gdiplus::Bitmap *bmp = VPictureCodec_WIC::FromWIC( fWICBitmap);
		VPictureData* pictData = testAssert(bmp) ? static_cast<VPictureData *>(new VPictureData_GDIPlus(bmp, TRUE)) : NULL;

		if (oldUseCount > 0)
		{
			//restore drawing session
			_Unlock();
			fLockParentContext = oldLockType;
		}

		return pictData;
	}
	return NULL;
}

void VWinD2DGraphicContext::FillUniformColor(const VPoint& inContextPos, const VColor& inColor, const VPattern* inPattern)
{
	_Lock();

	VColor*	pixelColor = GetPixelColor(inContextPos);
	COLORREF	winColor = 0x02000000 | pixelColor->GetRGBColor();
	
	HDC dc=_GetHDC();
	::ExtFloodFill(dc, inContextPos.GetX(), inContextPos.GetY(), winColor, FLOODFILLSURFACE);
	_ReleaseHDC(dc);

	_Unlock();
}


void VWinD2DGraphicContext::SetPixelColor(const VPoint& inContextPos, const VColor& inColor)
{
	_Lock(); //lock in order to access to render target
			 //(for compatibility: but if you need to call many times SetPixelColor
			 // it is preferable to call it outside BeginUsingContext/EndUsingContext
			 // in order to avoid multiple BeginDraw/EndDraw which can drastically reduce performance)
	if (fWICBitmap != NULL && fUseCount <= 0) //we can lock WIC bitmap only outside BeginUsingContext/EndUsingContext
											  //(because otherwise bitmap is yet locked with BeginDraw until EndDraw)
											  //otherwise try to use gdi (render target should be compatible)
	{
		WICRect bounds;
		UINT width, height;
		fWICBitmap->GetSize( &width, &height);
		bounds.X = 0;
		bounds.Y = 0;
		bounds.Width = width;
		bounds.Height = height;
		CComPtr<IWICBitmapLock> lockBmp;
		if (SUCCEEDED(fWICBitmap->Lock( &bounds, WICBitmapLockWrite, &lockBmp)))
		{
            UINT bufferSize = 0;
            UINT stride = 0;
            BYTE *data = NULL;
			WICPixelFormatGUID pf;

			lockBmp->GetStride(&stride);
			lockBmp->GetDataPointer( &bufferSize, &data);

			lockBmp->GetPixelFormat( &pf);
			xbox_assert(pf == GUID_WICPixelFormat32bppPBGRA
						||
						pf == GUID_WICPixelFormat32bppBGR);

			UINT32 *pixel = (UINT32 *)(data+(int)inContextPos.GetX()*4+(int)inContextPos.GetY()*stride);
			*pixel = inColor.GetRGBAColor(pf == GUID_WICPixelFormat32bppPBGRA ? TRUE : FALSE);

			lockBmp = (IWICBitmapLock*)NULL;
			_Unlock();
			return;
		}
	}
	HDC hdc = _GetHDC();
	if (hdc)
		::SetPixel(hdc, inContextPos.GetX(), inContextPos.GetY(), inColor.WIN_ToCOLORREF());
	_ReleaseHDC(hdc);
	_Unlock();
}


VColor* VWinD2DGraphicContext::GetPixelColor(const VPoint& inContextPos) const
{
	StParentContextNoDraw hdcNoDraw(this);
	_Lock(); //lock in order to access to render target
			 //(for compatibility: but if you need to call many times SetPixelColor
			 // it is preferable to call it outside BeginUsingContext/EndUsingContext
			 // in order to avoid multiple BeginDraw/EndDraw which can drastically reduce performance)
	if (fWICBitmap != NULL && fUseCount <= 0) //we can lock WIC bitmap only outside BeginUsingContext/EndUsingContext
											  //(because otherwise bitmap is yet locked with BeginDraw until EndDraw)
											  //otherwise try to use gdi (render target should be compatible)
	{
		WICRect bounds;
		UINT width, height;
		fWICBitmap->GetSize( &width, &height);
		bounds.X = 0;
		bounds.Y = 0;
		bounds.Width = width;
		bounds.Height = height;
		CComPtr<IWICBitmapLock> lockBmp;
		if (SUCCEEDED(fWICBitmap->Lock( &bounds, WICBitmapLockRead, &lockBmp)))
		{
            UINT bufferSize = 0;
            UINT stride = 0;
            BYTE *data = NULL;
			WICPixelFormatGUID pf;

			lockBmp->GetStride(&stride);
			lockBmp->GetDataPointer( &bufferSize, &data);

			lockBmp->GetPixelFormat( &pf);
			xbox_assert(pf == GUID_WICPixelFormat32bppPBGRA
						||
						pf == GUID_WICPixelFormat32bppBGR);

			UINT32 *pixel = (UINT32 *)(data+(int)inContextPos.GetX()*4+(int)inContextPos.GetY()*stride);
			VColor *color = new VColor();
			color->FromRGBAColor( (uLONG)(*pixel));
			if (pf == GUID_WICPixelFormat32bppPBGRA && color->GetAlpha() != 255)
			{
				//for transparent color, convert PBGRA to BGRA (because VColor is not pre-multiplied)
				sLONG val = (sLONG)((color->GetRed()*255.0f+(color->GetAlpha()*0.5f))/color->GetAlpha());
				if (val > 255)
					val = 255;
				color->SetRed( (uBYTE)val);

				val = (sLONG)((color->GetGreen()*255.0f+(color->GetAlpha()*0.5f))/color->GetAlpha());
				if (val > 255)
					val = 255;
				color->SetGreen( (uBYTE)val);

				val = (sLONG)((color->GetBlue()*255.0f+(color->GetAlpha()*0.5f))/color->GetAlpha());
				if (val > 255)
					val = 255;
				color->SetBlue( (uBYTE)val);
			}

			lockBmp = (IWICBitmapLock*)NULL;
			_Unlock();
			return color;
		}
	}
	VColor* result;
	HDC dc=_GetHDC();
	if (dc)
	{
		result = new VColor;
		if (result != NULL)
			result->WIN_FromCOLORREF( ::GetPixel(dc, inContextPos.GetX(), inContextPos.GetY()));
	}
	else
		result = new VColor();
	_ReleaseHDC(dc);
	
	_Unlock();
	return result;
}



/** return current clipping bounding rectangle 
 @remarks
 bounding rectangle is expressed in VGraphicContext normalized coordinate space 
 (ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
 */  
void VWinD2DGraphicContext::_GetClipBoundingBox( VRect& outBounds) const
{
	if (fGDI_HDC)
	{
		VRegion region;
		//as GetClip adjusts clip box with viewport offset, we need to get clip region in transformed space
		{
		StGDIResetTransform resetCTM(fGDI_GC->GetParentPort());
		fGDI_GC->GetClip( region);
		}
		outBounds = region.GetBounds();

		//for GDI, GetClip returns clipping specified in GDI viewport space:
		//we need to transform it to local user space

		VPoint translation;

		VAffineTransform ctm;
		_GetTransform(ctm);
		if (ctm.GetScaleX() == 1.0f && ctm.GetScaleY() == 1.0f
			&&
			ctm.GetShearX() == 0.0f && ctm.GetShearY() == 0.0f)
		{
			if (ctm.GetTranslateX() != 0.0f || ctm.GetTranslateY() != 0.0f)
				translation.SetPosBy( -ctm.GetTranslateX(), -ctm.GetTranslateY());
		}
		else
		{
			outBounds = ctm.Inverse().TransformRect( outBounds);
			return;
		}

		if (translation.GetX() || translation.GetY())
			outBounds.SetPosBy( translation.GetX(), translation.GetY());
		return;
	}

	if (fUseCount > 0)
	{
		//inside BeginUsingContext/EndUsingContext:
		//get render target clipping 

		//get transformed clipping bounds
		VRect bounds = fClipBoundsCur;

		//convert to current coordinate space
		VAffineTransform ctm;
		_GetTransform(ctm);
		bounds = ctm.Inverse().TransformRect(bounds);
		outBounds.FromRect( bounds);
	}
	//if outside BeginUsingContext/EndUsingContext,
	//return GDI clipping
	else if (fHDC)
	{
		//get HDC clipping
		VRegion region;
		GetClip( fHDC, region);
		outBounds = region.GetBounds();
	}
	else if (fHwnd)
	{
		//get HWND clipping
		VRegion region;
		GetClip( fHwnd, region);
		outBounds = region.GetBounds();
	}
	else
		//bitmap context: return bitmap bounds
		outBounds.FromRect( fHDCBounds);
}

bool VWinD2DGraphicContext::ShouldDrawTextOnTransparentLayer() const
{
	TextRenderingMode trm = GetTextRenderingMode();
	if (trm & TRM_LEGACY_ON)
		return false;

	if (!(trm & TRM_WITHOUT_ANTIALIASING))
		if (!(trm & TRM_WITH_ANTIALIASING_NORMAL))
			return false;
	return true;
}

/** get next layer from internal cache or create new one */
ID2D1Layer *VWinD2DGraphicContext::_AllocateLayer() const
{
	D2D_GUI_RESOURCE_MUTEX_LOCK(fIsRTHardware)

	if (fVectorOfLayer[fIsRTHardware].empty())
	{
		//allocate new layer 
		ID2D1Layer *layer = NULL;
		bool ok;
		ok = SUCCEEDED(fRscRenderTarget[fIsRTHardware]->CreateLayer( &layer));
		if (!ok)
		{
			//dipose shared resources & try again
			_DisposeSharedResources(false);

			ok = SUCCEEDED(fRscRenderTarget[fIsRTHardware]->CreateLayer( &layer));
		}
		if (ok)
			return layer;
		else
			return NULL;
	}
	else
	{
		//pick layer from internal cache
		ID2D1Layer *layer = fVectorOfLayer[fIsRTHardware].back();
		layer->AddRef();
		fVectorOfLayer[fIsRTHardware].pop_back();
		return layer;
	}
}

/** free layer: store it in internal cache if cache is not full */
void VWinD2DGraphicContext::_FreeLayer( ID2D1Layer *inLayer) const
{
	D2D_GUI_RESOURCE_MUTEX_LOCK(fIsRTHardware)

	if (fVectorOfLayer[fIsRTHardware].size() < D2D_LAYER_CACHE_MAX)
		fVectorOfLayer[fIsRTHardware].push_back( inLayer);
	inLayer->Release();
}

/** clear layer cache */
void VWinD2DGraphicContext::_ClearLayers() const
{
	D2D_GUI_RESOURCE_MUTEX_LOCK(fIsRTHardware)

	fVectorOfLayer[fIsRTHardware].clear();
}


// Create a filter or alpha layer graphic context
// with specified bounds and filter
// @remark : retain the specified filter until EndLayer is called
bool VWinD2DGraphicContext::_BeginLayer(const VRect& _inBounds, GReal inAlpha, VGraphicFilterProcess *inFilterProcess, VImageOffScreenRef inBackBuffer, bool inInheritParentClipping, bool inTransparent)
{
	FlushGDI();

	if (inFilterProcess == NULL && inBackBuffer == NULL)
	{
		//push D2D opacity layer

		StUseContext_NoRetain	context(this);

		//create new D2D layer
		ID2D1Layer *layer = NULL;
		layer = _AllocateLayer();
		if (!layer)
		{
			SaveClip();
			return true;
		}

		//push D2D layer
		fRenderTarget->PushLayer(
			D2D1::LayerParameters(	D2D_RECT(_inBounds), 
									NULL,
									D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
									D2D1::IdentityMatrix(),
									inAlpha,
									NULL,
									IsTransparent() ? D2D1_LAYER_OPTIONS_NONE : D2D1_LAYER_OPTIONS_INITIALIZE_FOR_CLEARTYPE
									),
			layer
			);

		//push layer element 
		VAffineTransform ctm;
		GetTransform(ctm);
		fStackLayerClip.push_back( VLayerClipElem( ctm, _inBounds, NULL, fClipBoundsCur, fClipRegionCur, layer, NULL, inAlpha, fHasClipOverride));
		fClipBoundsCur.Intersect( fStackLayerClip.back().GetBounds());
		if (fClipRegionCur)
		{
			VRect bounds = fStackLayerClip.back().GetBounds();
			bounds.NormalizeToInt();
			VRegion region( bounds);
			fClipRegionCur->Intersect( region);
		}
		fHasClipOverride = false;

		layer->Release();

		return true;
	}


	//determine if backbuffer is owned by context (temporary offscreen used by filter effect for instance) 
	//or by caller (permanent offscreen used to cache document drawing for instance)
	bool ownBackBuffer = inBackBuffer == NULL;
	if (ownBackBuffer) 
	{
		//TODO: skip until filters are implemented
		SaveClip();
		return true; 
	}

	StUseContext_NoRetain	context(this);

	if (inBackBuffer == (VImageOffScreenRef)0xFFFFFFFF)
		inBackBuffer = NULL;
	bool bNewBackground = true;

	ID2D1BitmapRenderTarget *bmpRT = inBackBuffer ? (ID2D1BitmapRenderTarget *)(*inBackBuffer) : NULL;
	if (bmpRT)
	{
		//reset bitmap render target if it is not compatible with current render target
		D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
			fIsRTHardware ? D2D1_RENDER_TARGET_TYPE_HARDWARE : D2D1_RENDER_TARGET_TYPE_SOFTWARE,
			D2D1::PixelFormat(),
			0.0f,
			0.0f,
			D2D_RENDER_TARGET_USE_GDI_COMPATIBLE_DC ? D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE : D2D1_RENDER_TARGET_USAGE_NONE,
			fIsRTHardware ? D2D1_FEATURE_LEVEL_10 : D2D1_FEATURE_LEVEL_DEFAULT
			);
		if (!bmpRT->IsSupported( props))
		{
			inBackBuffer->SetAndRetain( (ID2D1BitmapRenderTarget *)NULL);
			bmpRT = NULL;
		}
	}

	//compute transformed bbox
	VAffineTransform ctm;
	GetTransform( ctm);
	VRect bounds;
	VPoint ctmTranslate;

	VRect boundsClip;
	if (inInheritParentClipping)
	{
		boundsClip = fClipBoundsCur;
		boundsClip.NormalizeToInt();
	}
	if (ownBackBuffer)
	{
		//for temporary layer,
		//we draw directly in target device space

		//clip with current context clipping bounds
		bounds = _inBounds;
		bounds = ctm.TransformRect( bounds);
		if (inInheritParentClipping)
			//we can clip input bounds with current clipping bounds
			//because for temporary layer we will never draw outside current clipping bounds
			bounds.Intersect( boundsClip);
	}
	else
	{
		//for background drawing, in order to avoid discrepancies
		//we draw in target device space minus user to device transform translation
		//we cannot draw directly in screen user space otherwise we would have discrepancies
		//(because background layer is re-used by caller and layer origin will not match always the same pixel origin in screen space)
		ctmTranslate = ctm.GetTranslation();
		ctm.SetTranslation( 0.0f, 0.0f);
		bounds = ctm.TransformRect( _inBounds);

	}

	if (bounds.GetWidth() == 0.0 || bounds.GetHeight() == 0.0)
	{
		//draw area is empty: push empty layer
		
		SaveClip();
		return true;
	}	
	else
	{
		//draw area is not empty:
		//build the layer back buffer and graphic context and stack it

		//inflate a little bit to deal with antialiasing 
		bounds.SetPosBy(-2.0f,-2.0f);
		bounds.SetSizeBy(4.0f,4.0f); 
		bounds.NormalizeToInt();

		//TODO: implement filter layers (using D3D HLSL shaders)
		//		for now only offscreen layer is implemented

		//stack layer filter process
		//if (inFilterProcess)
		//{
		//	inFilterProcess->Retain();
		//	
		//	//inflate bounds to take account of filter (for blur filter for instance)
		//	VPoint offsetFilter;
		//	offsetFilter.x = ceil(inFilterProcess->GetInflatingOffset()*ctm.GetScaleX());
		//	offsetFilter.y = ceil(inFilterProcess->GetInflatingOffset()*ctm.GetScaleY());
		//	bounds.SetPosBy(-offsetFilter.x,-offsetFilter.y);
		//	bounds.SetSizeBy(offsetFilter.x*2.0f, offsetFilter.y*2.0f);

		//	GReal blurRadius = inFilterProcess->GetBlurMaxRadius();
		//	if (blurRadius != 0.0f)
		//	{
		//		//if (sLayerFilterInflatingOffset > blurRadius)
		//		//	blurRadius = VGraphicContext::sLayerFilterInflatingOffset;

		//		bounds.SetPosBy(-blurRadius,-blurRadius);
		//		bounds.SetSizeBy(blurRadius*2, blurRadius*2);
		//	}
		//}
		//fStackLayerFilter.push_back( inFilterProcess);

		//create bitmap render target
		//(we cannot use a D2D layer because layer content cannot be preserved outside PushLayer/PopLayer
		// and we cannot apply filter on a D2D layer)

		D2D1_PIXEL_FORMAT pixelFormatDesired = D2D1::PixelFormat(
			DXGI_FORMAT_UNKNOWN, //use parent render target pixel format
			inTransparent ? D2D1_ALPHA_MODE_PREMULTIPLIED : D2D1_ALPHA_MODE_IGNORE //alpha mode depending on desired transparency
			);

		HRESULT hr = S_OK;
		if (bmpRT == NULL)
		{
#if D2D_RENDER_TARGET_USE_GDI_COMPATIBLE_DC
			D2D1_SIZE_F size = D2D1::SizeF(bounds.GetWidth(), bounds.GetHeight());
			hr = fRenderTarget->CreateCompatibleRenderTarget(
					&size, NULL,
					&pixelFormatDesired,
					D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_GDI_COMPATIBLE,
					&bmpRT);
#else
			hr = fRenderTarget->CreateCompatibleRenderTarget(
					D2D1::SizeF(bounds.GetWidth(), bounds.GetHeight()), &bmpRT);
#endif
		}
		else
		{
			D2D1_ALPHA_MODE alphaMode = bmpRT->GetPixelFormat().alphaMode;
			D2D1_SIZE_F size = bmpRT->GetSize();
			if ((size.width != bounds.GetWidth())
				||
				(size.height != bounds.GetHeight())
				||
				(alphaMode != pixelFormatDesired.alphaMode)
				)
			{
				//reset back buffer
				inBackBuffer->SetAndRetain( (ID2D1BitmapRenderTarget *)NULL);
				bmpRT = NULL;

#if D2D_RENDER_TARGET_USE_GDI_COMPATIBLE_DC
				D2D1_SIZE_F size = D2D1::SizeF(bounds.GetWidth(), bounds.GetHeight());
				hr = fRenderTarget->CreateCompatibleRenderTarget(
						&size, NULL,
						&pixelFormatDesired,
						D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_GDI_COMPATIBLE,
						&bmpRT);
#else
				hr = fRenderTarget->CreateCompatibleRenderTarget(
						D2D1::SizeF(bounds.GetWidth(), bounds.GetHeight()), &bmpRT);
#endif
			}
			else
			{
				//use external back buffer unmodified
				bmpRT->AddRef();
				bNewBackground = false;
			}
		}
		if (FAILED(hr))
		{
			SaveClip();
			return true;
		}
		if (inBackBuffer)
		{
			inBackBuffer->SetAndRetain( bmpRT, (ID2D1RenderTarget *)fRenderTarget);
			inBackBuffer->Retain();
		}
		else
			inBackBuffer = new VImageOffScreen( bmpRT, (ID2D1RenderTarget *)fRenderTarget);
		if (bmpRT)
			bmpRT->Release();

		//push offscreen layer element
		VAffineTransform ctmOffset;
		if (!ownBackBuffer)
			ctmOffset.SetTranslation( ctmTranslate.GetX(), ctmTranslate.GetY());
		fStackLayerClip.push_back( VLayerClipElem( ctmOffset, bounds, fClipBoundsCur, fClipRegionCur, inBackBuffer, inAlpha, fHasClipOverride, ownBackBuffer));
		if (inInheritParentClipping)
		{
			fClipBoundsCur.Intersect( fStackLayerClip.back().GetBounds());
			if (fClipRegionCur)
			{
				VRect bounds = fStackLayerClip.back().GetBounds();
				bounds.NormalizeToInt();
				VRegion region( bounds);
				fClipRegionCur->Intersect( region);
			}
		}
		inBackBuffer->Release();	
		fHasClipOverride = false;

		//set current render target to bitmap render target
		CComPtr<ID2D1RenderTarget> rtParent = fRenderTarget;
		fRenderTarget = (ID2D1RenderTarget *)NULL;
		hr = bmpRT->QueryInterface(__uuidof(ID2D1RenderTarget), (void **)&fRenderTarget);
		xbox_assert(SUCCEEDED(hr));
		fRenderTarget->BeginDraw();

		D2D1_PIXEL_FORMAT pf = fRenderTarget->GetPixelFormat();
		fIsTransparent = pf.alphaMode != D2D1_ALPHA_MODE_IGNORE;

		//inherit settings from parent render target

		//CComPtr<IDWriteRenderingParams> textRenderingParams;
		//rtParent->GetTextRenderingParams( &textRenderingParams);
		//if (textRenderingParams)
		//	fRenderTarget->SetTextRenderingParams( textRenderingParams);

		D2D1_TEXT_ANTIALIAS_MODE antialiasMode = rtParent->GetTextAntialiasMode();
		if (antialiasMode == D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE)
			if (fIsTransparent)
				//disable ClearType on alpha render target 
				antialiasMode = D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE;
		fRenderTarget->SetTextAntialiasMode( antialiasMode);

		//inherit clipping from parent if appropriate
		if (inInheritParentClipping && (!ownBackBuffer))
			//get parent clipping in layer local space
			boundsClip.SetPosBy(-bounds.GetX()-ctmTranslate.GetX(), -bounds.GetY()-ctmTranslate.GetY());

		//set clipping
		SetTransform(VAffineTransform()); //clip in layer local space
		VRect boundsClipLocal = bounds;
		boundsClipLocal.SetPosTo(0.0f, 0.0f);
		if (bNewBackground)
			//for new offscreen, we need to clear surface 
			fRenderTarget->Clear( D2D1::ColorF(0, 0.0f));

		//transform global clipping to layer local space
		if (inInheritParentClipping)
		{
			fClipBoundsCur.SetPosBy( -bounds.GetX()-ctmTranslate.GetX(), -bounds.GetY()-ctmTranslate.GetY());
			fClipBoundsCur.NormalizeToInt();
			if (fClipRegionCur)
				fClipRegionCur->SetPosBy( -bounds.GetX()-ctmTranslate.GetX(), -bounds.GetY()-ctmTranslate.GetY());
		}
		else
		{
			fClipBoundsCur = boundsClipLocal;
			if (fClipRegionCur)
			{
				delete fClipRegionCur;
				fClipRegionCur = NULL;
			}
		}

		if (inInheritParentClipping && (!ownBackBuffer))
			boundsClipLocal.Intersect(boundsClip);
		ClipRect( boundsClipLocal);

		//set layer transform
		ctm.Translate( -bounds.GetX(), -bounds.GetY(),
						VAffineTransform::MatrixOrderAppend);
		SetTransform( ctm);

		fLayerOffScreenCount++;
	}
	if (!ownBackBuffer)
		return bNewBackground;
	else
		return 	true;
}


// Create a shadow layer graphic context
// with specified bounds and transparency
void VWinD2DGraphicContext::BeginLayerShadow(const VRect& _inBounds, const VPoint& inShadowOffset, GReal inShadowBlurDeviation, const VColor& inShadowColor, GReal inAlpha)
{
	FlushGDI();
	_SaveClip();
	////compute transformed bbox
	//VAffineTransform ctm;
	//GetTransform( ctm);
	//
	////intersect current clipping rect with input bounds
	//Gdiplus::RectF boundsClipNative;
	//Gdiplus::Status status = fRenderTarget->GetClipBounds( &boundsClipNative);
	//VRect boundsClip( boundsClipNative.X, boundsClipNative.Y, boundsClipNative.Width, boundsClipNative.Height);
	//VRect bounds = _inBounds;
	//bounds.Intersect( boundsClip);
	//bounds = ctm.TransformRect( bounds);

	//if (bounds.GetWidth() == 0.0 || bounds.GetHeight() == 0.0)
	//{
	//	//draw area is empty: push empty layer
	//	
	//	fStackLayerBounds.push_back( VRect(0,0,0,0));    
	//	fStackLayerBackground.push_back( NULL);
	//	fStackLayerOwnBackground.push_back( true);
	//	fStackLayerGC.push_back( NULL);
	//	fStackLayerAlpha.push_back( 1.0);
	//	fStackLayerFilter.push_back( NULL);
	//}	
	//else
	//{
	//	//draw area is not empty:
	//	//build the layer back buffer and graphic context and stack it
	//	
	//	GReal dx = bounds.GetX()-floor(bounds.GetX());
	//	GReal dy = bounds.GetY()-floor(bounds.GetY());
	//	bounds.SetX( floor(bounds.GetX()));
	//	bounds.SetY( floor(bounds.GetY()));
	//	bounds.SetWidth( ceil(bounds.GetWidth()+dx));
	//	bounds.SetHeight( ceil(bounds.GetHeight()+dy));
	//	
	//	//create shadow filter sequence
	//	VGraphicFilterProcess *filterProcess = new VGraphicFilterProcess();	
	//	
	//	//add color matrix filter to set shadow color (if appropriate)
	//	VValueBag attributes;
	//	if (inShadowColor != VColor(0,0,0,255))
	//	{
	//		VGraphicFilter filterColor(eGraphicFilterType_ColorMatrix);
	//		attributes.SetReal(	VGraphicFilterBagKeys::in,	(sLONG)eGFIT_SOURCE_ALPHA);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM00,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM01,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM02,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM03,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM04,	inShadowColor.GetRed()*1.0/255.0);
	//		
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM10,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM11,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM12,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM13,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM14,	inShadowColor.GetGreen()*1.0/255.0);
	//		
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM20,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM21,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM22,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM23,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM24,	inShadowColor.GetBlue()*1.0/255.0);
	//		
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM30,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM31,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM32,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM33,	inShadowColor.GetAlpha()*1.0/255.0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM34,	0);
	//		filterColor.SetAttributes( attributes);
	//		filterProcess->Add( filterColor);	
	//	}
	//	
	//	//add blur filter
	//	attributes.Clear();
	//	VGraphicFilter filterBlur( eGraphicFilterType_Blur);
	//	if (inShadowColor == VColor(0,0,0,255))
	//		attributes.SetReal(	VGraphicFilterBagKeys::in,	(sLONG)eGFIT_SOURCE_ALPHA);
	//	else
	//		attributes.SetReal(	VGraphicFilterBagKeys::in,	(sLONG)eGFIT_LAST_RESULT);
	//	attributes.SetReal(	VGraphicFilterBagKeys::blurDeviation, inShadowBlurDeviation);
	//	filterBlur.SetAttributes( attributes);
	//	filterProcess->Add( filterBlur);	
	//		
	//	//add offset filter
	//	attributes.Clear();
	//	VGraphicFilter filterOffset( eGraphicFilterType_Offset);
	//	attributes.SetReal(	VGraphicFilterBagKeys::in,	(sLONG)eGFIT_LAST_RESULT);
	//	attributes.SetReal(	VGraphicFilterBagKeys::offsetX,	inShadowOffset.GetX());
	//	attributes.SetReal(	VGraphicFilterBagKeys::offsetY,	inShadowOffset.GetY());
	//	filterOffset.SetAttributes( attributes);
	//	filterProcess->Add( filterOffset);	
	//	
	//	//add blend filter
	//	attributes.Clear();
	//	VGraphicFilter filterBlend( eGraphicFilterType_Blend);
	//	attributes.SetReal( VGraphicFilterBagKeys::in,	(sLONG)eGFIT_SOURCE_GRAPHIC);
	//	attributes.SetReal( VGraphicFilterBagKeys::in2,	(sLONG)eGFIT_LAST_RESULT);
	//	attributes.SetReal( VGraphicFilterBagKeys::blendType, (sLONG)eGraphicImageBlendType_Over);
	//	filterBlend.SetAttributes( attributes);
	//	filterProcess->Add( filterBlend);	

	//	//add transparency filter if appropriate
	//	if (inAlpha != 1.0f)
	//	{
	//		attributes.Clear();
	//		VGraphicFilter filterColor(eGraphicFilterType_ColorMatrix);
	//		attributes.SetReal(	VGraphicFilterBagKeys::in,	(sLONG)eGFIT_LAST_RESULT);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM00,	1);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM01,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM02,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM03,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM04,	0);
	//		
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM10,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM11,	1);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM12,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM13,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM14,	0);
	//		
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM20,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM21,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM22,	1);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM23,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM24,	0);
	//		
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM30,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM31,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM32,	0);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM33,	inAlpha);
	//		attributes.SetReal(	VGraphicFilterBagKeys::colorMatrixM34,	0);
	//		filterColor.SetAttributes( attributes);
	//		filterProcess->Add( filterColor);	
	//	}
	//		
	//	//stack layer filter
	//	fStackLayerFilter.push_back( filterProcess);

	//	//inflate bounds to take account of filter (for blur filter for instance)
	//	GReal pageScaleParent = fRenderTarget->GetPageScale();
	//	VPoint offsetFilter;
	//	offsetFilter.x = ceil(filterProcess->GetInflatingOffset()*ctm.GetScaleX());
	//	offsetFilter.y = ceil(filterProcess->GetInflatingOffset()*ctm.GetScaleY());
	//	bounds.SetPosBy(-offsetFilter.x,-offsetFilter.y);
	//	bounds.SetSizeBy(offsetFilter.x*2.0f, offsetFilter.y*2.0f);

	//	GReal blurRadius = filterProcess->GetBlurMaxRadius();
	//	if (blurRadius != 0.0f)
	//	{
	//		//if (sLayerFilterInflatingOffset > blurRadius)
	//		//	blurRadius = VGraphicContext::sLayerFilterInflatingOffset;

	//		bounds.SetPosBy(-blurRadius,-blurRadius);
	//		bounds.SetSizeBy(blurRadius*2, blurRadius*2);
	//	}

	//	//stack layer transformed bounds
	//	fStackLayerBounds.push_back( bounds);   

	//	//adjust bitmap bounds if parent graphic context has page scale not equal to 1
	//	//(while printing for instance, page scale can be 300/72 if dpi == 300
	//	// and to ensure bitmap will not be pixelized while printing, bitmap
	//	// must be scaled to printer device resolution)
	//	VRect boundsBitmap = bounds;
	//	if (pageScaleParent != 1.0f)
	//	{
	//		boundsBitmap.SetWidth( ceil(bounds.GetWidth() * pageScaleParent));
	//		boundsBitmap.SetHeight( ceil(bounds.GetHeight() * pageScaleParent));
	//	}
	//	
	//	//stack layer back buffer
	//	Gdiplus::Bitmap *bmpLayer = new Gdiplus::Bitmap(	boundsBitmap.GetWidth(), 
	//														boundsBitmap.GetHeight(), 
	//														(PixelFormat)PixelFormat32bppARGB);
	//	fStackLayerBackground.push_back( bmpLayer);
	//	fStackLayerOwnBackground.push_back( true);
	//	
	//	//stack current graphic context
	//	fStackLayerGC.push_back( fRenderTarget);
	//	
	//	//stack layer alpha transparency 
	//	fStackLayerAlpha.push_back( 1.0);
	//	
	//	bmpLayer->SetResolution(fRenderTarget->GetDpiX(),fRenderTarget->GetDpiY());
	//	
	//	//set current graphic context to the layer graphic context
	//	Gdiplus::Graphics *gcLayer = new Gdiplus::Graphics( bmpLayer);
	//	
	//	//layer inherit from parent context settings
	//	status = gcLayer->SetCompositingMode( CompositingModeSourceOver);
	//	status = gcLayer->SetCompositingQuality( fRenderTarget->GetCompositingQuality());
	//	status = gcLayer->SetSmoothingMode( fRenderTarget->GetSmoothingMode());
	//	status = gcLayer->SetInterpolationMode( fRenderTarget->GetInterpolationMode());
	//	status = gcLayer->SetTextRenderingHint( fRenderTarget->GetTextRenderingHint());
	//	TextRenderingHint textRenderingHint = fRenderTarget->GetTextRenderingHint();
	//	//fallback to text default antialias for layer graphic context if text cleartype antialias is used
	//	//to avoid graphic artifacts
	//	status = gcLayer->SetTextRenderingHint( textRenderingHint == TextRenderingHintClearTypeGridFit ? 
	//											TextRenderingHintAntiAlias : textRenderingHint);
	//	status = gcLayer->SetPixelOffsetMode( fRenderTarget->GetPixelOffsetMode());
	//	status = gcLayer->SetTextContrast( fRenderTarget->GetTextContrast());
	//	status = gcLayer->SetPageUnit( fRenderTarget->GetPageUnit());
	//	status = gcLayer->SetPageScale( 1.0f);
	//	
	//	//ensure that there is no scaling between parent and layer context
	//	xbox_assert( gcLayer->GetDpiX() == fRenderTarget->GetDpiX());
	//	xbox_assert( gcLayer->GetDpiY() == fRenderTarget->GetDpiY());
	//	
	//	//reset clip to layer bounds
	//	fRenderTarget = gcLayer;
	//	VRect rectClip = boundsBitmap;
	//	rectClip.SetPosBy( -boundsBitmap.GetX(), -boundsBitmap.GetY());
	//	fRenderTarget->SetClip( GDIPLUS_RECT(rectClip));
	//	
	//	//set layer transform
	//	ctm.Translate( -boundsBitmap.GetX(), -boundsBitmap.GetY(),
	//					VAffineTransform::MatrixOrderAppend);
	//	ctm.Scale( pageScaleParent, pageScaleParent, VAffineTransform::MatrixOrderAppend);
	//	SetTransform( ctm);
	//}
}


VImageOffScreenRef VWinD2DGraphicContext::_EndLayerOffScreen() const
{
	FlushGDI();

	StUseContext_NoRetain	context(this);

	const VLayerClipElem& layer = fStackLayerClip.back();

	VImageOffScreen *backbuffer = layer.GetImageOffScreen();
	if (!testAssert(backbuffer))
		return NULL;

	fLayerOffScreenCount--;

	VAffineTransform ctm;
	_GetTransform(ctm);

	//restore parent render target (should always succeed)
	fRenderTarget = (ID2D1RenderTarget *)NULL;
	ID2D1RenderTarget *parentRT = backbuffer->GetParentRenderTarget();
	xbox_assert(parentRT);
	fRenderTarget = parentRT;
	backbuffer->ClearParentRenderTarget();

	D2D1_PIXEL_FORMAT pf = fRenderTarget->GetPixelFormat();
	fIsTransparent = pf.alphaMode != D2D1_ALPHA_MODE_IGNORE;

	//restore parent global clipping bounds & transform
	fClipBoundsCur = layer.GetPrevClipBounds();
	if (fClipRegionCur)
		delete fClipRegionCur;
	fClipRegionCur = layer.GetOwnedPrevClipRegion();

	VPoint transLayerToParent = layer.GetBounds().GetTopLeft();
	ctm.SetTranslation( ctm.GetTranslateX()+transLayerToParent.GetX(),
						ctm.GetTranslateY()+transLayerToParent.GetY());

	//get bitmap render target
	ID2D1BitmapRenderTarget *bmpRT = (ID2D1BitmapRenderTarget *)(*backbuffer);
	if (!testAssert(bmpRT))
		return NULL;
	
	//end offscreen drawing 
	HRESULT hr = bmpRT->EndDraw();
	if (FAILED(hr))
	{
		xbox_assert(false);
		return NULL;
	}

	//draw bitmap render target over parent render target
	ID2D1Bitmap *bmp = NULL;
	hr = bmpRT->GetBitmap( &bmp);
	if (FAILED(hr))
	{
		xbox_assert(false);
		return NULL;
	}

	D2D1_BITMAP_INTERPOLATION_MODE interpolationMode = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;
	if (floor(transLayerToParent.GetX()) == transLayerToParent.GetX()
		&&
		floor(transLayerToParent.GetY()) == transLayerToParent.GetY())
		interpolationMode = D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;

	_SetTransform(VAffineTransform());
	D2D1_RECT_F rect = D2D1::RectF( transLayerToParent.GetX(), 
									transLayerToParent.GetY(),
									transLayerToParent.GetX()+bmp->GetSize().width,
									transLayerToParent.GetY()+bmp->GetSize().height);
	fRenderTarget->DrawBitmap( bmp, rect, layer.GetOpacity(), interpolationMode);
	bmp->Release();

	_SetTransform(ctm);

	backbuffer->Retain();
	return backbuffer;
}


// Draw the layer graphic context in the container graphic context 
// (which can be either the main graphic context or another layer graphic context)
VImageOffScreenRef VWinD2DGraphicContext::_EndLayer( ) 
{
	return _RestoreClip();
}


/** return true if offscreen layer needs to be cleared or resized on next call to DrawLayerOffScreen or BeginLayerOffScreen/EndLayerOffScreen
@remarks
	return true if transformed input bounds size is not equal to the background layer size
    so only transformed bounds translation is allowed.
	return true the offscreen layer is not compatible with the current gc
 */
bool VWinD2DGraphicContext::ShouldClearLayerOffScreen(const VRect& inBounds, VImageOffScreenRef inBackBuffer)
{
	if (!inBackBuffer)
		return true;

	ID2D1BitmapRenderTarget *bmpRT = (ID2D1BitmapRenderTarget *)(*inBackBuffer);
	if (!bmpRT)
		return true;

	//bitmap render target should be compatible with current render target
	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		fIsRTHardware ? D2D1_RENDER_TARGET_TYPE_HARDWARE : D2D1_RENDER_TARGET_TYPE_SOFTWARE,
		D2D1::PixelFormat(),
		0.0f,
		0.0f,
		D2D_RENDER_TARGET_USE_GDI_COMPATIBLE_DC ? D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE : D2D1_RENDER_TARGET_USAGE_NONE,
		fIsRTHardware ? D2D1_FEATURE_LEVEL_10 : D2D1_FEATURE_LEVEL_DEFAULT
		);
	if (!bmpRT->IsSupported( props))
		return true;

	FlushGDI();

	StUseContext_NoRetain	context(this);

	//compute transformed bbox
	VAffineTransform ctm;
	GetTransform( ctm);
	VRect bounds;
	VPoint ctmTranslate;

	//for background drawing, in order to avoid discrepancies
	//we draw in target device space minus user to device transform translation
	//we cannot draw directly in screen user space otherwise we would have discrepancies
	//(because background layer is re-used by caller and layer origin will not match always the same pixel origin in screen space)
	VPoint ctmTranslation = ctm.GetTranslation();
	VAffineTransform xform = ctm;
	xform.SetTranslation( 0.0f, 0.0f);
	bounds = xform.TransformRect( inBounds);

	//inflate a little bit to deal with antialiasing 
	bounds.SetPosBy(-2.0f,-2.0f);
	bounds.SetSizeBy(4.0f,4.0f); 
	bounds.NormalizeToInt();

	if (bmpRT->GetSize().width != bounds.GetWidth()
		||
		bmpRT->GetSize().height != bounds.GetHeight())
		return true;

	return false;
}


/** draw offscreen layer using the specified bounds 
@remarks
	this is like calling BeginLayerOffScreen & EndLayerOffScreen without any drawing between
	so it is faster because only the offscreen bitmap is rendered
 
	if transformed bounds size is not equal to background layer size, return false and do nothing 
    so only transformed bounds translation is allowed.
	if inBackBuffer is NULL, return false and do nothing

	ideally if offscreen content is not dirty, you should first call this method
	and only call BeginLayerOffScreen & EndLayerOffScreen if it returns false
 */
bool VWinD2DGraphicContext::DrawLayerOffScreen(const VRect& inBounds, VImageOffScreenRef inBackBuffer, bool inDrawAlways)
{
	if (!inBackBuffer)
		return false;

	ID2D1BitmapRenderTarget *bmpRT = (ID2D1BitmapRenderTarget *)(*inBackBuffer);
	if (!bmpRT)
		return false;

	FlushGDI();

	//reset bitmap render target if it is not compatible with current render target
	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		fIsRTHardware ? D2D1_RENDER_TARGET_TYPE_HARDWARE : D2D1_RENDER_TARGET_TYPE_SOFTWARE,
		D2D1::PixelFormat(),
		0.0f,
		0.0f,
		D2D_RENDER_TARGET_USE_GDI_COMPATIBLE_DC ? D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE : D2D1_RENDER_TARGET_USAGE_NONE,
		fIsRTHardware ? D2D1_FEATURE_LEVEL_10 : D2D1_FEATURE_LEVEL_DEFAULT
		);
	if (!bmpRT->IsSupported( props))
	{
		inBackBuffer->SetAndRetain( (ID2D1BitmapRenderTarget *)NULL);
		return false;
	}

	StUseContext_NoRetain	context(this);

	//compute transformed bbox
	VAffineTransform ctm;
	GetTransform( ctm);
	VRect bounds;
	VPoint ctmTranslate;

	//for background drawing, in order to avoid discrepancies
	//we draw in target device space minus user to device transform translation
	//we cannot draw directly in screen user space otherwise we would have discrepancies
	//(because background layer is re-used by caller and layer origin will not match always the same pixel origin in screen space)
	VPoint ctmTranslation = ctm.GetTranslation();
	VAffineTransform xform = ctm;
	xform.SetTranslation( 0.0f, 0.0f);
	bounds = xform.TransformRect( inBounds);

	//inflate a little bit to deal with antialiasing 
	bounds.SetPosBy(-2.0f,-2.0f);
	bounds.SetSizeBy(4.0f,4.0f); 

	bool needInterpolation = false;
	if (inDrawAlways)
		needInterpolation = bounds.GetX() != floor(bounds.GetX()) || bounds.GetY() != floor(bounds.GetY());
	else
	{
		bounds.NormalizeToInt();

		if (bmpRT->GetSize().width != bounds.GetWidth()
			||
			bmpRT->GetSize().height != bounds.GetHeight())
			return false;
	}

	//draw bitmap render target over parent render target
	ID2D1Bitmap *bmp = NULL;
	HRESULT hr = bmpRT->GetBitmap( &bmp);
	if (FAILED(hr))
		return false;

	if (!needInterpolation)
		needInterpolation = (floor(ctmTranslation.GetX()) != ctmTranslation.GetX()
							||
							floor(ctmTranslation.GetY()) != ctmTranslation.GetY());

	D2D1_BITMAP_INTERPOLATION_MODE interpolationMode = needInterpolation ? D2D1_BITMAP_INTERPOLATION_MODE_LINEAR : D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;

	SetTransform(VAffineTransform());
	D2D1_RECT_F rect = D2D1::RectF( ctmTranslation.GetX()+bounds.GetX(), 
									ctmTranslation.GetY()+bounds.GetY(),
									ctmTranslation.GetX()+bounds.GetX()+bmp->GetSize().width,
									ctmTranslation.GetY()+bounds.GetY()+bmp->GetSize().height);
	fRenderTarget->DrawBitmap( bmp, rect, 1.0f, interpolationMode);
	bmp->Release();

	SetTransform(ctm);
	return true;
}



// clear all layers and restore main graphic context
void VWinD2DGraphicContext::ClearLayers() 
{
	if (fUseCount > 0)
	{
		fUseCount = 1;
		EndUsingContext();
	}
}



void VWinD2DGraphicContext::_GetTransformToLayer(VAffineTransform &outTransform, sLONG inIndexLayer) const 
{
	if (fStackLayerClip.size() == 0)
	{
		_GetTransform(outTransform);
		return;
	}

	FlushGDI();

	_GetTransform(outTransform);
	VStackLayerClip::const_reverse_iterator itLayer = fStackLayerClip.rbegin();
	sLONG index = fStackLayerClip.size()-1;
	for (;itLayer != fStackLayerClip.rend(); itLayer++, index--)
	{
		if (inIndexLayer == index)
			break;

		const VLayerClipElem& layer = *itLayer;
		if (layer.IsImageOffScreen())
		{
			//append offscreen translation
			VPoint transLayerToParent = layer.GetBounds().GetTopLeft();
			outTransform.SetTranslation( 
						outTransform.GetTranslateX()+transLayerToParent.GetX(), 
						outTransform.GetTranslateY()+transLayerToParent.GetY());
		}
	}
}

/** return index of current layer 
@remarks
	index is zero-based
	return -1 if there is no layer
*/
sLONG VWinD2DGraphicContext::GetIndexCurrentLayer() const
{
	if (fStackLayerClip.size() == 0)
		return -1;

	FlushGDI();

	VStackLayerClip::const_reverse_iterator itLayer = fStackLayerClip.rbegin();
	sLONG index = fStackLayerClip.size()-1;
	for (;itLayer != fStackLayerClip.rend(); itLayer++, index--)
	{
		const VLayerClipElem& layer = *itLayer;
		if (layer.IsImageOffScreen() || layer.IsLayer())
			return index;
	}
	return -1;
}

/** return true if current layer is a valid bitmap layer (otherwise might be a null layer - for clipping only - or shadow or transparency layer) */
bool VWinD2DGraphicContext::IsCurrentLayerOffScreen() const
{
	if (fStackLayerClip.size() == 0)
		return false;
	//skip clipping 
	VStackLayerClip::const_reverse_iterator itLayer = fStackLayerClip.rbegin();
	for (;itLayer != fStackLayerClip.rend(); itLayer++)
	{
		const VLayerClipElem& layer = *itLayer;
		if (layer.IsImageOffScreen())
			return true;
		else if (layer.IsLayer())
			return false;
	}
	return false;
}


/** clear current graphic context area */
void VWinD2DGraphicContext::Clear( const VColor& inColor, const VRect *inBounds, bool inBlendCopy)
{
	StUseContext_NoRetain	context(this);
	FlushGDI();
	if (inBlendCopy && inColor.GetAlpha() != 255)
	{
		if (inBounds == NULL)
			fRenderTarget->Clear( D2D_COLOR(inColor));
		else
		{
			VAffineTransform ctm;
			GetTransform(ctm);
			fRenderTarget->SetTransform( D2D1::IdentityMatrix()); 

			VRect bounds = ctm.TransformRect(*inBounds);
			bounds.NormalizeToInt();
			fRenderTarget->PushAxisAlignedClip( D2D_RECT( bounds), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE); 
			fRenderTarget->Clear( D2D_COLOR(inColor));
			fRenderTarget->PopAxisAlignedClip();

			SetTransform(ctm);
		}
	}
	else
	{
		//blend over

		CComPtr<ID2D1SolidColorBrush> brush;
		if (FAILED(fRenderTarget->CreateSolidColorBrush( D2D_COLOR( inColor), &brush)))
			return;

		if (inBounds == NULL)
		{
			VAffineTransform ctm;
			GetTransform(ctm);
			fRenderTarget->SetTransform( D2D1::IdentityMatrix()); 

			D2D1_SIZE_F size = fRenderTarget->GetSize();
			D2D1_RECT_F bounds = D2D1::RectF( 0, 0, size.width, size.height);
			fRenderTarget->FillRectangle( bounds, brush);

			SetTransform(ctm);
		}
		else
		{
			VAffineTransform ctm;
			GetTransform(ctm);
			fRenderTarget->SetTransform( D2D1::IdentityMatrix()); 

			VRect bounds = ctm.TransformRect(*inBounds);
			bounds.NormalizeToInt();

			fRenderTarget->FillRectangle( D2D_RECT( bounds), brush);

			SetTransform(ctm);
		}
	}
}


void VWinD2DGraphicContext::QDFrameRect (const VRect& inHwndRect)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));
	fGDI_QDCompatible = true;

	VRect r=inHwndRect;
	GReal penSize = fStrokeWidth;
	if(penSize>1 || fPrinterScale)
	{
		r.SetPosBy((penSize/2.0),(penSize/2.0));
		r.SetSizeBy(-penSize,-penSize);
	}
	else
	{
		r.SetPosBy(0.5,0.5);
		r.SetSizeBy(-penSize,-penSize);
	}
	FrameRect(r);

	fGDI_QDCompatible = false;
}

void VWinD2DGraphicContext::QDFrameRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));
	fGDI_QDCompatible = true;

	VRect r=inHwndRect;
	GReal penSize = fStrokeWidth;
	if(penSize>1)
	{
		r.SetPosBy((penSize/2.0),(penSize/2.0));
		r.SetSizeBy(-penSize,-penSize);
	}
	else
	{
		r.SetPosBy(0.5,0.5);
		r.SetSizeBy(-penSize,-penSize);
	}
	FrameRoundRect(r,inOvalWidth,inOvalHeight);
	fGDI_QDCompatible = false;
}

void VWinD2DGraphicContext::QDFrameOval (const VRect& inHwndRect)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));
	fGDI_QDCompatible = true;

	VRect r=inHwndRect;
	GReal penSize = fStrokeWidth;
	if(penSize>1)
	{
		r.SetPosBy((penSize/2.0),(penSize/2.0));
		r.SetSizeBy(-penSize,-penSize);
	}
	else
	{
		r.SetPosBy(0.5,0.5);
		r.SetSizeBy(-penSize,-penSize);
	}
	FrameOval(r);
	fGDI_QDCompatible = false;
}

void VWinD2DGraphicContext::QDDrawRect (const VRect& inHwndRect)
{
	QDFillRect(inHwndRect);
	QDFrameRect(inHwndRect);
}

void VWinD2DGraphicContext::QDDrawRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)
{
	QDFillRoundRect (inHwndRect,inOvalWidth,inOvalHeight);
	QDFrameRoundRect (inHwndRect,inOvalWidth,inOvalHeight);
}

void VWinD2DGraphicContext::QDDrawOval (const VRect& inHwndRect)
{
	QDFillOval(inHwndRect);
	QDFrameOval(inHwndRect);
}

void VWinD2DGraphicContext::QDFillRect (const VRect& inHwndRect)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));
	fGDI_QDCompatible = true;

	FillRect(inHwndRect);

	fGDI_QDCompatible = false;
}

void VWinD2DGraphicContext::QDFillRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));
	fGDI_QDCompatible = true;

	FillRoundRect (inHwndRect,inOvalWidth,inOvalHeight);

	fGDI_QDCompatible = false;
}

void VWinD2DGraphicContext::QDFillOval (const VRect& inHwndRect)
{
	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));
	fGDI_QDCompatible = true;

	FillOval(inHwndRect);

	fGDI_QDCompatible = false;
}


void VWinD2DGraphicContext::QDDrawLine(const GReal inHwndStartX,const GReal inHwndStartY, const GReal inHwndEndX,const GReal inHwndEndY)
{
	StUseContext_NoRetain	context(this);

	if (fGDI_Fast && fGDI_HDC && fDrawingMode != DM_SHADOW && fGlobalAlpha == 1.0f 
		&& 
		!fGDI_HasStrokeCustomDashesInherit.back() && fGDI_HasSolidStrokeColorInherit.back())
	{
		fGDI_GC->QDDrawLine( inHwndStartX, inHwndStartY, inHwndEndX, inHwndEndY);
		return;
	}

	FlushGDI();

	//crisp Edges is not compatible with QD methods so disable it locally
	StDisableShapeCrispEdges disableCrispEdges(static_cast<VGraphicContext *>(this));
	fGDI_QDCompatible = true;

	GReal HwndStartX=inHwndStartX;
	GReal HwndStartY=inHwndStartY; 
	GReal HwndEndX=inHwndEndX;
	GReal HwndEndY=inHwndEndY;
	
	//override stroke style in order to be QD-compliant
	D2D1_STROKE_STYLE_PROPERTIES strokeStyleProps = fStrokeStyleProps;
	strokeStyleProps.startCap = D2D1_CAP_STYLE_FLAT;
	strokeStyleProps.endCap = D2D1_CAP_STYLE_FLAT;
	strokeStyleProps.dashCap = D2D1_CAP_STYLE_FLAT;

	CComPtr<ID2D1StrokeStyle> strokeStyleBackup = fStrokeStyle;
	fStrokeStyle = (ID2D1StrokeStyle *)NULL;
	fMutexFactory.Lock();
	HRESULT hr = GetFactory()->CreateStrokeStyle( 
			&strokeStyleProps,
			fStrokeDashCount ? fStrokeDashPattern : NULL, 
			fStrokeDashCount, 
			&fStrokeStyle);
	fMutexFactory.Unlock();
	xbox_assert(SUCCEEDED(hr) && fStrokeStyle);

	GReal penSize = fStrokeWidth;
	
	if (HwndStartX == HwndEndX) 
	{
		HwndStartX += penSize / 2.0;
		HwndEndX += penSize / 2.0;
	}
    else if (HwndStartY == HwndEndY) 
	{
		HwndStartY += penSize/2.0;
		HwndEndY += penSize/2.0;
	}
	
	DrawLine(HwndStartX,HwndStartY,HwndEndX,HwndEndY);
	
	//restore stroke style
	fStrokeStyle = strokeStyleBackup;

	fGDI_QDCompatible = false;
}

void VWinD2DGraphicContext::QDDrawLines(const GReal* inCoords, sLONG inCoordCount)
{
	if (inCoordCount<4)
		return;

	if (fGDI_Fast && fGDI_HDC && fDrawingMode != DM_SHADOW && fGlobalAlpha == 1.0f 
		&& 
		!fGDI_HasStrokeCustomDashesInherit.back() && fGDI_HasSolidStrokeColorInherit.back())
	{
		fGDI_GC->QDDrawLines( inCoords, inCoordCount);
		return;
	}

	FlushGDI();

	StUseContext_NoRetain	context(this);

	GReal lastX = inCoords[0];
	GReal lastY = inCoords[1];
	for(VIndex n=2;n<inCoordCount;n+=2)
	{
		QDDrawLine(lastX,lastY, inCoords[n],inCoords[n+1]);
		lastX = inCoords[n];
		lastY = inCoords[n+1];
	}
}

void VWinD2DGraphicContext::QDMoveBrushTo (const VPoint& inHwndPos)
{
	MoveBrushTo (inHwndPos);
}
void VWinD2DGraphicContext::QDDrawLine (const VPoint& inHwndStart, const VPoint& inHwndEnd)
{
	QDDrawLine (inHwndStart.GetX(),inHwndStart.GetY(),inHwndEnd.GetX(),inHwndEnd.GetY());
}
void VWinD2DGraphicContext::QDDrawLineTo (const VPoint& inHwndEnd)
{
	QDDrawLine(fBrushPos,inHwndEnd);
	fBrushPos=inHwndEnd;
}


#pragma mark-

void VWinD2DGraphicContext::SelectBrushDirect(HGDIOBJ inBrush)
{
}


void VWinD2DGraphicContext::SelectPenDirect(HGDIOBJ inPen)
{
}


/** build state from passed gc */
VD2DDrawingState::VD2DDrawingState(const VWinD2DGraphicContext *inGC)
{
	//save native drawing state
	if (testAssert(inGC->fRenderTarget && inGC->fUseCount > 0 && inGC->fLockCount <= 0)) //only if we are inside BeginDraw/EndDraw
	{
		inGC->fMutexFactory.Lock();
		HRESULT hr = inGC->GetFactory()->CreateDrawingStateBlock(&fDrawingStateNative);
		xbox_assert(SUCCEEDED(hr));
		inGC->fMutexFactory.Unlock();
		inGC->fRenderTarget->SaveDrawingState(fDrawingStateNative);
	}

	//save impl drawing state

	fMaxPerfFlag = inGC->fMaxPerfFlag;
	fIsHighQualityAntialiased = inGC->fIsHighQualityAntialiased;
	fHighQualityTextRenderingMode = inGC->fHighQualityTextRenderingMode;
	fFillRule = inGC->fFillRule;
	fCharKerning = inGC->fCharKerning;
	fCharSpacing = inGC->fCharSpacing;
	fUseShapeBrushForText = inGC->fUseShapeBrushForText;
	fShapeCrispEdgesEnabled = inGC->fShapeCrispEdgesEnabled;
	fAllowCrispEdgesOnPath = inGC->fAllowCrispEdgesOnPath;
	fAllowCrispEdgesOnBezier = inGC->fAllowCrispEdgesOnBezier;

	fStrokeBrush = inGC->fStrokeBrush;
	fStrokeStyle = inGC->fStrokeStyle;
	fStrokeStyleProps = inGC->fStrokeStyleProps;
	fStrokeDashPatternInit = inGC->fStrokeDashPatternInit;
	fStrokeWidth = inGC->fStrokeWidth;
	fIsStrokeDotPattern = inGC->fIsStrokeDotPattern;
	fIsStrokeDashPatternUnitEqualToStrokeWidth = inGC->fIsStrokeDashPatternUnitEqualToStrokeWidth;

	fFillBrush = inGC->fFillBrush;
	fTextBrush = inGC->fTextBrush;

	fStrokeBrushSolid = inGC->fStrokeBrushSolid;
	fFillBrushSolid = inGC->fFillBrushSolid;
	fTextBrushSolid = inGC->fTextBrushSolid;

	fStrokeBrushLinearGradient = inGC->fStrokeBrushLinearGradient;
	fStrokeBrushRadialGradient = inGC->fStrokeBrushRadialGradient;

	fFillBrushLinearGradient = inGC->fFillBrushLinearGradient;
	fFillBrushRadialGradient = inGC->fFillBrushRadialGradient;

	fTextFont = NULL;
	CopyRefCountable(&fTextFont, inGC->fTextFont);
	fTextFontMetrics = inGC->fTextFontMetrics ? new VFontMetrics( *(inGC->fTextFontMetrics)) : NULL;
	fWhiteSpaceWidth = inGC->fWhiteSpaceWidth;
	fTextRect = inGC->fTextRect;
	fBrushPos = inGC->fBrushPos;

	fDrawingMode = inGC->fDrawingMode;
	fTextRenderingMode = inGC->fTextRenderingMode;
	fTextMeasuringMode = inGC->fTextMeasuringMode;

	fGlobalAlpha = inGC->fGlobalAlpha;
	fShadowHOffset = inGC->fShadowHOffset;
	fShadowVOffset = inGC->fShadowVOffset;
	fShadowBlurness = inGC->fShadowBlurness;
}

VD2DDrawingState::~VD2DDrawingState()
{
	ReleaseRefCountable(&fTextFont);
	if (fTextFontMetrics)
		delete fTextFontMetrics;
}

/** copy constructor */
VD2DDrawingState::VD2DDrawingState( const VD2DDrawingState& inSource)
{
	fDrawingStateNative = inSource.fDrawingStateNative;

	fMaxPerfFlag = inSource.fMaxPerfFlag;
	fIsHighQualityAntialiased = inSource.fIsHighQualityAntialiased;
	fHighQualityTextRenderingMode = inSource.fHighQualityTextRenderingMode;
	fFillRule = inSource.fFillRule;
	fCharKerning = inSource.fCharKerning;
	fCharSpacing = inSource.fCharSpacing;
	fUseShapeBrushForText = inSource.fUseShapeBrushForText;
	fShapeCrispEdgesEnabled = inSource.fShapeCrispEdgesEnabled;
	fAllowCrispEdgesOnPath = inSource.fAllowCrispEdgesOnPath;
	fAllowCrispEdgesOnBezier = inSource.fAllowCrispEdgesOnBezier;

	fStrokeBrush = inSource.fStrokeBrush;
	fStrokeStyle = inSource.fStrokeStyle;
	fStrokeStyleProps = inSource.fStrokeStyleProps;
	fStrokeDashPatternInit = inSource.fStrokeDashPatternInit;
	fStrokeWidth = inSource.fStrokeWidth;
	fIsStrokeDotPattern = inSource.fIsStrokeDotPattern;
	fIsStrokeDashPatternUnitEqualToStrokeWidth = inSource.fIsStrokeDashPatternUnitEqualToStrokeWidth;

	fFillBrush = inSource.fFillBrush;
	fTextBrush = inSource.fTextBrush;

	fStrokeBrushSolid = inSource.fStrokeBrushSolid;
	fFillBrushSolid = inSource.fFillBrushSolid;
	fTextBrushSolid = inSource.fTextBrushSolid;

	fStrokeBrushLinearGradient = inSource.fStrokeBrushLinearGradient;
	fStrokeBrushRadialGradient = inSource.fStrokeBrushRadialGradient;

	fFillBrushLinearGradient = inSource.fFillBrushLinearGradient;
	fFillBrushRadialGradient = inSource.fFillBrushRadialGradient;

	fTextFont = NULL;
	CopyRefCountable(&fTextFont, inSource.fTextFont);
	fTextFontMetrics = inSource.fTextFontMetrics ? new VFontMetrics( *(inSource.fTextFontMetrics)) : NULL;
	fWhiteSpaceWidth = inSource.fWhiteSpaceWidth;
	fTextRect = inSource.fTextRect;
	fBrushPos = inSource.fBrushPos;

	fDrawingMode = inSource.fDrawingMode;
	fTextRenderingMode = inSource.fTextRenderingMode;
	fTextMeasuringMode = inSource.fTextMeasuringMode;

	fGlobalAlpha = inSource.fGlobalAlpha;
	fShadowHOffset = inSource.fShadowHOffset;
	fShadowVOffset = inSource.fShadowVOffset;
	fShadowBlurness = inSource.fShadowBlurness;
}

/** restore state to passed gc */
void VD2DDrawingState::Restore(const VWinD2DGraphicContext *inGC)
{
	//restore native drawing context
	if (testAssert(inGC->fRenderTarget != NULL && inGC->fUseCount > 0 && inGC->fLockCount <= 0)) //only if we are inside BeginDraw/EndDraw
		inGC->fRenderTarget->RestoreDrawingState( fDrawingStateNative);

	//restore impl drawing context

	inGC->fMaxPerfFlag = fMaxPerfFlag;
	inGC->fIsHighQualityAntialiased = fIsHighQualityAntialiased;
	inGC->fHighQualityTextRenderingMode = fHighQualityTextRenderingMode;
	inGC->fFillRule = fFillRule;
	inGC->fCharKerning = fCharKerning;
	inGC->fCharSpacing = fCharSpacing;
	inGC->fUseShapeBrushForText = fUseShapeBrushForText;
	inGC->fShapeCrispEdgesEnabled = fShapeCrispEdgesEnabled;
	inGC->fAllowCrispEdgesOnPath = fAllowCrispEdgesOnPath;
	inGC->fAllowCrispEdgesOnBezier = fAllowCrispEdgesOnBezier;

	inGC->fStrokeBrush = fStrokeBrush;
	inGC->fStrokeStyle = fStrokeStyle;
	inGC->fStrokeStyleProps = fStrokeStyleProps;
	inGC->fStrokeDashPatternInit = fStrokeDashPatternInit;
	inGC->fStrokeWidth = fStrokeWidth;
	inGC->fIsStrokeDotPattern = fIsStrokeDotPattern;
	inGC->fIsStrokeDashPatternUnitEqualToStrokeWidth = fIsStrokeDashPatternUnitEqualToStrokeWidth;

	if (fStrokeDashPatternInit.size() > inGC->fStrokeDashCountInit)
	{
		inGC->fStrokeDashCountInit = fStrokeDashPatternInit.size();
		if (inGC->fStrokeDashPattern)
			delete [] inGC->fStrokeDashPattern;
		inGC->fStrokeDashPattern = new FLOAT[ inGC->fStrokeDashCountInit];
	}
	xbox_assert(!(inGC->fStrokeDashCountInit) || inGC->fStrokeDashPattern);
	inGC->fStrokeDashCount = fStrokeDashPatternInit.size();
	
	VLineDashPattern::const_iterator it = fStrokeDashPatternInit.begin();
	FLOAT *patternValue = inGC->fStrokeDashPattern;
	if (fIsStrokeDashPatternUnitEqualToStrokeWidth || fIsStrokeDotPattern)
	{
		for (;it != fStrokeDashPatternInit.end(); it++, patternValue++)
		{
			*patternValue = (FLOAT)(*it); //D2D pattern unit is equal to pen width 
			
			//workaround for D2D dash pattern bug
			if (*patternValue == 0.0f)
				*patternValue = 0.0000001f;
		}
	}
	else
		for (;it != fStrokeDashPatternInit.end(); it++, patternValue++)
		{
			*patternValue = (FLOAT)(*it) / (fStrokeWidth == 0.0f ? 0.0001f : fStrokeWidth); //D2D pattern unit is equal to pen width 
														 //so as we use absolute user units divide first by pen width	
			//workaround for D2D dash pattern bug
			if (*patternValue == 0.0f)
				*patternValue = 0.0000001f;
		}

	inGC->fFillBrush = fFillBrush;
	inGC->fTextBrush = fTextBrush;

	inGC->fStrokeBrushSolid = fStrokeBrushSolid;
	inGC->fFillBrushSolid = fFillBrushSolid;
	inGC->fTextBrushSolid = fTextBrushSolid;

	inGC->fStrokeBrushLinearGradient = fStrokeBrushLinearGradient;
	inGC->fStrokeBrushRadialGradient = fStrokeBrushRadialGradient;

	inGC->fFillBrushLinearGradient = fFillBrushLinearGradient;
	inGC->fFillBrushRadialGradient = fFillBrushRadialGradient;

	CopyRefCountable(&(inGC->fTextFont), fTextFont);
	if (inGC->fTextFontMetrics)
		delete inGC->fTextFontMetrics;
	inGC->fTextFontMetrics = fTextFontMetrics ? new VFontMetrics( *(fTextFontMetrics)) : NULL;
	inGC->fWhiteSpaceWidth = fWhiteSpaceWidth;
	inGC->fTextRect = fTextRect;
	inGC->fBrushPos = fBrushPos;

	inGC->fDrawingMode = fDrawingMode;
	inGC->fTextRenderingMode = fTextRenderingMode;
	inGC->fTextMeasuringMode = fTextMeasuringMode;

	inGC->fGlobalAlpha = fGlobalAlpha;
	inGC->fShadowHOffset = fShadowHOffset;
	inGC->fShadowVOffset = fShadowVOffset;
	inGC->fShadowBlurness = fShadowBlurness;
}


/** save drawing state from passed context */
void VD2DDrawingContext::Save(const VWinD2DGraphicContext *inGC)
{
	xbox_assert(inGC->fRenderTarget);
	xbox_assert(inGC->fUseCount > 0);
	fDrawingState.push_back( VD2DDrawingState(inGC));
}

/** restore drawing state to passed context */
void VD2DDrawingContext::Restore(const VWinD2DGraphicContext *inGC)
{
	if (!testAssert(!fDrawingState.empty()))
		return;
	xbox_assert(inGC->fRenderTarget);
	xbox_assert(inGC->fUseCount > 0);
	
	fDrawingState.back().Restore( inGC);
	fDrawingState.pop_back();
}


#pragma mark-

StD2DUseHDC::StD2DUseHDC(ID2D1RenderTarget* inRT)
{
	fHDC = NULL;
	if(inRT)
	{
		fRenderTarget = inRT;
		
		CComPtr<ID2D1GdiInteropRenderTarget> rtGDI;
		fRenderTarget->QueryInterface( &rtGDI);
		
		rtGDI->GetDC( D2D1_DC_INITIALIZE_MODE_COPY, &fHDC);
	}
}
StD2DUseHDC::~StD2DUseHDC()
{
	if(fRenderTarget != NULL && fHDC)
	{
		CComPtr<ID2D1GdiInteropRenderTarget> rtGDI;
		fRenderTarget->QueryInterface( &rtGDI);
		
		rtGDI->ReleaseDC( NULL);
	}
}

#endif


