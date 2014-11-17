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
#include "VServerNetPrecompiled.h"
#include "VHTTPCookie.h"
#include "HTTPTools.h"


BEGIN_TOOLBOX_NAMESPACE
USING_TOOLBOX_NAMESPACE


//--------------------------------------------------------------------------------------------------


static
void _GetRFC1123DateString (const XBOX::VTime& inDate, XBOX::VString& outString)
{
	sWORD year, month, day, hour, minute, second, millisecond;
	inDate.GetUTCTime (year, month, day, hour, minute, second, millisecond);

	outString.Clear();

	switch (inDate.GetWeekDay())
	{
	case 0:		outString.AppendCString ("Sun");	break;
	case 1:		outString.AppendCString ("Mon");	break;
	case 2:		outString.AppendCString ("Tue");	break;
	case 3:		outString.AppendCString ("Wed");	break;
	case 4:		outString.AppendCString ("Thu");	break;
	case 5:		outString.AppendCString ("Fri");	break;
	case 6:		outString.AppendCString ("Sat");	break;
	}

	outString.AppendPrintf (", %02d-", day);

	switch (month)
	{
	case 1:		outString.AppendCString ("Jan");	break;
	case 2:		outString.AppendCString ("Feb");	break;
	case 3:		outString.AppendCString ("Mar");	break;
	case 4:		outString.AppendCString ("Apr");	break;
	case 5:		outString.AppendCString ("May");	break;
	case 6:		outString.AppendCString ("Jun");	break;
	case 7:		outString.AppendCString ("Jul");	break;
	case 8:		outString.AppendCString ("Aug");	break;
	case 9:		outString.AppendCString ("Sep");	break;
	case 10:	outString.AppendCString ("Oct");	break;
	case 11:	outString.AppendCString ("Nov");	break;
	case 12:	outString.AppendCString ("Dec");	break;
	}

	outString.AppendPrintf ("-%04d %02d:%02d:%02d GMT", year, hour, minute, second);
}


//--------------------------------------------------------------------------------------------------


VHTTPCookie::VHTTPCookie()
: fVersion (0)
, fSecure (false)
, fMaxAge (-1)
, fHTTPOnly (false)
{

}


VHTTPCookie::VHTTPCookie (const XBOX::VString& inCookieString)
: fVersion (0)
, fSecure (false)
, fMaxAge (-1)
, fHTTPOnly (false)
{
	FromString (inCookieString);
}


VHTTPCookie::VHTTPCookie (const VHTTPCookie& inCookie)
{
	*this = inCookie;
}


VHTTPCookie::~VHTTPCookie()
{
}

	
VHTTPCookie& VHTTPCookie::operator = (const VHTTPCookie& inCookie)
{
	if (&inCookie != this)
	{
		fVersion  = inCookie.fVersion;
		fName     = inCookie.fName;
		fValue    = inCookie.fValue;
		fComment  = inCookie.fComment;
		fDomain   = inCookie.fDomain;
		fPath     = inCookie.fPath;
		fSecure   = inCookie.fSecure;
		fMaxAge   = inCookie.fMaxAge;
		fHTTPOnly = inCookie.fHTTPOnly;
	}

	return *this;
}


bool VHTTPCookie::operator == (const VHTTPCookie& inCookie) const
{
	if (&inCookie == this)
		return true;

	return (HTTPTools::EqualASCIIVString (fName, inCookie.fName) && HTTPTools::EqualASCIIVString (fValue, inCookie.fValue));
}


void VHTTPCookie::Clear()
{
	fVersion = 0;
	fName.Clear();
	fValue.Clear();
	fComment.Clear();
	fDomain.Clear();
	fPath.Clear();
	fSecure = false;
	fMaxAge = -1;
	fHTTPOnly = false;
}


bool VHTTPCookie::IsValid() const
{
	if (!fName.IsEmpty() && !fValue.IsEmpty())
		return true;

	return false;
}


XBOX::VString VHTTPCookie::ToString (bool inAlwaysExpires) const
{
	XBOX::VString result;

	if (!IsValid())
		return result;

	result.AppendString (fName);
	result.AppendUniChar (CHAR_EQUALS_SIGN);

	if (0 == fVersion)
	{
		// Netscape cookie
		result.AppendString (fValue);
		if (!fDomain.IsEmpty())
		{
			result.AppendCString ("; Domain=");
			result.AppendString (fDomain);
		}

		if (!fPath.IsEmpty())
		{
			result.AppendCString ("; Path=");
			result.AppendString (fPath);
		}

		if (fMaxAge >= 0)
		{
			XBOX::VTime		curTime;
			XBOX::VString	timeString;

			XBOX::VTime::Now (curTime);
			
			curTime.AddSeconds (fMaxAge);
			_GetRFC1123DateString (curTime, timeString);
			result.AppendCString ("; Expires=");
			result.AppendString (timeString);
		}

		if (fSecure)
		{
			result.AppendCString ("; Secure");
		}

		if (fHTTPOnly)
		{
			result.AppendCString ("; HttpOnly");
		}
	}
	else
	{
		// RFC 2109 cookie
		result.AppendString (fValue);

		if (!fComment.IsEmpty())
		{
			result.AppendCString ("; Comment=\"");
			result.AppendString (fComment);
			result.AppendUniChar (CHAR_QUOTATION_MARK);
		}

		if (!fDomain.IsEmpty())
		{
			result.AppendCString ("; Domain=");
			result.AppendString (fDomain);
		}

		if (!fPath.IsEmpty())
		{
			result.AppendCString ("; Path=");
			result.AppendString (fPath);
		}
		else
		{
			result.AppendCString ("; Path=/");
		}

		if (fMaxAge >= 0)
		{
			result.AppendCString ("; Max-Age=");
			result.AppendLong (fMaxAge);

			/* For Internet Explorer 6, 7 & 8 which does not support 'max-age' */ 
			if (inAlwaysExpires)
			{
				XBOX::VTime		curTime;
				XBOX::VString	timeString;

				XBOX::VTime::Now (curTime);

				curTime.AddSeconds (fMaxAge);
				_GetRFC1123DateString (curTime, timeString);
				result.AppendCString ("; Expires=");
				result.AppendString (timeString);
			}
		}

		if (fSecure)
		{
			result.AppendCString ("; Secure");
		}

		if (fHTTPOnly)
		{
			result.AppendCString ("; HttpOnly");
		}

		result.AppendCString ("; Version=1");
	}

	return result;
}


void VHTTPCookie::FromString (const XBOX::VString& inString)
{
	XBOX::VectorOfVString	stringValues;
	XBOX::VString			string;
	XBOX::VString			nameString;
	XBOX::VString			valueString;

	Clear();

	inString.GetSubStrings (CHAR_SEMICOLON, stringValues, false, true);

	for (VectorOfVString::iterator it = stringValues.begin(); it != stringValues.end(); ++it)
	{
		sLONG pos = 0;
		string.FromString (*it);

		if ((pos = HTTPTools::FindASCIICString (string, "secure")) == 1)
		{
			SetSecure (true);
		}
		else if ((pos = HTTPTools::FindASCIICString (string, "httpOnly")) == 1)
		{
			SetHttpOnly (true);
		}
		else if ((pos = HTTPTools::FindASCIICString (string, "version")) == 1)
		{
			SetVersion (1);
		}
		else if ((pos = HTTPTools::FindASCIICString (string, "max-age")) == 1)
		{
			HTTPTools::ExtractFieldNameValue (string, nameString, valueString);
			fMaxAge = valueString.GetLong();
		}
		else if ((pos = HTTPTools::FindASCIICString (string, "expires")) == 1)
		{
			XBOX::VTime curTime;
			XBOX::VTime expiresTime;
			XBOX::VTime::Now (curTime);

			HTTPTools::ExtractFieldNameValue (string, nameString, valueString);
			expiresTime.FromRfc822String (valueString);

			if (expiresTime.GetMilliseconds() > curTime.GetMilliseconds())
				fMaxAge = (expiresTime.GetMilliseconds() - curTime.GetMilliseconds()) / 1000;
		}
		else if ((pos = HTTPTools::FindASCIICString (string, "path")) == 1)
		{
			HTTPTools::ExtractFieldNameValue (string, nameString, fPath);
		}
		else if ((pos = HTTPTools::FindASCIICString (string, "domain")) == 1)
		{
			HTTPTools::ExtractFieldNameValue (string, nameString, fDomain);
		}
		else if ((pos = HTTPTools::FindASCIICString (string, "comment")) == 1)
		{
			HTTPTools::ExtractFieldNameValue (string, nameString, fComment);
		}
		else if ((pos = HTTPTools::FindASCIICString (string, "=")))
		{
			HTTPTools::ExtractFieldNameValue (string, fName, fValue);
		}
	}
}


END_TOOLBOX_NAMESPACE
