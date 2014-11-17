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
#ifndef __XMLGenerator__
#define __XMLGenerator__


BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API AXMLGenericElement : public VObject
{
public:
								AXMLGenericElement();
	virtual						~AXMLGenericElement();

	virtual void				GetRawData(VString &pDestination,bool pIndented) const = 0;
	virtual void				AppendRawData(VString &pDestination,bool pIndented) const = 0;
	virtual	void				GetData( VString &pCData) { pCData.Clear();}
	virtual	Boolean				IsAXML()				{return false;}

			AXMLGenericElement*	Get_FirstChild() const	{return fFirstChild;}
			AXMLGenericElement*	Get_LastChild()			{return fLastChild;}
			AXMLGenericElement*	GetNext()				{return fNext;}
			AXMLGenericElement*	GetPrev()				{return fPrev;}
			AXMLGenericElement*	GetParent()				{return fParent;}

			void				Attach(AXMLGenericElement *pNewParent);	// l'element appartient a pNewParent
			void				Detach();
			void				SetIndent(VString &pDestination) const;

private:
	AXMLGenericElement		*fNext;
	AXMLGenericElement		*fPrev;
	AXMLGenericElement		*fParent;
	AXMLGenericElement		*fFirstChild;
	AXMLGenericElement		*fLastChild;
	long					fLevel;


private:
	void Remove();

	friend class AXMLElement;
};


class XTOOLBOX_API AXMLAttribute : public VObject
{
friend class AXMLElement;
public:
	AXMLAttribute();
	~AXMLAttribute();

	void Set(const VString &pAttributeName,const VString &pAttributeValue);
	void GetRawData(VString &pDestination) const;
	void AppendRawData(VString &pDestination) const;
	VString&		GetName() {return fName;};
	VString&		GetValue() {return fValue;};
	AXMLAttribute*	GetNext();

private:
	AXMLAttribute	*fNext;
	AXMLAttribute	*fPrev;
	VString			fName;
	VString			fValue;
};

class XTOOLBOX_API AXMLElement : public AXMLGenericElement
{
public:
	AXMLElement();
	virtual ~AXMLElement();

	Boolean					IsAXML() {return true;};
	VString&				GetName() {return fElementName;};
	virtual	void			SetName(const VString &pElementname);
	virtual	void			AddAttribute(const VString &pAttributeName,const VString &pAttributeValue); // ajout d'un attribut
	virtual	void			AddAttribute(const char* pAttributeName,const char* pAttributeValue); // ajout d'un attribut
	void					AddChild(AXMLElement *pNewChild);	// l'element pNewChild appartient a son parent
	virtual	AXMLElement*	AddChild(const VString &pChildName); // ajout d'un element de plus bas niveau
	virtual	AXMLElement*	AddChild(const char* pChildName);
	void					AddText(const VString &pText);	// ajout de texte
	void					AddText(const char* pText);
	void					AddCDATA(const VString &pCData);
	void					AddCDATA(const char* pText);
	void					AddDOCTYPE(const VString &pDOCTYPE);
	void					AddComment(const VString &pComment);
	void					AddComment(const char* pComment);
	void					SetChild(AXMLElement	*child);	// Ajouter un element dérivé
	AXMLElement*			GetFirstChildElement();
	AXMLElement*			GetElement(const VString &pElementname);
	AXMLElement*			GetNextElement();
	AXMLElement*			GetNextElement(AXMLElement* pAfterElem);
	AXMLElement*			GetNextElement(AXMLElement* pAfterElem,const VString &pElementname);
	void					GetAttribute(const VString &pAttributename,VString &pValue);
	void					GetData( VString &pCData);

// With resfile
	void					SetName(sWORD pStrId, sWORD pElementRef);
	void					AddAttribute(sWORD pStrId, sWORD pAttributeRef,const VString &pAttributeValue); // ajout d'un attribut
	AXMLElement*			AddChild(sWORD pStrId,sWORD pChildRef); // ajout d'un element de plus bas niveau
	AXMLElement*			GetElement(sWORD pStrId, sWORD pElementRef);
	AXMLElement*			GetNextElement(AXMLElement* pAfterElem,sWORD pStrId, sWORD pElementRef);
	void					GetAttribute(sWORD pStrId, sWORD pElementRef,VString &pValue);

	void			GetRawData(VString &pDestination,bool pIndented) const;
	void			AppendRawData(VString &pDestination,bool pIndented) const;

private:

	VString			fElementName;

	AXMLAttribute	*fFirstAttribute;
	AXMLAttribute	*fLastAttribute;
};

class XTOOLBOX_API AXMLText : public AXMLGenericElement
{
public:
	AXMLText();
	virtual ~AXMLText();

	void GetData( VString &pData);
	void SetText(const VString &pText); 
	void GetRawData(VString &pDestination,bool pIndented) const;
	void AppendRawData(VString &pDestination,bool pIndented) const;

private:

	VString			fText;
};

class XTOOLBOX_API AXMLCData : public AXMLGenericElement
{
public:
	AXMLCData();
	virtual ~AXMLCData();

	void SetData(const VString &pData); 
	void GetData( VString &pData); 
	void GetRawData(VString &pDestination,bool pIndented) const;
	void AppendRawData(VString &pDestination,bool pIndented) const;

private:

	VString			fCData;
};

class XTOOLBOX_API AXMLDOCTYPE : public AXMLGenericElement
{
public:
	AXMLDOCTYPE();
	virtual ~AXMLDOCTYPE();

	void SetData(const VString &pData); 
	void GetRawData(VString &pDestination,bool pIndented) const;
	void AppendRawData(VString &pDestination,bool pIndented) const;

private:

	VString			fDOCTYPE;
};
class XTOOLBOX_API AXMLComment : public AXMLGenericElement
{
public:
	AXMLComment();
	virtual ~AXMLComment();

	void SetComment(const VString &pComment);
	void GetRawData(VString &pDestination,bool pIndented) const;
	void AppendRawData(VString &pDestination,bool pIndented) const;

private:

	VString			fComment;
};



class XTOOLBOX_API AXMLDocument : public AXMLElement
{
public:
	AXMLDocument( const VString& inVersion, const VString& inEncoding);
	virtual ~AXMLDocument();

	void	GetRawData(VString &pDestination,bool pIndented) const;
	void	AppendRawData(VString &pDestination,bool pIndented) const;
#if WITH_VMemoryMgr
	VError	WriteToHandle( VHandle *pHandle,sLONG pParam = 0) const;
#endif
	VError	WriteToStream( VStream *pStream,sLONG pParam = 0) const;

private:
	VStr15			fVersion;
	VStr31			fEncoding;
};

END_TOOLBOX_NAMESPACE

#endif
