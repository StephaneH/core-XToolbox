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
#ifndef __VFULLURL__
#define __VFULLURL__

#include "Kernel/Sources/VString.h"
#include "Kernel/Sources/VValueBag.h"


BEGIN_TOOLBOX_NAMESPACE



class XTOOLBOX_API VFullURL
{
public:
	VFullURL()
	{
		fIsValid = false;
	}

	VFullURL(const VString& inTextURL, bool CheckForRest = false);
	bool ParseURL(const VString& inTextURL, bool CheckForRest = false);

	const VString* NextPart();
	const VString* PeekNextPart(sLONG offset = 1);

	inline sLONG GetCurPartPos() const
	{
		return fCurPathPos;
	}

	inline void SetCurPartPos(sLONG newpos)
	{
		fCurPathPos = newpos;
	}

	bool GetValue(const VValueBag::StKey& inVarName, VString& outValue);

	bool IsValid() const
	{
		return fIsValid;
	}

	inline const VString& GetSource() const
	{
		return fBaseURL;
	}

	const VValueBag& GetValues() const
	{
		return fQuery;
	}

	const std::vector<VString>& GetPath() const
	{
		return fPath;
	}

protected:
	VString fBaseURL;
	std::vector<VString> fPath;
	VValueBag fQuery;
	sLONG fCurPathPos;
	bool fIsValid;
};


END_TOOLBOX_NAMESPACE

#endif