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
#ifndef __XMacFontMgr__
#define __XMacFontMgr__

BEGIN_TOOLBOX_NAMESPACE

class XMacFontMgr : public VObject
{
public:
							XMacFontMgr ();
virtual						~XMacFontMgr ();
	
		void				GetStdFont (StdFont inFont, VString& outName, VFontFace& outFace, GReal& outSize);
		void				BuildFontList ();
		
		void				GetFontNameList(VectorOfVString& outFontNames, bool inWithScreenFonts);
		void				GetFontFullNameList(VectorOfVString& outFontNames, bool inWithScreenFonts);
		void				GetUserActiveFontNameList(VectorOfVString& outFontNames);
		void				GetFontNameListForContextualMenu(VectorOfVString& outFontNames);

        /** localize font name (if font name is not localized) according to current system UI language */
        void                LocalizeFontName(VString& ioFontName) const;
    
static	CTFontUIFontType	StdFontToThemeFontID (StdFont inFont);
	
private:
mutable VCriticalSection	fCriticalSection;

		VArrayWord			fFontFamilies;
		VectorOfVString		fFontNames;
		VectorOfVString 	fFontFavoritesNames;
		VectorOfVString		fFontNamesForContextMenu;
		VectorOfVString 	fFontFullNames;
};

typedef XMacFontMgr XFontMgrImpl;


END_TOOLBOX_NAMESPACE


#endif