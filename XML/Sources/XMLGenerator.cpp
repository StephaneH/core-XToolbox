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
#include "VXMLPrecompiled.h"

#include "XMLGenerator.h"

BEGIN_TOOLBOX_NAMESPACE

AXMLGenericElement::AXMLGenericElement()
{
	fNext = NULL;
	fPrev = NULL;
	fParent = NULL;
	fFirstChild = NULL;
	fLastChild = NULL;
	fLevel = 0;
}

AXMLGenericElement::~AXMLGenericElement()
{
	Remove();
}



void AXMLGenericElement::Remove()
{
	AXMLGenericElement	*elem;
	AXMLGenericElement	*next;

	Detach();

	elem = fFirstChild;
	while (elem)
	{
		next = elem->fNext;
		delete elem;
		elem = next;
	}
}

void AXMLGenericElement::Attach(AXMLGenericElement *pNewParent)
{
	Detach();
	fParent = pNewParent;
	fLevel = pNewParent->fLevel + 1;

	if (!pNewParent->fFirstChild)
		pNewParent->fFirstChild = this;
	if (pNewParent->fLastChild)
		pNewParent->fLastChild->fNext = this;
	pNewParent->fLastChild = this;
}

void AXMLGenericElement::Detach()
{
	if (fParent)
	{
		if (fParent->fFirstChild == this)
		{
			fParent->fFirstChild = fNext;
		}
	}

	if (fNext)
		fNext->fPrev = fPrev;
	if (fPrev)
		fPrev->fNext = fNext;
}

void AXMLGenericElement::SetIndent(VString &pDestination) const
{
	long	i;

#if WINVER
	pDestination.AppendCString("\r\n");
#else
	pDestination.AppendCString("\n");
#endif
	for(i = 1;i <= fLevel;i++)
		pDestination.AppendCString("\t");
}



AXMLAttribute::AXMLAttribute()
{
	fNext = NULL;
	fPrev = NULL;

}

AXMLAttribute::~AXMLAttribute()
{
	
}

void AXMLAttribute::Set(const VString &pAttributeName,const VString &pAttributeValue)
{
	fName.FromString(pAttributeName);
	fValue.FromString(pAttributeValue);
	
}

void AXMLAttribute::GetRawData(VString &pDestination) const
{
	pDestination.Clear();
	AppendRawData(pDestination);
}

void AXMLAttribute::AppendRawData(VString &pDestination) const
{
	pDestination.AppendString(fName);
	pDestination.AppendCString("=\"");
	pDestination.AppendString(fValue);
	pDestination.AppendCString("\"");
}

AXMLAttribute* AXMLAttribute::GetNext()
{
	return fNext;	
}


AXMLElement::AXMLElement()
{
	fFirstAttribute = NULL;
	fLastAttribute = NULL;
}

AXMLElement::~AXMLElement()
{
	AXMLAttribute	*elem;
	AXMLAttribute	*next;

	elem = fFirstAttribute;
	while (elem)
	{
		next = elem->GetNext();
		delete elem;
		elem = next;
	}
}


void AXMLElement::SetName(const VString &pElementname)
{
	fElementName.FromString(pElementname);
}

AXMLElement* AXMLElement::AddChild(const VString &pChildName)
{
	AXMLElement	*child = new AXMLElement();

	child->SetName(pChildName);
	child->fParent = this;
	child->fLevel = fLevel + 1;

	if (!fFirstChild)
		fFirstChild = child;
	if (fLastChild)
		fLastChild->fNext = child;
	fLastChild = child;

	return child;
}

void AXMLElement::AddChild(AXMLElement *pNewChild)
{
	// dechainage
	pNewChild->Detach();

	// chainage
	pNewChild->fParent = this;
	pNewChild->fLevel = fLevel + 1;

	if (!fFirstChild)
		fFirstChild = pNewChild;
	if (fLastChild)
		fLastChild->fNext = pNewChild;
	fLastChild = pNewChild;
}

void AXMLElement::AddAttribute(const VString &pAttributeName,const VString &pAttributeValue)
{
	AXMLAttribute	*attr;

	attr = new AXMLAttribute();
	if (attr)
	{
		attr->Set(pAttributeName,pAttributeValue);
		if (!fFirstAttribute)
			fFirstAttribute = attr;
		if (fLastAttribute)
			fLastAttribute->fNext = attr;
		fLastAttribute = attr;
	}
}

void AXMLElement::AddText(const VString &pText)
{
	AXMLText	*child = new AXMLText();

	child->SetText(pText);
	child->fParent = this;
	child->fLevel = fLevel+1;

	if (!fFirstChild)
		fFirstChild = child;
	if (fLastChild)
		fLastChild->fNext = child;
	fLastChild = child;	
}
void AXMLElement::AddText(const char* pText)
{
	VString text;
	text.FromCString(pText);
	AddText(text);
}

void AXMLElement::AddCDATA(const VString &pCData)
{
	AXMLCData	*child = new AXMLCData();

	child->SetData(pCData);
	child->fParent = this;
	child->fLevel = fLevel+1;

	if (!fFirstChild)
		fFirstChild = child;
	if (fLastChild)
		fLastChild->fNext = child;
	fLastChild = child;	
}
void AXMLElement::AddCDATA(const char* pCData)
{
	VString cdata;
	cdata.FromCString(pCData);
	AddCDATA(cdata);
}


void AXMLElement::AddDOCTYPE(const VString &pDOCTYPE)
{
	AXMLDOCTYPE	*child = new AXMLDOCTYPE();

	child->SetData(pDOCTYPE);
	child->fParent = this;
	child->fLevel = fLevel + 1;

	if (!fFirstChild)
		fFirstChild = child;
	if (fLastChild)
		fLastChild->fNext = child;
	fLastChild = child;	
}
void AXMLElement::AddComment(const VString &pComment)
{
	AXMLComment	*child = new AXMLComment();

	child->SetComment(pComment);
	child->fParent = this;
	child->fLevel = fLevel + 1;

	if (!fFirstChild)
		fFirstChild = child;
	if (fLastChild)
		fLastChild->fNext = child;
	fLastChild = child;	
}

void AXMLElement::AddComment(const char* pComment)
{
	VString comment;
	comment.FromCString(pComment);
	AddComment(comment);
}

void AXMLElement::GetRawData(VString &pDestination,bool pIndented) const
{
	pDestination.Clear();
	AppendRawData(pDestination,pIndented);
}

void AXMLElement::AppendRawData(VString &pDestination,bool pIndented) const
{
	AXMLGenericElement		*elem;
	AXMLAttribute			*attr;

	if (pIndented)
		SetIndent(pDestination);

	pDestination.AppendCString("<");
	pDestination.AppendString(fElementName);
	
	// attributs
	attr = fFirstAttribute;
	while(attr)
	{
		pDestination.AppendCString(" ");
		attr->AppendRawData(pDestination);
		attr = attr->fNext;
	}
	

	if (!fFirstChild)
	{	
		pDestination.AppendCString(" />");
	}
	else
	{
		pDestination.AppendCString(">");

		// enfants
		elem = Get_FirstChild();
		while (elem)
		{
			elem->AppendRawData(pDestination,pIndented);
			elem = elem->GetNext();
		}
		
		if (pIndented)
			SetIndent(pDestination);
		pDestination.AppendCString("</");
		pDestination.AppendString(fElementName);
		pDestination.AppendCString(">");
	}

}

void AXMLElement::GetAttribute(const VString &pAttributename,VString &pValue) {
	AXMLAttribute	*attribute = NULL;
	attribute = fFirstAttribute;
	while (attribute)
	{
		if (attribute->GetName() == pAttributename) break;
		attribute = attribute->GetNext();
	}
	if (attribute) pValue.FromString(attribute->GetValue());
	else pValue.Clear();
}

AXMLElement* AXMLElement::GetFirstChildElement() {
	AXMLGenericElement	*elem;
	elem = Get_FirstChild();
	if (elem) {
		while (!elem->IsAXML()) {
			elem = elem->GetNext();
			if (!elem) break;
		}
	}
	return (AXMLElement*)elem;
}
AXMLElement* AXMLElement::GetNextElement() {
	AXMLGenericElement	*elem;
	elem = GetNext();
	if (elem) {
		while (!elem->IsAXML()) {
			elem = elem->GetNext();
			if (!elem) break;
		}
	}
	return (AXMLElement*)elem;

}
AXMLElement* AXMLElement::GetElement(const VString &pElementname) {
	AXMLElement	*elem = NULL;
	elem = GetFirstChildElement();
	while (elem)
	{	
		if (elem->GetName() == pElementname) break;
		elem = elem->GetNextElement();
	}
	return elem;
}

AXMLElement* AXMLElement::GetNextElement(AXMLElement* pAfterElem) {
	AXMLElement	*elem = NULL;
	elem = GetFirstChildElement();
	if (pAfterElem) {
		while (elem)
		{
			if (elem == pAfterElem) break;
			elem = elem->GetNextElement();
		}
		if (elem) elem = elem->GetNextElement();
	}
	return elem;
}
AXMLElement* AXMLElement::GetNextElement(AXMLElement* pAfterElem,const VString &pElementname) {
	AXMLElement	*elem = NULL;
	elem = GetFirstChildElement();
	if (pAfterElem) {
		while (elem)
		{
			if (elem == pAfterElem) break;
			elem = elem->GetNextElement();
		}
		if (elem) elem = elem->GetNextElement();
	}
	while (elem)
	{
		if (elem->GetName() == pElementname) break;
		elem = elem->GetNextElement();
	}
	return elem;
}

void AXMLElement::SetName(sWORD pStrId, sWORD pElementRef) {
	VString elem;
	//gResFile->GetString(elem,pStrId,pElementRef);
	SetName(elem);
}
void AXMLElement::AddAttribute(sWORD pStrId,sWORD pAttributeRef,const VString &pAttributeValue) {
	VString attribute;
	//gResFile->GetString(attribute,pStrId,pAttributeRef);
	AddAttribute(attribute,pAttributeValue);
}
void AXMLElement::AddAttribute(const char* pAttributeName,const char* pAttributeValue) {
	VString attribute;
	VString value;
	attribute.FromCString(pAttributeName);
	value.FromCString(pAttributeValue);
	AddAttribute(attribute,value);
}
AXMLElement* AXMLElement::AddChild(sWORD pStrId,sWORD pChildRef) {

	VString		childname;
	//gResFile->GetString(childname,pStrId,pChildRef);
	return AddChild(childname);
}

AXMLElement* AXMLElement::AddChild(const char* pChildName) {

	VString		childname;
	childname.FromCString(pChildName);
	return AddChild(childname);
}

AXMLElement* AXMLElement::GetElement(sWORD pStrId, sWORD pElementRef) {
	AXMLElement	*elem = NULL;
	VString		elemname;
	//gResFile->GetString(elemname,pStrId,pElementRef);
	return GetElement(elemname);
}

AXMLElement* AXMLElement::GetNextElement(AXMLElement* pAfterElem,sWORD pStrId, sWORD pElementRef) {
	AXMLElement	*elem = NULL;
	VString		elemname;
	//gResFile->GetString(elemname,pStrId,pElementRef);
	return GetNextElement(pAfterElem,elemname);
}

void AXMLElement::GetAttribute(sWORD pStrId, sWORD pElementRef,VString &pValue) {
	VString	Attributename;
	//gResFile->GetString(Attributename,pStrId,pElementRef);
	GetAttribute(Attributename, pValue);
}

void AXMLElement::GetData( VString &pCData)
{
	AXMLGenericElement	*elem;
	VString			vCData;

	vCData.Clear();
	pCData.Clear();
	elem = Get_FirstChild();
	while (elem) {
		elem->GetData(vCData);
		pCData.AppendString(vCData);
//		pCData.AppendChar((uBYTE)0x0D);
		elem = elem->GetNext();
	}
}

AXMLText::AXMLText()
{

}

AXMLText::~AXMLText()
{

}

void AXMLText::GetData( VString &pData)
{
	pData.FromString(fText);
}


void AXMLText::SetText(const VString &pText)
{
	fText.FromString(pText);
}

void AXMLText::GetRawData(VString &pDestination,bool pIndented) const
{
	pDestination.Clear();
	AppendRawData(pDestination,pIndented);
}

void AXMLText::AppendRawData(VString &pDestination,bool pIndented) const
{
	if (pIndented)
		SetIndent(pDestination);
	pDestination.AppendString(fText);
}

AXMLCData::AXMLCData()
{

}

AXMLCData::~AXMLCData()
{

}

void AXMLCData::GetData( VString &pData)
{
	pData.FromString(fCData);
}

void AXMLCData::SetData(const VString &pData)
{
	fCData.FromString(pData);
}
void AXMLCData::GetRawData(VString &pDestination,bool pIndented) const
{
	pDestination.Clear();
	AppendRawData(pDestination,pIndented);
}


void AXMLCData::AppendRawData(VString &pDestination,bool pIndented) const
{
	if (pIndented)
		SetIndent(pDestination);
	pDestination.AppendCString("<![CDATA[");
	pDestination.AppendString(fCData);
	pDestination.AppendCString("]]>");
}

AXMLDOCTYPE::AXMLDOCTYPE()
{

}

AXMLDOCTYPE::~AXMLDOCTYPE()
{

}

void AXMLDOCTYPE::SetData(const VString &pData)
{
	fDOCTYPE.FromString(pData);
}


void AXMLDOCTYPE::GetRawData(VString &pDestination,bool pIndented) const
{
	pDestination.Clear();
	AppendRawData(pDestination,pIndented);

}

void AXMLDOCTYPE::AppendRawData(VString &pDestination,bool pIndented) const
{

	if (pIndented)
		SetIndent(pDestination);
	pDestination.AppendCString("<!DOCTYPE ");
	pDestination.AppendString(fDOCTYPE);
	pDestination.AppendCString(">");
}

AXMLComment::AXMLComment()
{

}

AXMLComment::~AXMLComment()
{

}

void AXMLComment::SetComment(const VString &pComment)
{
	fComment.FromString(pComment);
}

void AXMLComment::GetRawData(VString &pDestination,bool pIndented) const
{
	pDestination.Clear();
	AppendRawData(pDestination,pIndented);
}


void AXMLComment::AppendRawData(VString &pDestination,bool pIndented) const
{
	if (pIndented)
		SetIndent(pDestination);
	pDestination.AppendCString("<!--");
	pDestination.AppendString(fComment);
	pDestination.AppendCString("-->");
}

AXMLDocument::AXMLDocument( const VString& inVersion, const VString& inEncoding) : AXMLElement()
{
	fVersion = inVersion;
	fEncoding = inEncoding;
}

AXMLDocument::~AXMLDocument()
{

}

void AXMLDocument::GetRawData(VString &pDestination,bool pIndented) const
{
	pDestination.Clear();
	AppendRawData(pDestination,pIndented);
}

void AXMLDocument::AppendRawData(VString &pDestination,bool pIndented) const
{
	AXMLGenericElement		*elem;

	pDestination.AppendCString("<?xml version=\"");
	pDestination.AppendString(fVersion);
	pDestination.AppendCString("\" encoding=\"");
	pDestination.AppendString(fEncoding);
	pDestination.AppendCString("\"?>");

	elem = Get_FirstChild();
	while (elem)
	{
		elem->AppendRawData(pDestination,pIndented);
//		pDestination.AppendCString((uBYTE*)pDestination.LockAndGetCPointer());
//		pDestination.Unlock();
		elem = elem->GetNext();
	}
}


#if WITH_VMemoryMgr

VError AXMLDocument::WriteToHandle( VHandle *pHandle,sLONG pParam) const
{
	VString	XmlText;
	GetRawData( XmlText, true);
	
	VStringConvertBuffer buffer( XmlText, VTextConverters::Get()->GetCharSetFromName( fEncoding));

//	sLONG	inMaxDestBytes = XmlText.GetSpace();

	if (VMemory::SetHandleSize( *pHandle, buffer.GetSize()) == VE_OK)
	{
		void *inDestination = VMemory::LockHandle( *pHandle);
		::memmove( inDestination, buffer.GetCPointer(), buffer.GetSize());
		VMemory::UnlockHandle( *pHandle);
	}

	return VE_OK;
}
#endif

VError AXMLDocument::WriteToStream( VStream *pStream,sLONG pParam ) const
{
	VString	XmlText;
	GetRawData( XmlText, true);

	VStringConvertBuffer buffer( XmlText, VTextConverters::Get()->GetCharSetFromName( fEncoding));
	VError result = pStream->PutData( buffer.GetCPointer(), buffer.GetSize());

	return result;

}

END_TOOLBOX_NAMESPACE
