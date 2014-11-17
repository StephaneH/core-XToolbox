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
#include "VURL.h"
#include "VFolder.h"
#include "VFilePath.h"

const VString UNSAFE_CHARS( "<>\"%{}|\\^[]` ");
const VString UNSAFE_CHARS_WITHOUT_BRACKETS( "<>\"%{}|\\^` ");

// see http://tools.ietf.org/html/rfc3986
#define UNRESERVED_CHARS "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-._~"
#define GENERIC_DELIMITERS ":/?#[]@"
#define SUB_DELIMITERS "!$&'()*+,;="
#define RESERVED_CHARS GENERIC_DELIMITERS SUB_DELIMITERS
#define SAFE_CHARS RESERVED_CHARS UNRESERVED_CHARS "%"

const VString FILE_SCHEME( "file");

VURL::VURL()
{
	fBaseURL = NULL;
	for (sWORD i = e_Scheme;i<e_LastPart;i++)
	{
		fURLArray[i].pos = 0;
		fURLArray[i].size = 0;
	}
	fIsConform = false;
}


VURL::VURL( const VURL& inURL)
: fBaseURL( (inURL.fBaseURL == NULL) ? NULL : new VURL( *inURL.fBaseURL))
, fURLString( inURL.fURLString)
, fURLStringEncoded (inURL.fURLStringEncoded)
, fIsConform( inURL.fIsConform)
{
	for (sWORD i = e_Scheme;i<e_LastPart;i++)
	{
		fURLArray[i].pos = inURL.fURLArray[i].pos;
		fURLArray[i].size = inURL.fURLArray[i].size;
	}
}


VURL::VURL( const VString& inURL, bool inEncodedString)
{
	fBaseURL = NULL;
	fIsConform = false;
	
	FromString( inURL, inEncodedString);
}


VURL::VURL( const VString& inFilePath, EURLPathStyle inStyle, const VString& inScheme)
{
	fBaseURL = NULL;
	fIsConform = false;
	
	FromFilePath(inFilePath, inStyle, inScheme);
}


VURL::VURL( const VFolder& inBaseFolder, const VString& inRelativPath, EURLPathStyle inStyle)
{
	fBaseURL = NULL;
	fIsConform = false;
	FromRelativePath( inBaseFolder, inRelativPath, inStyle);
}


VURL::VURL( const VFilePath& inPath)
{
	fBaseURL = NULL;
	fIsConform = false;
	FromFilePath( inPath);
}


VURL::~VURL()
{
	delete fBaseURL;
}


void VURL::FromURL( const VURL& inURL)
{
	if (&inURL != this)
	{
		delete fBaseURL;
		fBaseURL = (inURL.fBaseURL != NULL) ? new VURL(*inURL.fBaseURL) : NULL;

		fURLString = inURL.fURLString;
		fURLStringEncoded = inURL.fURLStringEncoded;
		fIsConform = inURL.fIsConform;

		for (sWORD i = e_Scheme;i<e_LastPart;i++)
		{
			fURLArray[i].pos = inURL.fURLArray[i].pos;
			fURLArray[i].size = inURL.fURLArray[i].size;
		}
	}
}


void VURL::FromFilePath( const VFilePath& inPath)
{
	VString temp;
	inPath.GetPosixPath( temp);
	fURLStringEncoded.SetEmpty();

	if (testAssert( !temp.IsEmpty()))
	{
		delete fBaseURL;

		if ( (temp.GetLength() >= 2) && (temp[0] == '/') && (temp[1] == '/') )
		{
			// it seems to be a UNC windows file path
			// take server name as network location for url
			fURLString = CVSTR("file:");
			fURLString += temp;
		}
		else
		{
			// no network location
			fURLString = CVSTR("file://");	// empty network location
			// the path on mac begins with a '/'
			if ( (temp.GetLength() <= 0) || (temp[0] != '/') )
			{
				fURLString += (UniChar) '/';
			}
			fURLString += temp;
		}

		fIsConform = _Parse( true);
	}
	else
	{
		Clear();
	}
}


bool VURL::GetFilePath( VFilePath& outPath) const
{
	bool ok;
	VString scheme;
	_GetPart( scheme, e_Scheme);
	if (scheme.IsEmpty() || (scheme == FILE_SCHEME) )
	{
		VString fullPath;
		_GetPart( fullPath, e_Path);
#if VERSIONMAC || VERSION_LINUX
		if ( fullPath.IsEmpty() || (fullPath[0] != '/') )
			fullPath.Insert( '/', 1);
#elif VERSIONWIN
		// prefix with network location if any and if not already in path
		if ( (fullPath.GetLength() < 2) || (fullPath[0] != '/') || (fullPath[1] != '/') )
		{
			VString networkLocation;
			_GetPart( networkLocation, e_NetLoc);
			if ( !networkLocation.IsEmpty())
			{
				fullPath.Insert( '/', 1);
				fullPath.Insert( networkLocation, 1);
				fullPath.Insert( '/', 1);
				fullPath.Insert( '/', 1);
			}
		}
#endif
		Convert( fullPath, eURL_POSIX_STYLE, eURL_NATIVE_STYLE, false);
		
		outPath.FromFullPath( fullPath);
		ok = outPath.IsValid();
	}
	else
	{
		outPath.Clear();
		ok = false;
	}
	return ok;
}



/*!
	@function	FromString
	@abstract	Rebuilds a VURL from a file path as VString.
	@param		inFilePath the full file path string
	@param		inStyle the style of the incomming string
*/
void VURL::FromFilePath( const VString& inFilePath, EURLPathStyle inStyle, const VString& inScheme)
{
	xbox_assert( inScheme.EndsWith( CVSTR( "//")));

	delete fBaseURL;
	fBaseURL = NULL;
	fIsConform = false;
	
	VString sPath = inFilePath;
	switch( inStyle)
	{
		case eURL_HFS_STYLE:
			HFS_to_POSIX( sPath, true);
			break;

		case eURL_WIN_STYLE:
			WIN_to_POSIX( sPath );
			break;

		default:
			break;
	}

	VString url( inScheme);

	if (!sPath.BeginsWith(CVSTR("/")))
		url.AppendString(CVSTR("/"));
	else if (sPath.BeginsWith(CVSTR("//")))
		sPath.SubString(3,sPath.GetLength()-2);

	url.AppendString(sPath);

	fURLStringEncoded.SetEmpty();
	fURLString.FromString( url);
	fIsConform = _Parse( true);
}


bool VURL::_ContainsOnlySafeCharacters( const VString& inURL)
{
	const UniChar *begin = inURL.GetCPointer();
	const UniChar *end = begin + inURL.GetLength();
	const UniChar *p = begin;

	while( (p != end) && (*p <= 127) && (strchr( SAFE_CHARS, (char) *p) != NULL) )
		++p;

	return p == end;
}


void VURL::FromString( const VString& inURL, bool inEncodedString)
{
	delete fBaseURL;
	fBaseURL = NULL;
	fIsConform = false;
	
	fURLString = inURL;
	if (inEncodedString && testAssert( _ContainsOnlySafeCharacters( inURL)))
		fURLStringEncoded = inURL;
	else
		fURLStringEncoded.SetEmpty();

	fIsConform = _Parse( false);
	
	if (inEncodedString)
		_DecodeParts();
}


void VURL::FromString( const VString& inURL, const VString& inScheme, bool inEncodedString)
{
	delete fBaseURL;
	fBaseURL = NULL;
	fIsConform = false;
	
	XBOX::VString url( inURL);

	if (url.Find(CVSTR("://")) <= 0)
	{
		if (!inScheme.IsEmpty() && !url.BeginsWith(CVSTR("/")))
			url.Insert((UniChar)'/', 1);
		url.Insert( inScheme, 1);
	}

	fURLString = url;
	if (inEncodedString && testAssert( _ContainsOnlySafeCharacters( inURL)))
		fURLStringEncoded = inURL;
	else
		fURLStringEncoded.SetEmpty();

	fIsConform = _Parse( false);

	if (inEncodedString)
		_DecodeParts();
}


void VURL::FromRelativePath( const VFolder& inBaseFolder, const VString& inRelativPath, EURLPathStyle inStyle)
{
	delete fBaseURL;
	fBaseURL = new VURL( inBaseFolder.GetPath());

	fURLString = inRelativPath;
	fURLStringEncoded.SetEmpty();

	Convert( fURLString, inStyle, eURL_POSIX_STYLE, true);

	fIsConform = _Parse( true);
}


void VURL::GetAbsoluteURL( VString& outURLString, bool inEncoded) const
{
	if (inEncoded)
	{
		//JQ 13/12/2012: fixed again ACI0075055
		if (fURLString.EqualToString("about:blank", true))
		{
			outURLString = fURLString;
			return;
		}

		if (!fURLStringEncoded.IsEmpty())
			outURLString = fURLStringEncoded;
		else
		{
			outURLString.Clear();

			VString tempstr;
			GetScheme( tempstr);
			outURLString += tempstr;
			outURLString += "://";

			GetNetworkLocation(tempstr, inEncoded);
			outURLString.AppendString(tempstr);
			outURLString.AppendUniChar('/');

			GetPath(tempstr, eURL_POSIX_STYLE, inEncoded);
			if (!tempstr.IsEmpty())
			{
				outURLString += tempstr;
			}

			GetQuery(tempstr, inEncoded);
			if (!tempstr.IsEmpty())
			{
				outURLString.AppendUniChar('?');
				outURLString += tempstr;
			}

			GetParameters(tempstr, inEncoded);
			if (!tempstr.IsEmpty())
			{
				outURLString.AppendUniChar(';');
				outURLString += tempstr;
			}

			GetFragment(tempstr, inEncoded);
			if (!tempstr.IsEmpty())
			{
				outURLString.AppendUniChar('#');
				outURLString += tempstr;
			}
		}
	}
	else
	{
		_GetAbsoluteURL( outURLString);
	}
}


void VURL::GetRelativeURL( VString& outURLString, bool inEncoded) const
{
	outURLString = fURLString;
	if (inEncoded)
	{
		// on encode partie par partie
		URLPartArray	relativeURLarray;
		VString			tempstr;
		
		_BuildURLArray(fURLString, false, relativeURLarray);
		outURLString.Clear();
		for (sWORD i = e_Scheme;i<e_LastPart;i++)
		{
			if ((relativeURLarray[i].pos) && (relativeURLarray[i].size))
			{
				tempstr.FromString(fURLString);
				tempstr.SubString(relativeURLarray[i].pos, relativeURLarray[i].size);
				Encode(tempstr);
			}
			else
				tempstr.Clear();
			if (!tempstr.IsEmpty())
			{
				switch (i)
				{
					case e_Scheme : 
						outURLString.AppendString(tempstr);
						if (relativeURLarray[e_NetLoc].size)
							outURLString.AppendString(L"://");
						else
							outURLString.AppendString(L":///");
						break;

					case e_NetLoc :
						outURLString.AppendString(tempstr);
						outURLString.AppendUniChar('/');
						break;

					case e_Path	 :
						outURLString.AppendString(tempstr);
						break;

					case e_Params :
						outURLString.AppendUniChar(';');
						outURLString.AppendString(tempstr);
						break;

					case e_Query:
						outURLString.AppendUniChar('?');
						outURLString.AppendString(tempstr);
						break;

					case e_Fragment :
						outURLString.AppendUniChar('#');
						outURLString.AppendString(tempstr);
						break;
				} // switch
			}
		} // loop
	}
	else
	{
		outURLString.FromString(fURLString);
	}
}	


bool VURL::IsRoot() const
{
	sLONG pos;
	VString path;
	GetPath(path);
	pos = path.FindUniChar('/', 1);
	if ((pos==0) || (pos == path.GetLength()))
		if (fBaseURL) return fBaseURL->IsRoot();
		else return true;
	else
		return false;
}


VURL* VURL::RetainParent()
{
	VURL* parentURL = new VURL(*this);
	if (parentURL->ToParent())
		return parentURL;
	else
	{
		delete parentURL;
		return NULL;
	}
}


bool VURL::ToParent()
{
	sLONG	pos;
	VString path;
	bool result;
	
	if (IsRoot())
		return false;
	
	_GetRelativePath(path);
	if (path.GetLength())
	{	
		if (path[path.GetLength()-1] == '/')
			path.SubString(1, path.GetLength()-1);
		pos = path.FindUniChar('/', path.GetLength(), true);
		if (pos)
		{
			path.SubString(1, pos);
			SetRelativePath(path, eURL_POSIX_STYLE, false);
			result = true;
		}
		else
		{
			if (fBaseURL)
			{
				path.Clear();	// pas de '/' dans le path relatif, c'etait un fichier, la base est le parent
				SetRelativePath(path, eURL_POSIX_STYLE, false);
				result = true;
			}
			else
				result = false; // on est deja au root
		}
	}
	else if (fBaseURL)
	{
		result = fBaseURL->ToParent();
		_Parse( false);
	}
	else result = false;			// pas de parent
	return result;
}


bool VURL::ToSubFolder(const VString& inName)
{
	VString path;
	_GetRelativePath(path);
	if ((path.GetLength() == 0 && fBaseURL) || (path[path.GetLength()-1] == '/'))
	{
		path.AppendString(inName);
		SetRelativePath(path, eURL_POSIX_STYLE, false);
		return true;
	}
	else return false;
}



void VURL::_GetPart(VString& outString, EURLParts inPart) const
{
	if (fURLArray[inPart].size != 0)
	{
		_GetAbsoluteURL(outString);
		outString.SubString(fURLArray[inPart].pos, fURLArray[inPart].size);
	}
	else
	{
		outString.Clear();
	}
}


void VURL::_DecodeParts()
{
	for( sLONG i = e_Fragment ; i >= e_Scheme  ; --i)
	{
		if ((fURLArray[i].pos > 0) && (fURLArray[i].size > 0))
		{
			VString part;
			fURLString.GetSubString( fURLArray[i].pos, fURLArray[i].size, part);
			Decode( part);
			fURLString.Replace( part, fURLArray[i].pos, fURLArray[i].size);
			sLONG delta = part.GetLength() - fURLArray[i].size;
			fURLArray[i].size = part.GetLength();
			for( sLONG j = i+1 ; j <= e_Fragment ; ++j)
				fURLArray[j].pos += delta;
		}
	}
}


void VURL::_SetPart(const VString& inString, EURLParts inPart)
{
	URLPartArray	relativeURLarray;
	VString			newURLString;
	VString			tempstr;
	
	if (!_BuildURLArray(fURLString, false, relativeURLarray))
		return;

	fURLStringEncoded.SetEmpty();

	for (sWORD i = e_Scheme;i<e_LastPart;i++)
	{
		if (i == inPart)
		{
			tempstr.FromString(inString);
		}
		else
		{
			if ((relativeURLarray[i].pos) && (relativeURLarray[i].size))
			{
				tempstr.FromString(fURLString);
				tempstr.SubString(relativeURLarray[i].pos, relativeURLarray[i].size);
			}
			else tempstr.Clear();
		}
		if (!tempstr.IsEmpty())
		switch (i)
		{
			case e_Scheme : 
				newURLString.AppendString(tempstr);
				if (relativeURLarray[e_NetLoc].size) newURLString.AppendString(L"://");
				else newURLString.AppendString(L":///");
			break;
			case e_NetLoc :
				newURLString.AppendString(tempstr);
				newURLString.AppendUniChar('/');
			break;
			case e_Path	 :
				newURLString.AppendString(tempstr);
			break;
			case e_Params :
				newURLString.AppendUniChar(';');
				newURLString.AppendString(tempstr);
			break;
			case e_Query	 :
				newURLString.AppendUniChar('?');
				newURLString.AppendString(tempstr);
			break;
			case e_Fragment :
				newURLString.AppendUniChar('#');
				newURLString.AppendString(tempstr);
			break;
		} // switch
	} // loop
	fURLString.FromString(newURLString);
}


void VURL::GetScheme( VString& outString) const
{
	_GetPart(outString, e_Scheme);
}


void VURL::GetNetworkLocation( VString& outString, bool inEncoded) const
{
	_GetPart(outString, e_NetLoc);

	//jmo - IPv6 : On ne veut pas que les crochets, ex [::1], soient encodes
	//voir http://tools.ietf.org/html/rfc2732
	if (inEncoded)
	{
		Encode( outString, false /* inConvertColon */, false /* inDecomposeString */, false /* inEncodeBrackets */);
	}
}


void VURL::GetPath( VString& outString, EURLPathStyle inStyle, bool inEncoded) const
{
	_GetPart(outString, e_Path);
	if (inEncoded)
		Encode(outString);

	if (inStyle != eURL_POSIX_STYLE)
		Convert( outString, eURL_POSIX_STYLE, inStyle, fBaseURL != NULL);
}


void VURL::GetRelativePath(VString& outString, EURLPathStyle /*inStyle*/, bool inEncoded) const
{
	_GetRelativePath(outString);
	if (inEncoded)
		Encode(outString);
}


void VURL::GetHostName(VString& outString, bool inEncoded) const
{
	sLONG pos;
	_GetPart(outString, e_NetLoc);
	pos = outString.FindUniChar('@', 1);
	if (pos)
		outString.SubString(pos+1, outString.GetLength()-pos);		// now we have <host>:<port>
	pos = outString.FindUniChar(':', 1);
	if (pos)
		outString.SubString(1, pos-1);	// remove port
	if (inEncoded)
		Encode(outString);
}


void VURL::GetPortNumber(VString& outString, bool inEncoded) const
{
	sLONG pos;
	_GetPart(outString, e_NetLoc);
	pos = outString.FindUniChar('@', 1);
	if (pos) outString.SubString(pos+1, outString.GetLength()-pos);		// now we have <user>:<password>
	pos = outString.FindUniChar(':', 1);
	if (pos)
		outString.SubString(pos+1, outString.GetLength()-pos);
	else
		outString.Clear();
	if (inEncoded)
		Encode(outString);
}


void VURL::GetUserName( VString& outString, bool inEncoded) const
{
	sLONG apos, bpos;
	_GetPart(outString, e_NetLoc);
	apos = outString.FindUniChar('@', 1);
	if (apos)
	{
		bpos = outString.FindUniChar(':', 1);
		if ( bpos == 0 )
			bpos = apos; // There is no user password, just a user name

		if (bpos <= apos)
			outString.SubString(1, bpos-1);	// password after ':'
		else
			outString.SubString(1, apos-1);	// no password
	}
	else
		outString.Clear();
	if (inEncoded)
		Encode(outString);
}


void VURL::GetPassword( VString& outString, bool inEncoded) const
{
	// cf GetUserName
	sLONG startpos, endpos;
	_GetPart(outString, e_NetLoc);
	endpos = outString.FindUniChar('@', 1);
	if (endpos)
	{
		startpos = outString.FindUniChar(':', 1);
		if (startpos > 0 && startpos < endpos)
		{						// password after ':'
			outString.SubString(startpos+1, endpos-startpos-1);
		}
		else								// no password
			outString.Clear();
	}
	else
		outString.Clear();
	if (inEncoded)
		Encode(outString);
}


void VURL::GetParameters( VString& outString, bool inEncoded) const
{
	_GetPart(outString, e_Params);
	if (inEncoded)
		Encode(outString);
}


void VURL::GetQuery( VString& outString, bool inEncoded) const
{
	_GetPart(outString, e_Query);
	if (inEncoded)
		Encode(outString);
}


void VURL::GetFragment( VString& outString, bool inEncoded) const
{
	_GetPart(outString, e_Fragment);
	if (inEncoded)
		Encode(outString);
}


void VURL::GetResourceSpecifier(VString& outString, bool inEncoded) const
{
	VIndex pos = fURLArray[e_Path].pos+fURLArray[e_Path].size;
	_GetAbsoluteURL(outString);
	if (pos > outString.GetLength())
		outString.Clear();
	else
		outString.SubString(pos, outString.GetLength()-pos+1);
	if (inEncoded)
		Encode(outString);
}		


void VURL::GetLastPathComponent( VString& outName, bool inEncoded) const
{
	sLONG	pos;
	VString path;
	GetPath(outName);
	if (outName[outName.GetLength()-1] == '/')
		outName.SubString(1, outName.GetLength()-1);
	pos = outName.FindUniChar('/', outName.GetLength(), true);
	if (pos)
		outName.SubString(pos+1, outName.GetLength()- pos);
	if (inEncoded)
		Encode(outName);
}


void VURL::SetRelativePath( const VString& inString, EURLPathStyle inStyle, bool inEncodedString)
{
	VString tempstr(inString);

	if (inEncodedString)
		Decode(tempstr);

	bool isRelative = ( fBaseURL == NULL );

	switch ( inStyle )
	{
		case eURL_HFS_STYLE:
			HFS_to_POSIX( tempstr, isRelative );

		case eURL_WIN_STYLE:
			WIN_to_POSIX( tempstr );
			break;

		case eURL_POSIX_STYLE:
			break;
	}
	_SetPart(tempstr, e_Path);
	fIsConform = _Parse( false);
}


void VURL::SetScheme(const VString& inString)
{
	VString tempstr;
	assert(fBaseURL == NULL);
	if (!fBaseURL)
	{
		tempstr.FromString(inString);
		_SetPart(tempstr, e_Scheme);
		fIsConform = _Parse( false);
	}
}


void VURL::SetNetWorkLocation(const VString& inString, bool inEncodedString)
{
	VString tempstr;
	assert(fBaseURL == NULL);
	if (!fBaseURL)
	{
		tempstr.FromString(inString);
		if (inEncodedString)
			Decode(tempstr);
		_SetPart(tempstr, e_NetLoc);
		fIsConform = _Parse( false);
	}
}


void VURL::SetParameters(const VString& inString, bool inEncodedString)
{
	VString tempstr;
	tempstr.FromString(inString);
	if (inEncodedString)
		Decode(tempstr);
	_SetPart(tempstr, e_Params);
	fIsConform = _Parse( false);
}


void VURL::SetQuery(const VString& inString, bool inEncodedString)
{
	VString tempstr;
	tempstr.FromString(inString);
	if (inEncodedString)
		Decode(tempstr);
	_SetPart(tempstr, e_Query);
	fIsConform = _Parse( false);
}


void VURL::SetFragment(const VString& inString, bool inEncodedString)
{
	VString tempstr;
	tempstr.FromString(inString);
	if (inEncodedString)
		Decode(tempstr);
	_SetPart(tempstr, e_Fragment);
	fIsConform = _Parse( false);
}


void VURL::Convert(VString& ioString, EURLPathStyle inOrgStyle, EURLPathStyle inDestStyle, bool inRelative)
{
	switch (inOrgStyle)
	{
		case eURL_HFS_STYLE : 
			switch (inDestStyle)
			{
				case eURL_HFS_STYLE:
					break;

				case eURL_WIN_STYLE:
					assert(false);
					break;

				case eURL_POSIX_STYLE:
					HFS_to_POSIX( ioString, inRelative );
					break;
			}
			break;

		case eURL_WIN_STYLE :
			switch (inDestStyle)
			{
				case eURL_HFS_STYLE:
					assert(false);
					break;

				case eURL_WIN_STYLE:
					break;

				case eURL_POSIX_STYLE:
					WIN_to_POSIX( ioString );
					break;
			}
			break;

		case eURL_POSIX_STYLE:
			switch (inDestStyle)
			{
				case eURL_HFS_STYLE:
					POSIX_to_HFS( ioString, inRelative );
					break;

				case eURL_WIN_STYLE:
					POSIX_to_WIN( ioString );
					break;

				case eURL_POSIX_STYLE:
					break;
			}
			break;
	}
}


static std::vector<uBYTE>	sCharsToEscape;
static std::vector<uBYTE>	sCharsToEscape_withoutBrackets;

void VURL::Encode (XBOX::VString& ioString, bool inConvertColon, bool inDecomposeString, bool inEncodeBrackets)
{
	std::vector<uBYTE>& charsToEscape = inEncodeBrackets ? sCharsToEscape : sCharsToEscape_withoutBrackets;

	if (charsToEscape.empty())
	{
		// build the array of escape characters
		VStringConvertBuffer utf8String( inEncodeBrackets ? UNSAFE_CHARS : UNSAFE_CHARS_WITHOUT_BRACKETS, VTC_UTF_8, eCRM_NATIVE);

		const uBYTE *buf = (uBYTE*)utf8String.GetCPointer();
		for( VIndex pos = 0 ; pos <  utf8String.GetLength() ; ++pos )
			charsToEscape.push_back( *(buf + pos));
		
		for( uBYTE escapeChar = 0x00 ; escapeChar < 0xFF ; ++escapeChar )
		{
			if ((escapeChar<= 0x1F) || (escapeChar==0x7F) || (escapeChar>=0x80))
				charsToEscape.push_back( escapeChar);
		}
		charsToEscape.push_back( 0xFF);
		
		std::sort( charsToEscape.begin(), charsToEscape.end());
	}
	
	const uBYTE HEXTABLE[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	std::vector<uBYTE> utf8CharsArray;	// the encoded string

	// first, decompose the url
	if (inDecomposeString)
		ioString.Decompose();

	// convert it in utf8 string
	VStringConvertBuffer utf8String( ioString, VTC_UTF_8, eCRM_NATIVE);

	const uBYTE *buf = (uBYTE*)utf8String.GetCPointer();
	for( VIndex pos = 0 ; pos <  utf8String.GetLength() ; ++pos )
	{
		uBYTE utf8Char = *(buf + pos);
		if ((inConvertColon && (utf8Char == ':')) || std::find (charsToEscape.begin(), charsToEscape.end(), utf8Char) == charsToEscape.end())
		{
			utf8CharsArray.push_back( utf8Char);
		}
		else
		{
			// encode the character
			utf8CharsArray.push_back ('%');
			utf8CharsArray.push_back (HEXTABLE[utf8Char>>4]);
			utf8CharsArray.push_back (HEXTABLE[utf8Char&0x0F]);
		}
	}
	
	if (!utf8CharsArray.empty())
	{
		ioString.FromBlock( &utf8CharsArray.at(0), utf8CharsArray.size(), VTC_UTF_8);
	}
	else
	{
		ioString.Clear();
	}
}


void VURL::Decode (XBOX::VString& ioString)
{
	sLONG	startPos = 1;
	sLONG	pos = ioString.FindUniChar (CHAR_PERCENT_SIGN, startPos);

	if (pos > 0)
	{
		size_t				bufferSize = 512L;
		uBYTE *				buffer = (uBYTE *) malloc (bufferSize);
		XBOX::VString		resultString;
		sLONG				strLen = ioString.GetLength();
		std::vector<uBYTE>	byteArray;

		do {
			if (pos > startPos)
				resultString.AppendBlock (ioString.GetCPointer() + startPos - 1, (pos - startPos) * sizeof (UniChar), XBOX::VTC_UTF_16);

			sLONG			savedPos = pos;
			const UniChar *	it = ioString.GetCPointer() + (pos - 1);

			while ((pos <= strLen) && ((*it++ == CHAR_PERCENT_SIGN) && (pos <= strLen - 2)))
			{
				// on decode les 2 unichars suivants pour en faire un seul caractere UTF8
				uBYTE	c = 0;
				UniChar hi = *it++;
				UniChar lo = *it++;

				if (hi >= '0' && hi <= '9')
					c = uBYTE(hi - '0');
				else if (hi >= 'A' && hi <= 'F')
					c = uBYTE(hi - 'A' + 10);
				else if (hi >= 'a' && hi <= 'f')
					c = uBYTE(hi - 'a' + 10);
				c *= 16;
				if (lo >= '0' && lo <= '9')
					c += uBYTE(lo - '0');
				else if (lo >= 'A' && lo <= 'F')
					c += uBYTE(lo - 'A' + 10);
				else if (lo >= 'a' && lo <= 'f')
					c += uBYTE(lo - 'a' + 10);
				byteArray.push_back (c);

				pos += 3;
			}

			if (!byteArray.empty())
			{
				// sc 15/06/2007 les caracteres UTF8 consecutifs doivent etre traites comme une seule chaine UTF8
				if (byteArray.size() > bufferSize)
				{
					bufferSize = byteArray.size();
					buffer = (uBYTE *) realloc (buffer, bufferSize);
				}

				if (buffer != NULL)
				{
					std::copy (byteArray.begin(), byteArray.end(), buffer);

					if (byteArray.size() < 2) // YT 27-Jul-2009 - ACI0062530 - Less than 2 consecutives bytes, it's probably not a QP-encoded UTF-8 string...
						resultString.AppendBlock (buffer, byteArray.size(), VTC_ISO_8859_1);
					else
						resultString.AppendBlock (buffer, byteArray.size(), VTC_UTF_8);
				}

				byteArray.clear();
			}
			
			startPos = pos;
			pos = savedPos;
			if (pos < strLen && startPos <= strLen)
				pos = ioString.FindUniChar (CHAR_PERCENT_SIGN, startPos);
			else
				pos = 0;

			// YT 02-Oct-2012 - ACI0078536
			if (savedPos == pos)
				break;
		}
		while (pos > 0);

		if (NULL != buffer)
			free (buffer);

		if (startPos <= strLen) // YT 31-Jul-2009 - ACI0062825
			resultString.AppendUniCString (ioString.GetCPointer() + startPos - 1);

		ioString.FromString (resultString);
	}

	ioString.Compose();		// sc 15/06/2007
}



/* remove last path component in order to keep only the base path without the file name if any */
void VURL::_RemoveLastPathComponent()
{
	XBOX::VString path;
	_GetPart( path, XBOX::e_Path);
	if (!path.EndsWith(CVSTR("/")))
	{
		XBOX::VIndex iPosLast = path.FindUniChar( (UniChar)'/', (XBOX::VIndex)path.GetLength(), true);
		if (iPosLast > 0)
		{
			XBOX::VString temp;
			path.GetSubString( 1, iPosLast, temp);
			path = temp;
		}	else
		{
			path = CVSTR("/");
		}

		_SetPart( path, XBOX::e_Path);
	}
}


void VURL::_GetAbsoluteURL(VString& outURLString) const
{
	//JQ 13/12/2012: fixed again ACI0075055
	if (fURLString.EqualToString("about:blank", true))
	{
		outURLString = fURLString;
		return;
	}

	// Internal use, no escape convertion
	outURLString.Clear();
	if (fBaseURL)
		fBaseURL->_GetAbsoluteURL(outURLString);
	if (!fURLString.IsEmpty())
	{
		if ((!outURLString.IsEmpty()) && (!outURLString.EndsWith(CVSTR("/"))))
			outURLString.AppendString(CVSTR("/"));
		outURLString.AppendString(fURLString);
	}
}


void VURL::_GetRelativePath(VString& outString) const
{
	sLONG size, pos;
	_GetAbsoluteURL(outString);
	pos = _GetBaseLength();
	if (fURLArray[e_Path].pos > pos)
		pos = fURLArray[e_Path].pos-1;
	size = fURLArray[e_Path].size+fURLArray[e_Path].pos-pos-1;
	if (size>0)
		outString.SubString(pos+1, size);
	else
		outString.Clear();
}


const VString kPOSIX_SEP( "../");

void VURL::HFS_to_POSIX( VString& ioString, bool inRelative)
{
	if (!ioString.IsEmpty())
	{
		bool done = false;
		VString lstr( ioString);
		ioString.Clear();
		
		#if VERSIONMAC
		if (!inRelative)
		{
			Boolean isFolder = lstr[lstr.GetLength()-1] == ':';
			
			CFStringRef cfPath = lstr.MAC_RetainCFStringCopy();
			CFURLRef cfUrl = (cfPath == NULL) ? NULL : ::CFURLCreateWithFileSystemPath( kCFAllocatorDefault, cfPath, kCFURLHFSPathStyle, isFolder);
			if (cfPath != NULL)
				CFRelease( cfPath);

			if (cfUrl != NULL)
			{
				cfPath = ::CFURLCopyFileSystemPath( cfUrl, kCFURLPOSIXPathStyle);
				if (cfPath != NULL)
				{
					ioString.MAC_FromCFString( cfPath);
					ioString.Compose();
					if ( isFolder && ioString[ioString.GetLength()-1] != '/')
						ioString.AppendUniChar( '/');
					done = true;
					CFRelease( cfPath);
				}
				CFRelease( cfUrl);
			}
		}
		#endif

		if (!done)
		{
			sLONG pos = 0, len = lstr.GetLength(), pendingSeparator = 0;
			while (pos < len)
			{
				// ':' becomes '/', :: sequence becomes /../ and ::: sequence becomes /../../
				if (lstr[pos] == FOLDER_SEPARATOR)
				{
					++pendingSeparator;
					++pos;
					continue;
				}
				
				if (pendingSeparator > 0)
				{
					if (pos != 1)
						ioString.AppendUniChar( '/');
					--pendingSeparator;
		
					while (pendingSeparator > 0)
					{
						ioString.AppendString( kPOSIX_SEP);
						--pendingSeparator;
					}
				}
				
				if (lstr[pos]=='/')
					ioString.AppendUniChar(':');
				else
					ioString.AppendUniChar( lstr[pos]);

				++pos;
			}		
			
			if (pendingSeparator > 0)
			{
				if (pos != 1)
					ioString.AppendUniChar( '/');
				--pendingSeparator;
				
				while (pendingSeparator > 0)
				{
					ioString.AppendString( kPOSIX_SEP);
					--pendingSeparator;
				}
			}
			
			#if VERSIONMAC
			if (!inRelative)
				ioString.Insert( CVSTR("/Volumes/"), 1);
			#endif
		}
	}
}


void VURL::POSIX_to_HFS( VString& ioString, bool inRelative)
{
	bool done = false;

	#if VERSIONMAC
	if (!inRelative)
	{
		/*
			L.E. 08/08/02 on a besoin d'acceder au file system pour les pbs de volumes
			
			ex: /volumes/folder/file doit parfois devenir macintoshhd:folder:file
			
			a voir les sources de CFURL.c c'est assez complexe alors autant l'appeler.
		*/
	
		VString path = ioString;
		
		bool isFolder = !path.IsEmpty() && path[path.GetLength()-1] == '/';
		
		CFStringRef cfPath = ::CFStringCreateWithCharactersNoCopy( kCFAllocatorDefault, path.GetCPointer(), path.GetLength(), kCFAllocatorNull);
		CFURLRef cfUrl = ::CFURLCreateWithFileSystemPath( kCFAllocatorDefault, cfPath, kCFURLPOSIXPathStyle, isFolder);
		CFRelease(cfPath);

		if (cfUrl != NULL)
		{
			cfPath = ::CFURLCopyFileSystemPath( cfUrl, kCFURLHFSPathStyle);
			if (cfPath != NULL)
			{
				ioString.MAC_FromCFString( cfPath);
				ioString.Compose();	// everything that comes from the file system is in decomposed form

				if (isFolder && !ioString.IsEmpty() && (ioString[ioString.GetLength()-1] != FOLDER_SEPARATOR) )	// CFURLCopyFileSystemPath removes trailing /
					ioString += FOLDER_SEPARATOR;
				
				::CFRelease( cfPath);
				done = true;
			 }
			::CFRelease( cfUrl);
		}
	}
	#endif
	
	if (!done)
	{
		VString lstr( ioString);
		ioString.Clear();
		
		sLONG pos = 0, len = lstr.GetLength(), pendingSeparator = 0;
		while (pos < len)
		{
			// ignore ./ sequence
			if ((pos + 1 < len) && (lstr[pos] == CHAR_FULL_STOP) && (lstr[pos+1] == '/'))
			{
				pos += 2;
				continue;
			}

			// '/' becomes ':'
			if (lstr[pos] == '/')
			{
				++pendingSeparator;
				++pos;
				continue;
			}
			
			// ../ sequence becomes : and ../../ sequence becomes ::
			if ((pos + 2 < len) && (lstr[pos] == CHAR_FULL_STOP) && (lstr[pos+1] == CHAR_FULL_STOP) && (lstr[pos+2] == '/'))
			{
				++pendingSeparator;
				pos += 3;
				continue;
			}
			
			while (pendingSeparator > 0)
			{
				ioString.AppendUniChar( FOLDER_SEPARATOR);
				--pendingSeparator;
			}
			
			ioString.AppendUniChar( lstr[pos]);
			++pos;
		}
		
		while (pendingSeparator > 0)
		{
			ioString.AppendUniChar( FOLDER_SEPARATOR);
			--pendingSeparator;
		}		
	}
}


void VURL::WIN_to_POSIX( VString& ioString )
{
	ioString.ExchangeAll( '\\', '/' );
}


void VURL::POSIX_to_WIN( VString& ioString )
{
	ioString.ExchangeAll( '/', '\\' );	
}


void VURL::Clear()
{
	delete fBaseURL;
	fBaseURL = NULL;
	fURLString.Clear();
	fURLStringEncoded.Clear();
	fIsConform = false;

	for (sWORD i = e_Scheme;i<e_LastPart;i++)
	{
		fURLArray[i].pos = 0;
		fURLArray[i].size = 0;
	}
}


bool VURL::_BuildURLArray(const VString& inURLString, bool inOnlySchemeHostAndPath, URLPartArray& outURLArray) const
{
	bool result = true;
	VString tempstr;
	VIndex endpos, startpos = 1;
	
	for (sWORD i = e_Scheme;i<e_LastPart;i++)
	{
		outURLArray[i].pos = 0;
		outURLArray[i].size = 0;
	}
	
	_GetAbsoluteURL(tempstr);
	if (inURLString.GetLength())
	{
		endpos = inURLString.Find( CVSTR( "://"), startpos);		// network location separator
		
		// rfc 1738 says
		// "Scheme names consist of a sequence of characters. The lower case letters "a"--"z", digits, and the characters plus ("+"), period ("."), and hyphen ("-") are allowed."
		
		if (endpos > 1)	// at least one char
		{
			// reject schemes that contain '/' or ':'
			if ( (inURLString.FindUniChar( '/', endpos-1, true) > 0) || (inURLString.FindUniChar( ':', endpos-1, true) > 0) )
				endpos = 0;
		}
		
		if (endpos)
		{
			outURLArray[e_Scheme].pos = startpos;						// scheme
			outURLArray[e_Scheme].size = endpos -startpos;
			startpos = endpos + 3;
			
			endpos = (startpos <= inURLString.GetLength()) ? inURLString.FindUniChar( '/', startpos) : 0;				// network location
			if (endpos > 0)
			{
				outURLArray[e_NetLoc].pos = startpos;
				outURLArray[e_NetLoc].size = endpos - startpos;                
				startpos = endpos+1;
			}
			else
			{
				// there's a scheme but no '/' (ex: http://www.apple.com instead of http://www.apple.com/ ) everything is considered to be the network location
				outURLArray[e_NetLoc].pos = startpos;
				outURLArray[e_NetLoc].size = inURLString.GetLength() + 1 - startpos;
				startpos = inURLString.GetLength()+1;
				result = false;
			}
		}
		else
		{
			result = false;
		}
		outURLArray[e_Path].pos = startpos;							// path
		
		if (!inOnlySchemeHostAndPath && (startpos <= inURLString.GetLength()) )
		{
			endpos = inURLString.FindUniChar(';', startpos);		// param
			if (endpos > 0)
				outURLArray[e_Params].pos = endpos+1;
			
			endpos = inURLString.FindUniChar('?', startpos);		// query
			if (endpos > 0)
				outURLArray[e_Query].pos = endpos+1;
			
			endpos = inURLString.FindUniChar('#', startpos);		// fragment
			if (endpos > 0)
				outURLArray[e_Fragment].pos = endpos+1;
		}
		
		// path size
		endpos = inURLString.GetLength() +1;
		if (outURLArray[e_Params].pos)
		{
			if (endpos > outURLArray[e_Params].pos)
				endpos = outURLArray[e_Params].pos-1;
		}
		if (outURLArray[e_Query].pos) 
		{
			if (endpos > outURLArray[e_Query].pos)
				endpos = outURLArray[e_Query].pos-1;
		}
		if (outURLArray[e_Fragment].pos)
		{
			if (endpos > outURLArray[e_Fragment].pos)
				endpos = outURLArray[e_Fragment].pos-1;
		}
		
		outURLArray[e_Path].size = endpos- outURLArray[e_Path].pos;
		
		// param size
		if (outURLArray[e_Params].pos)
		{
			endpos = inURLString.GetLength() +1;
			if (outURLArray[e_Query].pos > outURLArray[e_Params].pos)
			{
				if (endpos>outURLArray[e_Query].pos)
					endpos = outURLArray[e_Query].pos-1;
			}
			if (outURLArray[e_Fragment].pos > outURLArray[e_Params].pos)
			{
				if (endpos>outURLArray[e_Fragment].pos)
					endpos = outURLArray[e_Fragment].pos-1;
			}
			outURLArray[e_Params].size = endpos - outURLArray[e_Params].pos;
		}

		// query size
		if (outURLArray[e_Query].pos)
		{
			endpos = inURLString.GetLength() +1;
			if (outURLArray[e_Params].pos > outURLArray[e_Query].pos)
			{
				if (endpos>outURLArray[e_Params].pos)
					endpos = outURLArray[e_Params].pos-1;
			}
			if (outURLArray[e_Fragment].pos > outURLArray[e_Query].pos)
			{
				if (endpos>outURLArray[e_Fragment].pos)
					endpos = outURLArray[e_Fragment].pos-1;
			}
			outURLArray[e_Query].size = endpos - outURLArray[e_Query].pos;
		}

		// fragment size
		if (outURLArray[e_Fragment].pos)
		{
			endpos = inURLString.GetLength() +1;			
			outURLArray[e_Fragment].size = endpos - outURLArray[e_Fragment].pos;
		}
		
	}
	return result;	
}


bool VURL::_Parse( bool inOnlySchemeHostAndPath)
{
	VString tempstr;
	_GetAbsoluteURL( tempstr);
	fIsConform = _BuildURLArray( tempstr, inOnlySchemeHostAndPath, fURLArray);
	return fIsConform;
}


