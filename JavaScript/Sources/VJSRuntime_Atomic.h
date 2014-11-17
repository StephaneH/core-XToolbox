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
#ifndef __VJSRuntime_Atomic__
#define __VJSRuntime_Atomic__

#include "JS4D.h"
#include "VJSClass.h"

BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API jsAtomicSection : public XBOX::IRefCountable
{
	public:
		XBOX::VCriticalSection fMutex;

		static jsAtomicSection* RetainAtomicSection(const VString& inSectionName);

		static void EraseAtomicSection(const VString& inSectionName);

	private:
		typedef std::map<XBOX::VString, jsAtomicSection*> AtomicSectionMap;

		static VCriticalSection sMapMutex;
		static AtomicSectionMap sAtomicSections;

};


class XTOOLBOX_API VJSAtomicSection : public VJSClass<VJSAtomicSection, jsAtomicSection>
{
public:
	typedef VJSClass<VJSAtomicSection, jsAtomicSection> inherited;

	static	void			Initialize( const VJSParms_initialize& inParms, jsAtomicSection* inSection);
	static	void			Finalize( const VJSParms_finalize& inParms, jsAtomicSection* inSection);
	static	void			GetDefinition( ClassDefinition& outDefinition);

	static void _Lock(VJSParms_callStaticFunction& ioParms, jsAtomicSection* inSection);
	static void _TryToLock(VJSParms_callStaticFunction& ioParms, jsAtomicSection* inSection);
	static void _Unlock(VJSParms_callStaticFunction& ioParms, jsAtomicSection* inSection);
};


// ------------------------------------------------------------------------------------------------------



class XTOOLBOX_API jsSyncEvent : public XBOX::IRefCountable
{
public:
	XBOX::VSyncEvent fSync;

	static jsSyncEvent* RetainSyncEvent(const VString& inSectionName);

	static void EraseSyncEvent(const VString& inSectionName);

private:
	typedef std::map<XBOX::VString, jsSyncEvent*> SyncEventMap;

	static VCriticalSection sMapMutex;
	static SyncEventMap sSyncEvents;

};



class XTOOLBOX_API VJSSyncEvent : public VJSClass<VJSSyncEvent, jsSyncEvent>
{
public:
	typedef VJSClass<VJSSyncEvent, jsSyncEvent> inherited;

	static	void			Initialize( const VJSParms_initialize& inParms, jsSyncEvent* inSection);
	static	void			Finalize( const VJSParms_finalize& inParms, jsSyncEvent* inSection);
	static	void			GetDefinition( ClassDefinition& outDefinition);

	static void _Wait(VJSParms_callStaticFunction& ioParms, jsSyncEvent* inSection);
	static void _TryToWait(VJSParms_callStaticFunction& ioParms, jsSyncEvent* inSection);
	static void _Signal(VJSParms_callStaticFunction& ioParms, jsSyncEvent* inSection);
	static void _Reset(VJSParms_callStaticFunction& ioParms, jsSyncEvent* inSection);
};




END_TOOLBOX_NAMESPACE

#endif