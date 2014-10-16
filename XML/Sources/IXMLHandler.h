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
#ifndef __IXMLHandler__
#define __IXMLHandler__

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API IXMLHandler : public IRefCountable
{
public:
							IXMLHandler():fEntityLocalFolder( NULL)	{};
	virtual					~IXMLHandler();

	virtual	IXMLHandler*	StartElement( const VString& inElementName);
	virtual	void			EndElement(const VString& inElementName);
	virtual	void			SetAttribute( const VString& inName, const VString& inValue);
    virtual	void			SetText( const VString& inText);
	virtual	XBOX::VFile*	ResolveEntity( const VString& inSystemID);

			/** return true if character is valid XML 1.1
			(some control & special chars are not supported like [#x1-#x8], [#xB-#xC], [#xE-#x1F], [#x7F-#x84], [#x86-#x9F] - cf W3C) */
	static	bool			IsXMLChar(UniChar inChar);

	/**
	* Receive notification of a processing instruction (like xml-stylesheet tag)
	*
	* @param inTarget The processing instruction target. (for instance 'xml-stylesheet')
	* @param inData The processing instruction data, or empty if 
	*             none is supplied. (for instance 'href="mystyle.css" type="text/css"')
	* @remarks
	*	if tag is 'xml-stylesheet', 
	*	internally call user handler 
	*		IXMLHandler::StartStyleSheet, 
	*		IXMLHandler::SetStyleSheetAttribute and 
	*		IXMLHandler::EndStyleSheet
	*/
    virtual void			processingInstruction(const VString& inTarget, const VString& inData);

	/** start style sheet element */
	virtual	void			StartStyleSheet();
	/** set style sheet attribute */
	virtual	void			SetStyleSheetAttribute( const VString& inName, const VString& inValue);
	/** end style sheet element */
	virtual void			EndStyleSheet();

			void			SetEntitiesLocalFolder( const VString& inSystemIDBase, VFolder *inFolder);
protected:
	static bool WCharIsSpace( UniChar c );

private:
			VString			fEntityBase;			// base url for locally cached entities
			VFolder*		fEntityLocalFolder;		// base folder for locally cached entities
};



class XTOOLBOX_API VNullXMLHandler: public VObject, public IXMLHandler
{
public:
	virtual	IXMLHandler*	StartElement( const VString& inElementName);
	virtual	void			SetAttribute( const VString& inName, const VString& inValue);
	virtual	void			SetText( const VString& inText);
};

END_TOOLBOX_NAMESPACE

#endif