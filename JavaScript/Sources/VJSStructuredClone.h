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
#ifndef __VJS_STRUCTURED_CLONE__
#define __VJS_STRUCTURED_CLONE__

#include "VJSClass.h"
#include "VJSValue.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VJSStructuredClone : public XBOX::IRefCountable
{
public:

	// Return NULL if unable to apply structured clone algorithm (DATA_CLONE_ERR).
	
	static VJSStructuredClone	*RetainClone (const XBOX::VJSValue& inValue);

	static VJSStructuredClone	*RetainCloneForVValueSingle( const XBOX::VValueSingle& inValue);

	static VJSStructuredClone	*RetainCloneForVBagArray( const XBOX::VBagArray& inBagArray, bool inUniqueElementsAreNotArrays);
	
	static VJSStructuredClone	*RetainCloneForVValueBag( const XBOX::VValueBag& inBag, bool inUniqueElementsAreNotArrays);

	// Make a JavaScript value in the given context. The created value is "undefined" if an error occured. 
	
	XBOX::VJSValue				MakeValue (XBOX::VJSContext inContext);

private:

	enum {

		// Primitive values.

		eNODE_UNDEFINED,
		eNODE_NULL,
		eNODE_BOOLEAN,
		eNODE_NUMBER,
		eNODE_STRING,

		// Primitive objects.

		eNODE_BOOLEAN_OBJECT,
		eNODE_NUMBER_OBJECT,
		eNODE_STRING_OBJECT,
		eNODE_DATE_OBJECT,
		eNODE_REG_EXP_OBJECT,		
		
		// A serializable object implements a serialize() method to convert its full state into a JSON string.
		// It also has a constructorName attribute to use to re-create it from the JSON string.
		
		eNODE_SERIALIZABLE,

		// Object or array.

		eNODE_OBJECT,
		eNODE_ARRAY,

		// Reference to an object or array.

		eNODE_REFERENCE,
		
		// Native objects.

//**	TODO: Decide which native C++ objects to clone.

	};

	struct SNode {

		sLONG					fType;

		XBOX::VString			fName;				// "Root" node has empty name.
	
		union {

			bool				fBoolean;			// Both primitive and object Boolean.
			
			Real				fNumber;			// Both primitive and object Number, also store Date in UNIX time.
			
			XBOX::VString		*fString;			// Both primitive and object String. also store RegExp.
			
			struct {
				
				XBOX::VString	*fConstructorName;	// Constructor name to use.
				XBOX::VString	*fJSON;				// JSON argument (full state of the cloned object).
				
			} fSerializationData;
			
			struct SNode		*fFirstChild;		// First child of object or Array.
			
			struct SNode		*fReference;		// Reference to an object or an Array.
			
		} fValue;

		struct SNode			*fNextSibling;		// Not used by "root" node.

	};
	
	struct SEntry {
		SEntry(const VJSContext& inContext) : fValue(inContext) {;}
		XBOX::VJSValue			fValue;
		SNode					*fNode;

	};
	
	SNode						*fRoot;

								VJSStructuredClone ();
	virtual						~VJSStructuredClone ();

	// Create a node from a value, return NULL if erroneous.

	static SNode				*_ValueToNode (XBOX::VJSValue inValue, std::map<XBOX::VJSValue, SNode *> *ioAlreadyCloned, std::list<SEntry> *ioToDoList);

	static SNode				*_VValueSingleToNode( const XBOX::VValueSingle& inValue);

	static SNode				*_VBagArrayToNode( const XBOX::VBagArray& inBagArray, bool inUniqueElementsAreNotArrays);

	static SNode				*_VValueBagToNode( const XBOX::VValueBag& inBag, bool inUniqueElementsAreNotArrays);

	// Create a value from a node.

	static XBOX::VJSValue		_NodeToValue (XBOX::VJSContext inContext, SNode *inNode, std::map<SNode *, XBOX::VJSValue> *ioAlreadyCreated, std::list<SEntry> *ioToDoList);
	
	// Return true if VJSValue is serializable (has a constructorName attribute and a serialize() method);
	
	static bool					_IsSerializable (XBOX::VJSValue inValue);

	// Call a constructor with a single argument, return constructed object or undefined if failed.

	static XBOX::VJSValue		_ConstructObject (XBOX::VJSContext inContext, const XBOX::VString &inConstructorName, XBOX::VJSValue inArgument);

	// Free a node and all its children (but not its sibling(s)).

	static void					_FreeNode (SNode *inNode);
};

END_TOOLBOX_NAMESPACE

#endif