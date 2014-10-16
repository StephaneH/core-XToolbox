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
#ifndef __IXMLDomWrapper__
#define __IXMLDomWrapper__

#include "Kernel/Sources/VKernelTypes.h"
#include "IXMLHandler.h"

BEGIN_TOOLBOX_NAMESPACE

class VXMLDOMNode_opaque;
typedef VXMLDOMNode_opaque	*VXMLDOMNodeRef;

class VXMLDOMDocument_opaque;
typedef VXMLDOMDocument_opaque	*VXMLDOMDocumentRef;


class VXMLDOMElement_opaque;
typedef VXMLDOMElement_opaque	*VXMLDOMElementRef;

class VXMLMemBufFormatTarget_opaque;

class XTOOLBOX_API IXMLDomWrapper
{
public:
	// parse a xerces DOM Node
	static bool	Parse( VXMLDOMNodeRef inNode, IXMLHandler *inHandler);


	// serialize the specified DOM tree to a blob
	static bool	SerializeToBlob( VXMLDOMNodeRef inNode, 
							     VBlob *inBlob, VIndex inOffset, VSize& outSize, bool inPrettyPrint = true);

	// serialize the specified DOM tree to the specified file stream
	static bool SerializeToFile( VXMLDOMNodeRef inNode, VFileStream& ioFileStream, VSize& outSize, bool inPrettyPrint = true);

	// write the specified DOM tree to the specified file 
	static bool WriteToFile( VXMLDOMNodeRef inNode, const VFile& inFile, bool inPrettyPrint = true);

	/** copy xercesc::MemBufFormatTarget object buffer to the destination blob 
		and consolidate line endings (optional)
	@param inBlobDst
		destination blob
	@param inOffset 
		offset in destination blob from which to start writing
	@param outSize
		count of bytes written to destination blob
	@param inBufFormatSrc
		opaque reference on xerces::MemBufFormatTarget * (source buffer)
	@param inEncodingSrc
		source buffer encoding
	@param inConsolidateLF
		true: line endings are consolidated - converted to platform-specific line endings (default)
	@remarks
		this method can be useful to convert not conforming line endings 
		to platform-specific line endings
	*/
	static bool CopyMemBufFormatTargetToBlob( VBlob *inBlobDst, VIndex inOffset, VSize& outSize, VXMLMemBufFormatTarget_opaque *inBufFormatSrc, const VString& inEncodingSrc = CVSTR("UTF-8"), bool inConsolidateLF = true);

	// clone the specified DOMDocument node 
	static VXMLDOMNodeRef  CloneNodeDocument( VXMLDOMNodeRef inNode);

	// clone the specified DOMDocument node 
	static VXMLDOMDocumentRef  CloneNodeDocument( VXMLDOMDocumentRef inNode);

	// get the node owner document
	static VXMLDOMNodeRef  GetNodeDocument( VXMLDOMNodeRef inNode);

	// get the document URL
	static bool GetDocumentURL(  VXMLDOMNodeRef inNode, VURL& outDocURL);

	// release the specified DOM tree
	static void	ReleaseNode( VXMLDOMNodeRef inNode);

	// add element node 
	static VXMLDOMElementRef  AddNodeElement( VXMLDOMElementRef inNodeParent, const VString& inName);

	// add element node with the specified value as its unique child text node
	static VXMLDOMElementRef  AddNodeElement( VXMLDOMElementRef inNodeParent, const VString& inName, Boolean inValue);

	// add element node with the specified value as its unique child text node
	static VXMLDOMElementRef  AddNodeElement( VXMLDOMElementRef inNodeParent, const VString& inName, sLONG inValue);

	// add element node with the specified value as its unique child text node
	static VXMLDOMElementRef  AddNodeElement( VXMLDOMElementRef inNodeParent, const VString& inName, Real inValue);

	// add element node with the specified value as its unique child text node
	static VXMLDOMElementRef  AddNodeElement( VXMLDOMElementRef inNodeParent, const VString& inName, const VTime& inValue, XMLStringOptions inOptions = XSO_Default);

	// add element node with the specified value as its unique child text node
	static VXMLDOMElementRef  AddNodeElement( VXMLDOMElementRef inNodeParent, const VString& inName, const VDuration& inValue, XMLStringOptions inOptions = XSO_Default);

	// add element node with the specified value as its unique child text node
	static VXMLDOMElementRef  AddNodeElement( VXMLDOMElementRef inNodeParent, const VString& inName, const VString& inValue);

	// add element node with the specified value as its unique child text node
	static VXMLDOMElementRef  AddNodeElement( VXMLDOMElementRef inNodeParent, const VString& inName, const VValueSingle * inValue);

	// set element node value (add a DOMText child element if not yet created)
	static void SetNodeElementValue( VXMLDOMElementRef inNodeParent, Boolean inValue);

	// set element node value (add a DOMText child element if not yet created)
	static void SetNodeElementValue( VXMLDOMElementRef inNodeParent, sLONG inValue);

	// set element node value (add a DOMText child element if not yet created)
	static void SetNodeElementValue( VXMLDOMElementRef inNodeParent, Real inValue);

	// set element node value (add a DOMText child element if not yet created)
	static void SetNodeElementValue( VXMLDOMElementRef inNodeParent, const VTime& inValue, XMLStringOptions inOptions = XSO_Default);

	// set element node value (add a DOMText child element if not yet created)
	static void SetNodeElementValue( VXMLDOMElementRef inNodeParent, const VDuration& inValue, XMLStringOptions inOptions = XSO_Default);

	// set element node value (add a DOMText child element if not yet created)
	static void SetNodeElementValue( VXMLDOMElementRef inNodeParent, const VString& inValue);


	//return node element value of the specified type
	static Boolean GetNodeElementValueBoolean( VXMLDOMElementRef inNodeParent);

	//return node element value of the specified type
	static sLONG GetNodeElementValueLong( VXMLDOMElementRef inNodeParent);

	//return node element value of the specified type
	static Real GetNodeElementValueReal( VXMLDOMElementRef inNodeParent);

	//return node element value of the specified type
	static bool GetNodeElementValueTime( VXMLDOMElementRef inNodeParent, VTime& outValue);

	//return node element value of the specified type
	static bool GetNodeElementValueDuration( VXMLDOMElementRef inNodeParent, VDuration& outValue);

	//return node element value of the specified type
	static void GetNodeElementValueString( VXMLDOMElementRef inNodeParent, VString& outValue);

#ifdef XERCES_3_0_1
	/** workaround for Xerces 3.0.1 bug (does not properly propagate default namespace URI) */
	static VXMLDOMElementRef CreateElement( VXMLDOMDocumentRef inDocument, VXMLDOMElementRef inNodeParent, const UniChar *inName);
#endif

	/** return element with the specified ID */
	static VXMLDOMElementRef FindNodeElementByID( VXMLDOMNodeRef inNode, const VString& inID);

	/** return root element  */
	static VXMLDOMElementRef GetRootNodeElement( VXMLDOMNodeRef inNode);

	/** set element attribute */
	static bool	SetNodeElementAttribute( VXMLDOMElementRef inNodeElement, const VString& inName, const VString& inValue, bool inRemoveIfValueEmpty = true, bool *outIsDirtyMapID = NULL);

};

END_TOOLBOX_NAMESPACE

#endif