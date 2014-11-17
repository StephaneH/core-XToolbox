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
#ifndef __VChain__
#define __VChain__

#include "Kernel/Sources/VIterator.h"

BEGIN_TOOLBOX_NAMESPACE

/*!
	@class	IChainable
	@abstract Interface that adds GetNext/GetPrevious functionnality.
	@discussion
		An object that inherits this interface can be used by VChainOf.
*/

template<class Type>
class IChainable
{
public:
			IChainable () { fNext = NULL; fPrevious = NULL; };
	virtual ~IChainable () {};
	
	Type*	GetPrevious () const { return fPrevious; };
	Type*	SetPrevious (Type* inPrevious) { return fPrevious = inPrevious; };

	Type*	GetNext () const { return fNext; };
	Type*	SetNext (Type* inNext) { return fNext = inNext; };

private:
	Type*	fNext;
	Type*	fPrevious;
};


/*!
	@class VChainOf
	@abstract template class that manage a double-linked list of objects.
	@discussion
		Type must provide GetNext/SetNext & GetPrevious/SetPrevious methods.
		The IChainable interface eases the implementation of these methods but is not mandatory.
*/

template<class Type>
class VChainOf : public VObject
{
public:
			VChainOf (Type* inHead = NULL);
	virtual ~VChainOf () {};
	
	// Adding items
	void	AddHead (Type* inNewElement) { AddBefore(inNewElement, fHead); };
	void	AddTail (Type* inNewElement) { AddAfter(inNewElement, fTail); };
	void	AddNth (Type* inNewElement, VIndex inIndex);
	void	AddBefore (Type* inNewElement, Type* inBeforeElement);
	void	AddAfter (Type* inNewElement, Type* inAfterElement);

	// Accessing items
	Type*	GetFirst () const { return fHead; };
	Type*	GetLast () const;
	Type*	GetNth (VIndex inIndex) const;

	VIndex	GetCount () const;
	Boolean	IsEmpty () const { return (fHead == NULL); };
	
	Type*	Remove (Type* inElement);	// Returns next to inElement
	void	RemoveAll ();

	void	Push (Type* inNewElement) { AddAfter(inNewElement, fTail); };
	Type*	Pop ();
	
	// Searching items
	Type*	Find (const Type* inElement) const;
	VIndex	FindPos (const Type* inElement) const;
	
protected:
	Type*	fHead;
	mutable Type*	fTail;
};


template<class Type>
VChainOf<Type>::VChainOf(Type* inHead)
{
	fTail = NULL;
	fHead = inHead;
	
	if (inHead != NULL)
	{
		while (fHead->GetPrevious() != NULL)
			fHead = fHead->GetPrevious();
	}
}


template<class Type>
VIndex VChainOf<Type>::GetCount() const
{
	VIndex count = 0L;
	for (Type* elem = fHead ; elem != NULL ; elem = elem->GetNext())
		++count;
	return count;
}


template<class Type>
void VChainOf<Type>::AddNth(Type* inNewElement, VIndex inIndex)
{
	if (inIndex <= 0)
		AddBefore(inNewElement, fHead);
	else
		AddAfter(inNewElement, GetNth(inIndex));
}


template<class Type>
void VChainOf<Type>::AddBefore(Type* inNewElement, Type* inBeforeElement)
{
	if (inNewElement == NULL)
		return;
	
	Type* first = inNewElement;
	while (first->GetPrevious() != NULL)
		first = first->GetPrevious();

	Type* last = inNewElement;
	while (last->GetNext() != NULL)
		last = last->GetNext();

	if (inBeforeElement == NULL)
		inBeforeElement = fHead;

	if (inBeforeElement == NULL)
	{
		assert(fTail == NULL && fHead == NULL);
		fTail = first;
		fHead = last;
	}
	else
	{
		last->SetNext(inBeforeElement);
		first->SetPrevious(inBeforeElement->GetPrevious());
		if (fHead == inBeforeElement)
			fHead = first;
		else
			inBeforeElement->GetPrevious()->SetNext(first);
		inBeforeElement->SetPrevious(last);
	}
}


template<class Type>
void VChainOf<Type>::AddAfter(Type* inNewElement, Type* inAfterElement)
{
	if (inNewElement == NULL)
		return;
	
	Type* first = inNewElement;
	while (first->GetPrevious() != NULL)
		first = first->GetPrevious();

	Type* last = inNewElement;
	while (last->GetNext() != NULL)
		last = last->GetNext();

	if (inAfterElement == NULL)
		inAfterElement = GetLast();

	if (inAfterElement == NULL)
	{
		assert(fTail == NULL && fHead == NULL);
		fTail = first;
		fHead = last;
	}
	else
	{
		last->SetNext(inAfterElement->GetNext());
		first->SetPrevious(inAfterElement);
		if (fTail != NULL)
		{	// the tail is optionnal
			if (fTail == inAfterElement)
				fTail = first;
			else
				inAfterElement->GetNext()->SetPrevious(last);
		}
		else if (inAfterElement->GetNext() != NULL)
		{
			inAfterElement->GetNext()->SetPrevious(last);
		}
		inAfterElement->SetNext(first);
	}
}


template<class Type>
Type* VChainOf<Type>::GetNth(VIndex inIndex) const
{
	Type* elem = fHead;
	assert(inIndex>0);
	for (VIndex i = 1 ; (i < inIndex) && (elem != NULL) ; ++i)
		elem = elem->GetNext();
	return elem;
}


template<class Type>
Type* VChainOf<Type>::Pop()
{
	Type* last = GetLast();
	if (last != NULL)
	{
		if (fHead == last)
		{
			fHead = fTail = NULL;
		}
		else
		{
			fTail = last->GetPrevious();
			fTail->SetNext(NULL);
		}
		last->SetPrevious(NULL);
	}
	
	return last;
}


template<class Type>
Type* VChainOf<Type>::Remove(Type* inElement)
{
	if (inElement == NULL)
		return NULL;
	
	Type* next = inElement->GetNext();
	
	if (inElement == fHead)
		fHead = inElement->GetNext();
	else if (inElement->GetPrevious() != NULL)
		inElement->GetPrevious()->SetNext(inElement->GetNext());

	if (inElement == fTail)
		fTail = inElement->GetPrevious();
	else if (inElement->GetNext() != NULL)	// no assert because fTail is optionnal
		inElement->GetNext()->SetPrevious(inElement->GetPrevious());
	
	inElement->SetNext(NULL);
	inElement->SetPrevious(NULL);
	
	return next;
}


template<class Type>
void VChainOf<Type>::RemoveAll()
{
	fTail = fHead = NULL;
}


template<class Type>
Type* VChainOf<Type>::GetLast() const
{
	if (fTail == NULL && fHead != NULL)
	{
		fTail = fHead;
		while (fTail->GetNext() != NULL)
			fTail = fTail->GetNext();
	}
	return fTail;
}


template<class Type>
VIndex VChainOf<Type>::FindPos(const Type* inElement) const
{
	VIndex pos = -1;
	VIndex i = 1;
	for (Type* elem = fHead ; elem != NULL ; elem = elem->GetNext())
	{
		if (elem == inElement)
		{
			pos = i;
			break;
		}
		++i;
	}
	
	return pos;
}


template<class Type>
Type* VChainOf<Type>::Find(const Type* inElement) const
{
	Type*	elem;
	for (elem = fHead; elem != NULL; elem = elem->GetNext())
	{
		if (elem == inElement) break;
	}
	
	return elem;
}

END_TOOLBOX_NAMESPACE

#endif