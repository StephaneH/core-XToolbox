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

BEGIN_TOOLBOX_NAMESPACE

typedef enum eFontNameKind
{
	kFONTNAMEKIND_FAMILY,			//family name
	kFONTNAMEKIND_POSTSCRIPT,		//postscript name
	kFONTNAMEKIND_FULL,				//full name with styles
	kFONTNAMEKIND_DESIGN			//design name -> name which was used to create the font reference

} eFontNameKind;

END_TOOLBOX_NAMESPACE

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
			LocalizeFontName(fontName, kFONTNAMEKIND_FAMILY);
            VLastUsedFontNameMgr::Get()->AddFontName( fontName);
        }
								
							/** get last used font names */
		void				GetLastUsedFontNameList(VectorOfVString& outFontNames) const { VLastUsedFontNameMgr::Get()->GetFontNames( outFontNames); }
		
							/** clear list of last used font names */
		void				ClearLastUsedFontNameList() const { VLastUsedFontNameMgr::Get()->Clear(); }
    
                            /** localize font name (if font name is not localized) according to current system UI language */
        void                LocalizeFontName(VString& ioFontName, const eFontNameKind inFontNameKind = kFONTNAMEKIND_FAMILY) const;

                            /** unlocalize font name (if font name is localized) - convert to neutral font name */
        void                UnLocalizeFontName(VString& ioFontName, const eFontNameKind inFontNameKind = kFONTNAMEKIND_FAMILY) const;

#if ENABLE_CUSTOM_FONTS_SUBSTITUTES

							/** notify that current database has changed (in order to update database specific fonts substitutes) */
		void				NotifyChangeDatabase() { _BuildFontsSubstitutes(false); }

							/** return true if custom font family substitute is available for the passed family name */
		bool				HasFontFamilySubstitute(const VString& inName) const;

							/** return true if custom font family substitutes are available */
		bool				HasFontFamilySubstitutes() const;

							/** get font family name substitute */
		void				GetFontFamilySubstitute(const VString& inName, VString& outNameSubstitute, const GraphicContextType inGCType = eDefaultGraphicContext, const TextRenderingMode inTRM = TRM_NORMAL);

#endif

#if VERSIONWIN
							/** return true if GDI font with passed family name exists */
		bool				FontGDIExists(const VString& inName) { return fFontMgr.FontGDIExists(inName); }
#endif

							/** set application interface */
		void				SetApplicationInterface(IGraphics_ApplicationIntf *inApplication)
		{
			fAppIntf = inApplication;
		}

							/** get application interface */
		IGraphics_ApplicationIntf*	GetApplicationInterface() { return fAppIntf; }

private:
#if ENABLE_CUSTOM_FONTS_SUBSTITUTES
							/** build fonts substitutes tables */
		void				_BuildFontsSubstitutes(bool inApplication = true);

		void				_ReadStringFromFile(VFile *inFile, VString& outString);

							/** get fonts substitutes file name	*/ 
		void				_GetFontsSubstitutesFileName(VString& outFileName) { fFontMgr.GetFontsSubstitutesFileName( outFileName); }
#endif

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

		/** global font substitutes  */
		VJSONValue			fFontsSubstitutes;

		/** current database font substitutes (override global font substitutes) */
		VJSONValue			fDatabaseFontsSubstitutes;

		IGraphics_ApplicationIntf*		fAppIntf;
};



END_TOOLBOX_NAMESPACE

#endif
