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
#ifndef __VJSRuntime_stream__
#define __VJSRuntime_stream__

#include "JS4D.h"
#include "VJSClass.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VJSStream : public VJSClass<VJSStream, XBOX::VStream>
{
public:
	typedef VJSClass<VJSStream, XBOX::VStream> inherited;
	
	static void	Initialize( const VJSParms_initialize& inParms, XBOX::VStream* inStream);
	static void	Finalize( const VJSParms_finalize& inParms, XBOX::VStream* inStream);
	static void	GetDefinition( ClassDefinition& outDefinition);
	static void	do_BinaryStream( VJSParms_callStaticFunction& ioParms);
	
	static void _Close(VJSParms_callStaticFunction& ioParms, XBOX::VStream* inStream); // Close()
	static void _Flush(VJSParms_callStaticFunction& ioParms, XBOX::VStream* inStream); // flush()
	
	static void _GetByte(VJSParms_callStaticFunction& ioParms, XBOX::VStream* inStream); // number : GetByte()
	static void _GetWord(VJSParms_callStaticFunction& ioParms, XBOX::VStream* inStream); // number : GetWord()
	static void _GetLong(VJSParms_callStaticFunction& ioParms, XBOX::VStream* inStream); // number : GetLong()
	static void _GetLong64(VJSParms_callStaticFunction& ioParms, XBOX::VStream* inStream); // number : GetLong64()
	static void _GetReal(VJSParms_callStaticFunction& ioParms, XBOX::VStream* inStream); // number : GetReal()
	static void _GetString(VJSParms_callStaticFunction& ioParms, XBOX::VStream* inStream); // string : GetString()
	static void	_GetBlob (VJSParms_callStaticFunction &ioParms, XBOX::VStream *inStream); // Blob : GetBlob(size)
	static void	_GetBuffer (VJSParms_callStaticFunction &ioParms, XBOX::VStream *inStream); // Buffer : GetBuffer(size)

	static void _PutByte(VJSParms_callStaticFunction& ioParms, XBOX::VStream* inStream); // PutByte(number)
	static void _PutWord(VJSParms_callStaticFunction& ioParms, XBOX::VStream* inStream); // PutWord(number)
	static void _PutLong(VJSParms_callStaticFunction& ioParms, XBOX::VStream* inStream); // PutLong(number)
	static void _PutLong64(VJSParms_callStaticFunction& ioParms, XBOX::VStream* inStream); // PutLong64(number)
	static void _PutReal(VJSParms_callStaticFunction& ioParms, XBOX::VStream* inStream); // PutReal(number)
	static void _PutString(VJSParms_callStaticFunction& ioParms, XBOX::VStream* inStream); // PutString(string)
	static void	_PutBlob (VJSParms_callStaticFunction &ioParms, XBOX::VStream *inStream); // PutBlob(blob, optionalIndex, optionalSize)
	static void	_PutBuffer (VJSParms_callStaticFunction &ioParms, XBOX::VStream *inStream); // PutBuffer(buffer, optionalIndex, optionalSize)
	
	static void _GetPos(VJSParms_callStaticFunction& ioParms, XBOX::VStream* inStream); // number : GetPos()
	static void _SetPos(VJSParms_callStaticFunction& ioParms, XBOX::VStream* inStream); // SetPos(number)
	static void _GetSize(VJSParms_callStaticFunction& ioParms, XBOX::VStream* inStream); // number : GetSize()
	
	static void _ChangeByteOrder(VJSParms_callStaticFunction& ioParms, XBOX::VStream* inStream); // ChangeByteOrder();
	static void _IsByteSwapping(VJSParms_callStaticFunction& ioParms, XBOX::VStream* inStream); // bool : IsByteSwapping();

private: 

	// Default time out for BinaryStream using net.SocketSync.

	static const sLONG	kDefaultTimeOut	= 1000;

	static void	_GetBinary (VJSParms_callStaticFunction &ioParms, XBOX::VStream *inStream, bool inIsBuffer);
	static void	_PutBinary (VJSParms_callStaticFunction &ioParms, XBOX::VStream *inStream);	
};

class VJSTextStreamState : public XBOX::VObject 
{
friend class VJSTextStream;

	// Number of characters of look ahead when reading for a delimiter character.

	static const sLONG	kREAD_SLICE_SIZE	=	256;

	XBOX::VStream	*fStream;
	XBOX::VString	fBuffer;
	VIndex			fIndex;		// Zero starting index.
	sLONG			fPosition;

	// Read until delimiter character (not returned in result). If delimiter is an empty string, read a line.
	// Return true if delimiter has been found.

	bool	_ReadUntilDelimiter (const XBOX::VString &inDelimiter, XBOX::VString *outResult);

	// Read requested number of characters (use zero to read till end of file).

	void	_ReadCharacters (sLONG inNumberCharacters, XBOX::VString *outResult);

public:
	XBOX::VStream* GetStream()
	{
		return fStream;
	}
};

class XTOOLBOX_API VJSTextStream : public VJSClass<VJSTextStream, VJSTextStreamState>
{
public:

	static void				GetDefinition (ClassDefinition &outDefinition);
	static	void			Construct( VJSParms_construct &ioParms);

private:
	
	typedef VJSClass<VJSTextStream, VJSTextStreamState> inherited;

	static void				_Finalize (const VJSParms_finalize &inParms, VJSTextStreamState *inStreamState);

	static bool				_HasInstance (const VJSParms_hasInstance &inParms);

	static void				_Close (VJSParms_callStaticFunction &ioParms, VJSTextStreamState *inStreamState);	// close()
	
	static void				_GetSize (VJSParms_callStaticFunction &ioParms, VJSTextStreamState *inStreamState);	// number : getSize()
	static void				_GetPos (VJSParms_callStaticFunction &ioParms, VJSTextStreamState *inStreamState);	// number : getPos()

	static void				_Rewind (VJSParms_callStaticFunction &ioParms, VJSTextStreamState *inStreamState);	// rewind()
	static void				_Read (VJSParms_callStaticFunction& ioParms, VJSTextStreamState *inStreamState);	// read(string, { number : nbchar })  // if nbchar is missing then performs a read until linefeed or CR or eof
	static void				_End (VJSParms_callStaticFunction &ioParms, VJSTextStreamState *inStreamState);		// bool : end()

	static void				_Write (VJSParms_callStaticFunction &ioParms, VJSTextStreamState *inStreamState);	// write(string)	
	static void				_Flush (VJSParms_callStaticFunction &ioParms, VJSTextStreamState *inStreamState);	// flush()	
};

END_TOOLBOX_NAMESPACE

#endif