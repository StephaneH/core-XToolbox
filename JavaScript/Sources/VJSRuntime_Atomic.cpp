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
#include "VJavaScriptPrecompiled.h"

#include "VJSValue.h"
#include "VJSContext.h"
#include "VJSClass.h"

#include "VJSRuntime_Atomic.h"

USING_TOOLBOX_NAMESPACE


VCriticalSection jsAtomicSection::sMapMutex;
jsAtomicSection::AtomicSectionMap jsAtomicSection::sAtomicSections;


jsAtomicSection* jsAtomicSection::RetainAtomicSection(const VString& inSectionName)
{
	jsAtomicSection* result = nil;
	VTaskLock lock(&sMapMutex);
	AtomicSectionMap::iterator found = sAtomicSections.find(inSectionName);
	if (found == sAtomicSections.end())
	{
		jsAtomicSection* newsection = new jsAtomicSection();
		sAtomicSections[inSectionName] = newsection;
		result = RetainRefCountable(newsection);
	}
	else
	{
		result = RetainRefCountable(found->second);
	}
	return result;
}

void jsAtomicSection::EraseAtomicSection(const VString& inSectionName)
{
	VTaskLock lock(&sMapMutex);
	AtomicSectionMap::iterator found = sAtomicSections.find(inSectionName);
	if (found != sAtomicSections.end())
	{
		found->second->Release();
		sAtomicSections.erase(found);
	}
}


void VJSAtomicSection::Initialize( const VJSParms_initialize& inParms, jsAtomicSection* inSection)
{
	inSection->Retain();
}


void VJSAtomicSection::Finalize( const VJSParms_finalize& inParms, jsAtomicSection* inSection)
{
	inSection->Release();
}


void VJSAtomicSection::_Lock( VJSParms_callStaticFunction& ioParms, jsAtomicSection* inSection)
{
	inSection->fMutex.Lock();
	ioParms.ReturnBool(true);
}

void VJSAtomicSection::_TryToLock( VJSParms_callStaticFunction& ioParms, jsAtomicSection* inSection)
{
	ioParms.ReturnBool(inSection->fMutex.TryToLock());
}


void VJSAtomicSection::_Unlock( VJSParms_callStaticFunction& ioParms, jsAtomicSection* inSection)
{
	inSection->fMutex.Unlock();
	ioParms.ReturnBool(true);
}


void VJSAtomicSection::GetDefinition( ClassDefinition& outDefinition)
{
	static inherited::StaticFunction functions[] = 
	{
		{ "lock", js_callStaticFunction<_Lock>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "tryToLock", js_callStaticFunction<_TryToLock>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "unlock", js_callStaticFunction<_Unlock>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ 0, 0, 0}
	};

	outDefinition.className = "Mutex";
	outDefinition.initialize = js_initialize<Initialize>;
	outDefinition.finalize = js_finalize<Finalize>;
	outDefinition.staticFunctions = functions;
}




// ----------------------------------------------------------------------------------



VCriticalSection jsSyncEvent::sMapMutex;
jsSyncEvent::SyncEventMap jsSyncEvent::sSyncEvents;


jsSyncEvent* jsSyncEvent::RetainSyncEvent(const VString& inSectionName)
{
	jsSyncEvent* result = nil;
	VTaskLock lock(&sMapMutex);
	SyncEventMap::iterator found = sSyncEvents.find(inSectionName);
	if (found == sSyncEvents.end())
	{
		jsSyncEvent* newsection = new jsSyncEvent();
		sSyncEvents[inSectionName] = newsection;
		result = RetainRefCountable(newsection);
	}
	else
	{
		result = RetainRefCountable(found->second);
	}
	return result;
}


void jsSyncEvent::EraseSyncEvent(const VString& inSectionName)
{
	VTaskLock lock(&sMapMutex);
	SyncEventMap::iterator found = sSyncEvents.find(inSectionName);
	if (found != sSyncEvents.end())
	{
		found->second->Release();
		sSyncEvents.erase(found);
	}
}



void VJSSyncEvent::Initialize( const VJSParms_initialize& inParms, jsSyncEvent* inSection)
{
	inSection->Retain();
}


void VJSSyncEvent::Finalize( const VJSParms_finalize& inParms, jsSyncEvent* inSection)
{
	inSection->Release();
}



void VJSSyncEvent::_Wait(VJSParms_callStaticFunction& ioParms, jsSyncEvent* inSection)
{
	sLONG nbmill = 0;
	if (ioParms.CountParams() > 0 && !ioParms.IsNumberParam(1))
	{
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
	}
	ioParms.GetLongParam(1, &nbmill);
	if (nbmill == 0)
		ioParms.ReturnBool(inSection->fSync.Lock());
	else
		ioParms.ReturnBool(inSection->fSync.Lock(nbmill));
}


void VJSSyncEvent::_TryToWait(VJSParms_callStaticFunction& ioParms, jsSyncEvent* inSection)
{
	ioParms.ReturnBool(inSection->fSync.TryToLock());
}


void VJSSyncEvent::_Signal(VJSParms_callStaticFunction& ioParms, jsSyncEvent* inSection)
{
	ioParms.ReturnBool(inSection->fSync.Unlock());
}


void VJSSyncEvent::_Reset(VJSParms_callStaticFunction& ioParms, jsSyncEvent* inSection)
{
	ioParms.ReturnBool(inSection->fSync.Reset());
}


void VJSSyncEvent::GetDefinition( ClassDefinition& outDefinition)
{
	static inherited::StaticFunction functions[] = 
	{
		{ "wait", js_callStaticFunction<_Wait>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "waitForSignal", js_callStaticFunction<_Wait>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "tryToWait", js_callStaticFunction<_TryToWait>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "signal", js_callStaticFunction<_Signal>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "unlock", js_callStaticFunction<_Signal>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "reset", js_callStaticFunction<_Reset>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ 0, 0, 0}
	};

	outDefinition.className = "SyncEvent";
	outDefinition.initialize = js_initialize<Initialize>;
	outDefinition.finalize = js_finalize<Finalize>;
	outDefinition.staticFunctions = functions;
}

