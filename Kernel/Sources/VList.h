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
#ifndef __VList__
#define __VList__

#include "Kernel/Sources/VChain.h"

BEGIN_TOOLBOX_NAMESPACE

// Defined bellow
template<class Type> class VListIteratorOf;
template<class Type> class VListItem;


/*!
	@class	VListOf
	@abstract	Double-linked chain list class.
	@discussion
		It is a double-linked list where each node encapsulates the data.

		Use VListPtrOf if you want to use pointers to objects.
		
		DEPRECATED: use STL instead
*/

template<class Type>
class VListOf : public VObject
{
public:
	typedef VListItem<Type> VItem;
	
	/*!
		@function	VListOf
		@abstract	Creates an empty chain list.
	*/
	VListOf ();

	/*!
		@function	VListOf
		@abstract	Creates a chain list with a copy of the data of the given list.
	*/
	VListOf (const VListOf<Type>& inSourceList);
	
	virtual ~VListOf ();
	
	/*!
		@function	IsEmpty
		@abstract	returns true is the list is empty.
	*/
	Boolean	IsEmpty () const { return fList.IsEmpty(); };

	/*!
		@function	GetCount
		@abstract	Count the number of elements in the list.
		@discussion
			It scans the whole list.
	*/
	VIndex	GetCount () const { return fList.GetCount(); };

	/*!
		@function	AddHead
		@abstract	Inserts a new item before all elements.
		@discussion
			It creates a new VListItem object to store the data.
			New element will be at position 1.
	*/
	void	AddHead (const Type& inData) { AddBeforeElement(inData, NULL); };

	/*!
		@function	AddTail
		@abstract	Inserts a new item after all elements.
		@discussion
			It creates a new VListItem object to store the data.
	*/
	void	AddTail (const Type& inData) { AddAfterElement(inData, NULL); };

	/*!
		@function	AddNth
		@abstract	Inserts a new item after given element position.
		@discussion
			It creates a new VListItem object to store the data.
			New element will be at position inIndex+1.
			If inIndex is zero or negative, the element is inserted before all elements.
			If inIndex is greater than the count of elements, the element is inserted after all elements.
	*/
	void	AddNth (const Type& inData, VIndex inIndex);

	/*!
		@function	AddAfter
		@abstract	Inserts a new item after given element data.
		@discussion
			It first tries to find the element which has specified data.
			If found, the new element is inserted after it.
			If not found, the new element is inserted after all elements.
	*/
	void	AddAfter (const Type& inData, const Type& inAfter) { AddAfterElement(inData, FindElement(inAfter)); };

	/*!
		@function	AddBefore
		@abstract	Inserts a new item before given element data.
		@discussion
			It first tries to find the element which has specified data.
			If found, the new element is inserted before it.
			If not found, the new element is inserted before all elements.
	*/
	void	AddBefore (const Type& inData, const Type& inBefore) { AddBeforeElement(inData, FindElement(inBefore)); };

	/*!
		@function	GetFirst
		@abstract	Returns first element data.
		@discussion
			Returns NULL data if the list is empty.
	*/
	Type	GetFirst () const;
	
	/*!
		@function	GetLast
		@abstract	Returns last element data.
		@discussion
			Returns NULL data if the list is empty.
	*/
	Type	GetLast () const;
	
	/*!
		@function	Next
		@abstract	Returns the data of the element that follows given element data.
		@discussion
			It first tries to find the element which has specified data.
			If found, it returns the next element data.
			If not found, it returns NULL data.
	*/
	Type	Next (const Type& inData) const;

	/*!
		@function	Previous
		@abstract	Returns the data of the element that is before given element data.
		@discussion
			It first tries to find the element which has specified data.
			If found, it returns the previous element data.
			If not found, it returns NULL data.
	*/
	Type	Previous (const Type& inData) const;

	Type	GetNth (VIndex inIndex) const;
	void	SetNth (const Type& inData,VIndex inIndex);

	// Search support
	VIndex	FindPos (const Type& inData) const;
	Type	Find (const Type& inData) const;

	// Item deletion support
	void	DeleteNth (VIndex inIndex,ListDestroy inDestroy = NO_DESTROY);
	void	Delete (const Type& inData, ListDestroy = NO_DESTROY);
	void	Destroy (ListDestroy inDestroy = NO_DESTROY);
	
	// Attributes accessors
	Boolean GetUnique() const { return fUnique; };
	void	SetUnique(Boolean inSet) { fUnique = inSet; };
	
	// Sort support
	void	Sort (CompareResult (*inCmpProc) (const Type& inData1, const Type& inData2, void* inUserData), void* inUserData = 0);

	// Low level element support
	void	AddAfterElement (const Type& inData, VItem* inAfterElement);
	void	AddBeforeElement (const Type& inData, VItem* inBeforeElement);
	
	VItem*	GetFirstElement () const { return fList.GetFirst(); };
	VItem*	GetLastElement () const { return fList.GetLast(); };
	VItem*	GetNthElement (VIndex inIndex) const { return fList.GetNth(inIndex); };
	
	VItem*	DeleteElement (VItem* inElement, ListDestroy inDestroy);
	VItem*	FindElement (const Type& inData) const;
	
	// Utilities
	VListIteratorOf<Type>*	NewIterator (VIndex inIndex = -1) const;
	void	StealList (VListOf<Type>& inSrcList);
	
	static Type	GetNullData();

protected:
	VChainOf<VItem>	fList;
	Boolean	fUnique;

	// Override to customize accessing and disposing elements
	virtual Boolean	DoEqualData (const Type& inData1, const Type& inData2) const;
	virtual void	DoDeleteData (Type* inData);
};



/*!
	@class VListPtrOf
	@abstract Specific list for VObjects of 'new-allocated' data
*/

template<class Type>
class VListPtrOf : public VListOf<Type>
{
protected:
	virtual Boolean	DoEqualData (const Type& inData1,const Type& inData2) const { return inData1 == inData2; };
	virtual void	DoDeleteData (Type* inData) { delete *inData; };
};



/*!
	@class VListItem
	@abstract Node element for VListOf
	@discussion
		Should be rarely used directly.
*/

template<class Type>
class VListItem : public VObject
{
public:
			VListItem (const Type& inData, VListItem* inNext = NULL, VListItem* inPrevious = NULL);
	virtual ~VListItem ();
	
	// Linking support
	void	SetPrevious (VListItem* inPrevious) { fPrevious = inPrevious; };
	VListItem*	GetPrevious () const { return fPrevious; };

	void	SetNext (VListItem* inNext) { fNext = inNext; };
	VListItem*	GetNext () const { return fNext; };

	// Data accessing
	void	SetData (const Type& inData) { fData = inData; };
	const Type&	GetData () const { return fData; };
	Type& GetData () { return fData; };

	// List browsing utilities
	VListItem<Type>*	GetFirst () const;
	VListItem<Type>*	GetLast () const;

private:
	VListItem*	fPrevious;
	VListItem*	fNext;
	Type	fData;
};


/*!
	@class VListIteratorOf
	@abstract List iterating support
	@discussion
*/

template<class Type>
class VListIteratorOf : public VIteratorOf<Type>
{
public:
	VListIteratorOf (const VListOf<Type>& inList);
	
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

	void	UseList (const VListOf<Type>& inList);
	
protected: 
	const VListOf<Type>*	fList;
	VListItem<Type>*	fCurrent;
	VListItem<Type>*	fNext;
	VListItem<Type>*	fPrevious;
};


template<class Type>
VListOf<Type>::VListOf()
{
	fUnique = false;
}


template<class Type>
VListOf<Type>::VListOf (const VListOf<Type>& inSourceList)
{
	fUnique = false;
	for (VItem* item = inSourceList.fList.GetFirst(); item != NULL; item = item->GetNext())
		fList.AddTail(new VItem(item->GetData()));
}


template<class Type>
VListOf<Type>::~VListOf ()
{
	Destroy();
}


template<class Type>
void VListOf<Type>::StealList(VListOf<Type>& inSourceList)
{
  Destroy();

	fUnique = inSourceList.fUnique;
	fList.AddTail(inSourceList.GetFirst());
	inSourceList.RemoveAll();
}


template<class Type>
void VListOf<Type>::AddNth (const Type& inData, VIndex inIndex)
{
	if (fUnique && Find(inData))
		return;

	fList.AddNth(new VItem(inData), inIndex);
}


template<class Type>
void VListOf<Type>::AddAfterElement(const Type& inData, VItem* inAfterElement)
{
	if (fUnique && Find(inData))
		return;

	fList.AddAfter(new VItem(inData), inAfterElement);
}


template<class Type>
void VListOf<Type>::AddBeforeElement(const Type& inData, VItem* inBeforeElement)
{
	if (fUnique && Find(inData))
		return;

	fList.AddBefore(new VItem(inData), inBeforeElement);
}


template<class Type>
Type VListOf<Type>::GetNullData()
{
	return Type();
}


template<class Type>
Type VListOf<Type>::GetFirst() const
{
	VItem* item = fList.GetFirst();
	return (item == NULL) ? Type() : item->GetData();
}


template<class Type>
Type VListOf<Type>::GetLast() const
{
	VItem* item = fList.GetLast();
	return (item == NULL) ? Type() : item->GetData();
}


template<class Type>
Type VListOf<Type>::GetNth (VIndex inIndex) const
{
	VItem* item = fList.GetNth(inIndex);
	return (item == NULL) ? Type() : item->GetData();
}


template<class Type>
void VListOf<Type>::SetNth (const Type& inData, VIndex inIndex) 
{
	VItem* item = fList.GetNth(inIndex);
	if (item != NULL)
		item->SetData(inData);
}


template<class Type>
void VListOf<Type>::DeleteNth(VIndex inIndex, ListDestroy inDestroy)
{
	DeleteElement(fList.GetNth(inIndex), inDestroy);
}


template<class Type>
void VListOf<Type>::Destroy(ListDestroy inDestroy)
{
	for (VItem* item = fList.GetFirst() ; item != NULL ;)
		item = DeleteElement(item, inDestroy);
}


template<class Type>
VListIteratorOf<Type>* VListOf<Type>::NewIterator(VIndex inIndex) const
{
	VListIteratorOf<Type>* scan = new VListIteratorOf<Type>(*this);
	scan->SetPos(inIndex);
	return scan;
}


template<class Type>
VIndex VListOf<Type>::FindPos(const Type& inData) const
{
	VIndex i = 1;
  for (VItem* item = fList.GetFirst() ; item != NULL ; item = item->GetNext(), i++)
  {
		if (item->GetData() == inData)
			break;
  }
  return i;
}


template<class Type>
Type VListOf<Type>::Find(const Type& inData) const
{
  VItem* item = FindElement(inData);
	return (item == NULL) ? Type() : item->GetData();
}


template<class Type>
Type VListOf<Type>::Next(const Type& inData) const
{
  VItem* item = FindElement(inData);
	return (item == NULL || item->GetNext() == NULL) ? Type() : item->GetNext()->GetData();
}


template<class Type>
Type VListOf<Type>::Previous(const Type& inData) const
{
  VItem* item = FindElement(inData);
	return (item == NULL || item->GetPrevious() == NULL) ? Type() : item->GetPrevious()->GetData();
}


template<class Type>
void VListOf<Type>::Delete(const Type& inData, ListDestroy inDestroy)
{
	for (VItem* item = fList.GetFirst() ; item != NULL ;)
	{
		if (item->GetData() == inData)
			item = DeleteElement(item, inDestroy);
		else
			item = item->GetNext();
	}
}


template<class Type>
void VListOf<Type>::Sort(CompareResult (*inCmpProc) (const Type& inData1,const Type& inData2,void* inUserData),void* inUserData)
{
	if (fList.GetFirst() == fList.GetLast())
		return;

	VItem* head = fList.GetFirst();
	fList.RemoveAll();
	
	Boolean permut;
	do {
		permut = false;
		VItem *elem1, *elem2;
		for (elem1 = head, elem2 = head->GetNext(); elem2 != NULL; elem1 = elem2, elem2 = elem2->GetNext())
		{
			CompareResult cr = inCmpProc(elem1->GetData(), elem2->GetData(), inUserData);
			if (cr == CR_BIGGER)
			{
				permut = true;
				
				if (elem1->GetPrevious() != NULL)
					elem1->GetPrevious()->SetNext(elem2);
				
				if (elem2->GetNext() != NULL)
					elem2->GetNext()->SetPrevious(elem1);
				
				elem1->SetNext(elem2->GetNext());
				elem2->SetPrevious(elem1->GetPrevious());

				elem1->SetPrevious(elem2);
				elem2->SetNext(elem1);
				
				if (head == elem1)
					head = elem2;
				
				elem2 = elem1;
			}
		}
  } while(permut);
  
  fList.AddTail(head);
}


template<class Type>
VListItem<Type>* VListOf<Type>::DeleteElement(VItem* inElement, ListDestroy inDestroy)
{
	VItem*	next = fList.Remove(inElement);
	if (inElement != NULL)
	{
		if (inDestroy == DESTROY)
			DoDeleteData(&inElement->GetData());
		delete inElement;
	}
	return next;
}


template<class Type>
VListItem<Type>* VListOf<Type>::FindElement(const Type& inData) const
{
	VItem* item = fList.GetFirst();
	while((item != NULL) && (item->GetData() != inData))
		item = item->GetNext();
	return item;
}


template<class Type>
void VListOf<Type>::DoDeleteData(Type* /*inData*/)
{
}


template<class Type>
Boolean VListOf<Type>::DoEqualData(const Type& inData1, const Type& inData2) const
{
	return 0 == ::memcmp((VPtr)&inData1,(VPtr)&inData2,sizeof(Type));
}


//==================================================================================
#pragma mark-

template<class Type>
VListItem<Type>::VListItem (const Type& inData, VListItem* inNext, VListItem* inPrevious)
{
	fData = inData;
	fNext = inNext;
	fPrevious = inPrevious;
}


template<class Type>
VListItem<Type>::~VListItem ()
{
}


template<class Type>
VListItem<Type>* VListItem<Type>::GetFirst () const
{
	VListItem<Type>*	first = fPrevious;
	
	while (first->fPrevious != NULL)
		first = first->fPrevious;
		
	return first;
}


template<class Type>
VListItem<Type>* VListItem<Type>::GetLast () const
{
	VListItem<Type>*	last = fNext;
	
	while (last->fNext != NULL)
		last = last->fNext;
		
	return last;
}
	

//==================================================================================
#pragma mark-

template<class Type>
VListIteratorOf<Type>::VListIteratorOf (const VListOf<Type>& inList) 
{
	fList = &inList;
	fCurrent = (fList == NULL) ? NULL : (VListItem<Type>*) -1;
	fPrevious = fNext = NULL;
}


template<class Type>
void VListIteratorOf<Type>::UseList (const VListOf<Type>& inList)
{
	fList = &inList;
	fCurrent = (fList == NULL) ? NULL : (VListItem<Type>*) -1;
	fPrevious = fNext = NULL;
}


template<class Type>
Type VListIteratorOf<Type>::Next()
{
	if (!testAssert(fList != NULL)) return Type();
	
	if (fCurrent == (VListItem<Type>*) -1) 
		fCurrent = fList->GetFirstElement();
	else
		fCurrent = fNext;
	
	// This ensure the iterator is safe if the current item has been deleted
	fNext = (fCurrent != NULL) ? fCurrent->GetNext() : NULL;
	fPrevious = (fCurrent != NULL) ? fCurrent->GetPrevious() : NULL;

	return (fCurrent == NULL) ? Type() : fCurrent->GetData();
}


template<class Type>
Type VListIteratorOf<Type>::Current() const
{
	if (fCurrent == (VListItem<Type>*) -1)
		return const_cast<VListIteratorOf<Type>*>(this)->First();
	
	return (fCurrent == NULL) ? Type() : fCurrent->GetData();
}


template<class Type>
Type VListIteratorOf<Type>::Previous() 
{
	if (!testAssert(fList != NULL)) return Type();
	
	if (fCurrent == (VListItem<Type>*) -1)
		fCurrent = fList->GetLastElement();
	else
		fCurrent = fPrevious;
	
	// This ensure the iterator is safe if the current item has been deleted
	fNext = (fCurrent != NULL) ? fCurrent->GetNext() : NULL;
	fPrevious = (fCurrent != NULL) ? fCurrent->GetPrevious() : NULL;

	return (fCurrent == NULL) ? Type() : fCurrent->GetData();
}


template<class Type>
Type VListIteratorOf<Type>::First() 
{
	assert(fList != NULL);
	fCurrent = (fList == NULL) ? NULL : fList->GetFirstElement();
	
	fNext = (fCurrent != NULL) ? fCurrent->GetNext() : NULL;
	fPrevious = NULL;

	return (fCurrent == NULL) ? Type() : fCurrent->GetData();
}


template<class Type>
Type VListIteratorOf<Type>::Last()
{
	assert(fList != NULL);
	fCurrent = (fList == NULL) ? NULL : fList->GetLastElement();
	
	fNext = NULL;
	fPrevious = (fCurrent != NULL) ? fCurrent->GetPrevious() : NULL;

	return (fCurrent == NULL) ? Type() : fCurrent->GetData();
}


template<class Type>
void VListIteratorOf<Type>::SetPos (VIndex inIndex)
{
	if (inIndex == -1)
		fCurrent = (VListItem<Type>*) -1;
	else if (testAssert(fList != NULL))
	{
		fCurrent = fList->GetNthElement(inIndex);
	
		// This ensure the iterator is safe if the current item has been deleted
		fNext = (fCurrent != NULL) ? fCurrent->GetNext() : NULL;
		fPrevious = (fCurrent != NULL) ? fCurrent->GetPrevious() : NULL;
	}
}


template<class Type>
void VListIteratorOf<Type>::SetCurrent (const Type& inData)
{
	if (testAssert(fList != NULL))
	{
		fCurrent = fList->FindElement(inData);

		fNext = (fCurrent != NULL) ? fCurrent->GetNext() : NULL;
		fPrevious = (fCurrent != NULL) ? fCurrent->GetPrevious() : NULL;
	}
}


template<class Type>
VIndex VListIteratorOf<Type>::GetPos() const
{
	VIndex pos = 0;
	VListItem<Type>* item = (fList == NULL) ? NULL : fList->GetFirstElement();
 	while(item != NULL && item != fCurrent)
 	{
 		item = item->GetNext();
 		++pos;
 	}
	return (item == NULL) ? 0 : pos;
}


// Utility definitions for compatibility only
typedef VListPtrOf<VObject*>	VChainList;

END_TOOLBOX_NAMESPACE

#endif
