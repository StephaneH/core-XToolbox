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
#ifndef __IRefCountable__
#define __IRefCountable__

#include "Kernel/Sources/VKernelTypes.h"
#include "Kernel/Sources/VTextTypes.h"


BEGIN_TOOLBOX_NAMESPACE

/*!
	@class	IRefCountable
	@discussion
	At creation, the refcount is set to 1.
	At destruction, the refcount must still be 1.
	Retain and Release are thread-safe.

*/


template <class Type>
void CopyRefCountable (Type** ioCopy, Type* ioOriginal, const char* /*DebugInfoCopy*/ = NULL, const char* /*DebugInfoOriginal*/ = NULL)
{
	if (ioCopy != NULL && ioOriginal != *ioCopy)
	{
		if (ioOriginal != NULL)
			ioOriginal->Retain();

		// Copy before releasing to avoid ioCopy refering a released object (thread safe)
		Type*	toRelease = *ioCopy;
		*ioCopy = ioOriginal;

		if (toRelease != NULL)
			toRelease->Release();
	}
}


template<class Type>
void ReleaseRefCountable( Type** ioToRelease, const char* DebugInfo = NULL)
{
	Type *toRelease = *ioToRelease;	// temp to clear ioToRelease before Release()
	*ioToRelease = NULL;

	if (toRelease != NULL)
	{
		toRelease->Release(DebugInfo);
	}
}


template<class Type>
void ReleaseRef( Type** ioToRelease, const char* DebugInfo = NULL)
{
	Type *toRelease = *ioToRelease;	// temp to clear ioToRelease before Release()
	*ioToRelease = NULL;

	if (toRelease != NULL)
	{
		toRelease->Release(DebugInfo);
	}
}


template<class Type>
void QuickReleaseRefCountable( Type* ioToRelease, const char* DebugInfo = NULL)
{
	if (ioToRelease != NULL)
	{
		ioToRelease->Release(DebugInfo);
	}
}


template<class Type>
Type* RetainRefCountable(Type *inToRetain, const char* DebugInfo = NULL)
{
	if (inToRetain != NULL)
		inToRetain->Retain(DebugInfo);
	return inToRetain;
}


template<class Type>
Type* RetainRef(Type *inToRetain, const char* DebugInfo = NULL)
{
	if (inToRetain != NULL)
		inToRetain->Retain(DebugInfo);
	return inToRetain;
}


class XTOOLBOX_API IRefCountable
{
public:
#if VERSIONDEBUG
						IRefCountable();
#else
						IRefCountable():fRefCount( 1)				{;}
#endif
	virtual				~IRefCountable();

	// Using refcount value returned by Retain and Release is dangerous
	// as it's opened door to thread _unsafe_ code. However it is supported
	// for convenience as the value is the exact refcount at calling time.
	virtual	sLONG		Retain(const char* DebugInfo = 0) const;
	virtual	sLONG		Release(const char* DebugInfo = 0) const;

	/*
		To help cut circular dependencies, you should call ReleaseDependencies()
		before releasing an object that you know has stopped being usable.
		The implementor of ReleaseDependencies() should Release any dependency.
	*/
	virtual	void		ReleaseDependencies();

	// Accessors
			sLONG		GetRefCount() const							{ return fRefCount; }

	// Utilities
	template<class Type>
	static	void		Copy( Type** ioCopy, Type* ioOriginal)
	{
		XBOX::CopyRefCountable( ioCopy, ioOriginal);
	}

protected:
	// Override to handle last release (e.g. dispose yourseft)
	virtual	void		DoOnRefCountZero ();

private:
	mutable	sLONG		fRefCount;
	#if VERSIONDEBUG
	mutable	uLONG		fTag;	// to help at debugging
	static	uLONG		fBreakTag;	// to help at debugging
	#endif
};



class XTOOLBOX_API INonVirtualRefCountable
{
public:
	INonVirtualRefCountable():fRefCount( 1)				{;}
	~INonVirtualRefCountable();

	sLONG		Retain() const;
	sLONG		Release() const;

	sLONG		GetRefCount() const							{ return fRefCount; }

	template<class Type>
	static	void		Copy( Type** ioCopy, Type* ioOriginal)
	{
		XBOX::CopyRefCountable( ioCopy, ioOriginal);
	}

private:
	mutable	sLONG		fRefCount;
};

/*
	Release an object that has stopped being usable.
	Hence the call to ReleaseDependencies before Release() in an attempt to cut possible circular dependencies.
*/
template<class Type>
void DisposeRefCountable( Type** ioToDispose, const char* DebugInfo = NULL)
{
	Type *toDispose = *ioToDispose;	// temp to clear ioToKill before Release()
	*ioToDispose = NULL;

	if (toDispose != NULL)
	{
		toDispose->ReleaseDependencies();
		toDispose->Release(DebugInfo);
	}
}


template <class Type>
class VRefPtr
{
public:
	VRefPtr() : fData(NULL)
	{
	}

	VRefPtr( Type* inData) : fData(inData)
	{
		if (fData != NULL)
			fData->Retain();
	}

	VRefPtr( Type* inData, bool inRetainIt) : fData(inData)
	{
		if (inRetainIt && fData != NULL)
			fData->Retain();
	}

	VRefPtr( const VRefPtr& inOther) : fData( inOther.fData)
	{
		if (fData != NULL)
			fData->Retain();
	}

	~VRefPtr()
	{
		if (fData != NULL)
			fData->Release();
	}


	VRefPtr& operator = ( const VRefPtr& inOther)
	{
		XBOX::CopyRefCountable( &fData, inOther.fData);

		return *this;
	}


	Type& operator * () const						{ return *fData; }
	Type* operator -> () const						{ return fData; }

	bool operator!=(const VRefPtr& inOther) const	{ return fData != inOther.fData; }
	bool operator==(const VRefPtr& inOther) const	{ return fData == inOther.fData; }

	bool operator!=(const Type* inOther) const		{ return fData != inOther; }
	bool operator==(const Type* inOther) const		{ return fData == inOther; }

	operator Type*() const							{ return fData; }
	Type* Get() const								{ return fData; };
	bool IsNull() const								{ return fData == NULL; }

	Type* Forget()
	{
		Type*	temp = fData;
		fData = NULL;
		return temp;
	}

	void Adopt( Type *inData)
	{
		if (fData != NULL/* && inData != fData*/)
#if 0 && VERSIONDEBUG
			if (inData == fData)
			{
				assert(inData->GetRefCount() > 1);
			}
#endif
			fData->Release();
            
		fData = inData;
	}

	void Set( Type *inData)
	{
		XBOX::CopyRefCountable( &fData, inData);
	}

	Type* Retain() const
	{
		if (fData != NULL)
			fData->Retain();
		return fData;
	}

	void Clear()
	{
		if (fData != NULL)
			fData->Release();
		fData = NULL;
	}
 
private:
    Type*	fData;
};


template <class Type>
bool operator == (const VRefPtr<Type>& a, const VRefPtr<Type>& b)
{
	return (a.Get() == b.Get());
}



END_TOOLBOX_NAMESPACE

#endif
