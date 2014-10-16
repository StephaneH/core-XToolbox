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
#ifndef __VCollator__
#define __VCollator__

#if VERSIONMAC
    #include "Kernel/Sources/XMacStringCompare.h"
#elif VERSION_LINUX
    #include "Kernel/Sources/XLinuxStringCompare.h"
#elif VERSIONWIN
    #include "Kernel/Sources/XWinStringCompare.h"
#endif

#include <map>
#include "Kernel/Sources/VSyncObject.h"


BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VCollator : public VObject, public IRefCountable
{
public:
	virtual	CompareResult			CompareString( const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, bool inWithDiacritics) = 0;
			CompareResult			CompareString_Like( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics);
	virtual	bool					EqualString( const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, bool inWithDiacritics) = 0;
			bool					EqualString_Like( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics);
	

	virtual	sLONG					FindString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength) = 0;
	virtual	sLONG					ReversedFindString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength) = 0;
	virtual	bool					BeginsWithString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength);
	virtual	bool					EndsWithString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength);

	virtual	VCollator*				Clone() const = 0;

			UniChar					GetWildChar() const					{ return fWildChar;}
	virtual	void					SetWildChar( UniChar inWildChar);

			bool					GetIgnoreWildCharInMiddle() const								{ return (fOptions & COL_IgnoreWildCharInMiddle) != 0;}

			DialectCode				GetDialectCode() const				{ return fDialect;}
			CollatorOptions			GetOptions() const					{ return fOptions;}

	virtual	void					GetStringComparisonAlgorithmSignature( VString& outSignature) const = 0;
			
			// restore default settings
	virtual	void					Reset();
			
			bool					IsPatternCompatibleWithDichotomyAndDiacritics( const UniChar *inPattern, const sLONG inSize);
			
	static	void					SetDefaultWildChar( UniChar inWildChar)	{ sDefaultWildChar = inWildChar;}
	static	UniChar					GetDefaultWildChar()					{ return sDefaultWildChar;}
	
protected:
	virtual	CompareResult			_CompareString_Like_IgnoreWildCharInMiddle( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics);
	virtual	CompareResult			_CompareString_Like_SupportWildCharInMiddle( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics);

									VCollator( const VCollator& inCollator) : fDialect( inCollator.fDialect), fWildChar( inCollator.fWildChar), fOptions( inCollator.fOptions)	{;}
									VCollator( DialectCode inDialect, CollatorOptions inOptions) : fDialect( inDialect), fWildChar( sDefaultWildChar), fOptions( inOptions)	{;}
	
	static	UniChar					sDefaultWildChar;
	
			DialectCode				fDialect;
			UniChar					fWildChar;
			CollatorOptions			fOptions;

};


#if !VERSION_LINUX

/************************************************************/
// system implementation
/************************************************************/

class XTOOLBOX_API VCollator_system : public VCollator
{
typedef  VCollator inherited;
public:
	virtual	sLONG					FindString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength);
	virtual	sLONG					ReversedFindString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength);
	virtual	CompareResult			CompareString (const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, bool inWithDiacritics);
	virtual bool					EqualString(const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, bool inWithDiacritics);

	virtual	VCollator_system*		Clone() const	{ return new VCollator_system(*this);}

	static	VCollator_system*		Create( DialectCode inDialect, VIntlMgr *inIntlMgr);

			XStringCompareImpl&		GetImpl()		{ return fImpl;}

	virtual	void					GetStringComparisonAlgorithmSignature( VString& outSignature) const;
			
private:
									VCollator_system( DialectCode inDialect, VIntlMgr *inIntlMgr);
									VCollator_system( const VCollator_system& inCollator);
								
			XStringCompareImpl		fImpl;
};

#endif

END_TOOLBOX_NAMESPACE


/************************************************************/
// icu
/************************************************************/




#if USE_ICU

namespace xbox_icu
{
	class Locale;
	class Collator;
	class CollationElementIterator;
}

struct UStringSearch;
struct UCollationElements;

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VICUCollator : public VCollator
{
	typedef  VCollator inherited;

public:
	virtual ~VICUCollator();
	virtual CompareResult				CompareString (const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, bool inWithDiacritics);
	virtual bool						EqualString(const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, bool inWithDiacritics);

	virtual	sLONG						FindString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength);
	virtual	sLONG						ReversedFindString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength);
	virtual	bool						BeginsWithString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength);
	virtual	bool						EndsWithString( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, sLONG *outFoundLength);
	
	static	VICUCollator*				Create( DialectCode inDialect, const xbox_icu::Locale *inLocale, CollatorOptions inOptions);
	virtual VICUCollator*				Clone() const	{ return new VICUCollator(*this);}

	virtual	void						SetWildChar( UniChar inWildChar);

	static	void						GetLocalesHavingCollator( const xbox_icu::Locale& inDisplayNameLocale, std::vector<const char*>& outLocales, std::vector<VString>& outCollatorDisplayNames);

			xbox_icu::Locale*			GetLocale() const	{ return fLocale;}

	virtual	void						GetStringComparisonAlgorithmSignature( VString& outSignature) const;			
private:
										VICUCollator( const VICUCollator& inCollator);
										VICUCollator( DialectCode inDialect, xbox_icu::Locale *inLocale, xbox_icu::Collator *inPrimary, xbox_icu::Collator *inTertiary, UCollationElements *inTextElements, UCollationElements *inPatternElements, CollatorOptions inOptions);

	virtual	CompareResult				_CompareString_Like_SupportWildCharInMiddle( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics);
			CompareResult				_CompareString_LikeNoDiac_primary( const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2);
			CompareResult				_CompareString_LikeNoDiac_secondary( const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2);
			CompareResult				_CompareString_LikeDiac( const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2);
			CompareResult				_BeginsWithString_NoDiac( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize);
	
			UStringSearch*				_InitSearch( const UniChar* inText, sLONG inTextSize, const UniChar* inPattern, sLONG inPatternSize, bool inWithDiacritics, void *outStatus);
			void						_UpdateWildCharPrimaryKey();
	
			xbox_icu::Locale*				fLocale;
			xbox_icu::Collator*				fPrimaryCollator;
			xbox_icu::Collator*				fTertiaryCollator;
			
			UCollationElements*			fTextElements;
			UCollationElements*			fPatternElements;
			sLONG						fElementKeyMask;
	
			UStringSearch*				fSearchWithDiacritics;	// allocated at first use and kept.
			UStringSearch*				fSearchWithoutDiacritics;	// allocated at first use and kept.
			sLONG						fWildCharKey;	// primary key for wild char using fTextElements

			bool						fUseOptimizedSort;
};

END_TOOLBOX_NAMESPACE

#endif	// ICU

#endif
