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
#ifndef __VMemoryBuffer__
#define __VMemoryBuffer__

#include "Kernel/Sources/VObject.h"
#include "Kernel/Sources/VMemoryCpp.h"

BEGIN_TOOLBOX_NAMESPACE

template<VSize PAD_SIZE = 1024, VSize INCREASE_RATIO = 2, VSize MAX_BONUS = 1024*1024>
class VMemoryBuffer : public VObject
{
public:
			VMemoryBuffer() : fMemMgr(VObject::GetMainMemMgr()), fData( NULL), fSize(0), fAllocatedSize(0) {}
	virtual	~VMemoryBuffer()										{ _Dispose();}

			const void*		GetDataPtr() const						{ return fData; }	// may be NULL
			void*			GetDataPtr()							{ return fData; }	// may be NULL
			VSize			GetDataSize() const						{ return fSize; }
			VSize			GetAllocatedSize() const				{ return fAllocatedSize;}

			bool			IsEmpty() const							{ return (fData == NULL) || (fSize == 0);}

			void			SetDataPtr( void* inData, VSize inSize, VSize inAllocatedSize)
				{
					if (inData != fData)
						_Dispose();
					fData = inData;
					fSize = inSize;
					fAllocatedSize = inAllocatedSize;
				}

			void			ForgetData()							{ fData = NULL; fSize = fAllocatedSize = 0;}

			void			Clear()									{ _Dispose(); ForgetData();}

			bool			PutData( VSize inOffset, const void* inBuffer, VSize inNbBytes)
				{
					bool ok = _EnsureSize( inOffset + inNbBytes);
					if (ok)
					{
						::memcpy( (char*) fData + inOffset, inBuffer, inNbBytes);
						if (fSize < inOffset + inNbBytes)
							fSize = inOffset + inNbBytes;
					}

					return ok;
				}

			bool			PutDataAmortized( VSize inOffset, const void* inBuffer, VSize inNbBytes)
				{
					bool ok = _EnsureSizeAmortized( inOffset + inNbBytes);
					if (ok)
					{
						::memcpy( (char*) fData + inOffset, inBuffer, inNbBytes);
						if (fSize < inOffset + inNbBytes)
							fSize = inOffset + inNbBytes;
					}

					return ok;
				}

			bool			AddData( const void* inBuffer, VSize inNbBytes)
				{
					bool ok = _EnsureSizeAmortized( fSize + inNbBytes);
					if (ok)
					{
						::memcpy( (char*) fData + fSize, inBuffer, inNbBytes);
						fSize += inNbBytes;
					}

					return ok;
				}

			bool			GetData( VSize inOffset, void* outBuffer, VSize inNbBytes, VSize* outCount = NULL)
				{
					VSize copied;
					if (testAssert(inOffset <= fSize))
					{
						copied = (inOffset + inNbBytes > fSize) ? (fSize - inOffset) : inNbBytes;
						::memcpy( outBuffer, (const char *) fData + inOffset, copied);
					}
					else
						copied = 0;
					if (outCount)
						*outCount = copied;
					return copied == inNbBytes;
				}

			bool			Fill( VSize inOffset, uBYTE inFiller, VSize inNbBytes)
				{
					bool ok = _EnsureSize( inOffset + inNbBytes);
					if (ok)
					{
						::memset( (char*) fData + inOffset, inFiller, inNbBytes);
						if (fSize < inOffset + inNbBytes)
							fSize = inOffset + inNbBytes;
					}

					return ok;
				}

			bool			SetSize( VSize inNewSize)
				{
					bool ok = _Reallocate( inNewSize);
					if (ok)
						fSize = inNewSize;
					return ok;
				}

			bool			SetSize( VSize inNewSize, uBYTE inFiller)
				{
					bool ok = _Reallocate( inNewSize);
					if (ok)
					{
						if (inNewSize > fSize)
							::memset( (char *) fData + fSize, inFiller, inNewSize - fSize);
						fSize = inNewSize;
					}
					return ok;
				}

			bool			Reserve( VSize inAllocatedSize)
				{
					return _EnsureSize( inAllocatedSize);
				}

			void			ShrinkSizeNoReallocate( VSize inNewSize)
				{
					if (testAssert( inNewSize <= fSize))
						fSize = inNewSize;
				}

protected:
			VCppMemMgr*		fMemMgr;
			void*			fData;
			VSize			fSize;
			VSize			fAllocatedSize;

			VSize			_AdjusteSize( VSize inSize)				{ return (inSize+PAD_SIZE-1) & (~(PAD_SIZE-1)); }
			bool			_Allocate( VSize inSize)
				{
					fData = (inSize != 0) ? fMemMgr->Malloc( inSize, false, 'memb') : NULL;
					fSize = fAllocatedSize = (fData != NULL) ? inSize : 0;
					return (fData != NULL) || (inSize == 0);
				}

			void			_Dispose()								{ if (fData) fMemMgr->Free( fData);}

			bool			_Reallocate( VSize inSize)
				{
					bool ok = true;
					if (fAllocatedSize != inSize)
					{
						if (inSize == 0)
						{
							Clear();
						}
						else if (fData != NULL)
						{
							void *newData = fMemMgr->SetPtrSize( fData, inSize);
							if (newData != NULL)
							{
								fData = newData;
								fAllocatedSize = inSize;
								if (fSize > fAllocatedSize)
									fSize = fAllocatedSize;
							}
							else
							{
								ok = false;
							}
						}
						else
						{
							ok = _Allocate( inSize);
						}
					}
					return ok;
				}

		bool			_Enlarge( VSize inNeededBytes)
				{
						void *newData = (fData == NULL) ? fMemMgr->Malloc( inNeededBytes, false, 'memb') : fMemMgr->Realloc( fData, inNeededBytes);
						if (newData != NULL)
						{
							fData = newData;
							fAllocatedSize = inNeededBytes;
						}
						return newData != NULL;
				}

		bool			_EnlargeAmortized( VSize inNeededBytes)
				{

						VSize bonus = inNeededBytes / INCREASE_RATIO;
						if (bonus > MAX_BONUS)
							bonus = MAX_BONUS;
						VSize newAllocatedSize = _AdjusteSize( inNeededBytes + bonus);

						void *newData = (fData == NULL) ? fMemMgr->Malloc( newAllocatedSize, false, 'memb') : fMemMgr->Realloc( fData, newAllocatedSize);
						if (newData == NULL)
						{
							// retry with what is strictly necessary
							newAllocatedSize = inNeededBytes;
							newData = (fData == NULL) ? fMemMgr->Malloc( newAllocatedSize, false, 'memb') : fMemMgr->Realloc( fData, newAllocatedSize);
						}
						if (newData != NULL)
						{
							fData = newData;
							fAllocatedSize = newAllocatedSize;
						}
						return newData != NULL;
				}

		bool			_EnsureSize( VSize inNeededBytes)			{ return ( inNeededBytes <= fAllocatedSize) ? true : _Enlarge( inNeededBytes);}
		bool			_EnsureSizeAmortized( VSize inNeededBytes)	{ return ( inNeededBytes <= fAllocatedSize) ? true : _EnlargeAmortized( inNeededBytes);}

};



END_TOOLBOX_NAMESPACE

#endif
