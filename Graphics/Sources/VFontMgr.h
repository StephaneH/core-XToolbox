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
#ifndef __VFontMgr__
#define __VFontMgr__

// Native declarations
#if VERSIONWIN
#include "Graphics/Sources/XWinFontMgr.h"
#elif VERSIONMAC
#include "Graphics/Sources/XMacFontMgr.h"
#endif

BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VFont;

/** max size for list of last used font names */
#define VFONTMGR_LAST_USED_FONT_LIST_SIZE	10


class XTOOLBOX_API VFontMgr : public VObject
{
public:
							VFontMgr ();
virtual						~VFontMgr ();
	
							/** return available font family names */
		void				GetFontNameList (VectorOfVString& outFontNames, bool inWithScreenFonts = true);

							/** return available font full names 
							@remarks 
								might be NULL:
								as it is used only on Mac for FONT LIST compatibility with v12 (with parameter *, FONT LIST should return full names)
								it returns only a valid reference on Mac OS X, otherwise it returns NULL
							*/
		void				GetFontFullNameList(VectorOfVString& outFontNames, bool inWithScreenFonts = true);

		void				GetUserActiveFontNameList (VectorOfVString& outFontNames);
		void				GetFontNameListForContextualMenu (VectorOfVString& outFontNames);

		VFont*				RetainStdFont (StdFont inFont);
		VFont*				RetainFont (const VString& inFontFamilyName, const VFontFace& inStyle, GReal inSize, const GReal inDPI = 0, bool inReturnNULLIfNotExist = false);
	
							/** add font name to the list of last used font names */
		void				AddLastUsedFontName( const VString& inFontName) const
        {
            VString fontName(inFontName);
            LocalizeFontName(fontName);
            VLastUsedFontNameMgr::Get()->AddFontName( fontName);
        }
								
							/** get last used font names */
		void				GetLastUsedFontNameList(VectorOfVString& outFontNames) const { VLastUsedFontNameMgr::Get()->GetFontNames( outFontNames); }
		
							/** clear list of last used font names */
		void				ClearLastUsedFontNameList() const { VLastUsedFontNameMgr::Get()->Clear(); }
    
                            /** localize font name (if font name is not localized) according to current system UI language */
        void                LocalizeFontName(VString& ioFontName) const;
private:
		/** last used font name manager (private class only) */
		class VLastUsedFontNameMgr : public VObject
		{
		public:

		static	bool					Init();
		static	void					Deinit();
		static	VLastUsedFontNameMgr*	Get();

										VLastUsedFontNameMgr () {}
		virtual							~VLastUsedFontNameMgr () {}
			
										/** add font name to the list of last used font names */
				void					AddFontName( const VString& inFontName);			
										
										/** get last used font names */
				void					GetFontNames(VectorOfVString& outFontNames) const;
				
										/** clear list of last used font names */
				void					Clear() { VTaskLock protect(&fCriticalSection); fLastUsedFonts.clear(); }

		private:
										/** typedef for pair <font name, timestamp> */
				typedef					std::pair<VString,uLONG> LastUsedFont;

										/** typedef for vector of last used fonts */
				typedef					std::vector<LastUsedFont> VectorOfLastUsedFont;

		static	VLastUsedFontNameMgr*	sSingleton;

										/** vector of last used font names */
				VectorOfLastUsedFont	fLastUsedFonts;

		mutable VCriticalSection		fCriticalSection;
		};

mutable VCriticalSection	fCriticalSection;

		VArrayRetainedPtrOf<VFont*>	fFonts;
	
		VFont*				fStdFonts[STDF_LAST]; // cache for std font

		XFontMgrImpl		fFontMgr;
};



END_TOOLBOX_NAMESPACE

#endif
