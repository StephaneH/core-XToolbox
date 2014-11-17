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
#ifndef __XWinFontMgr__
#define __XWinFontMgr__

BEGIN_TOOLBOX_NAMESPACE

class XWinFontMgr : VObject
{
public:
			XWinFontMgr ();
	virtual	~XWinFontMgr ();
	
	void				GetFontNameList (VectorOfVString& outFontNames, bool inWithScreenFonts);
	/** not implemented on Windows (because not used) */
	void				GetFontFullNameList(VectorOfVString& outFontNames, bool inWithScreenFonts) { outFontNames.clear(); }

	void				GetUserActiveFontNameList(VectorOfVString& outFontNames);
	void				GetFontNameListForContextualMenu(VectorOfVString& outFontNames);
	
	void				BuildFontList();
	void				GetStdFont (StdFont inFont, VString& outName, VFontFace& outFace, GReal& outSize);
    
	/** get fonts substitutes file name */
	void				GetFontsSubstitutesFileName(VString& outFileName) 
	{
		outFileName = "WinFontsSubstitutes.json";
	}

	/** return true if GDI font with passed family name exists */
	bool				FontGDIExists(const VString& inName);

private:
mutable VCriticalSection	fCriticalSection;

	VArrayWord	fFontFamilies;
	VectorOfVString	fFontNames;
	VectorOfVString	fInactiveFontNames;
	VectorOfVString	fActiveFontNames;
	VectorOfVString	fContextualMenuFontNames;

	static BOOL CALLBACK EnumFamScreenCallBackEx(ENUMLOGFONTEXW* pelf, NEWTEXTMETRICEXW* /*lpntm*/, int FontType,LPVOID pThis);
	static BOOL CALLBACK FaceNameToFullNameCallBackEx(ENUMLOGFONTEXW* pelf, NEWTEXTMETRICEXW* /*lpntm*/, int FontType,LPVOID pThis);
	void	AddFont(const VString& inFontName);
	void	AddInactiveFont(const VString& inFontName);
};


typedef XWinFontMgr XFontMgrImpl;

END_TOOLBOX_NAMESPACE

#endif