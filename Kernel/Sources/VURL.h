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
#ifndef __VURL__
#define __VURL__

#include "Kernel/Sources/VString.h"


BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VFolder;
class VFilePath;

/*!
	@class	VURL
	@abstract	URL management class.
	@discussion
		VUrl class is a class for managing Uniform Resources locators as Define in RFC 1808
		VURL is composed of two parts : the base URL and the URL string
		an URL can be 
			- relative, with a base URL and a partial path string, 
			- absolute , with a NULL base URL and a full URL string.
		URL String is a ToolBox VString an is parsed as needed
		URL String can be decomposed in 6 part :
			<scheme>://<net_loc>/<path>;<params>?<query>#<fragment>
		each part can be accessed individualy.
		
		Internally the VURL always work on decoded strings.
		
*/
class XTOOLBOX_API VURL : public VObject
{
public:
	/*!
		@function	VURL
		@abstract	Creates an empty URL
	*/
	VURL();
	virtual ~VURL();
	
	/*!
		@function	VURL
		@abstract	Copy constructor
		@param		inURL the VURL to copy.
	*/
	VURL( const VURL& inURL);
	
	/*!
		@function	VURL
		@abstract	Creates a VURL from a VString.
		@param		inURL the full url string
	*/
	explicit	VURL( const VString& inURL, bool inEncodedString);
	
	/*!
		@function	VURL
		@param		inFilePath the full file path string. The file path must not be encoded.
		@param		inStyle the path style of the incomming string
		@param		inScheme must be ended by "//" ...
	*/
	explicit	VURL( const VString& inFilePath, EURLPathStyle inStyle, const VString& inScheme);

	/*!
		@function	VURL
		@abstract	Creates a Relative VURL from VFolder and a VString.
		@param		inBaseFolder the base url of the URL
		@param		inRelativPath the relative path string. Must not be encoded.
		@param		inStyle the style of the incomming string
	*/
	explicit	VURL( const VFolder& inBaseFolder, const VString& inRelativPath, EURLPathStyle inStyle = eURL_POSIX_STYLE);

	/*!
		@function	VURL
		@abstract	Creates a VURL from a file or folder path. Set the scheme to file:///
		@param		inPath the path to use
	*/
	explicit	VURL( const VFilePath& inPath);

	/*!
		@function	FromURL
		@abstract	Copy method
		@param		inURL the VURL to copy.
	*/
	void	FromURL( const VURL& inURL);
	
	/*!
		@function	FromString
		@abstract	Rebuilds a VURL from a VString.
		@param		inURL the full url string
	*/
	void	FromString( const VString& inURL, const VString& inScheme, bool inEncodedString);
	void	FromString( const VString& inURL, bool inEncodedString);

	/*!
		@function	FromFilePath
		@abstract	Rebuilds a VURL from a file or folder path.
		@param		inFilePath the path
	*/
	void	FromFilePath( const VFilePath& inFilePath);

	/*!
		@function	FromFilePath
		@abstract	Rebuilds a VURL from a file path as VString. The string is not decoded because it is not a url.
		@param		inFilePath the full file path string
		@param		inStyle the style of the incomming string
	*/
	void	FromFilePath( const VString& inFilePath, EURLPathStyle inStyle = eURL_POSIX_STYLE, const VString& inScheme = CVSTR("file://"));

	/*!
		@function	FromRelativePath
		@abstract	Rebuilds a Relative VURL from a VFolder and a VString.
		@param		inBaseFolder the base folder of the URL
		@param		inRelativPath the relative path string
		@param		inStyle the style of the incomming string
	*/
	void	FromRelativePath( const VFolder& inBaseFolder, const VString& inRelativPath, EURLPathStyle inStyle = eURL_POSIX_STYLE);

	/*!
		@function	GetAbsoluteURL
		@abstract	Gets the complete URL's string will all components. WARNING: this is NOT a file path.
		@param		outURLString the complete URL's string.
	*/
	void	GetAbsoluteURL( VString& outURLString, bool inEncoded) const;	

	/*!
		@function	GetFilePath
		@abstract	If the url has no scheme or has scheme file://, it builds a valid VFilePath. returns false if failed to build a valid path.
		@param		outPath the complete URL's path.
	*/
	bool	GetFilePath( VFilePath& outPath) const;	

	/*!
		@function	GetRelativeURL
		@abstract	Gets the relative part of URL's string.
		@param		outURLString the relative part of URL's string.
	*/
	void	GetRelativeURL( VString& outURLString, bool inEncoded) const;

	/*!
		@function	RetainParent
		@abstract	returns a new URL pointin to the Parent directory if exists or NULL if URL's path represents the root directory
		@discussion
			a new URL is created. user must release it.
	*/
	VURL*	RetainParent();

	/*!
		@function	ToParent
		@abstract	Sets 'this' URL to the parent; returns true if suceed
	*/
	bool	ToParent(); // up one level

	/*!
		@function	ToSubFolder
		@abstract	Sets 'this' URL to the child folder whose name is 'inName'; returns true if suceed
	*/
	bool	ToSubFolder( const VString& inName); // down one level

	/*!
		@function	ConformRFC
		@abstract	Returns true if 'this' URL conforms to RFC 1808
	*/
	bool	IsConformRFC() const	{return fIsConform;}

 	/*!
		@function	GetScheme
		@abstract	Gets the Scheme part of the URL
		@param		outURLString the result string.
	*/
	void	GetScheme( VString& outString) const;

 	/*!
		@function	GetNetworkLocation
		@abstract	Gets the Network Location part of the URL
		@param		outURLString the result string.
	*/
	void	GetNetworkLocation( VString& outString, bool inEncoded) const;

 	/*!
		@function	GetPath
		@abstract	Gets the absolute path part of the URL (Base URL if any + relative URL )
					Note that the "/" between the host (or port) and the url-path is NOT part of the url-path.
		@param		outURLString the result string.
	*/
	void	GetPath( VString& outString, EURLPathStyle inStyle = eURL_POSIX_STYLE, bool inEncoded = true) const;

 	/*!
		@function	GetRelativePath
		@abstract	Gets the relative path part of the URL (without Base URL )
		@param		outURLString the result string.
	*/
	void	GetRelativePath( VString& outString, EURLPathStyle inStyle = eURL_POSIX_STYLE, bool inEncoded = true) const;

 	/*!
		@function	GetHostName
		@abstract	Gets the <host> part of the Network Location
		@param		outURLString the result string.
		@discussion
				Network Location generic form is <user>:<password>@<host>:<port>
				The user name (and password), if present, are followed by a
				commercial at-sign "@". (RFC 1738)
	*/
	void	GetHostName( VString& outString, bool inEncoded) const;

 	/*!
		@function	GetPortNumber
		@abstract	Gets the <port> part of Network Location
		@param		outURLString the result string.
	*/
	void	GetPortNumber( VString& outString, bool inEncoded) const;		

 	/*!
		@function	GetUserName
		@abstract	Gets the <user> part of Network Location
		@param		outURLString the result string.
	*/
	void	GetUserName( VString& outString, bool inEncoded) const;

 	/*!
		@function	GetPassword
		@abstract	Get the <password> part of Network Location
		@param		outURLString the result string.
	*/
	void	GetPassword( VString& outString, bool inEncoded) const;		

 	/*!
		@function	GetParameters
		@abstract	Get the Parameters part of the URL
		@param		outURLString the result string.
	*/
	void	GetParameters( VString& outString, bool inEncoded) const;

 	/*!
		@function	GetQuery
		@abstract	Get the Query part of the URL
		@param		outURLString the result string.
	*/
	void	GetQuery( VString& outString, bool inEncoded) const;		

 	/*!
		@function	GetFragment
		@abstract	Get the Fragment part of the URL
		@param		outURLString the result string.
	*/
	void	GetFragment( VString& outString, bool inEncoded) const;		

 	/*!
		@function	GetFragment
		@abstract	Gets any additional resource specifiers after the path (;<params>?<query>#<fragment> ... )
		@param		outURLString the result string.
	*/
	void	GetResourceSpecifier( VString& outString, bool inEncoded) const;

 	/*!
		@function	GetLastPathComponent
		@abstract	Get last path component.
		@discussion /a/b returns b, /a/b/ returns also b
		@param		outComponent the result string.
	*/
	void	GetLastPathComponent( VString& outComponent, bool inEncoded) const; 

	/*!
		@function	IsRoot
		@abstract	Returns true if URL's path represents the root directory
	*/
	bool	IsRoot() const;

 	/*!
		@function	SetScheme
		@abstract	Sets the Scheme part of the URL (must be an absolute URL)
		@param		inString	the new Scheme.
	*/
	void	SetScheme( const VString& inString);

 	/*!
		@function	SetNetWorkLocation
		@abstract	Sets the NetWorkLocation part of the URL (must be an absolute URL)
		@param		inString	the new NetWorkLocation.
	*/
	void	SetNetWorkLocation( const VString& inString, bool inEncodedString);

 	/*!
		@function	SetParameters
		@abstract	Sets the Parameters part of the URL
		@param		inString	the new Parameters.
	*/
	void	SetParameters( const VString& inString, bool inEncodedString);

 	/*!
		@function	SetQuery
		@abstract	Sets the Query part of the URL
		@param		inString the new Query.
	*/
	void	SetQuery( const VString& inString, bool inEncodedString);

 	/*!
		@function	SetFragment
		@abstract	Sets the Fragment part of the URL
		@param		inString	the new Fragment.
	*/
	void	SetFragment( const VString& inString, bool inEncodedString);

 	/*!
		@function	SetRelativePath
		@abstract	sets URL's path name. 
		@param		inName	the new path.
	*/
	void	SetRelativePath( const VString& inString, EURLPathStyle inStyle /*= eURL_POSIX_STYLE*/, bool inEncodedString /*= true*/);
	

	/*!
		@function	Encode
		@abstract	Replace chars in 'inCharsToEscape' with the corresponding %escape sequence
		@param		ioString the string to be encoded
		@param		inCharsToEscape	the chars's list to be encoded.
		@discussion
					The usual set of characters to escape is 'unsafe' caracters as defined in RFC 1738.
					<, >, \, ", #, %, {, }, |, \, ^, ~, [, ], `, ", and space
	*/
	static	void		Encode (VString& ioString, bool inConvertColon = false, bool inDecomposeString = false, bool inEncodeBrackets = true);

	/*!
		@function	Decode
		@abstract	Replace % escape sequence with corresponding chars
		@param		ioString the string to be encoded
			
	*/
	static	void		Decode (VString& ioString);
	static	void		Convert (VString& ioString, EURLPathStyle inOrgStyle, EURLPathStyle inDestStyle, bool inRelative);


			bool		IsEmpty() const		{ return (fBaseURL == NULL) && fURLString.IsEmpty();}
			void		Clear();
	
	VURL&		operator=( const VURL& inOther)		{ FromURL( inOther); return *this;}
	
	void	RemoveLastPathComponent()	{ _RemoveLastPathComponent(); }

private:
			typedef URLPart	URLPartArray[e_LastPart];

			URLPartArray	fURLArray;
			VString			fURLString;		// url always in decoded form
	mutable	VString			fURLStringEncoded; //encoded string (it is equal to initial passed url if it is passed encoded)
			VURL*			fBaseURL;
			bool			fIsConform;

	static	void	HFS_to_POSIX( VString& ioString, bool inRelative);
	static	void	POSIX_to_HFS( VString& ioString, bool inRelative);
	
	static	void	WIN_to_POSIX( VString& ioString );
	static	void	POSIX_to_WIN( VString& ioString );
	
			void	_GetAbsoluteURL( VString& outURLString) const;	// Internal use, no escape convertion
			void	_GetRelativePath( VString& outString) const ;	// Internal use, no escape convertion
			
			bool	_BuildURLArray( const VString& inURLString, bool inOnlySchemeHostAndPath, URLPartArray& outURLArray) const;
			void	_DecodeParts();
			void	_GetPart( VString& outString, EURLParts inPart) const;
			void	_SetPart( const VString& inString, EURLParts inPart);
			sLONG	_GetBaseLength() const { if (fBaseURL) return (fBaseURL->_GetBaseLength()+fBaseURL->fURLString.GetLength()); else return 0;};

	/* remove last path component in order to keep only the base path without the file name if any */
			void	_RemoveLastPathComponent();

			bool	_Parse( bool inOnlySchemeHostAndPath);
		

	/*
		returns true if and only if passed url contains only safe character.
		Might be used as a hint to know if an url has been properly encoded.

		safe characters are the set of reserved and unreserved characters plus % used for percent encoding.
	*/
	static	bool	_ContainsOnlySafeCharacters( const VString& inURL);
};


END_TOOLBOX_NAMESPACE

#endif