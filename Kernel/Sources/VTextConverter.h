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
#ifndef __VTextConverter__
#define	__VTextConverter__

#include "Kernel/Sources/VObject.h"
#include "Kernel/Sources/IRefCountable.h"


BEGIN_TOOLBOX_NAMESPACE

// Needed declaration
class VString;

// Conversion d'un jeu de caracteres donne vers Unicode (UTF-16)
class XTOOLBOX_API VToUnicodeConverter : public VObject, public IRefCountable
{
public:
	virtual bool	IsValid () const;
	
	virtual bool	ConvertString( const void* inSource, VSize inSourceBytes, VSize* outBytesConsumed, VString& outDestination);
	virtual bool	Convert( const void* inSource, VSize inSourceBytes, VSize* outBytesConsumed, UniChar* inDestination, VIndex inDestinationChars, VIndex* outProducedChars) = 0;
	virtual VSize	GetCharSize() const		{ return sizeof(UniChar); }
};


// Conversion d'Unicode (UTF-16) vers un jeu de caracteres donne 
class XTOOLBOX_API VFromUnicodeConverter : public VObject, public IRefCountable
{
public:
	virtual bool	IsValid () const;

	virtual bool	ConvertRealloc( const UniChar *inSource, VIndex inSourceChars, void*& ioBuffer, VSize& ioBytesInBuffer, VSize inTrailingBytes);
	virtual bool	Convert( const UniChar *inSource, VIndex inSourceChars, VIndex *outCharsConsumed, void *inBuffer, VSize inBufferSize, VSize *outBytesProduced) = 0;
	virtual VSize	GetCharSize() const = 0;
};


class XTOOLBOX_API VToUnicodeConverter_Null : public VToUnicodeConverter
{
public:
	virtual bool Convert( const void* inSource, VSize inSourceBytes, VSize *outBytesConsumed, UniChar *inDestination, VIndex inDestinationChars, VIndex *outProducedChars);
	virtual bool ConvertString( const void* inSource, VSize inSourceBytes, VSize *outBytesConsumed, VString& outDestination);
};


class XTOOLBOX_API VFromUnicodeConverter_Null : public VFromUnicodeConverter
{
public:
	virtual	bool	Convert( const UniChar *inSource, VIndex inSourceChars, VIndex *outCharsConsumed, void* inBuffer, VSize inBufferSize, VSize *outBytesProduced);
	virtual VSize	GetCharSize () const	{ return sizeof(UniChar); }
};


class XTOOLBOX_API VToUnicodeConverter_UTF32_Raw : public VToUnicodeConverter
{
public:
	// fast and furious conversion (okay for hard coded wchar_t strings)
	VToUnicodeConverter_UTF32_Raw( bool inSwap) { fSwap = inSwap; }
	
	virtual bool Convert( const void* inSource, VSize inSourceBytes, VSize *outBytesConsumed, UniChar *inDestination, VIndex inDestinationChars, VIndex *outProducedChars);
	virtual bool ConvertString( const void* inSource, VSize inSourceBytes, VSize *outBytesConsumed, VString& outDestination);

protected:
	bool	fSwap;
};


class XTOOLBOX_API VFromUnicodeConverter_UTF32_Raw : public VFromUnicodeConverter
{
public:
	// fast and furious conversion  (okay for hard coded wchar_t strings)
	VFromUnicodeConverter_UTF32_Raw( bool inSwap) { fSwap = inSwap; }
	
	virtual bool	Convert( const UniChar *inSource, VIndex inSourceChars, VIndex *outCharsConsumed, void* inBuffer, VSize inBufferSize, VSize *outBytesProduced);
	virtual VSize	GetCharSize() const { return sizeof(uLONG); }

protected:
	bool	fSwap;
};


class XTOOLBOX_API VToUnicodeConverter_UTF32 : public VToUnicodeConverter
{
public:
					VToUnicodeConverter_UTF32 (bool inSwap) { fSwap = inSwap; }
	
	virtual bool	Convert (const void *inSource, VSize inSourceBytes, VSize *outBytesConsumed, UniChar *inDestination, VIndex inDestinationChars, VIndex *outProducedChars);

protected:
	bool			fSwap;
};


class XTOOLBOX_API VFromUnicodeConverter_UTF32 : public VFromUnicodeConverter
{
public:
					VFromUnicodeConverter_UTF32 (bool inSwap) { fSwap = inSwap; }
	
	virtual bool	Convert (const UniChar *inSource, VIndex inSourceChars, VIndex *outCharsConsumed, void *inBuffer, VSize inBufferSize, VSize *outBytesProduced);
	virtual VSize	GetCharSize() const { return sizeof(uLONG); }

protected:
	bool			fSwap;
};


class XTOOLBOX_API VToUnicodeConverter_Swap : public VToUnicodeConverter
{
public:
	virtual bool ConvertString( const void* inSource, VSize inSourceBytes, VSize *outBytesConsumed, VString& outDestination);
	virtual bool Convert( const void* inSource, VSize inSourceBytes, VSize *outBytesConsumed, UniChar* inDestination, VIndex inDestinationChars, VIndex *outProducedChars);
};


class XTOOLBOX_API VFromUnicodeConverter_Swap : public VFromUnicodeConverter
{
public:
	virtual bool	Convert( const UniChar *inSource, VIndex inSourceChars, VIndex *outCharsConsumed, void* inBuffer, VSize inBufferSize, VSize *outBytesProduced);
	virtual VSize	GetCharSize() const	{ return sizeof(UniChar); }
};


class XTOOLBOX_API VToUnicodeConverter_Void : public VToUnicodeConverter
{
public:
	virtual bool	IsValid() const;
	virtual bool	Convert( const void* inSource, VSize inSourceBytes, VSize *outBytesConsumed, UniChar *inDestination, VIndex inDestinationChars, VIndex *outProducedChars);
	virtual VSize	GetCharSize() const { return sizeof(char); }
};


class XTOOLBOX_API VFromUnicodeConverter_Void : public VFromUnicodeConverter
{
public:
	virtual bool	IsValid() const;
	virtual bool	Convert( const UniChar* inSource, VIndex inSourceChars, VIndex *outCharsConsumed, void *inBuffer, VSize inBufferSize, VSize *outBytesProduced);
	virtual VSize	GetCharSize () const	{ return sizeof(UniChar); }
};


class XTOOLBOX_API VToUnicodeConverter_UTF8 : public VToUnicodeConverter
{
public:
	virtual bool	Convert( const void *inSource, VSize inSourceBytes, VSize* outBytesConsumed, UniChar* inDestination, VIndex inDestinationChars, VIndex *outProducedChars);
};


class XTOOLBOX_API VFromUnicodeConverter_UTF8 : public VFromUnicodeConverter
{
public:
	virtual bool	Convert (const UniChar* inSource, VIndex inSourceChars, VIndex *outCharsConsumed, void* inBuffer, VSize inBufferSize, VSize *outBytesProduced);
	virtual VSize	GetCharSize () const	{ return sizeof(uBYTE); }

			bool	ConvertFrom_wchar( const wchar_t* inSource, VIndex inSourceChars, VIndex *outCharsConsumed, void* inBuffer, VSize inBufferSize, VSize *outBytesProduced);
};



class XTOOLBOX_API VTextConverters : public VObject
{
public:
												VTextConverters();
												~VTextConverters();

	static	VTextConverters*					Get();

			// name/charset
			CharSet								GetCharSetFromName( const VString &inName);
			void								GetNameFromCharSet( CharSet inCharSet, VString &outName);

			sLONG								GetMIBEnumFromCharSet( CharSet inCharSet );
			sLONG								GetMIBEnumFromName( const VString &inName );
			CharSet								GetCharSetFromMIBEnum( const sLONG inMIBEnum );
			void								GetNameFromMIBEnum( const sLONG inMIBEnum, VString &outName );
			
			void								EnumWebCharsets( std::vector<VString>& outNames, std::vector<CharSet>& outCharSets );
			void								EnumCharsets( std::vector<VString>& outNames, std::vector<CharSet>& outCharSets );

			VToUnicodeConverter*				RetainToUnicodeConverter( CharSet inCharSet);
			VFromUnicodeConverter*				RetainFromUnicodeConverter( CharSet inCharSet);

			// for standard library char * strings
			VToUnicodeConverter*				GetStdLibToUnicodeConverter();
			VFromUnicodeConverter*				GetStdLibFromUnicodeConverter();
			
			// for ansi system calls only
			VToUnicodeConverter*				GetNativeToUnicodeConverter()			{ return fToUniConverter; }
			VFromUnicodeConverter*				GetNativeFromUnicodeConverter()			{ return fFromUniConverter; }

			VToUnicodeConverter_Swap*			GetSwapToUnicodeConverter()				{ return fToUniSwapConverter; }
			VFromUnicodeConverter_Swap*			GetSwapFromUnicodeConverter()			{ return fFromUniSwapConverter; }

			VToUnicodeConverter_UTF32_Raw*		GetUTF32RawToUnicodeConverter()			{ return fToUni32RawConverter; }
			VFromUnicodeConverter_UTF32_Raw*	GetUTF32RawFromUnicodeConverter()		{ return fFromUni32RawConverter; }

			VToUnicodeConverter_UTF8*			GetUTF8ToUnicodeConverter()				{ return fToUTF8Converter; }
			VFromUnicodeConverter_UTF8*			GetUTF8FromUnicodeConverter()			{ return fFromUTF8Converter; }

			bool								ConvertToVString( const void* inBytes, VSize inByteCount, CharSet inCharSet, VString& outString);
			bool								ConvertFromVString( const VString& inString, VIndex* outCharsConsumed, void* inBuffer, VSize inBufferSize, VSize* outBytesProduced, CharSet inCharSet);

			// analyse the first bytes and try to recognize a unicode BOM.
			// return true if found a BOM and optionnally returns its size and its charset.
	static	bool								ParseBOM( const void *inBytes, size_t inByteCount, size_t *outBOMSize, CharSet *outFoundCharSet);

			// returns the BOM bytes for a charset. returns NULL if not relevant.
			// returned string is a global const. Don't try to release it.
	static	const char*							GetBOMForCharSet( CharSet inCharSet, size_t *outBOMSize);

private:
												VTextConverters( const VTextConverters&);
												VTextConverters& operator=(const VTextConverters&);
												
	static	VTextConverters*					sInstance;
	
			VToUnicodeConverter*				fToUniConverter;	// converter from VTC_MacOSAnsi or VTC_Win32Ansi to VTC_UTF_16
			VFromUnicodeConverter*				fFromUniConverter;	// converter from VTC_UTF_16 to VTC_MacOSAnsi or VTC_Win32Ansi

			VToUnicodeConverter_Null*			fToUniNullConverter;	// converter from VTC_UTF_16 to VTC_UTF_16 (null converter)
			VFromUnicodeConverter_Null*			fFromUniNullConverter;	// converter from VTC_UTF_16 to VTC_UTF_16 (null converter)

			VToUnicodeConverter_Swap*			fToUniSwapConverter;	// converter from VTC_UTF_16 to VTC_UTF_16 (null converter with swap)
			VFromUnicodeConverter_Swap*			fFromUniSwapConverter;	// converter from VTC_UTF_16 to VTC_UTF_16 (null converter with swap)

			VToUnicodeConverter_UTF32_Raw*		fToUni32RawConverter;	// converter from VTC_UTF_16 to wchar_t (quick and dirty 16 -> 32 bits conversion)
			VFromUnicodeConverter_UTF32_Raw*	fFromUni32RawConverter;	// converter from wchar_t to VTC_UTF_16 (quick and dirty 32 -> 16 bits conversion)

			VToUnicodeConverter_UTF32*			fToUni32Converter;		// converter from VTC_UTF_16 to wchar_t
			VFromUnicodeConverter_UTF32*		fFromUni32Converter;	// converter from wchar_t to VTC_UTF_16

			VToUnicodeConverter_UTF8*			fToUTF8Converter;	// converter from VTC_UTF_16 to VTC_UTF_8
			VFromUnicodeConverter_UTF8*			fFromUTF8Converter;	// converter from VTC_UTF_8 to VTC_UTF_16
};

END_TOOLBOX_NAMESPACE

#endif
