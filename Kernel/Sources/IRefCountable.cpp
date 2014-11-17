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
#include "VKernelPrecompiled.h"
#include "IRefCountable.h"
#include "VTask.h"
#include "VInterlocked.h"
#include "VRefCountDebug.h"

#if VERSIONDEBUG
uLONG IRefCountable::fBreakTag = 0;
#endif

#if WITH_REFCOUNT_DEBUG
// l'ordre d'initialisation des globales n'etant pas garanti, IRefCountable peut etre appele avant que gRefCountDebug ne soit initialise. Et ca fait crasher.
VRefCountDebug gRefCountDebug;
#endif


#if VERSIONDEBUG

IRefCountable::IRefCountable()
{
	fRefCount = 1;
//	gRefCountDebug.RetainInfo(this, "new");
}

#endif

IRefCountable::~IRefCountable()
{
	// As the object may be deleted with 'delete' operator
	// refcount may be 1 in this case. No more set the refcount
	// to 1 in DoOnRefCountZero as it is confusing.
	xbox_assert(GetRefCount() == 0 || GetRefCount() == 1);
#if WITH_REFCOUNT_DEBUG
	gRefCountDebug.ReleaseInfo(this);
#endif
}



sLONG IRefCountable::Retain(const char* DebugInfo) const
{
	xbox_assert(GetRefCount() < kMAX_sLONG);
	xbox_assert( fRefCount > 0);
	if (fRefCount == 0)
	{
		sLONG a = 1;
	}

#if VERSIONDEBUG
	xbox_assert( (fBreakTag == 0) || (fBreakTag != fTag));
#endif

	(void) DebugInfo; // avoir compiler warning
#if WITH_REFCOUNT_DEBUG
	if (DebugInfo != NULL)
	{
		gRefCountDebug.RetainInfo(this, DebugInfo);
	}
#endif
	return VInterlocked::Increment( &fRefCount);
}


sLONG IRefCountable::Release(const char* DebugInfo) const
{
	xbox_assert(GetRefCount() > 0);
	#if VERSIONDEBUG
	xbox_assert( (fBreakTag == 0) || (fBreakTag != fTag));
	#endif
	(void) DebugInfo; // avoid compiler warning
#if WITH_REFCOUNT_DEBUG
	if (DebugInfo != NULL)
	{
		gRefCountDebug.ReleaseInfo(this, DebugInfo);
	}
#endif
	sLONG newCount = VInterlocked::Decrement( &fRefCount);
	if (newCount == 0)
		const_cast<IRefCountable *>(this)->DoOnRefCountZero();

	return newCount;
}


void IRefCountable::DoOnRefCountZero()
{
	delete this;
}


void IRefCountable::ReleaseDependencies()
{
	/*
		This object is no longer usable (it has already being 'closed' and will soon be released by its owner).
		
		You should release now any pending dependency to cut possible circular dependencies.
	*/
}



			/* -------------------------------------- */


INonVirtualRefCountable::~INonVirtualRefCountable()
{
	xbox_assert(GetRefCount() == 0 || GetRefCount() == 1);
}


sLONG INonVirtualRefCountable::Retain() const
{
	xbox_assert(GetRefCount() < kMAX_sLONG);
	return VInterlocked::Increment( &fRefCount);
}


sLONG INonVirtualRefCountable::Release() const
{
	xbox_assert(GetRefCount() > 0);
	sLONG newCount = VInterlocked::Decrement( &fRefCount);
	if (newCount == 0)
		delete const_cast<INonVirtualRefCountable *>(this);

	return newCount;
}




