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
#ifndef __VArray__
#define __VArray__

#include "Kernel/Sources/VIterator.h"
#include "Kernel/Sources/VError.h"
#include "Kernel/Sources/VMemoryCpp.h"

#if VERSION_LINUX
    #include <memory>
#endif

BEGIN_TOOLBOX_NAMESPACE

// Defined bellow
template<class Type> class VArrayIteratorOf;


/*!
	@class	VArrayOf<>
	@abstract	Template-based vector of values.
	@discussion
		A VArrayOf<T> is an array of elements of type T.
		It is very much like the std++ vector template.
		
		Use VArrayPtrOf if you want to use pointers to objects.
		Use VArrayRetainedPtrOf if you want to use pointers to IRefCountable objects.
*/

template<class Type>
class VArrayOf : public VObject
{
public:

	typedef Type* Iterator;
	typedef const Type* ConstIterator;
	/*!
		@function	VArrayOf
		@abstract	Creates an empty array.
		@param inInitialCount the array may be created with a initial number of empty elements
		@param inInitialAllocation number of slots to preallocate (must be >= inInitialCount)
		@param inAllocationIncrement increment used to enlarge array buffer
	*/
	VArrayOf (VIndex inInitialCount = 0, VIndex inInitialAllocation = 0, VIndex inAllocationIncrement = 10);
	VArrayOf (const VArrayOf& inOriginal);


	/*!
		@function	~VArrayOf
		@abstract	Destructor.
		@discussion
			Calls ::Destroy();
		
	*/
	virtual ~VArrayOf ();

	/*!
		@function	NewIterator
		@abstract	Returns a new iterator of the proper kind.
		@discussion
			Its use should be rare since you can simply write:
			for (VIndex i = 0 ; i <= array.GetCount() ; ++i)
				oneElem = array[i];
		@param inIndex Optional start index for iterator.
		
	*/
	VArrayIteratorOf<Type>*	NewIterator (VIndex inIndex = -1) const;
	
	/*!
		@function SetOwnership
		@abstract Ownership attribute support
		@discussion
			By default, the array does not delete its elements
			call SetOwnership(true) so that elements are destroyed automatically
	*/
	void	SetOwnership (Boolean inSet) { fIsOwner = inSet; };
	Boolean	GetOwnership () const { return fIsOwner; };

	/*!
		@function	operator[]
		@abstract	Indexing operator.
		@discussion
			Can be used for get and set. Index starts at 0.
			It is an error to pass an out of range index (assert).
		@param inIndex Must be >= 0 and < GetCount ().
		
	*/
	Type&	operator[] (VIndex inIndex) { assert(inIndex >= 0 && inIndex < fCount); return fFirst[inIndex]; };
	const Type&	operator[] (VIndex inIndex) const { assert(inIndex >= 0 && inIndex < fCount); return fFirst[inIndex]; };

	/*!
		@function	GetNth
		@abstract	Returns a nth element.
		@discussion
			If the index is out of range, a new empty value is returned silently.
		@param inIndex must be >= 1 and <= GetCount().
	*/
	Type	GetNth (VIndex inIndex) const;
	Type	GetFirst () const { return GetNth(1); };
	Type	GetLast () const { return GetNth(fCount); };

	/*!
		@function	SetNth
		@abstract	Sets a nth element.
		@discussion
			If the index is out of range, a new empty value is returned silently.
		@param inData Value to be copied.
		@param inIndex Must be >= 1 and <= GetCount().
	*/
	void	SetNth (const Type& inData, VIndex inIndex);

	/*!
		@function	AddNth
		@abstract	Insert a new element.
		@discussion
			If the index is out of range, a new empty value is returned silently.
		@param inData Value to be inserted.
		@param inAtIndex The index given to the new element once inserted (1-based).
	*/
	Boolean	AddNth (const Type& inData, VIndex inAtIndex);
	Boolean	AddAfter (const Type& inData,const Type& inAfter) { return AddNth(inData, FindPos(inAfter) + 1); };
	Boolean	AddBefore (const Type& inData,const Type& inBefore)	{ return AddNth(inData, FindPos(inBefore)); };
	Boolean	AddHead (const Type& inData) { return AddNth(inData, 1); };
	Boolean	AddTail (const Type& inData) { return AddNth(inData, fCount + 1); };

#if USE_OBSOLETE_API
	Boolean	Add (const Type& inData) { return AddTail(inData); };
#endif

	/*!
		@function	Insert
		@abstract	Insert a/some new element[s].
		@discussion
			If the index is out of range, a new empty value is returned silently.
		@param inData Value[s] to be inserted.
		@param inNbElemToInsert The count of item to be added (using the 2d syntax, must be the size of the array).
		@param inAtIndex The index given to the new element once inserted (1-based).
	*/
	Boolean	Insert (VIndex inBeforeElem, VIndex inNbElemToInsert, const Type& inValue);
	Boolean	Insert (VIndex inBeforeElem, VIndex inNbElemToInsert, const Type inValues[]);

	/*!
		@function	AddNSpaces
		@abstract	Add N new elements.
		@discussion
			BE CAREFULL : The new elements are not initalized but eventually set to ZERO !
			returns false if it could not allocate the space needed.
		@param inNbElemToAdd Number of new element to create.
		@param InSetToZero the new elements will be set to ZERO if true.
	*/
	Boolean	AddNSpaces (VIndex inNbElemToAdd, Boolean inSetToZero);

	Boolean AddNSpacesWithConstructor(VIndex inNbElemToAdd);

	/*!
		@function	InsertNSpaces
		@abstract	Insert N new elements.
		@discussion
			BE CAREFULL : The new elements are not initalized but eventually set to ZERO !
			returns false if it could not allocate the space needed.
		@param inBeforeElem Before what elem to insert.
		@param inNbElemToAdd Number of new element to create.
		@param InSetToZero the new elements will be set to ZERO if true.
	*/
	Boolean	InsertNSpaces (VIndex inBeforeElem, VIndex inNbElemToAdd, Boolean inSetToZero);
	
	/*!
		@function First
		@abstract Returns the address of the first element
		@discussion
			May return a non null pointer even for empty arrays.
	*/
	Type*	First () { return fFirst; };
	Type*	End () { return fFirst+fCount; };

	const Type*	First () const { return fFirst; };
	const Type*	End () const { return fFirst+fCount; };

	/*!
		@function Last
		@abstract Returns the address of the last element.
		@discussion
			May return a non null pointer even for empty arrays.

			Warning: do not write;
			for (Type* elem = list.First() ; elem <= list.Last() ; elem++)
				...
			since this would fail in the case where there's no element.
			Both First and Last would return NULL making the loop being executed once.
	*/
	Type*	Last () { return (fCount == 0) ? NULL : (fFirst + fCount - 1); };
	const Type*	Last () const { return (fCount == 0) ? NULL : (fFirst + fCount - 1); };

	/*!
		@function AfterLast
		@abstract Returns the address of the element after the last one, or NULL if none.
		@discussion
			May return a non null pointer even for empty arrays.

			Warning: This returns an address pointing to void space.
			But this function lets you safely write;
			for (Type* elem = list.First() ; elem < list.AfterLast() ; elem++)
				...
	*/
	Type*	AfterLast () { return fFirst+fCount; };
	const Type*	AfterLast () const { return fFirst+fCount; };

	// Stack-like item accessing
	Boolean	Push (const Type& inData) { return AddNth(inData, fCount+1); };
	Type	Pop () { assert(fCount > 0); Type a = GetNth(fCount); DeleteNth(fCount); return a; };

	// Item deletion support
	void	DeleteNth (VIndex inIndex, ListDestroy inDestroy = DESTROY_IF_OWNER) { DeleteRange(inIndex, 1, inDestroy); };
	void	Delete (const Type& inData, ListDestroy inDestroy = DESTROY_IF_OWNER);
	void	DeleteRange (VIndex inFirst, VIndex inCount, ListDestroy inDestroy = DESTROY_IF_OWNER);
	void	Destroy (ListDestroy inDestroy = DESTROY_IF_OWNER);

	// Array size support
	VIndex	GetCount () const { return fCount; };
	Boolean	IsEmpty () const { return fCount == 0; };

	Boolean	SetCount (VIndex inNewCount, const Type& inValueToAdd = Type(), ListDestroy inDestroy = DESTROY_IF_OWNER);
	Boolean	ReduceCount (VIndex inNewCount, ListDestroy inDestroy = DESTROY_IF_OWNER);
	
	Boolean	SetAllocatedSize (VIndex inNewCount, ListDestroy inDestroy = DESTROY_IF_OWNER);
	VSize	GetAllocatedSize () const { if (fFirst == NULL || fFirstNotAllocated) return 0; else return GetAllocator()->GetPtrSize((VPtr)fFirst); };
	void	SetSizeIncrement (VIndex inNewSizeInc) { fAllocationIncrement = inNewSizeInc; };

	// Copy support
	Boolean	CopyFrom (const VArrayOf<Type>& inOriginal);
	void	operator = (const VArrayOf& inOriginal);

	// Search support
	VIndex	FindPos (const Type& inData) const;
	Type	Find (const Type& inData) const;
	
	// Attributes accessors
	Boolean	GetUnique () const;
	void	SetUnique (Boolean inSet);
	
	// Utilities
	VCppMemMgr*	GetAllocator () const { return VObject::GetAllocator(fUseAlternateMemory); };
	static Type	GetNullData ();

protected:
	friend class VArrayIteratorOf<Type>;
	
	Type*	fFirst;
	VIndex	fCount;
	VIndex	fAllocated;
	VIndex	fAllocationIncrement;
	Boolean	fUnique;
	Boolean	fIsOwner;	// the array is the owner of its elements
	Boolean	fFirstNotAllocated;
	bool	fUseAlternateMemory;
	
	// Override to customize item accessing and disposing
	virtual const Type*	DoFindData (const Type* inFirst, const Type* inLast, const Type& inData) const;
	virtual void	DoDeleteDataRange (Type* inFirst, Type* inLast);

	Boolean	_AllocateAndCopy (VIndex inAllocCount, VIndex inCopyCount);
	Boolean	_InsertSpaces (VIndex inBeforeElem, VIndex inNbElemToAdd);
};


template<class Type>
VArrayOf<Type>::VArrayOf(VIndex inInitialCount, VIndex inInitialAllocation, VIndex inAllocationIncrement)
{
	assert(inInitialCount >= 0);
	assert(inInitialAllocation >= 0);
	assert(inAllocationIncrement > 0);
	
	fFirst = NULL;
	fFirstNotAllocated = false;
	fCount = 0;
	fAllocated = 0;
	fUnique = false;
	fIsOwner = false;
	fAllocationIncrement = inAllocationIncrement;

	fUseAlternateMemory = VTask::CurrentTaskUsesAlternateAllocator();

	if (inInitialCount > 0)
	{
		if (inInitialAllocation < inInitialCount)
			inInitialAllocation = inInitialCount;

		fFirst = (Type*) GetAllocator()->NewPtr(inInitialAllocation * sizeof(Type), false, 'arr ');
		if ((fFirst == NULL) && (inInitialAllocation != inInitialCount))
		{
			// try again with just what is necessary
			inInitialAllocation = inInitialCount;
			fFirst = (Type*) GetAllocator()->NewPtr(inInitialAllocation * sizeof(Type), false, 'arr ');
		}
		
		if (fFirst == NULL)
		{
			vThrowError(VE_MEMORY_FULL);
		}
		else
		{
			fAllocated = inInitialAllocation;
			
			// build empty elements
			fCount = inInitialCount;
			std::uninitialized_fill(fFirst, fFirst + fCount, Type());
		}
	}
	else if (inInitialAllocation > 0)
	{
		fFirst = (Type*) GetAllocator()->NewPtr(inInitialAllocation * sizeof(Type), false, 'arr ');
		if (fFirst != NULL)	// don't report mem failure
			fAllocated = inInitialAllocation;
	}
}


template<class Type> VArrayOf<Type>::VArrayOf(const VArrayOf& inOriginal)
{
	fUseAlternateMemory = VTask::CurrentTaskUsesAlternateAllocator();

	fFirstNotAllocated = false;
	if (inOriginal.fFirst == NULL || inOriginal.fCount == 0)
	{
		fFirst = NULL;
		fCount = 0;
		fAllocated = 0;		
	}
	else
	{
		fFirst = (Type*) GetAllocator()->NewPtr(inOriginal.fCount * sizeof(Type), false, 'arr ');
		if (fFirst == NULL)
		{
			fCount = 0;
			fAllocated = 0;
			vThrowError(VE_MEMORY_FULL);
		}
		else
		{
			fCount = inOriginal.fCount;
			fAllocated = fCount;
			std::uninitialized_copy(inOriginal.fFirst, inOriginal.fFirst + inOriginal.fCount, fFirst);
		}
	}
	
	fAllocationIncrement = inOriginal.fAllocationIncrement;
	fUnique = inOriginal.fUnique;
	fIsOwner = false;
}


template<class Type>
VArrayOf<Type>::~VArrayOf()
{
	Destroy(DESTROY_IF_OWNER);
}


template<class Type>
Boolean VArrayOf<Type>::GetUnique() const
{
	return fUnique;
}


template<class Type>
void VArrayOf<Type>::SetUnique(Boolean inSet)
{
	fUnique = inSet;
}


template<class Type>
Type VArrayOf<Type>::GetNullData()
{
	return Type();
}


template<class Type>
Type VArrayOf<Type>::GetNth(VIndex inIndex) const
{
	if (inIndex <= 0 || inIndex > fCount)
		return Type();

	return fFirst[inIndex-1];
}


template<class Type>
void VArrayOf<Type>::SetNth(const Type& inData, VIndex inIndex)
{
	if (testAssert(inIndex > 0 && inIndex <= fCount))
		fFirst[inIndex-1] = inData;
}


template<class Type>
VArrayIteratorOf<Type>* VArrayOf<Type>::NewIterator(VIndex inIndex) const
{
	VArrayIteratorOf<Type>* scan = new VArrayIteratorOf<Type>(*this);
	scan->SetPos(inIndex);
	return scan;
}


template<class Type>
VIndex VArrayOf<Type>::FindPos(const Type& inData) const
{
	const Type* found = DoFindData(fFirst, fFirst + fCount, inData);
	if (found >= fFirst)
	{
		size_t pos = found - fFirst;
	
		return (pos >= fCount) ? -1 : (VIndex) (pos+1);
	}
	else
	{
		return -1;
	}
}


template<class Type>
Type VArrayOf<Type>::Find(const Type& inData) const
{
	VIndex pos = FindPos(inData);
	if (pos <= 0)
		return Type();

	return GetNth(pos);
}


template<class Type>
void VArrayOf<Type>::Delete(const Type& inData, ListDestroy inDestroy)
{
	VIndex	pos = FindPos(inData);
	if (pos > 0)
		DeleteNth(pos, inDestroy);
}


template<class Type>
void VArrayOf<Type>::DeleteRange(VIndex inFirst, VIndex inCount, ListDestroy inDestroy)
{
	if (inCount == 0 || fCount == 0) return;

	if (!testAssert(inCount > 0)) return;
	
	if (testAssert((inFirst + inCount - 1) <= fCount))
	{
		/*
			First shift elements to fill the deleted range, then delete the end of the list.
			
			This is optimized for simple types like int, char, long...
			but this may induce leaks for pointer types.
			For these types, one should use a VArrayOf<auto_ptr<T*>> or VArrayPtrOf.
		*/

		if ((inDestroy == DESTROY) || (fIsOwner && inDestroy == DESTROY_IF_OWNER))
			DoDeleteDataRange(fFirst + (inFirst - 1), fFirst + (inFirst + inCount - 1));

		// Move elements one by one
		if (fCount > (inFirst + inCount - 1))
			std::copy(fFirst + (inFirst + inCount - 1), fFirst + fCount , fFirst + (inFirst - 1));
		
		std::allocator<Type> stdAllocator;
		
        for (sLONG index = 0; index < inCount; index++)
             stdAllocator.destroy(fFirst + fCount - inCount + index);

		fCount -= inCount;
	}
}


template<class Type>
void VArrayOf<Type>::Destroy(ListDestroy inDestroy)
{
	if ((inDestroy == DESTROY) || (fIsOwner && inDestroy == DESTROY_IF_OWNER))
		DoDeleteDataRange(fFirst, fFirst + fCount);

	std::allocator<Type> stdAllocator;
    
    for (sLONG index = 0; index < fCount; index++)
         stdAllocator.destroy(fFirst + index);
	
	if (fFirst != NULL && !fFirstNotAllocated)
		GetAllocator()->Free(fFirst);
	
	fFirst = NULL;
	fFirstNotAllocated = false;
	fCount = 0;
	fAllocated = 0;
}


template<class Type>
uBYTE VArrayOf<Type>::AddNth(const Type& inData, VIndex inAtIndex)
{
 	if (fUnique && (FindPos(inData) > 0))
		return false;
	
	if (fCount >= fAllocated)
	{
		if (!_AllocateAndCopy(fCount + fAllocationIncrement, fCount))
		{
			if (!_AllocateAndCopy(fCount + 1, fCount))
			{
				vThrowError(VE_MEMORY_FULL);
				return false;
			}
		}
	}

	if (inAtIndex <= 0)
		inAtIndex = 1;
	else if (inAtIndex > fCount+1)
		inAtIndex = fCount + 1;
	
	if (inAtIndex <= fCount)
	{
		// last one must be built separatly
		std::raw_storage_iterator<Type*, Type> stdIterator(&fFirst[fCount]);
		stdIterator = fFirst[fCount-1];
		
		// move elements by one pos(last one allready copied)
		std::copy_backward(fFirst + inAtIndex - 1, fFirst + fCount - 1, fFirst + fCount);
		
		// copy the one to be inserted
		fFirst[inAtIndex - 1] = inData;
	}
	else
	{
		std::allocator<Type> stdAllocator;
		stdAllocator.construct(&fFirst[inAtIndex - 1], inData);
	}
	++fCount;

	return true;
}


template<class Type>
Boolean VArrayOf<Type>::Insert(VIndex inBeforeElem, VIndex inNbElemToInsert, const Type inValues[])
{
	if (!_InsertSpaces(inBeforeElem, inNbElemToInsert))
		return false;
	
	std::uninitialized_copy(&inValues[0], &inValues[inNbElemToInsert], fFirst + inBeforeElem - 1);
	return true;
}


template<class Type>
Boolean VArrayOf<Type>::Insert(VIndex inBeforeElem, VIndex inNbElemToInsert, const Type& inValue)
{
	if (!_InsertSpaces(inBeforeElem, inNbElemToInsert))
		return false;
	
	std::uninitialized_fill_n(fFirst + inBeforeElem - 1, inNbElemToInsert, inValue);
	return true;
}


template<class Type>
Boolean VArrayOf<Type>::InsertNSpaces(VIndex inBeforeElem, VIndex inNbElemToAdd, Boolean inSetToZero)
{
	if (!_InsertSpaces(inBeforeElem, inNbElemToAdd))
		return false;
	
	if (inSetToZero)
		::memset(fFirst + inBeforeElem - 1, 0, sizeof(Type)*inNbElemToAdd);

	return true;
}


template<class Type>
Boolean VArrayOf<Type>::AddNSpaces(VIndex inNbElemToAdd, Boolean inSetToZero)
{
	assert(inNbElemToAdd >= 0);

	if (inNbElemToAdd > 0) 
	{
		if ((fCount + inNbElemToAdd) > fAllocated)
		{
			if (!_AllocateAndCopy(fCount + inNbElemToAdd + fAllocationIncrement, fCount)) 
			{
				if (!_AllocateAndCopy(fCount + inNbElemToAdd + 1, fCount)) 
				{
					vThrowError(VE_MEMORY_FULL);
					return false;
				}
			}
							
		}

		if (inSetToZero)
			::memset(fFirst + fCount, 0, sizeof(Type)*inNbElemToAdd);

		fCount += inNbElemToAdd;
	}

	return true;
}



template<class Type>
Boolean VArrayOf<Type>::AddNSpacesWithConstructor(VIndex inNbElemToAdd)
{
	assert(inNbElemToAdd >= 0);

	if (inNbElemToAdd > 0) 
	{
		if ((fCount + inNbElemToAdd) > fAllocated)
		{
			if (!_AllocateAndCopy(fCount + inNbElemToAdd + fAllocationIncrement, fCount)) 
			{
				if (!_AllocateAndCopy(fCount + inNbElemToAdd + 1, fCount)) 
				{
					vThrowError(VE_MEMORY_FULL);
					return false;
				}
			}

		}

		VIndex i;
		Type* p = fFirst + fCount;
		for (i = 0; i<inNbElemToAdd; i++)
		{
			p = new(p) Type();
			p++;
		}

		fCount += inNbElemToAdd;
	}

	return true;
}


template<class Type>
Boolean VArrayOf<Type>::_InsertSpaces(VIndex inBeforeElem, VIndex inNbElemToAdd)
{
	assert(inNbElemToAdd >= 0);

	if (inBeforeElem <= 0)
		inBeforeElem = 1;
	else if (inBeforeElem > fCount + 1)
		inBeforeElem = fCount + 1;
	
	if (inNbElemToAdd > 0) 
	{
		if ((fCount + inNbElemToAdd) > fAllocated)
		{
			if (!_AllocateAndCopy(fCount + inNbElemToAdd + fAllocationIncrement, fCount)) 
			{
				if (!_AllocateAndCopy(fCount + inNbElemToAdd, fCount)) 
				{
					vThrowError(VE_MEMORY_FULL);
					return false;
				}
			}
		}
		
		// dissociate copy from uninitialized copy
		if (fCount - (inBeforeElem - 1) > inNbElemToAdd)
		{
			// overlapping region
			std::uninitialized_copy(fFirst + fCount - inNbElemToAdd, fFirst + fCount, fFirst + fCount);
			std::copy_backward(fFirst + inBeforeElem - 1, fFirst + fCount - inNbElemToAdd, fFirst + fCount);
		}
		else
		{
			// no overlap
			std::uninitialized_copy(fFirst + inBeforeElem - 1, fFirst + fCount, fFirst + inBeforeElem + inNbElemToAdd - 1);
		}
		
		fCount += inNbElemToAdd;
	}

	return true;
}


template<class Type>
uBYTE VArrayOf<Type>::CopyFrom(const VArrayOf<Type>& inOriginal)
{
	if (&inOriginal == this)
		return true;
	
	uBYTE result = true;
	
	if (fAllocated >= inOriginal.fCount)
	{
		fCount = inOriginal.fCount;
		//#if COMPIL_VISUAL
		//stdext::unchecked_uninitialized_copy(inOriginal.fFirst, inOriginal.fFirst + inOriginal.fCount, fFirst);
		//#else
		std::uninitialized_copy(inOriginal.fFirst, inOriginal.fFirst + inOriginal.fCount, fFirst);
		//#endif
	}
	else
	{
		if (fFirst != NULL)
		{
			if (!fFirstNotAllocated)
				GetAllocator()->Free(fFirst);
			
			fFirst = NULL;
			fFirstNotAllocated = false;
		}

		if (inOriginal.fFirst == NULL || inOriginal.fCount == 0)
		{
			fCount = 0;
			fAllocated = 0;		
		}
		else
		{
			fFirst = (Type*) GetAllocator()->NewPtr(inOriginal.fCount * sizeof(Type), false, 'arra');
			fFirstNotAllocated = false;
			if (fFirst == NULL)
			{
				fCount = 0;
				fAllocated = 0;
				vThrowError(VE_MEMORY_FULL);
				result = false;
			}
			else
			{
				fCount = inOriginal.fCount;
				fAllocated = fCount;
				std::uninitialized_copy(inOriginal.fFirst, inOriginal.fFirst + inOriginal.fCount, fFirst);
			}
		}
	}
	
	fUnique = inOriginal.fUnique;
	fIsOwner = false;
	
	return result;
}


template<class Type>
void VArrayOf<Type>::operator = (const VArrayOf& inOriginal)
{	
	CopyFrom(inOriginal);
}


template<class Type>
Boolean VArrayOf<Type>::SetAllocatedSize(VIndex inNewCount, ListDestroy inDestroy)
{
	assert(inNewCount >= 0);

	if (inNewCount == 0)
	{
		Destroy(/*inDestroy*/);
	}
	else if (inNewCount < fCount)
	{
		// shrinking
		if ((inDestroy == DESTROY) || (fIsOwner && inDestroy == DESTROY_IF_OWNER))
			DoDeleteDataRange(fFirst + inNewCount, fFirst + fCount);
		_AllocateAndCopy(inNewCount, inNewCount);
	} else
		_AllocateAndCopy(inNewCount, fCount);

	return fAllocated >= inNewCount;
}



template<class Type>
Boolean VArrayOf<Type>::ReduceCount(VIndex inNewCount, ListDestroy inDestroy)
{
	Boolean isReduced;
	assert(inNewCount >= 0);

	if (inNewCount <= fCount)
	{
		isReduced = true;
		// shrinking
		if ((inDestroy == DESTROY) || (fIsOwner && inDestroy == DESTROY_IF_OWNER))
			DoDeleteDataRange(fFirst + inNewCount, fFirst + fCount);
		fCount = inNewCount;
	}
	else
		isReduced = false;

	return isReduced;
}


template<class Type>
Boolean VArrayOf<Type>::SetCount(VIndex inNewCount, const Type& inValueToAdd, ListDestroy inDestroy)
{
	Boolean isOK = true;
	assert(inNewCount >= 0);

	if (inNewCount < fCount)
	{
		// shrinking

		if ((inDestroy == DESTROY) || (fIsOwner && inDestroy == DESTROY_IF_OWNER))
			DoDeleteDataRange(fFirst + inNewCount, fFirst + fCount);
		fCount = inNewCount;
	}
	else if (inNewCount > fCount)
	{
		// enlarging
		
		if (inNewCount > fAllocated)
		{
			if (!_AllocateAndCopy(inNewCount, fCount)) 
			{
				vThrowError(VE_MEMORY_FULL);
				isOK = false;
			}
		}

		if (isOK)
		{
			std::uninitialized_fill(fFirst + fCount, fFirst + inNewCount, inValueToAdd);
			fCount = inNewCount;
		}
	}

	return isOK;
}


template<class Type>
uBYTE VArrayOf<Type>::_AllocateAndCopy(VIndex inAllocCount, VIndex inCopyCount)
{
	Type* newFirst = (Type*) GetAllocator()->NewPtr(inAllocCount * sizeof(Type), false, 'arr ');
	if (newFirst == NULL)
		return false;
	
	std::uninitialized_copy(fFirst, fFirst + inCopyCount, newFirst);
	std::allocator<Type>	stdAllocator;
	
    for (sLONG index = 0; index < fCount; index++)
        stdAllocator.destroy(fFirst + index);
	
	if (!fFirstNotAllocated)
		GetAllocator()->Free(fFirst);
	
	fFirst = newFirst;
	fFirstNotAllocated = false;
	fAllocated = inAllocCount;
	fCount = inCopyCount;

	return true;
}


template<class Type>
void VArrayOf<Type>::DoDeleteDataRange(Type* /*inFirst*/, Type* /*inLast*/)
{
}


template<class Type>
const Type* VArrayOf<Type>::DoFindData(const Type* inFirst, const Type* inLast, const Type& inData) const
{
	return std::find(inFirst, inLast, inData);
}


//=================================================================
#pragma mark-

template<class Type>
class VArrayPtrOf : public VArrayOf<Type>
{
public:
			VArrayPtrOf (VIndex inInitialCount = 0, VIndex inInitialAllocation = 0, VIndex inAllocationIncrement = 10) : VArrayOf<Type>(inInitialCount, inInitialAllocation, inAllocationIncrement) {};
	virtual ~VArrayPtrOf () { VArrayOf<Type>::Destroy(); };

protected:
	virtual void	DoDeleteDataRange (Type* inFirst, Type* inLast);
};


template<class Type>
void VArrayPtrOf<Type>::DoDeleteDataRange(Type* inFirst, Type* inLast)
{
	for (Type* item = inFirst; item < inLast ; item++)
	{
		if ((*item) != NULL)
			delete *item;
	}
}


//=================================================================
#pragma mark-

template<class Type>
class VArrayRetainedPtrOf : public VArrayOf<Type>
{
public:
	virtual	~VArrayRetainedPtrOf () { VArrayOf<Type>::Destroy(); };

protected:
	virtual void	DoDeleteDataRange (Type* inFirst, Type* inLast)
	{
		for (Type* item = inFirst; item < inLast ; item++)
		{
			if ((*item) != NULL)
				 (*item)->Release();
		}
	}
};


template<class Type>
class VArrayRetainedOwnedPtrOf : public VArrayOf<Type>
{
public:
	VArrayRetainedOwnedPtrOf (VIndex inInitialCount = 0, VIndex inInitialAllocation = 0, VIndex inAllocationIncrement = 10) : VArrayOf<Type>(inInitialCount, inInitialAllocation, inAllocationIncrement) { this->fIsOwner = true;};
	virtual	~VArrayRetainedOwnedPtrOf () { VArrayOf<Type>::Destroy(); };

protected:
	virtual void	DoDeleteDataRange (Type* inFirst, Type* inLast)
	{
		for (Type* item = inFirst; item < inLast ; item++)
		{
			if ((*item) != NULL)
				(*item)->Release();
		}
	}
};


//=================================================================
#pragma mark-

template<class Type>
class VArrayIteratorOf : public VIteratorOf<Type>
{
public:
	VArrayIteratorOf (const VArrayOf<Type>& inList);
	VArrayIteratorOf (const VArrayOf<Type>* inList);
 	
 	// Inherited from VIteratorOf
	Type	Next ();
	Type	Previous ();
	Type	Current () const;
	Type	First ();
	Type	Last ();
	
	void	SetCurrent (const Type& inData);
 	void	SetPos (VIndex inIndex);
 	VIndex	GetPos () const;
 	void	Reset () { SetPos(-1); };
	
	// Array accessing
 	void	UseList (const VArrayOf<Type>& inList);
 	void	UseList (const VArrayOf<Type>* inList);

private:
	const VArrayOf<Type>*	fList;
	VIndex fPos;
};


template<class Type>
VArrayIteratorOf<Type>::VArrayIteratorOf(const VArrayOf<Type>& inList)
{
	UseList(inList);
}


template<class Type>
VArrayIteratorOf<Type>::VArrayIteratorOf(const VArrayOf<Type>* inList)
{
	UseList(inList);
}


template<class Type>
Type VArrayIteratorOf<Type>::Next()
{
	if (fList == NULL)
		return Type();
	
	if (fPos == -1)
		return First();
	
	if (fPos < fList->GetCount())
		return fList->GetNth(++fPos);

	fPos = 0;
	return Type();
}


template<class Type>
Type VArrayIteratorOf<Type>::Previous()
{
 if (fList == NULL)
		return Type();
	
	if (fPos==-1)
		return Last();
	
	if (fPos>1)
		return fList->GetNth(--fPos);
	
	fPos = 0;
	return Type();
}


template<class Type>
Type VArrayIteratorOf<Type>::Current() const
{
	if (fList == NULL || fPos==0 || fPos==fList->fCount)
		return Type();
	
	if (fPos==-1)
		return ((VArrayIteratorOf<Type>*)this)->First();
	
	return fList->GetNth(fPos);
}


template<class Type>
void VArrayIteratorOf<Type>::UseList(const VArrayOf<Type>& inList)
{
	fList = &inList;
	fPos = -1;
}


template<class Type>
void VArrayIteratorOf<Type>::UseList(const VArrayOf<Type>* inList)
{
	fList = inList;
	fPos = -1;
}


template<class Type>
Type VArrayIteratorOf<Type>::First()
{
	if (fList == NULL)
		return Type();
	
	fPos = 1;
	return fList->GetNth(fPos);
}


template<class Type>
Type VArrayIteratorOf<Type>::Last()
{
	if (fList == NULL)
		return Type();
	
	fPos = fList->fCount;
	return fList->GetNth(fPos);
}


template<class Type>
void VArrayIteratorOf<Type>::SetCurrent(const Type& inData)
{
	fPos = fList->FindPos(inData);
}


template<class Type>
void	VArrayIteratorOf<Type>::SetPos(VIndex inIndex)
{
	if (inIndex==-1)
		fPos = -1;
	else if (inIndex<0 || inIndex>fList->GetCount())
		fPos = 0;
	else
		fPos = inIndex;
}


template<class Type>
VIndex	VArrayIteratorOf<Type>::GetPos() const
{
	return fPos;
}


typedef VArrayPtrOf<VObject*>		VArrayList;
typedef VArrayIteratorOf<VObject*>	VArrayIterator;


//=================================================================
#pragma mark-

template<class Type, sLONG inNbAllocatedElems>
class VStackArrayOf : public VArrayOf<Type>
{
public:
	VStackArrayOf ();

protected:
	Type	fPredefAlloc[inNbAllocatedElems];
};


template<class Type, sLONG inNbAllocatedElems>
VStackArrayOf<Type, inNbAllocatedElems>::VStackArrayOf()
{
	VArrayOf<Type>::fFirst = fPredefAlloc;
	VArrayOf<Type>::fFirstNotAllocated = true;
	VArrayOf<Type>::fAllocated = inNbAllocatedElems;
}


template<class Type, sLONG inNbAllocatedElems>
class VStackArrayPtrOf : public VStackArrayOf<Type, inNbAllocatedElems>
{
public:
	virtual	~VStackArrayPtrOf () { VStackArrayOf<Type, inNbAllocatedElems>::Destroy(); };

protected:
	virtual void DoDeleteDataRange (Type* inFirst, Type* inLast)
	{
		for (; inFirst < inLast; ++inFirst)
		{
			if (*inFirst != NULL) delete *inFirst;
		}
	}
};


template<class Type, sLONG inNbAllocatedElems>
class VStackArrayRetainedPtrOf : public VStackArrayOf<Type, inNbAllocatedElems>
{
public:
	virtual	~VStackArrayRetainedPtrOf () { VStackArrayOf<Type, inNbAllocatedElems>::Destroy(); };

protected:
	virtual void DoDeleteDataRange (Type* inFirst, Type* inLast)
	{
		for (; inFirst < inLast; ++inFirst)
		{
			if (*inFirst != NULL) (*inFirst)->Release();
		}
	}
};


//=================================================================
#pragma mark-

// attention: this class limits on purpose the size of the index to a 2 bytes integer
// the idea is that a pointer which usually takes 4 bytes (or 8 bytes on 64 bits machines) could fit on 2 bytes through an indirection
const sWORD kLastHoleInArray = -1;

template<class Type>
class VArrayWithHolesOf : public VArrayOf<Type>
{
public:
	VArrayWithHolesOf (VIndex inInitialCount = 0, VIndex inInitialAllocation = 0, VIndex inAllocationIncrement = 10)
		: VArrayOf<Type>(inInitialCount, inInitialAllocation, inAllocationIncrement) { fFirstHole = kLastHoleInArray; };

	sWORD	AddInAHole (const Type& inData); // Returning -1 means it could not allocate a new hole
	void	FreeHole (sWORD inHoleNumber);

private:
	sWORD	fFirstHole;
};


template<class Type>
sWORD VArrayWithHolesOf<Type>::AddInAHole(const Type& inData)
{
	sWORD result = -1; 

	if (fFirstHole == kLastHoleInArray)
	{
		if (VArrayOf<Type>::fCount <= 0x00007fff)
		{
			result = (sWORD) VArrayOf<Type>::fCount;
			if (!this->AddTail(inData))
			{
				result = -1;
			}
		}
	}
	else
	{
		result = fFirstHole;
		assert(result >= 0);
		fFirstHole = *((sWORD*)(VArrayOf<Type>::fFirst + result));
		VArrayOf<Type>::fFirst[result] = inData;
	}

	return result;
}


template<class Type>
void VArrayWithHolesOf<Type>::FreeHole(sWORD inHoleNumber)
{
	assert(inHoleNumber >= 0);
	if (inHoleNumber >= 0)
	{
		* ((sWORD*)(VArrayOf<Type>::fFirst+inHoleNumber)) = fFirstHole;
		fFirstHole = inHoleNumber;
	}
}


// Utility definitions for compatibility only
typedef VArrayOf<VObject*>	VArray;

END_TOOLBOX_NAMESPACE

#endif
