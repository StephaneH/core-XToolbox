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

#include "VFullURL.h"
#include "VValueBag.h"

VFullURL::VFullURL(const VString& inTextURL, bool CheckForRest)
{
	fIsValid = ParseURL(inTextURL, CheckForRest);
}


bool VFullURL::ParseURL(const VString& inTextURL, bool CheckForRest)
{
	bool result = true;
	fBaseURL = inTextURL;
	fPath.clear();
	fCurPathPos = -1;
	
	bool inquerypart = false;
	const UniChar* p = fBaseURL.GetCPointer();
	sLONG len = fBaseURL.GetLength();
	VString curword;
	bool inquotes = false, indoublequotes = false;
	while (len > 0)
	{
		UniChar c = *p;
		p++;
		len--;

		if (inquotes)
		{
			curword.AppendUniChar(c);
			if (c == 39)
				inquotes = false;
		}
		else if (indoublequotes)
		{
			curword.AppendUniChar(c);
			if (c == 34)
				indoublequotes = false;
		}
		else if (c == 39)
		{
			curword.AppendUniChar(c);
			inquotes = true;
		}
		else if (c == 34)
		{
			curword.AppendUniChar(c);
			indoublequotes = true;
		}
		else if (c == '/')
		{
			if (!curword.IsEmpty())
			{
				fPath.push_back(curword);
			}
			curword.Clear();
		}
		else if (c == '?')
		{
			inquerypart = true;
			if (!curword.IsEmpty())
			{
				fPath.push_back(curword);
			}
			curword.Clear();
			VString curvalue;
			VString curvar;
			bool inquotes2 = false, indoublequotes2 = false;
			bool waitforvar = true;
			bool waitforvalue = false;
			while (len > 0)
			{
				UniChar c2 = *p;
				p++;
				len--;

				if (inquotes2)
				{
					if (c2 == '\'')
					{
						inquotes2 = false;
					}
					else
						curword.AppendUniChar(c2);
				}
				else if (indoublequotes2)
				{
					if (c2 == '"')
					{
						indoublequotes2 = false;
					}
					else
						curword.AppendUniChar(c2);
				}
				else
				{
					switch(c2)
					{
						case '=':
							curvar = curword;
							curvalue.Clear();
							curword.Clear();
							waitforvar = false;
							waitforvalue = true;
							break;

						case '&':
							curvalue = curword;
							if (!curvar.IsEmpty())
							{
								VValueBag::StKey curvarKey(curvar);
								fQuery.SetString(curvarKey, curvalue);
								/*
								if (CheckForRest && !RestTools::IsValidRestKeyword(curvarKey))
								{
									ThrowBaseError(rest::unknown_rest_query_keyword, curvar);
									result = false;
								}
								*/
							}
							curvalue.Clear();
							curword.Clear();
							curvar.Clear();
							waitforvar = true;
							waitforvalue = false;
							break;

						case '\'':
							inquotes2 = true;
							break;

						case '"':
							indoublequotes2 = true;
							break;

						default:
							curword.AppendUniChar(c2);
							break;
					}
				}
			} // du while (len > 0) au 2eme niveau

			if (inquotes2)
			{
				result = false;
				vThrowError(expecting_closing_single_quote);
			}

			if (indoublequotes2)
			{
				result = false;
				vThrowError(expecting_closing_double_quote);
			}

			if (waitforvalue)
				curvalue = curword;
			if (!curvar.IsEmpty())
			{
				VValueBag::StKey curvarKey(curvar);
				fQuery.SetString(curvarKey, curvalue);
				/*
				if (CheckForRest && !RestTools::IsValidRestKeyword(curvarKey))
				{
					ThrowBaseError(rest::unknown_rest_query_keyword, curvar);
					result = false;
				}
				*/
			}
			curword.Clear();

		}
		else
		{
			curword.AppendUniChar(c);
		}
	} // du while (len > 0) au 1er niveau

	if (!curword.IsEmpty())
	{
		fPath.push_back(curword);
	}

	fIsValid = result;
	return result;
}


const VString* VFullURL::NextPart()
{
	fCurPathPos++;
	if (fCurPathPos >= 0 && fCurPathPos < fPath.size())
		return &(fPath[fCurPathPos]);
	else
		return nil;
}


const VString* VFullURL::PeekNextPart(sLONG offset)
{
	sLONG pos = fCurPathPos+offset;
	if (pos >= 0 && pos < fPath.size())
		return &(fPath[pos]);
	else
		return nil;
}



bool VFullURL::GetValue(const VValueBag::StKey& inVarName, VString& outValue)
{
	return fQuery.GetString(inVarName, outValue);
}


