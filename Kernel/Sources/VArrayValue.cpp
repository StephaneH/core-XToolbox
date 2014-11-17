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
#include "VArrayValue.h"
#include "VMemory.h"
#include "VStream.h"
#include "VFloat.h"
#include "VTime.h"


// Class constants
const uWORD		kArrayAtomSize = 64;


const VArrayBoolean::InfoType	VArrayBoolean::sInfo;
const VArrayByte::InfoType		VArrayByte::sInfo;
const VArrayWord::InfoType		VArrayWord::sInfo;
const VArrayLong::InfoType		VArrayLong::sInfo;
const VArrayLong8::InfoType		VArrayLong8::sInfo;
const VArrayReal::InfoType		VArrayReal::sInfo;
const VArrayFloat::InfoType		VArrayFloat::sInfo;
const VArrayTime::InfoType		VArrayTime::sInfo;
const VArrayDuration::InfoType	VArrayDuration::sInfo;
const VArrayString::InfoType	VArrayString::sInfo;
const VArrayImage::InfoType		VArrayImage::sInfo;


VArrayValue::VArrayValue()
{
	fElemSize = 0;
	fCount = 0;
	fDataPtr = NULL;
	fDataSize = 0;
}


VArrayValue::~VArrayValue()
{
	if (fDataPtr)
		VMemory::DisposePtr(fDataPtr);
}


Boolean VArrayValue::SetCount(sLONG inNb)
{
	if (inNb == fCount)
		return true;

	if (inNb < 0)
	{
		DebugMsg("negative number of elements for array");
		inNb = 0;
	}

	if (inNb < fCount)
		DeleteElements(inNb, fCount - inNb);

	if (SetDataSize(inNb * fElemSize))
	{
		fCount = inNb;
		return true;
	}

	return false;
}


Boolean VArrayValue::ValidIndex(sLONG inIndex) const
{
	if (inIndex < 1 || inIndex > fCount)
	{
		DebugMsg("bad index for VArrayValue::ValidIndex");
		return false;
	}
	else
		return true;
}

Boolean VArrayValue::Insert(sLONG inIndex, sLONG inNb)
{
	sLONG	toMove = fCount - inIndex + 1;
	if ( SetCount(fCount + inNb) )
	{
		if (toMove > 0)
			MoveElements(inIndex - 1, inIndex - 1 + inNb, toMove);

		InitElements(inIndex - 1, inNb);

		return true;
	}
	else
		return false;
}


void VArrayValue::InitElements(sLONG inIndex, sLONG inNb )
{
	uBYTE* pt = LockAndGetData();
	memset( pt + inIndex * fElemSize, 0, fElemSize * inNb ); 
	UnlockData();
}

// attention : on passe le numero de l'element que l'on veut detruire
// pour detruire le premier on passe 1
void VArrayValue::Delete(sLONG inIndex, sLONG inNb)
{
	if (fDataPtr == NULL) return;
	
	DeleteElements(inIndex - 1, inNb);
	sLONG	toMove = fCount - inIndex + 1 - inNb;

	if (toMove > 0)
		MoveElements(inIndex - 1 + inNb, inIndex - 1, toMove);
	
	fCount -= inNb;
	SetDataSize(fCount * fElemSize);
}


void VArrayValue::MoveElements(sLONG inFrom, sLONG inTo, sLONG inNb)
{
	if (inFrom < 0 || inTo < 0 || inFrom + inNb > fCount || inTo + inNb > fCount || inFrom == inTo || inNb <= 0)
	{
		DebugMsg("bad parameters for VArrayValue::MoveElements");
	}
	else
	{
		uBYTE* data = LockAndGetData();
	
		uBYTE* src = data + (inFrom * fElemSize);
		uBYTE* dst = data + (inTo * fElemSize);
		sLONG size = inNb * fElemSize;

		::CopyBlock(src, dst, size);

		UnlockData();
	}
}


Boolean VArrayValue::Copy(const VArrayValue& inOriginal)
{
	assert(inOriginal.GetValueKind() == GetValueKind());

	if (fCount)
		Destroy();
	
	fElemSize = inOriginal.fElemSize;
	fCount = inOriginal.fCount;
	sLONG len = inOriginal.GetDataSize();
	
	if (SetDataSize(len))
	{
		uBYTE* src = inOriginal.LockAndGetData();
		uBYTE* dst = LockAndGetData();

		::CopyBlock(src, dst, len);

		inOriginal.UnlockData();
		UnlockData();
		return true;
	}
	else
	{
		DebugMsg("Not enough memory to copy array");
		SetCount(0);
		return false;
	}
}


// Override if values are pointers that needs to be disposed
void VArrayValue::DeleteElements(sLONG /*inAt*/, sLONG /*inNb*/)
{
}


void* VArrayValue::WriteToPtr(void* inData, Boolean /*inRefOnly*/, VSize inMax) const
{
	return inData;
}


void* VArrayValue::LoadFromPtr( const void* inData, Boolean /*inRefOnly*/)
{
	return const_cast<void*>( inData);
}


void VArrayValue::Sort(sLONG inFrom, sLONG inTo, Boolean inDescending)
{
	ISortable* ar[2];

	ar[0] = this;
	ar[1] = 0;

	ISortable::MultiCriteriaQSort(ar, &inDescending, 1, inFrom, inTo);
}


void VArrayValue::SynchronizedSort(Boolean inDescending, VArrayValue* inArray, ...)
{
	ISortable* ar[16];
	Boolean invert[16];
	sLONG i = 0;
	va_list marker;

	ar[0] = inArray;
	invert[0] = inDescending;
	va_start(marker, inArray);	/* Initialize variable arguments. */
	for (i = 1; i < 15; i++)
	{
		ar[i] = va_arg(marker, VArrayValue*);
		invert[i] = inDescending;

		if (ar[i] == 0)
			break;
	}
	ar[i] = 0;
	va_end(marker);				/* Reset variable arguments. */

	if (i > 0)
		ISortable::MultiCriteriaQSort(ar, invert, 1, 0, inArray->GetCount() - 1);
}


void VArrayValue::Unsort()
{
	uBYTE*	datastart;
	uBYTE*	dataend;
	uBYTE*	dataleft;
	uBYTE*	dataright;
	uBYTE*	dataswap;
	
	if (fCount < 2)
		return;

	dataswap = (uBYTE*) VMemory::NewPtr(fElemSize, 'sort');
	
	if (dataswap)
	{	
		datastart = LockAndGetData();
		dataend = datastart + (fElemSize * (fCount - 1));
		dataleft = datastart;
		dataright = dataend;

		while (dataleft < dataend)
		{
			// on permute les data
			::CopyBlock(dataleft, dataswap, fElemSize);
			::CopyBlock(dataright, dataleft, fElemSize);
			::CopyBlock(dataswap, dataleft, fElemSize);

			dataleft += fElemSize*2;
			dataright -= fElemSize*3;
			
			if (dataright < datastart)
				dataright = dataend;
		}
		UnlockData();
		VMemory::DisposePtr((VPtr) dataswap);
	}
	else
	{
		DebugMsg("Not enough memory to allocate swap buffer");
	}
}


sLONG VArrayValue::AppendBoolean(Boolean inValue)
{
	Insert(fCount + 1, 1);
	FromBoolean(inValue, fCount);

	return fCount;
}


sLONG VArrayValue::AppendLong(sLONG inValue)
{
	Insert(fCount + 1, 1);
	FromLong(inValue, fCount);

	return fCount;
}


sLONG VArrayValue::AppendLong8(sLONG8 inValue)
{
	Insert(fCount + 1, 1);
	FromLong8(inValue, fCount);

	return fCount;
}


sLONG VArrayValue::AppendReal(Real inValue)
{
	Insert(fCount + 1, 1);
	FromReal(inValue, fCount);

	return fCount;
}

sLONG VArrayValue::AppendByte(sBYTE inValue)
{
	Insert(fCount + 1, 1);
	FromByte(inValue, fCount);

	return fCount;
}


sLONG VArrayValue::AppendWord(sWORD inValue)
{
	Insert(fCount + 1, 1);
	FromWord(inValue, fCount);

	return fCount;
}


sLONG VArrayValue::AppendFloat(const VFloat& inValue)
{
	Insert(fCount + 1, 1);
	FromFloat(inValue, fCount);

	return fCount;
}


sLONG VArrayValue::AppendDuration(const VDuration& inValue)
{
	Insert(fCount + 1, 1);
	FromDuration(inValue, fCount);

	return fCount;
}


sLONG VArrayValue::AppendTime(const VTime& inValue)
{
	Insert(fCount + 1, 1);
	FromTime(inValue, fCount);

	return fCount;
}


sLONG VArrayValue::AppendString(const VString& inValue)
{
	Insert(fCount + 1, 1);
	FromString(inValue, fCount);

	return fCount;
}


void VArrayValue::Destroy()
{
	SetCount(0);
}


void VArrayValue::Clear()
{
	SetCount(0);
}


uBYTE* VArrayValue::LockAndGetData() const
{
	return (uBYTE*) fDataPtr;
}


void VArrayValue::UnlockData() const
{
	;
}


sLONG VArrayValue::GetDataSize() const
{
	return fDataSize;
}


Boolean VArrayValue::SetDataSize(sLONG inNewSize)
{
	Boolean	succeed;
	
	if (fDataPtr == NULL)
	{
		fDataPtr = VMemory::NewPtrClear( inNewSize, 'varr');
		succeed = (fDataPtr != NULL);
	}
	else
	{
		fDataPtr = VMemory::SetPtrSize( fDataPtr, inNewSize);
		succeed = (inNewSize != 0) ? (fDataPtr != NULL) : true;
		if ( succeed && inNewSize > fDataSize )
			memset( fDataPtr + fDataSize, 0, inNewSize - fDataSize );
	}
	
	fDataSize = succeed ? inNewSize : 0;
	
	return succeed;
}


sLONG VArrayValue::GetCount() const
{
	return fCount;
}


#pragma mark-

VArrayBoolean::VArrayBoolean(sLONG inNbElems)
{
	fElemSize = 0;	
	SetCount(inNbElems);
}


VArrayBoolean::VArrayBoolean(const VArrayBoolean& inOriginal)
{
	Copy(inOriginal);
}


void VArrayBoolean::Delete(sLONG inIndex, sLONG inNb)
{
	if (fDataPtr == NULL) return;
	
	DeleteElements(inIndex - 1, inNb);
	sLONG	toMove = fCount - inIndex + 1 - inNb;

	if (toMove > 0)
		MoveElements(inIndex - 1 + inNb, inIndex - 1, toMove);
	
	fCount -= inNb;
	SetDataSize((fCount + 8) >> 3);
}


Boolean VArrayBoolean::SetCount(sLONG inNb)
{
	if (inNb == fCount)
		return true;

	if (inNb < 0)
	{
		DebugMsg("negative number of elements for array");
		inNb = 0;
	}

	sLONG	dataSize = (inNb + 8) >> 3; 

	if (SetDataSize(dataSize))
	{
		fCount = inNb;
		return true;
	}
	
	return false;
}


sLONG VArrayBoolean::Find(const VValueSingle& inValue, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	return Find(inValue.GetBoolean(), inFrom, inUp, inCm);
}


sLONG VArrayBoolean::Find(Boolean inValue, sLONG inFrom, Boolean inUp, CompareMethod /*inCm*/) const
{
	uBYTE*	ptr;
	uBYTE*	data;
	uBYTE 	mask;

	if (inFrom < 1 || inFrom > fCount) 
		return -1;
	
	data = (uBYTE*) LockAndGetData();
	inFrom--;

	if (inUp)
	{
		while (inFrom < fCount)
		{
			ptr = data + (inFrom >> 3);
			mask = (uBYTE) (0x01 << (inFrom & 0x00000007));
			
			if (((*ptr & mask) != 0) == inValue)
				break;
			
			inFrom++;
		}
		if (inFrom == fCount)
			inFrom = -2;
	}
	else
	{
		while (inFrom >= 0)
		{
			ptr = data + (inFrom >> 3);
			mask = (uBYTE) (0x01 << (inFrom & 0x00000007));
			
			if (((*ptr & mask) != 0) == inValue)
				break;

			inFrom--;			
		}
		if (inFrom == -1)
			inFrom = -2;
	}
	UnlockData();

	return inFrom + 1;
}


// tableau donnant le nombre de bits de chaque octet
static uBYTE sByteWeight[256] = 
{ 
//		x0 x1 x2 x3 x4 x5 x6 x7 x8 x9 xA xB xC xD xE xF
/*0x*/	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
/*1x*/	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
/*2x*/	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
/*3x*/	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
/*4x*/	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
/*5x*/	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
/*6x*/	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
/*7x*/	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
/*8x*/	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
/*9x*/	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
/*Ax*/	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
/*Bx*/	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
/*Cx*/	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
/*Dx*/	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
/*Ex*/	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
/*Ex*/	4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

void VArrayBoolean::Sort(sLONG inFrom, sLONG inTo, Boolean inDescending)
{
	sLONG nbSetBits;
	uBYTE* data;
	uBYTE* ptr;
	uBYTE* ptend;
	sLONG size;
	uBYTE mask;
	sLONG i;
	uBYTE value;
	sLONG reste;


	if (inFrom <= 0 || inTo <= inFrom || inTo > fCount)
	{
		DebugMsg("Bad parameters for sorting");
		return;
	}

	if (inFrom != 1 || inTo != fCount)
	{
		// pour les tris qui ne s'appliquent pas a tout le tableau
		// on passe par le tri generique
		ISortable* ar[2];
		ar[0] = this; ar[1] = 0;
		ISortable::MultiCriteriaQSort(ar, &inDescending, 1, inFrom, inTo);
	}
	else
	{
		// quand il faut trier tout le tableau,
		// on utilise un algo rapide qui compte les bits
		// et remplis ensuite le tableau.
		data = (uBYTE*) LockAndGetData();
		nbSetBits = 0;
		size = fCount >> 3;
		ptr = data;
		ptend = data + size;

		// on compte tous les bits a 1
		while (ptr < ptend)
		{
			nbSetBits += sByteWeight[ *ptr ];
			ptr++;
		}

		mask = 0x01;
		for (i = 0; i < (fCount & 0x00000007); i++)
		{
			if (*ptr & mask)
				nbSetBits++;

			mask <<= 1;
		}

		// une fois que l'on connait le nombre de bits a 1
		// du tableau, on remplit le tableau
		size = nbSetBits >> 3;
		ptr = data;
		ptend = data + size;

		if (inDescending)
			value = 0xFF;
		else
			value = 0x00;

		while (ptr < ptend)
			*ptr++ = value;

		mask = 0x01;

		if (inDescending)
			*ptr = 0x00;
		else
			*ptr = 0xFF;

		for (i = 0; i < (nbSetBits & 0x00000007); i++)
		{
			if (inDescending)
				*ptr |= mask;
			else
				*ptr &= mask;

			mask <<= 1;
		}

		reste = fCount - nbSetBits;
		reste -= 8 - (nbSetBits & 0x00000007);
		ptr++;

		ptend = ptr + ((reste + 8) >> 3);

		if (inDescending)
			value = 0x00;
		else
			value = 0xFF;

		while (ptr < ptend)
			*ptr++ = value;
	
		UnlockData();
	}
}


void VArrayBoolean::Unsort()
{
	uBYTE*	datastart;
	uBYTE*	dataend;
	uBYTE*	dataleft;
	uBYTE*	dataright;
	uBYTE	swap;
	
	if (fCount < 9)
		return;

	datastart = LockAndGetData();
	dataend = datastart + (fCount >> 3);
	dataleft = datastart;
	dataright = dataend;

	while(dataleft < dataend)
	{
		// on permute les octets
		swap = *dataleft;
		*dataleft = *dataright;
		*dataleft = swap;

		dataleft += 2;
		dataright -= 3;
		
		if (dataright < datastart)
			dataright = dataend;
	}
	UnlockData();
}


VError VArrayBoolean::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	sLONG count = inStream->GetLong();

	if (count >= 0 && SetCount(count))
	{
		uBYTE* data = LockAndGetData();
		VSize read = (fCount + 8) >> 3;
		inStream->GetData(data, &read);
		UnlockData();
	}
	else
		return VMemory::GetLastError();

	return inStream->GetLastError();
}


VError VArrayBoolean::WriteToStream(VStream* inStream, sLONG inParam) const
{
	if (inParam == 'VVAL')
		inStream->PutLong(GetValueKind());

	inStream->PutLong(fCount);

	if (fCount > 0)
	{
		uBYTE* data = LockAndGetData();
		inStream->PutData(data, (fCount + 8) >> 3);
		UnlockData();
	}

	return inStream->GetLastError();
}


void VArrayBoolean::MoveElements(sLONG inFrom, sLONG inTo, sLONG inNb)
{
	uBYTE* data;
	uBYTE* src;
	uBYTE* dest;
	uBYTE srcMask;
	uBYTE destMask;
	bool inc;
	
	if (inFrom < 0 || inTo < 0 || inFrom + inNb > fCount || inTo + inNb > fCount || inFrom == inTo || inNb <= 0)
	{
		DebugMsg("bad parameters for VArrayValue::MoveElements");
	}
	else
	{
		data = LockAndGetData();

		// gestion de l'overlap
		if (inFrom < inTo)
		{
			inFrom += inNb - 1;
			inTo += inNb - 1;
			inc = false;
		}
		else
			inc = true;

		src = data + (inFrom >> 3);
		dest = data + (inTo >> 3);

		srcMask = (uBYTE) (0x01 << (inFrom & 0x00000007));
		destMask = (uBYTE) (0x01 << (inTo & 0x00000007));

		if ( inc )
		{
			while (inNb)
			{
				if (*src & srcMask)
					*dest |= destMask;
				else
					*dest &= ~destMask;

				if (srcMask == 0x80)
				{
					srcMask = 0x01;
					src++;
				}
				else
					srcMask <<= 1;

				if (destMask == 0x80)
				{
					destMask = 0x01;
					dest++;
				}
				else
					destMask <<= 1;

				inNb--;
			}
		}
		else
		{
			while (inNb)
			{
				if (*src & srcMask)
					*dest |= destMask;
				else
					*dest &= ~destMask;

				if (srcMask == 0x01)
				{
					srcMask = 0x80;
					src--;
				}
				else
					srcMask >>= 1;

				if (destMask == 0x01)
				{
					destMask = 0x80;
					dest --;
				}
				else
					destMask >>= 1;

				inNb--;
			}
		}
		UnlockData();
	}
}


void VArrayBoolean::GetValue(VValueSingle& outValue, sLONG inElement) const
{
	outValue.FromBoolean(GetBoolean(inElement));
}

VValueSingle* VArrayBoolean::CloneValue (sLONG inElement) const
{
	VBoolean*			vboolResult = new VBoolean ( );
	GetValue ( *vboolResult, inElement);

	return vboolResult;
}


void VArrayBoolean::FromValue(const VValueSingle& inValue, sLONG inElement)
{
	FromBoolean(inValue.GetBoolean(), inElement);
}


CompareResult VArrayBoolean::CompareTo(const VValueSingle& inValue, Boolean /*inDiacritic*/, sLONG inElement) const
{
	Boolean a, b;
	
	a = GetBoolean(inElement);
	b = inValue.GetBoolean();

	if (a == b)
		return CR_EQUAL;
	else if (b)
		return CR_BIGGER;
	else
		return CR_SMALLER;
}

void VArrayBoolean::InitElements(sLONG inIndex, sLONG inNb )
{
	uBYTE* ptr;
	uBYTE mask;
	sLONG i;

	uBYTE* pt = LockAndGetData();

	uBYTE* ptstart = pt + 1 + ( inIndex >> 3 );
	uBYTE* ptend   = pt - 1 + ( ( inIndex + inNb ) >> 3 );

	if ( ptend > ptstart )
	{
		memset( ptstart, 0, ptend - ptstart );

		ptr = ptstart - 1;
		for ( i = inIndex & 0x00000007; i < 8; i++ )
		{
			mask = (uBYTE) (0x01 << (i & 0x00000007));
			*ptr &= ~mask;
		}

		ptr = ptend + 1;
		for ( i = 0; i < ( ( inIndex + inNb ) & 0x00000007 ); i++ )
		{
			mask = (uBYTE) (0x01 << (i & 0x00000007));
			*ptr &= ~mask;
		}
	}
	else
	{
		for ( i = inIndex; i < inIndex + inNb; i++ )
		{
			ptr = pt + ( i >> 3 );
			mask = (uBYTE) (0x01 << (i & 0x00000007));
			*ptr &= ~mask;
		}
	}

	UnlockData();
}


Boolean VArrayBoolean::GetBoolean(sLONG inElement) const
{
	if (ValidIndex(inElement))
	{
		inElement--;
		uBYTE* ptr = (uBYTE*) LockAndGetData();
		ptr += inElement >> 3;
		uBYTE mask = (uBYTE) (0x01 << (inElement & 0x00000007));
		Boolean result = (*ptr & mask) != 0;
		UnlockData();
		return result;
	}
	else
		return false;
}


void VArrayBoolean::FromBoolean(Boolean inValue, sLONG inElement)
{
	if (ValidIndex(inElement))
	{
		inElement--;
		uBYTE* ptr = (uBYTE*) LockAndGetData();
		ptr += inElement >> 3;
		uBYTE mask = (uBYTE) (0x01 << (inElement & 0x00000007));
	
		if (inValue)
			*ptr |= mask;
		else
			*ptr &= ~mask;
	
		UnlockData();
	}
}


void VArrayBoolean::SwapElements(uBYTE* data, sLONG inA, sLONG inB)
{
	uBYTE*	ptrA = data + (inA >> 3);
	uBYTE	maskA = (uBYTE) (0x01 << (inA & 0x00000007));
	Boolean	temp = (*ptrA & maskA) != 0;

	uBYTE*	ptrB = data + (inB >> 3);
	uBYTE	maskB = (uBYTE) (0x01 << (inB & 0x00000007));

	if (*ptrB & maskB)
		*ptrA |= maskA;
	else
		*ptrA &= ~maskA;

	if (temp)
		*ptrB |= maskB;
	else
		*ptrB &= ~maskB;
}


CompareResult VArrayBoolean::CompareElements(uBYTE* data, sLONG inA, sLONG inB)
{
	uBYTE* ptrA = data + (inA >> 3);
	uBYTE maskA = (uBYTE) (0x01 << (inA & 0x00000007));
	Boolean a = (*ptrA & maskA) != 0;

	uBYTE* ptrB = data + (inB >> 3);
	uBYTE maskB = (uBYTE) (0x01 << (inB & 0x00000007));
	Boolean b = (*ptrB & maskB) != 0;

	if (a == b)
		return CR_EQUAL;
	else if (b)
		return CR_BIGGER;
	else
		return CR_SMALLER;
}


const VValueInfo *VArrayBoolean::GetValueInfo() const
{
	return &sInfo;
}


#pragma mark-

VArrayByte::VArrayByte(sLONG inNbElems)
{
	fElemSize = sizeof(sBYTE);	
	SetCount(inNbElems);
}


VArrayByte::VArrayByte(const VArrayByte& inOriginal)
{
	Copy(inOriginal);
}


sLONG VArrayByte::Find(const VValueSingle& inValue, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	return Find( (sBYTE) inValue.GetWord(), inFrom, inUp, inCm);
}


sBYTE VArrayByte::GetMax() const
{
	sLONG count = fCount;
	sBYTE maxValue = kMIN_sBYTE;
	sBYTE value;

	sBYTE* ptr = (sBYTE*) LockAndGetData();
	while (count--)
	{
		value = *ptr++;
		if (value > maxValue)
			maxValue = value;
	}
	UnlockData();

	return maxValue;
}


sBYTE VArrayByte::GetMin() const
{
	sLONG count = fCount;
	sBYTE minValue = kMAX_sBYTE;
	sBYTE value;

	sBYTE* ptr = (sBYTE*) LockAndGetData();
	while (count--)
	{
		value = *ptr++;
		if (value < minValue)
			minValue = value;
	}
	UnlockData();

	return minValue;
}


void VArrayByte::GetValue(VValueSingle& outValue, sLONG inElement) const
{
	outValue.FromByte(GetByte(inElement));
}

VValueSingle* VArrayByte::CloneValue (sLONG inElement) const
{
	VByte*			vbyteResult = new VByte ( );
	GetValue ( *vbyteResult, inElement);

	return vbyteResult;
}



void VArrayByte::FromValue(const VValueSingle& inValue, sLONG inElement)
{
	FromByte(inValue.GetByte(), inElement);
}


sLONG VArrayByte::QuickFind(sBYTE inValue) const
{
	sLONG low, high, test, result;
	sBYTE* data;
	
	data = (sBYTE*) LockAndGetData();

	if (!data)
		return -1;

	result = -1;
	low = 0;
	high = fCount-1;

	while (low <= high)
	{
		test = (low + high) >> 1;

		if (data[ test ] == inValue)
		{
			result = test + 1;
			break;
		}
		else if (data[ test ] > inValue)
			high = test - 1;
		else
			low = test + 1;
	}
	UnlockData();

	return result;
}


void VArrayByte::Sort(sLONG inFrom, sLONG inTo, Boolean inDescending)
{
	if (inFrom <= 0 || inTo <= inFrom || inTo > fCount)
	{
		DebugMsg("Bad parameters for sorting");
		return;
	}

	inFrom--;

	sBYTE* data = (sBYTE*) LockAndGetData();
	QSort<sBYTE>(data + inFrom, inTo - inFrom, inDescending);
	UnlockData();
}


VError VArrayByte::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	sLONG count = inStream->GetLong();

	if (count >= 0 && SetCount(count))
	{
		sBYTE* data = (sBYTE*) LockAndGetData();
		VSize read = fCount;
		inStream->GetBytes(data, &read);
		UnlockData();
	}
	else
		return VMemory::GetLastError();

	return inStream->GetLastError();
}


VError VArrayByte::WriteToStream(VStream* inStream, sLONG inParam) const
{
	if (inParam == 'VVAL')
		inStream->PutLong(GetValueKind());

	inStream->PutLong(fCount);
	
	if (fCount > 0)
	{
		sBYTE* data = (sBYTE*) LockAndGetData();
		inStream->PutBytes(data, fCount);
		UnlockData();
	}
	
	return inStream->GetLastError();
}


sLONG VArrayByte::Find(sBYTE inValue, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	sLONG result=-1;
	sBYTE*	data;
	sBYTE*	current;
	sBYTE*	last;
	
	if (!fCount || inFrom > fCount)
		return -1;

	data = (sBYTE*) LockAndGetData();
	
	if (!data)
		return -1;

	current = data + inFrom - 1;
	last = data + fCount;
	
	switch (inCm)
	{
		case CM_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current == inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current == inValue)
						break;
					current--;
				}
			}
			break;

		case CM_SMALLER:
			if (inUp)
			{
				while (current < last)
				{
					if (*current > inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current > inValue)
						break;
					current--;
				}
			}
			break;

		case CM_SMALLER_OR_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current >= inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current >= inValue)
						break;
					current--;
				}
			}
			break;

		case CM_BIGGER:
			if (inUp)
			{
				while (current < last)
				{
					if (*current < inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current < inValue)
						break;
					current--;
				}
			}
			break;

		case CM_BIGGER_OR_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current <= inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current <= inValue)
						break;
					current--;
				}
			}
			break;
	}
	

	if (current < data || current >= last) 
		result= -1;
	else 
		result = (sLONG) (1 + current - data);
	UnlockData();
	return result;
}


CompareResult VArrayByte::CompareTo(const VValueSingle& inValue, Boolean /*inDiacritic*/, sLONG inElement) const
{
	sBYTE a, b;
	
	a = GetByte(inElement);
	b = inValue.GetByte();

	if (a < b)
		return CR_SMALLER;
	else if (a > b)
		return CR_BIGGER;
	else
		return CR_EQUAL;
}


void VArrayByte::FromByte(sBYTE inValue, sLONG inElement)
{
	if (ValidIndex(inElement))
	{
		sBYTE* data = (sBYTE*) LockAndGetData();
		data[ inElement - 1 ] = inValue;
		UnlockData();
	}
}


sBYTE VArrayByte::GetByte(sLONG inElement) const
{
	if (ValidIndex(inElement))
	{
		sBYTE* data = (sBYTE*) LockAndGetData();
		sBYTE value = data[ inElement - 1 ];
		UnlockData();
		return value;
	}
	else
		return 0;
}


void VArrayByte::SwapElements(uBYTE* inData, sLONG inA, sLONG inB)
{
	sBYTE* ptrA = ((sBYTE*) inData) + inA;
	sBYTE* ptrB = ((sBYTE*) inData) + inB;

	sBYTE temp = *ptrA;
	*ptrA = *ptrB;
	*ptrB = temp;
}


CompareResult VArrayByte::CompareElements(uBYTE* inData, sLONG inA, sLONG inB)
{
	sBYTE* ptrA = ((sBYTE*) inData) + inA;
	sBYTE* ptrB = ((sBYTE*) inData) + inB;

	if (*ptrA < *ptrB)
		return CR_SMALLER;
	else if (*ptrA > *ptrB)
		return CR_BIGGER;
	else
		return CR_EQUAL;
}


const VValueInfo *VArrayByte::GetValueInfo() const
{
	return &sInfo;
}


#pragma mark-

VArrayWord::VArrayWord(sLONG inNbElems)
{
	fElemSize = sizeof(sWORD);	
	SetCount(inNbElems);
}


VArrayWord::VArrayWord(const VArrayWord& inOriginal)
{
	Copy(inOriginal);
}


sLONG VArrayWord::Find(const VValueSingle& inValue, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	return Find(inValue.GetWord(), inFrom, inUp, inCm);
}


sWORD VArrayWord::GetMax() const
{
	sLONG count = fCount;
	sWORD maxValue = kMIN_sWORD;
	sWORD value;

	sWORD* ptr = (sWORD*) LockAndGetData();
	while (count--)
	{
		value = *ptr++;
		if (value > maxValue)
			maxValue = value;
	}
	UnlockData();

	return maxValue;
}


sWORD VArrayWord::GetMin() const
{
	sLONG count = fCount;
	sWORD minValue = kMAX_sWORD;
	sWORD value;

	sWORD* ptr = (sWORD*) LockAndGetData();
	while (count--)
	{
		value = *ptr++;
		if (value < minValue)
			minValue = value;
	}
	UnlockData();

	return minValue;
}


void VArrayWord::GetValue(VValueSingle& outValue, sLONG inElement) const
{
	outValue.FromWord(GetWord(inElement));
}

VValueSingle* VArrayWord::CloneValue (sLONG inElement) const
{
	VWord*			vwordResult = new VWord ( );
	GetValue ( *vwordResult, inElement);

	return vwordResult;
}



void VArrayWord::FromValue(const VValueSingle& inValue, sLONG inElement)
{
	FromWord(inValue.GetWord(), inElement);
}


sLONG VArrayWord::QuickFind(sWORD inValue) const
{
	sLONG low, high, test, result;
	sWORD* data;
	
	data = (sWORD*) LockAndGetData();

	if (!data)
		return -1;

	result = -1;
	low = 0;
	high = fCount-1;

	while (low <= high)
	{
		test = (low + high) >> 1;

		if (data[ test ] == inValue)
		{
			result = test + 1;
			break;
		}
		else if (data[ test ] > inValue)
			high = test - 1;
		else
			low = test + 1;
	}
	UnlockData();

	return result;
}


void VArrayWord::Sort(sLONG inFrom, sLONG inTo, Boolean inDescending)
{
	if (inFrom <= 0 || inTo <= inFrom || inTo > fCount)
	{
		DebugMsg("Bad parameters for sorting");
		return;
	}

	inFrom--;

	sWORD* data = (sWORD*) LockAndGetData();
	QSort<sWORD>(data + inFrom, inTo - inFrom, inDescending);
	UnlockData();
}


VError VArrayWord::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	sLONG count = inStream->GetLong();

	if (count >= 0 && SetCount(count))
	{
		sWORD* data = (sWORD*) LockAndGetData();
		sLONG read = fCount;
		inStream->GetWords(data, &read);
		UnlockData();
	}
	else
		return VMemory::GetLastError();

	return inStream->GetLastError();
}


VError VArrayWord::WriteToStream(VStream* inStream, sLONG inParam) const
{
	if (inParam == 'VVAL')
		inStream->PutLong(GetValueKind());

	inStream->PutLong(fCount);
	
	if (fCount > 0)
	{
		sWORD* data = (sWORD*) LockAndGetData();
		inStream->PutWords(data, fCount);
		UnlockData();
	}
	
	return inStream->GetLastError();
}


sLONG VArrayWord::Find(sWORD inValue, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	sLONG result=-1;
	sWORD*	data;
	sWORD*	current;
	sWORD*	last;
	
	if (!fCount || inFrom > fCount)
		return -1;

	data = (sWORD*) LockAndGetData();
	
	if (!data)
		return -1;

	current = data + inFrom - 1;
	last = data + fCount;
	
	switch (inCm)
	{
		case CM_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current == inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current == inValue)
						break;
					current--;
				}
			}
			break;

		case CM_SMALLER:
			if (inUp)
			{
				while (current < last)
				{
					if (*current > inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current > inValue)
						break;
					current--;
				}
			}
			break;

		case CM_SMALLER_OR_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current >= inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current >= inValue)
						break;
					current--;
				}
			}
			break;

		case CM_BIGGER:
			if (inUp)
			{
				while (current < last)
				{
					if (*current < inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current < inValue)
						break;
					current--;
				}
			}
			break;

		case CM_BIGGER_OR_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current <= inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current <= inValue)
						break;
					current--;
				}
			}
			break;
	}
	

	if (current < data || current >= last) 
		result= -1;
	else 
		result = (sLONG) (1 + current - data);
	UnlockData();
	return result;
}


CompareResult VArrayWord::CompareTo(const VValueSingle& inValue, Boolean /*inDiacritic*/, sLONG inElement) const
{
	sWORD a, b;
	
	a = GetWord(inElement);
	b = inValue.GetWord();

	if (a < b)
		return CR_SMALLER;
	else if (a > b)
		return CR_BIGGER;
	else
		return CR_EQUAL;
}


void VArrayWord::FromWord(sWORD inValue, sLONG inElement)
{
	if (ValidIndex(inElement))
	{
		sWORD* data = (sWORD*) LockAndGetData();
		data[ inElement - 1 ] = inValue;
		UnlockData();
	}
}


sWORD VArrayWord::GetWord(sLONG inElement) const
{
	if (ValidIndex(inElement))
	{
		sWORD* data = (sWORD*) LockAndGetData();
		sWORD value = data[ inElement - 1 ];
		UnlockData();
		return value;
	}
	else
		return 0;
}


void VArrayWord::SwapElements(uBYTE* inData, sLONG inA, sLONG inB)
{
	sWORD* ptrA = ((sWORD*) inData) + inA;
	sWORD* ptrB = ((sWORD*) inData) + inB;

	sWORD temp = *ptrA;
	*ptrA = *ptrB;
	*ptrB = temp;
}


CompareResult VArrayWord::CompareElements(uBYTE* inData, sLONG inA, sLONG inB)
{
	sWORD* ptrA = ((sWORD*) inData) + inA;
	sWORD* ptrB = ((sWORD*) inData) + inB;

	if (*ptrA < *ptrB)
		return CR_SMALLER;
	else if (*ptrA > *ptrB)
		return CR_BIGGER;
	else
		return CR_EQUAL;
}


const VValueInfo *VArrayWord::GetValueInfo() const
{
	return &sInfo;
}


#pragma mark-

VArrayLong::VArrayLong(sLONG inNbElems)
{
	fElemSize = sizeof(sLONG);
	SetCount(inNbElems);
}


VArrayLong::VArrayLong(const VArrayLong& inOriginal)
{
	Copy(inOriginal);
}

VArrayLong& VArrayLong::operator=(const VArrayLong& pFrom)
{
	Copy(pFrom);
	return (*this);
}


sLONG VArrayLong::Find(const VValueSingle& inValue, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	return Find(inValue.GetLong(), inFrom, inUp, inCm);
}


sLONG VArrayLong::GetMax() const
{
	sLONG count = fCount;
	sLONG maxValue = kMIN_sLONG;
	sLONG value;

	sLONG* ptr = (sLONG*) LockAndGetData();
	while (count--)
	{
		value = *ptr++;
		if (value > maxValue)
			maxValue = value;
	}
	UnlockData();

	return maxValue;
}


sLONG VArrayLong::GetMin() const
{
	sLONG	count = fCount;
	sLONG	minValue = kMAX_sLONG;
	sLONG	value;

	sLONG* ptr = (sLONG*) LockAndGetData();
	while (count--)
	{
		value = *ptr++;
		if (value < minValue)
			minValue = value;
	}
	UnlockData();

	return minValue;
}


void VArrayLong::GetValue(VValueSingle& outValue, sLONG inElement) const
{
	outValue.FromLong(GetLong(inElement));
}

VValueSingle* VArrayLong::CloneValue (sLONG inElement) const
{
	VLong*			vlongResult = new VLong ( );
	GetValue ( *vlongResult, inElement);

	return vlongResult;
}



void VArrayLong::FromValue(const VValueSingle& inValue, sLONG inElement)
{
	FromLong(inValue.GetLong(), inElement);
}


void VArrayLong::SwapElements(uBYTE* inData, sLONG inA, sLONG inB)
{
	sLONG*	ptrA = ((sLONG*) inData) + inA;
	sLONG*	ptrB = ((sLONG*) inData) + inB;

	sLONG	temp = *ptrA;
	*ptrA = *ptrB;
	*ptrB = temp;
}


CompareResult VArrayLong::CompareElements(uBYTE* inData, sLONG inA, sLONG inB)
{
	sLONG*	ptrA = ((sLONG*) inData) + inA;
	sLONG*	ptrB = ((sLONG*) inData) + inB;

	if (*ptrA < *ptrB)
		return CR_SMALLER;
	else if (*ptrA > *ptrB)
		return CR_BIGGER;
	else
		return CR_EQUAL;
}


sLONG VArrayLong::QuickFind(sLONG inValue) const
{
	sLONG*	data = (sLONG*) LockAndGetData();

	if (!data)
		return -1;

	sLONG	result = -1;
	sLONG	low = 0;
	sLONG	high = fCount-1;

	while (low <= high)
	{
		sLONG	test = (low + high) >> 1;

		if (data[ test ] == inValue)
		{
			result = test + 1;
			break;
		}
		else if (data[ test ] > inValue)
			high = test - 1;
		else
			low = test + 1;
	}
	UnlockData();

	return result;
}


void VArrayLong::Sort(sLONG inFrom, sLONG inTo, Boolean inDescending)
{
	if (inFrom <= 0 || inTo <= inFrom || inTo > fCount)
	{
		DebugMsg("Bad parameters for sorting");
		return;
	}

	inFrom--;

	sLONG* data = (sLONG*) LockAndGetData();
	QSort<sLONG>(data + inFrom, inTo - inFrom, inDescending);
	UnlockData();
}


VError VArrayLong::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	sLONG count = inStream->GetLong();

	if (count >= 0 && 	SetCount(count))
	{
		sLONG*	data = (sLONG*) LockAndGetData();
		sLONG	read = fCount;
		inStream->GetLongs(data, &read);
		UnlockData();
	}
	else
		return VMemory::GetLastError();

	return inStream->GetLastError();
}



VError VArrayLong::WriteToStream(VStream* inStream, sLONG inParam) const
{
	if (inParam == 'VVAL')
		inStream->PutLong(GetValueKind());

	inStream->PutLong(fCount);
	
	if (fCount > 0)
	{
		sLONG*	data = (sLONG*) LockAndGetData();
		inStream->PutLongs(data, fCount);
		UnlockData();
	}
	
	return inStream->GetLastError();
}


sLONG VArrayLong::Find(sLONG inValue, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	sLONG	result = -1;
	
	if (!fCount || inFrom > fCount)
		return -1;

	sLONG*	data = (sLONG*) LockAndGetData();
	
	if (!data)
		return -1;

	sLONG*	current = data + inFrom - 1;
	sLONG*	last = data + fCount;
	
	switch (inCm)
	{
		case CM_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current == inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current == inValue)
						break;
					current--;
				}
			}
			break;

		case CM_SMALLER:
			if (inUp)
			{
				while (current < last)
				{
					if (*current > inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current > inValue)
						break;
					current--;
				}
			}
			break;

		case CM_SMALLER_OR_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current >= inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current >= inValue)
						break;
					current--;
				}
			}
			break;

		case CM_BIGGER:
			if (inUp)
			{
				while (current < last)
				{
					if (*current < inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current < inValue)
						break;
					current--;
				}
			}
			break;

		case CM_BIGGER_OR_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current <= inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current <= inValue)
						break;
					current--;
				}
			}
			break;
	}
	
	if (current < data || current >= last) 
		result= -1;
	else 
		result = (sLONG) (1 + current - data);
	UnlockData();
	return result;
}


void VArrayLong::FromLong(sLONG inValue, sLONG inElement)
{
	if (ValidIndex(inElement))
	{
		sLONG*	data = (sLONG*) LockAndGetData();
		data[inElement - 1] = inValue;
		UnlockData();
	}
}


sLONG VArrayLong::GetLong(sLONG inElement) const
{
	if (ValidIndex(inElement))
	{
		sLONG*	data = (sLONG*) LockAndGetData();
		sLONG	value = data[inElement - 1];
		UnlockData();
		return value;
	}
	else
		return 0;
}


CompareResult VArrayLong::CompareTo(const VValueSingle& inValue, Boolean /*inDiacritic*/, sLONG inElement) const
{
	sLONG	a = GetLong(inElement);
	sLONG	b = inValue.GetLong();

	if (a < b)
		return CR_SMALLER;
	else if (a > b)
		return CR_BIGGER;
	else
		return CR_EQUAL;
}


const VValueInfo *VArrayLong::GetValueInfo() const
{
	return &sInfo;
}


#pragma mark-

VArrayLong8::VArrayLong8(sLONG inNbElems)
{
	fElemSize = sizeof(sLONG8);
	SetCount(inNbElems);
}


VArrayLong8::VArrayLong8(const VArrayLong8& inOriginal)
{
	Copy(inOriginal);
}


sLONG VArrayLong8::Find(const VValueSingle& inValue, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	return Find(inValue.GetLong8(), inFrom, inUp, inCm);
}


void VArrayLong8::GetValue(VValueSingle& outValue, sLONG inElement) const
{
	outValue.FromLong8(GetLong8(inElement));
}

VValueSingle* VArrayLong8::CloneValue (sLONG inElement) const
{
	VLong8*			vlong8Result = new VLong8 ( );
	GetValue ( *vlong8Result, inElement);

	return vlong8Result;
}


void VArrayLong8::FromValue(const VValueSingle& inValue, sLONG inElement)
{
	FromLong8(inValue.GetLong8(), inElement);
}


void VArrayLong8::SwapElements(uBYTE* inData, sLONG inA, sLONG inB)
{
	sLONG8*	ptrA = ((sLONG8*) inData) + inA;
	sLONG8*	ptrB = ((sLONG8*) inData) + inB;

	sLONG8 temp = *ptrA;
	*ptrA = *ptrB;
	*ptrB = temp;
}


CompareResult VArrayLong8::CompareElements(uBYTE* inData, sLONG inA, sLONG inB)
{
	sLONG8*	ptrA = ((sLONG8*) inData) + inA;
	sLONG8* ptrB = ((sLONG8*) inData) + inB;

	if (*ptrA < *ptrB)
		return CR_SMALLER;
	else if (*ptrA > *ptrB)
		return CR_BIGGER;
	else
		return CR_EQUAL;
}


sLONG VArrayLong8::QuickFind(sLONG8 inValue) const
{
	sLONG8*	data = (sLONG8*) LockAndGetData();

	if (!data)
		return -1;

	sLONG	result = -1;
	sLONG	low = 0;
	sLONG	high = fCount-1;

	while (low <= high)
	{
		sLONG	test = (low + high) >> 1;

		if (data[ test ] == inValue)
		{
			result = test + 1;
			break;
		}
		else if (data[ test ] > inValue)
			high = test - 1;
		else
			low = test + 1;
	}
	UnlockData();

	return result;
}


void VArrayLong8::Sort(sLONG inFrom, sLONG inTo, Boolean inDescending)
{
	if (inFrom <= 0 || inTo <= inFrom || inTo > fCount)
	{
		DebugMsg("Bad parameters for sorting");
		return;
	}

	inFrom--;

	sLONG8* data = (sLONG8*) LockAndGetData();
	QSort<sLONG8>(data + inFrom, inTo - inFrom, inDescending);
	UnlockData();
}


VError VArrayLong8::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	sLONG count = inStream->GetLong();

	if (count >= 0 && 	SetCount(count))
	{
		sLONG8* data = (sLONG8*) LockAndGetData();
		sLONG read = fCount;
		inStream->GetLong8s(data, &read);
		UnlockData();
	}
	else
		return VMemory::GetLastError();

	return inStream->GetLastError();
}


VError VArrayLong8::WriteToStream(VStream* inStream, sLONG inParam) const
{
	if (inParam == 'VVAL')
		inStream->PutLong(GetValueKind());

	inStream->PutLong(fCount);

	if (fCount > 0)
	{
		sLONG8* data = (sLONG8*) LockAndGetData();
		inStream->PutLong8s(data, fCount);
		UnlockData();
	}
	
	return inStream->GetLastError();
}


sLONG VArrayLong8::Find(sLONG8 inValue, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	sLONG	result = -1;
	
	if (!fCount || inFrom > fCount)
		return -1;

	sLONG8*	data = (sLONG8*) LockAndGetData();
	
	if (!data)
		return -1;

	sLONG8*	current = data + inFrom - 1;
	sLONG8*	last = data + fCount;
	
	switch (inCm)
	{
		case CM_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current == inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current == inValue)
						break;
					current--;
				}
			}
			break;

		case CM_SMALLER:
			if (inUp)
			{
				while (current < last)
				{
					if (*current > inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current > inValue)
						break;
					current--;
				}
			}
			break;

		case CM_SMALLER_OR_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current >= inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current >= inValue)
						break;
					current--;
				}
			}
			break;

		case CM_BIGGER:
			if (inUp)
			{
				while (current < last)
				{
					if (*current < inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current < inValue)
						break;
					current--;
				}
			}
			break;

		case CM_BIGGER_OR_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current <= inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current <= inValue)
						break;
					current--;
				}
			}
			break;
	}
	

	if (current < data || current >= last) 
		result= -1;
	else 
		result = (sLONG) (1 + current - data);
	UnlockData();
	return result;
}


void VArrayLong8::FromLong8(sLONG8 inValue, sLONG inElement)
{
	if (ValidIndex(inElement))
	{
		sLONG8*	data = (sLONG8*) LockAndGetData();
		data[ inElement - 1 ] = inValue;
		UnlockData();
	}
}


sLONG8 VArrayLong8::GetLong8(sLONG inElement) const
{
	if (ValidIndex(inElement))
	{
		sLONG8*	data = (sLONG8*) LockAndGetData();
		sLONG8 value = data[ inElement - 1 ];
		UnlockData();
		return value;
	}
	else
		return 0;
}


CompareResult VArrayLong8::CompareTo(const VValueSingle& inValue, Boolean /*inDiacritic*/, sLONG inElement) const
{
	sLONG8	a = GetLong8(inElement);
	sLONG8	b = inValue.GetLong8();

	if (a < b)
		return CR_SMALLER;
	else if (a > b)
		return CR_BIGGER;
	else
		return CR_EQUAL;
}


const VValueInfo *VArrayLong8::GetValueInfo() const
{
	return &sInfo;
}


#pragma mark-

VArrayReal::VArrayReal(sLONG inNbElems)
{
	fElemSize = sizeof(Real);
	SetCount(inNbElems);
}


VArrayReal::VArrayReal(const VArrayReal& inOriginal)
{
	Copy(inOriginal);
}


sLONG VArrayReal::Find(const VValueSingle& inValue, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	return Find(inValue.GetReal(), inFrom, inUp, inCm);
}


void VArrayReal::GetValue(VValueSingle& outValue, sLONG inElement) const
{
	outValue.FromReal(GetReal(inElement));
}

VValueSingle* VArrayReal::CloneValue (sLONG inElement) const
{
	VReal*			vrealResult = new VReal ( );
	GetValue ( *vrealResult, inElement);

	return vrealResult;
}



void VArrayReal::FromValue(const VValueSingle& inValue, sLONG inElement)
{
	FromReal( inValue.GetReal(), inElement);
}


void VArrayReal::SwapElements(uBYTE* inData, sLONG inA, sLONG inB)
{
	Real*	ptrA = ((Real*) inData) + inA;
	Real*	ptrB = ((Real*) inData) + inB;

	Real	temp = *ptrA;
	*ptrA = *ptrB;
	*ptrB = temp;
}


CompareResult VArrayReal::CompareElements(uBYTE* inData, sLONG inA, sLONG inB)
{
	Real*	ptrA = ((Real*) inData) + inA;
	Real*	ptrB = ((Real*) inData) + inB;

	if (*ptrA < *ptrB)
		return CR_SMALLER;
	else if (*ptrA > *ptrB)
		return CR_BIGGER;
	else
		return CR_EQUAL;
}


sLONG VArrayReal::QuickFind(Real inValue) const
{
	Real*	data = (Real*) LockAndGetData();

	if (!data)
		return -1;

	sLONG	result = -1;
	sLONG	low = 0;
	sLONG	high = fCount-1;

	while (low <= high)
	{
		sLONG	test = (low + high) >> 1;

		if (data[test] == inValue)
		{
			result = test + 1;
			break;
		}
		else if (data[test] > inValue)
			high = test - 1;
		else
			low = test + 1;
	}
	UnlockData();

	return result;
}


Real VArrayReal::GetAverage() const
{
	if (fCount)
		return GetSum() / fCount;
	else
		return 0;
}


Real VArrayReal::GetSquareSum() const
{
	if (!fCount)
		return 0;

	Real	average = GetAverage();
	Real	result = 0;
	Real*	start = (Real*) LockAndGetData();
	Real*	end = start + fCount;
	
	while (start < end)
	{
		Real	r = *start++;
		r -= average;
		result += r * r;
	}

	UnlockData();
	
	return result;
}


Real VArrayReal::GetSum() const
{
	if (!fCount)
		return 0;

	Real	result = 0;
	Real*	start = (Real*)LockAndGetData();
	Real*	end = start + fCount;
	
	while(start<end)
		result += *start++;
	
	UnlockData();
	return result;
}


Real VArrayReal::GetStdDev() const
{
	if (!fCount)
		return 0;

	return GetSquareSum() / fCount;
}


void VArrayReal::Sort(sLONG inFrom, sLONG inTo, Boolean inDescending)
{
	if (inFrom <= 0 || inTo <= inFrom || inTo > fCount)
	{
		DebugMsg("Bad parameters for sorting");
		return;
	}

	inFrom--;

	Real* data = (Real*) LockAndGetData();
	QSort<Real>(data + inFrom, inTo - inFrom, inDescending);
	UnlockData();
}


VError VArrayReal::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	sLONG	count = inStream->GetLong();

	if (count >= 0 && SetCount(count))
	{
		Real* data = (Real*) LockAndGetData();
		sLONG read = fCount;
		inStream->GetReals(data, &read);
		UnlockData();
	}
	else
		return VMemory::GetLastError();

	return inStream->GetLastError();
}


VError VArrayReal::WriteToStream(VStream* inStream, sLONG inParam) const
{
	if (inParam == 'VVAL')
		inStream->PutLong(GetValueKind());

	inStream->PutLong(fCount);
	
	if (fCount > 0)
	{
		Real* data = (Real*) LockAndGetData();
		inStream->PutReals(data, fCount);
		UnlockData();
	}
	
	return inStream->GetLastError();
}


sLONG VArrayReal::Find(Real inValue, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	sLONG	result = -1;
	
	if (!fCount || inFrom > fCount)
		return -1;

	Real*	data = (Real*) LockAndGetData();
	
	if (!data)
		return -1;

	Real*	current = data + inFrom - 1;
	Real*	last = data + fCount;
	
	switch (inCm)
	{
		case CM_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current == inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current == inValue)
						break;
					current--;
				}
			}
			break;

		case CM_SMALLER:
			if (inUp)
			{
				while (current < last)
				{
					if (*current > inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current > inValue)
						break;
					current--;
				}
			}
			break;

		case CM_SMALLER_OR_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current >= inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current >= inValue)
						break;
					current--;
				}
			}
			break;

		case CM_BIGGER:
			if (inUp)
			{
				while (current < last)
				{
					if (*current < inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current < inValue)
						break;
					current--;
				}
			}
			break;

		case CM_BIGGER_OR_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current <= inValue)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current <= inValue)
						break;
					current--;
				}
			}
			break;
	}
	

	if (current < data || current >= last) 
		result= -1;
	else 
		result = (sLONG) (1 + current - data);
	UnlockData();
	return result;
}


Real VArrayReal::GetReal(sLONG inElement) const
{
	if (ValidIndex(inElement))
	{
		Real* data = (Real*) LockAndGetData();
		Real value = data[ inElement - 1 ];
		UnlockData();
		return value;
	}
	else
		return 0;

}


void VArrayReal::FromReal(Real inValue, sLONG inElement)
{
	if (ValidIndex(inElement))
	{
		Real* data = (Real*) LockAndGetData();
		data[ inElement - 1 ] = inValue;
		UnlockData();
	}
}


CompareResult VArrayReal::CompareTo(const VValueSingle& inValue, Boolean /*inDiacritic*/, sLONG inElement) const
{
	Real a, b;
	
	a = GetReal(inElement);
	b = inValue.GetReal();

	if (a < b)
		return CR_SMALLER;
	else if (a > b)
		return CR_BIGGER;
	else
		return CR_EQUAL;
}


const VValueInfo *VArrayReal::GetValueInfo() const
{
	return &sInfo;
}


#pragma mark-

VArrayFloat::VArrayFloat(sLONG inNbElems)
{
	fElemSize = 4;
	SetCount(inNbElems);
}


VArrayFloat::VArrayFloat(const VArrayFloat& inOriginal)
{
	Copy(inOriginal);
}


VArrayFloat::~VArrayFloat()
{
	DeleteElements(0, fCount);
}


void VArrayFloat::DeleteElements(sLONG inAt, sLONG inNb)
{
	if (fDataPtr == NULL) return;
	
	if (!testAssert(inAt >= 0 && inNb >= 0 && inAt + inNb <= fCount))
		return;

	VFloat**	data = (VFloat**) LockAndGetData();
	for (sLONG i = inAt ; i < inAt + inNb ; ++i)
	{
		delete data[i];
		data[i] = NULL;
	}
	UnlockData();
}


sLONG VArrayFloat::Find(const VValueSingle& inValue, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	VFloat f;
	inValue.GetFloat(f);

	return Find(f, inFrom, inUp, inCm);
}


sLONG VArrayFloat::Find(const VFloat& inValue, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	sLONG result=-1;
	VFloat** data;
	VFloat** current;
	VFloat** last;
	
	if (!fCount || inFrom > fCount)
		return -1;

	data = (VFloat**) LockAndGetData();
	
	if (!data)
		return -1;

	current = data + inFrom - 1;
	last = data + fCount;
	
	switch (inCm)
	{
		case CM_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current && (**current == inValue))
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current && (**current == inValue))
						break;
					current--;
				}
			}
			break;

		case CM_SMALLER:
			if (inUp)
			{
				while (current < last)
				{
					if (*current && (**current > inValue))
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current && (**current > inValue))
						break;
					current--;
				}
			}
			break;

		case CM_SMALLER_OR_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current && (**current >= inValue))
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current && (**current >= inValue))
						break;
					current--;
				}
			}
			break;

		case CM_BIGGER:
			if (inUp)
			{
				while (current < last)
				{
					if (*current && (**current < inValue))
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current && (**current < inValue))
						break;
					current--;
				}
			}
			break;

		case CM_BIGGER_OR_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current && (**current <= inValue))
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current && (**current <= inValue))
						break;
					current--;
				}
			}
			break;
	}
	

	if (current < data || current >= last) 
		result= -1;
	else 
		result = (sLONG) (1 + current - data);
	UnlockData();
	return result;
}


void VArrayFloat::FromValue(const VValueSingle& inValue, sLONG inElement)
{
	VFloat f;
	f.FromValue(inValue);
	FromFloat(f, inElement);
}


void VArrayFloat::GetValue(VValueSingle& outValue, sLONG inElement) const
{
	if (ValidIndex(inElement))
	{
		VFloat** data = (VFloat**) LockAndGetData();
		if ( data[ inElement - 1 ] )
			outValue.FromFloat(*data[ inElement - 1 ]);
		else
			outValue.Clear();
		UnlockData();
	}
	else
		outValue.Clear();
}

VValueSingle* VArrayFloat::CloneValue (sLONG inElement) const
{
	VFloat*			vfloatResult = new VFloat ( );
	GetValue ( *vfloatResult, inElement);

	return vfloatResult;
}



void VArrayFloat::FromFloat(const VFloat& inFloat, sLONG inElement)
{
	if (ValidIndex(inElement))
	{
		VFloat** data = (VFloat**) LockAndGetData();
		if (data[ inElement - 1 ])
			inFloat.GetFloat(*data[ inElement - 1 ]);
		else 
			data[ inElement - 1 ] = (VFloat*)inFloat.Clone();
		UnlockData();
	}
}


void VArrayFloat::GetFloat(VFloat& outFloat, sLONG inElement) const
{
	if (ValidIndex(inElement))
	{
		VFloat** data = (VFloat**) LockAndGetData();
		if ( data[ inElement - 1 ] )
			outFloat.FromFloat(*data[ inElement - 1 ]);
		else
			outFloat.Clear();
		UnlockData();
	}
	else
		outFloat.Clear();
}

CompareResult VArrayFloat::CompareTo(const VValueSingle& inValue, Boolean /*inDiacritic*/, sLONG inElement) const
{
	VFloat f;
	GetFloat(f, inElement);
	return f.CompareTo(inValue);
}


VError VArrayFloat::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	sLONG count = inStream->GetLong();

	if (count >= 0 && SetCount(count))
	{
		VFloat** data = (VFloat**) LockAndGetData();
		count = fCount;
	
		while (count--)
		{
			uLONG kind = inStream->GetLong();
			
			if (kind == 'FLOA')
			{
				if (0 == *data)
					*data = new VFloat;

				if (*data)
					(*data)->ReadFromStream(inStream);
			}
			else 
			{
				if (*data)
					delete *data;

				assert(kind == 'NULL');
				*data = 0;
			}
			
			data++;
		}
		UnlockData();
	}
	else
		return VMemory::GetLastError();

	return inStream->GetLastError();
}


VError VArrayFloat::WriteToStream (VStream* inStream, sLONG inParam) const
{
	if (inParam == 'VVAL')
		inStream->PutLong(GetValueKind());

	inStream->PutLong(fCount);
	
	if (fCount > 0)
	{
		VFloat** f = (VFloat**) LockAndGetData();
		sLONG count = fCount;
		
		while (count--)
		{
			if (!*f)
				inStream->PutLong('NULL');
			else
			{
				inStream->PutLong('FLOA');
				(*f)->WriteToStream(inStream);
			}
			f++;
		}
		UnlockData();
	}
	return inStream->GetLastError();
}


sLONG VArrayFloat::QuickFind(const VFloat& inValue) const
{
	sLONG low, high, test, result;
	VFloat** data;
	CompareResult cr;
	VFloat f;
	
	data = (VFloat**) LockAndGetData();

	if (!data)
		return -1;

	result = -1;
	low = 0;
	high = fCount - 1;

	while (low <= high)
	{
		test = (low + high) >> 1;

		if ( data[test] )
			cr = inValue.CompareToSameKind(data[ test ]);
		else
			cr = inValue.CompareToSameKind( &f );
		
		if (cr == CR_EQUAL)
		{
			result = test + 1;
			break;
		}
		else if (cr == CR_SMALLER)
			high = test - 1;
		else
			low = test + 1;
	}
	UnlockData();

	return result;
}


void VArrayFloat::SwapElements(uBYTE* inData, sLONG inA, sLONG inB)
{
	VFloat**	ptrA = ((VFloat**) inData) + inA;
	VFloat**	ptrB = ((VFloat**) inData) + inB;

	VFloat* temp = *ptrA;
	*ptrA = *ptrB;
	*ptrB = temp;
}


CompareResult VArrayFloat::CompareElements(uBYTE* inData, sLONG inA, sLONG inB)
{
	VFloat*	fa = ((VFloat**) inData)[inA];
	VFloat*	fb = ((VFloat**) inData)[inB];

	if (!fa && ! fb)
		return CR_EQUAL;

	if (!fb)
	{
		VFloat tmp;
		return fa->CompareToSameKind( &tmp );
	}


	if (!fa)
	{
		VFloat tmp;
		return tmp.CompareToSameKind(fb);
	}

	return fa->CompareToSameKind(fb);
}


const VValueInfo *VArrayFloat::GetValueInfo() const
{
	return &sInfo;
}


#pragma mark-

VArrayString::VArrayString(sLONG inNbElems)
{
	fElemSize = sizeof(VString*);
	SetCount(inNbElems);	
}


VArrayString::VArrayString(const VArrayString& inOriginal)
{
	Copy(inOriginal);
}


void VArrayString::FromString(const VString& inValue, sLONG inElement)
{
	if (ValidIndex(inElement))
	{
		VString**	data = (VString**) LockAndGetData();
		VString**	str = data + inElement - 1;
	
		if (*str)
			(*str)->FromString(inValue);
		else 
			*str = (VString*) inValue.Clone();

		UnlockData();
	}
}

void VArrayString::GetString(VString& outString, sLONG inElement) const
{
	if (ValidIndex(inElement))
	{
		VString **data = (VString**) LockAndGetData();
		VString *curval = data[ inElement - 1 ];
		UnlockData();
		outString = *curval;
	}
	else
		outString.Clear();
}


sLONG VArrayString::Find(const VValueSingle& inValue, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	VString	str;

	inValue.GetString(str);
	return Find(str, inFrom, inUp, inCm);
}


Boolean VArrayString::Copy(const VArrayValue& inOriginal)
{
	if (VArrayValue::Copy(inOriginal))
	{
		VString** data = (VString**) LockAndGetData();
		VString** end = data + fCount;

		while (data < end) 
		{
			if (*data)
				*data = (VString*) (*data)->Clone();	

			data++;
		}
		
		UnlockData();
		return true;
	}
	else
		return false;
}


void VArrayString::DeleteElements(sLONG inAt, sLONG inNb)
{
	if (fDataPtr == NULL) return;
	
	if (!testAssert(inAt >= 0 && inNb >= 0 && inAt + inNb <= fCount))
		return;

	VString **data = (VString**) LockAndGetData();
	for (sLONG i = inAt ; i < inAt + inNb ; ++i)
	{
		delete data[i];
		data[i] = NULL;
	}

	UnlockData();
}


void VArrayString::SwapElements(uBYTE* inData, sLONG inA, sLONG inB)
{
	VString**	ptrA = ((VString**) inData) + inA;
	VString**	ptrB = ((VString**) inData) + inB;

	VString*	tempPtr = *ptrA;
	*ptrA = *ptrB;
	*ptrB = tempPtr;
}


CompareResult VArrayString::CompareElements(uBYTE* inData, sLONG inA, sLONG inB)
{
	VString*	sa = ((VString**) inData)[inA];
	VString*	sb = ((VString**) inData)[inB];

	if (!sa && !sb)
		return CR_EQUAL;

	if (!sb)
		return CR_BIGGER;

	if (!sa)
		return CR_SMALLER;

	return sa->CompareToString(*sb, true);
}


sLONG VArrayString::QuickFind(const VString& inValue) const
{
	sLONG low, high, test, result;
	VString** data;
	CompareResult cr;
	
	data = (VString**) LockAndGetData();

	if (!data)
		return -1;

	result = -1;
	low = 0;
	high = fCount - 1;

	while (low <= high)
	{
		test = (low + high) >> 1;

		cr = inValue.CompareTo(*data[ test ]);
		
		if (cr == CR_EQUAL)
		{
			result = test + 1;
			break;
		}
		else if (cr == CR_SMALLER)
			high = test - 1;
		else
			low = test + 1;
	}
	
	UnlockData();

	return result;
}


VError VArrayString::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	sLONG count = inStream->GetLong();

	if (count >= 0 && SetCount(count))
	{
		VString** str = (VString**) LockAndGetData();
		count = fCount;
	
		while (count--)
		{
			uLONG kind = inStream->GetLong();
			
			if (kind == 2)	// 2 = OLDBOX::SK_TEXT_UNI
			{
				if (0 == *str)
					*str = new VString;

				if (*str)
					(*str)->ReadFromStream(inStream);
			}
			else 
			{
				if (*str)
					delete *str;

				assert(kind == 'NULL');
				*str = 0;
			}
			
			str++;
		}
		UnlockData();
	}
	else
		return VMemory::GetLastError();

	return inStream->GetLastError();
}


VError VArrayString::WriteToStream(VStream* inStream,sLONG inParam) const
{
	if (inParam == 'VVAL')
		inStream->PutLong(GetValueKind());

	inStream->PutLong(fCount);
	
	if (fCount > 0) 
	{
		VString** str = (VString**) LockAndGetData();
		sLONG count = fCount;
		
		while (count--)
		{
			if (!*str)
				inStream->PutLong('NULL');
			else
			{
				inStream->PutLong(2); // 2 = OLDBOX::SK_TEXT_UNI
				(*str)->WriteToStream(inStream);
			}
			str++;
		}
		UnlockData();
	}
	return inStream->GetLastError();
}


void VArrayString::GetValue(VValueSingle& outValue, sLONG inElement) const
{
	if (ValidIndex(inElement))
	{
		VString **data = (VString**) LockAndGetData();
		VString *curval = data[ inElement - 1 ];
		UnlockData();
		outValue.FromString(*curval);
	}
	else
		outValue.Clear();
}

VValueSingle* VArrayString::CloneValue (sLONG inElement) const
{
	VString*			vstringResult = new VString ( );
	GetValue ( *vstringResult, inElement);

	return vstringResult;
}


void VArrayString::FromValue(const VValueSingle& inValue, sLONG inElement)
{
	VString str;
	str.FromValue(inValue);
	FromString(str, inElement);
}


sLONG VArrayString::Find(const VString& inValue, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	sLONG result=-1;
	VString** data;
	VString** current;
	VString** last;
	
	if (!fCount || inFrom > fCount)
		return -1;

	data = (VString**) LockAndGetData();
	
	if (!data)
		return -1;

	current = data + inFrom - 1;
	last = data + fCount;
	
	switch (inCm)
	{
		case CM_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current && (**current == inValue))
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current && (**current == inValue))
						break;
					current--;
				}
			}
			break;

		case CM_SMALLER:
			if (inUp)
			{
				while (current < last)
				{
					if (*current && (**current > inValue))
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current && (**current > inValue))
						break;
					current--;
				}
			}
			break;

		case CM_SMALLER_OR_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current && (**current >= inValue))
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current && (**current >= inValue))
						break;
					current--;
				}
			}
			break;

		case CM_BIGGER:
			if (inUp)
			{
				while (current < last)
				{
					if (*current && (**current < inValue))
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current && (**current < inValue))
						break;
					current--;
				}
			}
			break;

		case CM_BIGGER_OR_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current && (**current <= inValue))
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current && (**current <= inValue))
						break;
					current--;
				}
			}
			break;
	}
	

	if (current < data || current >= last) 
		result= -1;
	else 
		result = (sLONG) (1 + current - data);
	
	UnlockData();
	
	return result;
}


CompareResult VArrayString::CompareTo(const VValueSingle& inValue, Boolean inDiacritic, sLONG inElement) const
{
	if (ValidIndex(inElement))
	{
		VString **data = (VString**) LockAndGetData();
		VString *curval = data[ inElement - 1 ];
		UnlockData();
		
		VString	 str;
		inValue.GetString(str);
		
		return curval->CompareTo(str, inDiacritic);
	}
	else
		return CR_SMALLER;
}


VArrayString::~VArrayString()
{
	DeleteElements(0, fCount);
}


const VValueInfo *VArrayString::GetValueInfo() const
{
	return &sInfo;
}


#pragma mark-

VArrayDuration::VArrayDuration(sLONG inNbElems)
{
	fElemSize = sizeof(sLONG8);
	SetCount(inNbElems);
}


VArrayDuration::VArrayDuration(const VArrayDuration& inOriginal)
{
	Copy(inOriginal);
}


sLONG VArrayDuration::Find(const VValueSingle& inValue, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	VDuration duration;
	
	inValue.GetDuration(duration);
	return Find(duration, inFrom, inUp, inCm);	
}


sLONG VArrayDuration::Find(const VDuration& inValue, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	sLONG result=-1;
	sLONG8* data;
	sLONG8* current;
	sLONG8* last;
	sLONG8 value;
	
	if (!fCount || inFrom > fCount)
		return -1;

	data = (sLONG8*) LockAndGetData();
	
	if (!data)
		return -1;

	current = data + inFrom - 1;
	last = data + fCount;
	value = inValue.ConvertToMilliseconds();
	
	switch (inCm)
	{
		case CM_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current == value)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current == value)
						break;
					current--;
				}
			}
			break;

		case CM_SMALLER:
			if (inUp)
			{
				while (current < last)
				{
					if (*current > value)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current > value)
						break;
					current--;
				}
			}
			break;

		case CM_SMALLER_OR_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current >= value)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current >= value)
						break;
					current--;
				}
			}
			break;

		case CM_BIGGER:
			if (inUp)
			{
				while (current < last)
				{
					if (*current < value)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current < value)
						break;
					current--;
				}
			}
			break;

		case CM_BIGGER_OR_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current <= value)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current <= value)
						break;
					current--;
				}
			}
			break;
	}
	

	if (current < data || current >= last) 
		result = -1;
	else 
		result = (sLONG) (1 + current - data);
	UnlockData();
	return result;
}


void VArrayDuration::GetDuration(VDuration& outDuration, sLONG inElement) const
{
	if (ValidIndex(inElement))
	{
		sLONG8* data = (sLONG8*) LockAndGetData();
		outDuration.FromNbMilliseconds(data[ inElement - 1 ]);
		UnlockData();
	}
	else
		outDuration.FromNbMilliseconds(0);
}


void VArrayDuration::FromDuration(const VDuration& inDuration, sLONG inElement)
{
	if (ValidIndex(inElement))
	{
		sLONG8*	data = (sLONG8*) LockAndGetData();
		data[ inElement - 1 ] = inDuration.ConvertToMilliseconds();
		UnlockData();
	}
}


void VArrayDuration::GetValue(VValueSingle& outValue, sLONG inElement) const
{
	VDuration duration;
	
	GetDuration(duration, inElement);
	outValue.FromDuration(duration);
}

VValueSingle* VArrayDuration::CloneValue (sLONG inElement) const
{
	VDuration*			vdurationResult = new VDuration ( );
	GetValue ( *vdurationResult, inElement);

	return vdurationResult;
}


void VArrayDuration::FromValue(const VValueSingle& inValue, sLONG inElement)
{
	VDuration duration;
	
	inValue.GetDuration(duration);
	FromDuration(duration, inElement);
}


CompareResult VArrayDuration::CompareTo(const VValueSingle& inValue, Boolean inDiacritic, sLONG inElement) const
{
	VDuration duration;
	GetDuration(duration, inElement);
	
	return duration.CompareTo(inValue, inDiacritic);
}


VError VArrayDuration::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	sLONG count = inStream->GetLong();

	if (count >= 0 && SetCount(count))
	{
		sLONG8* data = (sLONG8*) LockAndGetData();
		sLONG read = fCount;
		inStream->GetLong8s(data, &read);
		UnlockData();
	}
	else
		return VMemory::GetLastError();

	return inStream->GetLastError();
}


VError VArrayDuration::WriteToStream(VStream* inStream, sLONG inParam) const
{
	if (inParam == 'VVAL')
		inStream->PutLong(GetValueKind());

	inStream->PutLong(fCount);

	if (fCount > 0)
	{
		sLONG8* data = (sLONG8*) LockAndGetData();
		inStream->PutLong8s(data, fCount);
		UnlockData();
	}
	return inStream->GetLastError();
}


sLONG VArrayDuration::QuickFind(const VDuration& inDuration) const
{
	sLONG low, high, test, result;
	sLONG8* data;
	sLONG8 value;
	
	data = (sLONG8*) LockAndGetData();
	value = inDuration.ConvertToMilliseconds();

	if (!data)
		return -1;

	result = -1;
	low = 0;
	high = fCount-1;

	while (low <= high)
	{
		test = (low + high) >> 1;

		if (data[ test ] == value)
		{
			result = test + 1;
			break;
		}
		else if (data[ test ] > value)
			high = test - 1;
		else
			low = test + 1;
	}
	UnlockData();

	return result;
}


void VArrayDuration::SwapElements(uBYTE* inData, sLONG inA, sLONG inB)
{
	sLONG8*	ptrA = ((sLONG8*) inData) + inA;
	sLONG8*	ptrB = ((sLONG8*) inData) + inB;

	sLONG8 temp = *ptrA;
	*ptrA = *ptrB;
	*ptrB = temp;
}


CompareResult VArrayDuration::CompareElements(uBYTE* inData, sLONG inA, sLONG inB)
{
	sLONG8* ptrA = ((sLONG8*) inData) + inA;
	sLONG8* ptrB = ((sLONG8*) inData) + inB;

	if (*ptrA < *ptrB)
		return CR_SMALLER;
	else if (*ptrA > *ptrB)
		return CR_BIGGER;
	else
		return CR_EQUAL;
}


const VValueInfo *VArrayDuration::GetValueInfo() const
{
	return &sInfo;
}


#pragma mark-

VArrayTime::VArrayTime(sLONG inNbElems)
{
	fElemSize = sizeof(uLONG8);
	SetCount(inNbElems);
}


VArrayTime::VArrayTime(const VArrayTime& inOriginal)
{
	Copy(inOriginal);
}


sLONG VArrayTime::Find(const VTime& inTime, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	sLONG	result=-1;
	
	if (!fCount || inFrom > fCount)
		return -1;

	uLONG8*	data = (uLONG8*) LockAndGetData();
	
	if (!data)
		return -1;

	uLONG8*	current = data + inFrom - 1;
	uLONG8*	last = data + fCount;
	uLONG8	value = inTime.GetStamp();
	
	switch (inCm)
	{
		case CM_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current == value)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current == value)
						break;
					current--;
				}
			}
			break;

		case CM_SMALLER:
			if (inUp)
			{
				while (current < last)
				{
					if (*current > value)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current > value)
						break;
					current--;
				}
			}
			break;

		case CM_SMALLER_OR_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current >= value)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current >= value)
						break;
					current--;
				}
			}
			break;

		case CM_BIGGER:
			if (inUp)
			{
				while (current < last)
				{
					if (*current < value)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current < value)
						break;
					current--;
				}
			}
			break;

		case CM_BIGGER_OR_EQUAL:
			if (inUp)
			{
				while (current < last)
				{
					if (*current <= value)
						break;
					current++;
				}
			}
			else
			{
				while (current >= data)
				{
					if (*current <= value)
						break;
					current--;
				}
			}
			break;
	}
	

	if (current < data || current >= last) 
		result= -1;
	else 
		result = (sLONG) (1 + current - data);
	UnlockData();
	return result;
}


sLONG VArrayTime::Find(const VValueSingle& inValue, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	VDuration	duration;
	duration.FromValue(inValue);
	
	return Find(duration, inFrom, inUp, inCm);
}


void VArrayTime::GetTime(VTime& outTime, sLONG inElement) const
{
	if (ValidIndex(inElement))
	{
		uLONG8* data = (uLONG8*) LockAndGetData();
		outTime.FromStamp(data[ inElement - 1 ]);
		UnlockData();
	}
}


void VArrayTime::FromTime(const VTime& inTime, sLONG inElement)
{
	if (ValidIndex(inElement))
	{
		uLONG8*	data = (uLONG8*) LockAndGetData();
		data[ inElement - 1 ] = inTime.GetStamp();
		UnlockData();
	}
}


void VArrayTime::GetValue(VValueSingle& outValue, sLONG inElement) const
{
	VTime	time;
	GetTime(time, inElement);
	outValue.FromTime(time);
}

VValueSingle* VArrayTime::CloneValue (sLONG inElement) const
{
	VTime*			vtimeResult = new VTime ( );
	GetValue ( *vtimeResult, inElement);

	return vtimeResult;
}


void VArrayTime::FromValue(const VValueSingle& inValue, sLONG inElement)
{
	VTime	time;
	time.FromValue(inValue);
	FromTime(time, inElement);
}


CompareResult VArrayTime::CompareTo(const VValueSingle& inValue, Boolean inDiacritic, sLONG inElement) const
{
	VTime	time;
	GetTime(time, inElement);
	
	return time.CompareTo(inValue, inDiacritic);
}


VError VArrayTime::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	sLONG count = inStream->GetLong();

	if (count >= 0 && SetCount(count))
	{
		uLONG8*	data = (uLONG8*) LockAndGetData();
		sLONG	read = fCount;
		inStream->GetLong8s(data, &read);
		UnlockData();
	}
	else
		return VMemory::GetLastError();

	return inStream->GetLastError();
}


VError VArrayTime::WriteToStream(VStream* inStream, sLONG inParam) const
{
	if (inParam == 'VVAL')
		inStream->PutLong(GetValueKind());

	inStream->PutLong(fCount);

	if (fCount > 0)
	{
		sLONG8*	data = (sLONG8*) LockAndGetData();
		inStream->PutLong8s(data, fCount);
		UnlockData();
	}
	
	return inStream->GetLastError();
}


void VArrayTime::SwapElements(uBYTE* inData, sLONG inA, sLONG inB)
{
	uLONG8*	ptrA = ((uLONG8*) inData) + inA;
	uLONG8*	ptrB = ((uLONG8*) inData) + inB;

	uLONG8	temp = *ptrA;
	*ptrA = *ptrB;
	*ptrB = temp;
}


CompareResult VArrayTime::CompareElements(uBYTE* inData, sLONG inA, sLONG inB)
{
	uLONG8*	ptrA = ((uLONG8*) inData) + inA;
	uLONG8*	ptrB = ((uLONG8*) inData) + inB;

	if (*ptrA < *ptrB)
		return CR_SMALLER;
	else if (*ptrA > *ptrB)
		return CR_BIGGER;
	else
		return CR_EQUAL;
}


const VValueInfo *VArrayTime::GetValueInfo() const
{
	return &sInfo;
}


#pragma mark-

VArrayImage::VArrayImage(sLONG inNbElems)
{
	fDataPtr = NULL;
	fCount = inNbElems;
	fValues.insert(fValues.end(), fCount, NULL);
}

Boolean	VArrayImage::SetCount (sLONG inNb)
{
	fCount = inNb;
	fValues.insert(fValues.end(), fCount, NULL);

	return true;
}

void VArrayImage::FromValue (const VValueSingle& inValue, sLONG inElement)
{
	if (inValue.GetTrueValueKind() == VK_IMAGE &&
		!ValidIndex(inElement))
	{
		xbox_assert(false);
		return;
	}

	delete fValues[inElement - 1];
	fValues[inElement - 1] = inValue.Clone();
}

void VArrayImage::GetValue (VValueSingle& outValue, sLONG inElement) const
{
	if (ValidIndex(inElement) &&
		fValues[inElement - 1] != NULL)
	{
		outValue = *(fValues[inElement - 1]);
	} else {
		xbox_assert(false);
	}
}

VValueSingle* VArrayImage::CloneValue (sLONG inElement) const
{
	VValueSingle *ret = NULL;

	if (ValidIndex(inElement) &&
		fValues[inElement - 1] != NULL)
	{
		ret = fValues[inElement - 1]->Clone();
	} else {
		xbox_assert(false);
	}

	return ret;
}

sLONG VArrayImage::Find(const VValueSingle& inValue, sLONG inFrom, Boolean inUp, CompareMethod inCm) const
{
	xbox_assert(false);
	return 0;
}


VError VArrayImage::ReadFromStream(VStream* inStream, sLONG /*inParam*/)
{
	// First we reset the vector
	DeleteAllElements();

	fCount = inStream->GetLong();

	const VValueInfo* vvalInfo = VValueSingle::ValueInfoFromValueKind ( VK_IMAGE );
	
	// Then we add all the elements we get from the stream
	if (vvalInfo != NULL)
	{
		VValue *vval = NULL;
		for (unsigned int i = 0; i < fCount; ++i)
		{
			vval = ( VValueSingle* ) vvalInfo->Generate();
			vval->ReadFromStream(inStream);
			fValues.push_back(dynamic_cast<VValueSingle *>(vval));
		}
	}

	return inStream->GetLastError();
}


VError VArrayImage::WriteToStream(VStream* inStream, sLONG inParam) const
{
	inStream->PutLong(fCount);
	
	for (unsigned int i = 0; i < fCount; ++i)
	{
		if (fValues[i] != NULL)
		{
			fValues[i]->WriteToStream(inStream);
		} else {
			xbox_assert(false);
			inStream->PutPointer(NULL);
		}
	}

	return inStream->GetLastError();
}

void VArrayImage::SwapElements (uBYTE* data, sLONG inA, sLONG inB)
{
	xbox_assert(false);
}

CompareResult	VArrayImage::CompareElements (uBYTE* data, sLONG inA, sLONG inB)
{
	xbox_assert(false);
	return CR_UNRELEVANT;
}

void VArrayImage::DeleteElements (sLONG inAt, sLONG inNb)
{
	if ( fCount == 0 )
		return;

	if (!ValidIndex(inAt) || !ValidIndex(inNb))
	{
		xbox_assert(false);
	}

	for (unsigned int i = inAt; i < (inAt + inNb); ++i)
	{
		delete fValues[i - 1];
		fValues[i - 1] = NULL;
	}
}

void VArrayImage::DeleteAllElements ()
{
	DeleteElements(1, fCount);
	fCount = 0;
	fValues.clear();
}

VArrayImage::~VArrayImage()
{
	DeleteAllElements();
}


const VValueInfo *VArrayImage::GetValueInfo() const
{
	return &sInfo;
}
