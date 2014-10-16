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
#include "VKernelPrecompiled.h"
#include "VFile.h"
#include "VJSONTools.h"
#include "VJSONValue.h"

// for debugging leaks
sLONG	VJSONObject::sCount = 0;
sLONG	VJSONArray::sCount = 0;

BEGIN_TOOLBOX_NAMESPACE

class VJSONGraph : public VObject, public IRefCountable
{
public:
							VJSONGraph(): fCyclic( false), fParent( NULL)	{}
							~VJSONGraph()						{ ReleaseRefCountable( &fParent);}

			VJSONGraph*		GetParent() const					{ return fParent;}
			void			SetParent( VJSONGraph *inParent)	{ CopyRefCountable( &fParent, inParent);}
			
			bool			IsCyclic() const					{ return fCyclic;}
			void			SetCyclic()							{ fCyclic = true;}

	static	void			Connect( VJSONGraph** inRetainerGraph, VJSONGraph** inRetainedGraph);
	static	void			Connect( VJSONGraph** inRetainerGraph, const VJSONValue& inRetainedValue);
		
	static	bool			IsPossiblyCyclic( VJSONGraph *inGraph);
	static	void			CollectObjectReferences( VJSONObject *inObject, std::vector<VJSONObject*>& ioObjects, std::vector<VJSONArray*>& ioArrays, sLONG& ioReferencesTotal, bool *outCyclic);
	static	void			CollectArrayReferences( VJSONArray *inArray, std::vector<VJSONObject*>& ioObjects, std::vector<VJSONArray*>& ioArrays, sLONG& ioReferencesTotal, bool *outCyclic);

	static	void			ReleaseObjectDependenciesIfNecessary( VJSONObject *inObject);
	static	void			ReleaseArrayDependenciesIfNecessary( VJSONArray *inArray);
	static	void			ReleaseDependenciesIfNecessary( IRefCountable *inTarget, const std::vector<VJSONObject*>& inObjects, const std::vector<VJSONArray*>& inArrays, sLONG inReferencesTotal);

private:
	VJSONGraph( const VJSONGraph&);
	VJSONGraph& operator=( const VJSONGraph&);

			VJSONGraph*		fParent;
			sLONG			fID;
			bool			fCyclic;
			
};

END_TOOLBOX_NAMESPACE


/*
	static
*/
void VJSONGraph::Connect( VJSONGraph** inRetainerGraph, VJSONGraph** inRetainedGraph)
{
	if (inRetainedGraph == inRetainerGraph)
	{
		// connecting to ourselves makes a cycle
		if (*inRetainerGraph == NULL)
		{
			*inRetainerGraph = new VJSONGraph;
			(*inRetainerGraph)->SetCyclic();
		}
		else
		{
			VJSONGraph *topMostGraph = *inRetainerGraph;
			while( topMostGraph->GetParent() != NULL)
				topMostGraph = topMostGraph->GetParent();
			topMostGraph->SetCyclic();
		}
	}
	else if ( (*inRetainerGraph == NULL) && (*inRetainedGraph == NULL) )
	{
		// new graph
		*inRetainerGraph = new VJSONGraph;
		*inRetainedGraph = RetainRefCountable( *inRetainerGraph);
	}
	else if (*inRetainedGraph == NULL)
	{
		*inRetainedGraph = RetainRefCountable( *inRetainerGraph);
	}
	else if (*inRetainerGraph == NULL)
	{
		*inRetainerGraph = RetainRefCountable( *inRetainedGraph);
	}
	else
	{
		// connecting two graphs together
		
		VJSONGraph *topMostGraph = *inRetainerGraph;
		while( topMostGraph->GetParent() != NULL)
			topMostGraph = topMostGraph->GetParent();
		
		VJSONGraph *otherTopMostGraph = *inRetainedGraph;
		while( otherTopMostGraph->GetParent() != NULL)
			otherTopMostGraph = otherTopMostGraph->GetParent();
	
		if (topMostGraph == otherTopMostGraph)
		{
			// cycle detected
			topMostGraph->SetCyclic();
		}
		else
		{
			if (otherTopMostGraph->IsCyclic())
				topMostGraph->SetCyclic();
			otherTopMostGraph->SetParent( topMostGraph);
		}
	}
}

/*
	static
*/
void VJSONGraph::Connect( VJSONGraph** inRetainerGraph, const VJSONValue& inRetainedValue)
{
	if (inRetainedValue.IsObject())
		Connect( inRetainerGraph, inRetainedValue.GetObject()->GetGraph());
	else if (inRetainedValue.IsArray())
		Connect( inRetainerGraph, inRetainedValue.GetArray()->GetGraph());
}


/*
	static
*/
bool VJSONGraph::IsPossiblyCyclic( VJSONGraph *inGraph)
{
	if (inGraph == NULL)
		return false;

	VJSONGraph *topMostGraph = inGraph;
	while( topMostGraph->GetParent() != NULL)
		topMostGraph = topMostGraph->GetParent();
	
	return topMostGraph->IsCyclic();
}


/*
	static
*/
void VJSONGraph::CollectObjectReferences( VJSONObject *inObject, std::vector<VJSONObject*>& ioObjects, std::vector<VJSONArray*>& ioArrays, sLONG& ioReferencesTotal, bool *outCyclic)
{
	ioReferencesTotal += 1;

	if (std::find( ioObjects.begin(), ioObjects.end(), inObject) == ioObjects.end())
	{
		ioObjects.push_back( inObject);

		for( VJSONPropertyConstIterator i( inObject) ; i.IsValid() ; ++i)
		{
			const VJSONValue& value = i.GetValue();
			if (value.IsObject())
				CollectObjectReferences( value.GetObject(), ioObjects, ioArrays, ioReferencesTotal, outCyclic);
			else if (value.IsArray())
				CollectArrayReferences( value.GetArray(), ioObjects, ioArrays, ioReferencesTotal, outCyclic);
		}
	}
	else
	{
		*outCyclic = true;
	}
}


/*
	static
*/
void VJSONGraph::CollectArrayReferences( VJSONArray *inArray, std::vector<VJSONObject*>& ioObjects, std::vector<VJSONArray*>& ioArrays, sLONG& ioReferencesTotal, bool *outCyclic)
{
	ioReferencesTotal += 1;

	if (std::find( ioArrays.begin(), ioArrays.end(), inArray) == ioArrays.end())
	{
		ioArrays.push_back( inArray);

		size_t count = inArray->GetCount();
		for( size_t i = 0 ; i < count ; ++i)
		{
			const VJSONValue& value = (*inArray)[i];
			if (value.IsObject())
				CollectObjectReferences( value.GetObject(), ioObjects, ioArrays, ioReferencesTotal, outCyclic);
			else if (value.IsArray())
				CollectArrayReferences( value.GetArray(), ioObjects, ioArrays, ioReferencesTotal, outCyclic);
		}
	}
	else
	{
		*outCyclic = true;
	}
}


/*
	static
*/
void VJSONGraph::ReleaseDependenciesIfNecessary( IRefCountable *inTarget, const std::vector<VJSONObject*>& inObjects, const std::vector<VJSONArray*>& inArrays, sLONG inReferencesTotal)
{
	sLONG refCountObjectTotal = 0;
	for( std::vector<VJSONObject*>::const_iterator i = inObjects.begin() ; i != inObjects.end() ; ++i)
		refCountObjectTotal += (*i)->GetRefCount();

	sLONG refCountArrayTotal = 0;
	for( std::vector<VJSONArray*>::const_iterator i = inArrays.begin() ; i != inArrays.end() ; ++i)
		refCountArrayTotal += (*i)->GetRefCount();

	xbox_assert( inReferencesTotal <= refCountObjectTotal + refCountArrayTotal);
	
	// if number of references is equal after release of this inObject
	// that means some inObject children would leak.
	if (inReferencesTotal == refCountObjectTotal + refCountArrayTotal)
	{
		// leak caused by cyclic structure detected
		// need to cut all references

		for( std::vector<VJSONObject*>::const_iterator i = inObjects.begin() ; i != inObjects.end() ; ++i)
		{
			(*i)->Retain();
			ReleaseRefCountable( (*i)->GetGraph());
		}

		for( std::vector<VJSONArray*>::const_iterator i = inArrays.begin() ; i != inArrays.end() ; ++i)
		{
			(*i)->Retain();
			ReleaseRefCountable( (*i)->GetGraph());
		}

		for( std::vector<VJSONObject*>::const_iterator i = inObjects.begin() ; i != inObjects.end() ; ++i)
			(*i)->Clear();

		for( std::vector<VJSONArray*>::const_iterator i = inArrays.begin() ; i != inArrays.end() ; ++i)
			(*i)->Clear();

		#if VERSIONDEBUG
		for( std::vector<VJSONObject*>::const_iterator i = inObjects.begin() ; i != inObjects.end() ; ++i)
			xbox_assert( ((*i == inTarget) && (*i)->GetRefCount() == 2) || ((*i != inTarget) && (*i)->GetRefCount() == 1) );
		for( std::vector<VJSONArray*>::const_iterator i = inArrays.begin() ; i != inArrays.end() ; ++i)
			xbox_assert( ((*i == inTarget) && (*i)->GetRefCount() == 2) || ((*i != inTarget) && (*i)->GetRefCount() == 1) );
		#endif

		for( std::vector<VJSONObject*>::const_iterator i = inObjects.begin() ; i != inObjects.end() ; ++i)
			(*i)->Release();

		for( std::vector<VJSONArray*>::const_iterator i = inArrays.begin() ; i != inArrays.end() ; ++i)
			(*i)->Release();
	}
}


/*
	static
*/
void VJSONGraph::ReleaseObjectDependenciesIfNecessary( VJSONObject *inObject)
{
	std::vector<VJSONObject*> objects;
	std::vector<VJSONArray*> arrays;
	sLONG referencesTotal = 0;

	bool cyclic = false;
	CollectObjectReferences( inObject, objects, arrays, referencesTotal, &cyclic);
	
	if (cyclic)
	{
		ReleaseDependenciesIfNecessary( inObject, objects, arrays, referencesTotal);
	}
}


/*
	static
*/
void VJSONGraph::ReleaseArrayDependenciesIfNecessary( VJSONArray *inArray)
{
	std::vector<VJSONObject*> objects;
	std::vector<VJSONArray*> arrays;
	sLONG referencesTotal = 0;

	bool cyclic = false;
	CollectArrayReferences( inArray, objects, arrays, referencesTotal, &cyclic);
	
	if (cyclic)
	{
		ReleaseDependenciesIfNecessary( inArray, objects, arrays, referencesTotal);
	}
}


//==============================================================================



const VJSONValue		VJSONValue::sUndefined( JSON_undefined);
const VJSONValue		VJSONValue::sNull( JSON_null);
const VJSONValue		VJSONValue::sTrue( JSON_true);
const VJSONValue		VJSONValue::sFalse( JSON_false);

const VString VJSONValue::sUndefinedString( "undefined");
const VString VJSONValue::sNullString( "null");
const VString VJSONValue::sTrueString( "true");
const VString VJSONValue::sFalseString( "false");
const VString VJSONValue::sObjectString( "[object Object]");


VJSONValue::VJSONValue( JSONType inType)
: fType( inType)
{
	switch( fType)
	{
		case JSON_string:	fString.Init(); break;
		case JSON_number:	fNumber = 0; break;
		case JSON_array:	fArray = new VJSONArray; break;
		case JSON_object:	fObject = new VJSONObject; break;
		case JSON_date:		fTimeStamp = 0; break;
		case JSON_null:
		case JSON_true:
		case JSON_false:
		case JSON_undefined:
			break;
	}
}


VJSONValue::VJSONValue( VJSONObject *inObject)
: fType( (inObject == NULL) ? JSON_undefined : JSON_object)
, fObject( RetainRefCountable( inObject))
{
}


VJSONValue::VJSONValue( VJSONArray *inArray)
: fType( (inArray == NULL) ? JSON_undefined : JSON_array)
, fArray( RetainRefCountable( inArray))
{
}


VJSONValue::VJSONValue( const VString& inString)
: fType( inString.IsNull() ? JSON_null : JSON_string)
{
	if (fType == JSON_string)
		fString.InitWithString( inString);
}


VJSONValue::VJSONValue( const VInlineString& inString)
: fType( JSON_string)
{
	fString.InitWithString( inString);
}


VJSONValue::VJSONValue( double inNumber)
: fType( JSON_number)
, fNumber( inNumber)
{
}


VJSONValue::VJSONValue(const VTime& inTime)
: fType(inTime.IsNull() ? JSON_null : JSON_date)
{
	if (fType == JSON_date)
		fTimeStamp = inTime.GetMilliseconds();
}



VJSONValue::~VJSONValue()
{
	_Dispose();
}


VJSONValue::VJSONValue( const VJSONValue& inOther)
{
	_CopyFrom( inOther);
}


void VJSONValue::_Dispose()
{
	switch( fType)
	{
		case JSON_string:	fString.Dispose(); break;
		case JSON_array:	ReleaseRefCountable( &fArray); break;
		case JSON_object:	ReleaseRefCountable( &fObject); break;
		case JSON_number:
		case JSON_null:
		case JSON_true:
		case JSON_false:
		case JSON_undefined:
			break;
	}
}


void VJSONValue::_CopyFrom( const VJSONValue& inOther)
{
	fType = inOther.fType;
	switch( fType)
	{
		case JSON_string:	fString.InitWithString( inOther.fString); break;
		case JSON_number:	fNumber = inOther.fNumber; break;
		case JSON_array:	fArray = RetainRefCountable( inOther.fArray); break;
		case JSON_object:	fObject = RetainRefCountable( inOther.fObject); break;
		case JSON_date:		fTimeStamp = inOther.fTimeStamp; break;
		case JSON_null:
		case JSON_true:
		case JSON_false:
		case JSON_undefined:
			break;
	}
}


VJSONValue& VJSONValue::operator=( const VJSONValue& inOther)
{
	if (this != &inOther)
	{
		_Dispose();
		_CopyFrom( inOther);
	}
	return *this;
}


void VJSONValue::SetNull()
{
	_Dispose();
	fType = JSON_null;
}


void VJSONValue::SetUndefined()
{
	_Dispose();
	fType = JSON_undefined;
}


void VJSONValue::SetString( const VString& inString)
{
	if (inString.IsNull())
	{
		_Dispose();
		fType = JSON_null;
	}
	else if (fType == JSON_string)
	{
		fString.FromString( inString);
	}
	else
	{
		_Dispose();
		fType = JSON_string;
		fString.InitWithString( inString);
	}
}


void VJSONValue::SetString( const VInlineString& inString)
{
	if (fType == JSON_string)
	{
		fString.FromString( inString);
	}
	else
	{
		_Dispose();
		fType = JSON_string;
		fString.InitWithString( inString);
	}
}


void VJSONValue::SetNumber( double inNumber)
{
	_Dispose();
	fType = JSON_number;
	fNumber = inNumber;
}


void VJSONValue::SetBool( bool inValue)
{
	_Dispose();
	fType = inValue ? JSON_true : JSON_false;
}


void VJSONValue::SetObject( VJSONObject *inObject)
{
	if (inObject == NULL)
	{
		_Dispose();
		fType = JSON_undefined;
	}
	else if (fType == JSON_object)
	{
		CopyRefCountable( &fObject, inObject);
	}
	else
	{
		_Dispose();
		fType = JSON_object;
		fObject = RetainRefCountable( inObject);
	}
}


void VJSONValue::SetArray( VJSONArray *inArray)
{
	if (inArray == NULL)
	{
		_Dispose();
		fType = JSON_undefined;
	}
	else if (fType == JSON_array)
	{
		CopyRefCountable( &fArray, inArray);
	}
	else
	{
		_Dispose();
		fType = JSON_array;
		fArray = RetainRefCountable( inArray);
	}
}


void VJSONValue::SetTime(const VTime& inTime)
{
	_Dispose();
	if (inTime.IsNull())
		fType = JSON_null;
	else
	{
		fType = JSON_date;
		fTimeStamp = inTime.GetMilliseconds();
	}
}


VJSONValue VJSONValue::GetProperty( const VString& inName) const
{
	switch( fType)
	{
		case JSON_object:
			return fObject->GetProperty( inName);
		
		case JSON_string:	// could support 'length'
		case JSON_array:	// could support 'length'
		default:
			return sUndefined;

	}
}

void VJSONValue::SetProperty( const VString& inName, const VJSONValue& inValue)
{
	if (testAssert( fType == JSON_object))
	{
		fObject->SetProperty( inName, inValue);
	}
}


double VJSONValue::GetNumber() const
{
	double r;
	switch( fType)
	{
		case JSON_true:
			{
				r = 1;
				break;
			}
			
		case JSON_string:
			{
				VString s( fString);
				r = s.GetReal();
				break;
			}
			
		case JSON_number:
			{
				r = fNumber;
				break;
			}

		case JSON_date:
		{
				r = fTimeStamp;
				break;
		}

		default:
			{
				r = 0;
				break;
			}
	}
	return r;
}


bool VJSONValue::GetBool() const
{
	bool r;
	switch( fType)
	{
		case JSON_true:
		case JSON_array:
		case JSON_object:
			{
				r = true;
				break;
			}
			
		case JSON_string:
			{
				r = !fString.IsEmpty();
				break;
			}
			
		case JSON_number:
			{
				r = fNumber != 0;
				break;
			}

		case JSON_date:
		{
				r = fTimeStamp != 0;
				break;
		}

		default:
			{
				r = false;
				break;
			}
	}
	return r;
}


VError VJSONValue::GetString( VString& outString) const
{
	VError err = VE_OK;
	switch( fType)
	{
		case JSON_undefined:
			{
				outString.Clear();	// not 'undefined'. toString() returns an empty string on an undefined value in a javascript array 
				break;
			}

		case JSON_null:
			{
				outString.Clear();	// not 'null'. toString() returns an empty string on a null value in a javascript array
				break;
			}

		case JSON_true:
			{
				outString = sTrueString;
				break;
			}

		case JSON_false:
			{
				outString = sFalseString;
				break;
			}
			
		case JSON_string:
			{
				outString.FromString( fString);
				break;
			}
			
		case JSON_number:
			{
				VReal r( fNumber);
				err = r.GetJSONString( outString, JSON_Default);
				break;
			}

		case JSON_date:
			{
				VTime dd;
				dd.FromMilliseconds(fTimeStamp);
				dd.GetJSONString(outString, JSON_Default);
				break;
			}

		case JSON_array:
			{
				err = fArray->GetString( outString);
				break;
			}

		case JSON_object:
			{
				err = fObject->GetString( outString);
				break;
			}
		
		default:
			xbox_assert( false);
	}
	
	return err;
}



void VJSONValue::GetTime(VTime& outTime) const
{
	VError err = VE_OK;
	switch (fType)
	{

		case JSON_string:
		{
			VString s;
			s.FromString(fString);
			outTime.FromString(s);
			break;
		}

		case JSON_date:
		{
			outTime.FromMilliseconds(fTimeStamp);
			break;
		}

		case JSON_number:
		{
			outTime.FromMilliseconds(fNumber);
			break;
		}

		default:
		{
			outTime.Clear();
			outTime.SetNull(true);
			break;
		}
	}
}


VError VJSONValue::Stringify( VString& outString, JSONOption inOptions) const
{
	VJSONWriter writer( inOptions);
	return writer.StringifyValue( *this, outString);
}


VError VJSONValue::ParseFromString(const VString& inString, bool allowDates)
{
	return VJSONImporter::ParseString(inString, *this, allowDates ? VJSONImporter::EJSI_Default | VJSONImporter::EJSI_AllowDates : VJSONImporter::EJSI_Default);
}

VError VJSONValue::LoadFromFile(const VFile* fromFile, bool allowDates)
{
	return VJSONImporter::ParseFile(fromFile, *this, allowDates ? VJSONImporter::EJSI_Default | VJSONImporter::EJSI_AllowDates : VJSONImporter::EJSI_Default);
}

VError VJSONValue::SaveToFile(VFile* toFile) const
{
	return VJSONWriter::StringifyValueToFile(toFile, *this);
}


VValueSingle* VJSONValue::CreateVValue(bool differentiateObjects) const
{
	VValueSingle* result = nil;
	switch (fType)
	{
		case JSON_true:
			result = new VBoolean(true);
			break;

		case JSON_false:
			result = new VBoolean(false);
			break;

		case JSON_string:
			result = new VString(fString);
			break;

		case JSON_number:
			result = new VReal(fNumber);
			break;

		case JSON_date:
			result = new VTime();
			((VTime*)result)->FromMilliseconds(fTimeStamp);
			break;

		case JSON_object:
			if (differentiateObjects)
			{
				result = new VString("\x01\x02\x03\x01");
				result->SetNull(false);
			}
			else
			{
				result = new VString();
				result->SetNull(true);
			}
			break;
		case JSON_array:
			if (differentiateObjects)
			{
				result = new VString("\x01\x02\x03\x02");
				result->SetNull(false);
			}
			else
			{
				result = new VString();
				result->SetNull(true);
			}
			break;
		default:
			result = new VString();
			result->SetNull(true);
			break;
	}
	return result;
}



void VJSONValue::test()
{
const char *sTestJson = "{\"good\":true,\"name\":\"smith\",\"paul\":{\"good\":false,\"name\":\"paul\",\"Nul\":null,\"age\":10.321,\"text\":\"\"\\/\b\f\n\r\t\"},\"age\":42}";

	VJSONValue	object( JSON_object);
	
	object.SetProperty( CVSTR( "name"), VJSONValue( CVSTR( "smith")));
	object.SetProperty( CVSTR( "age"), VJSONValue( 42));
	object.SetProperty( CVSTR( "good"), VJSONValue::sTrue);

	for( VJSONPropertyIterator i( object.GetObject()) ; i.IsValid() ; ++i)
	{
		VString propertyName = i.GetName();
		VJSONValue propertyValue = i.GetValue();
	}

	VJSONValue	object_child( JSON_object);

	object_child.SetProperty( CVSTR( "name"), VJSONValue( CVSTR( "paul")));
	object_child.SetProperty( CVSTR( "age"), VJSONValue( 10.321));
	object_child.SetProperty( CVSTR( "good"), VJSONValue::sFalse);
	object_child.SetProperty( CVSTR( "text"), VJSONValue( CVSTR( "\"\\/\b\f\n\r\t")));
	object_child.SetProperty( CVSTR( "Nul"), VJSONValue::sNull);
	object_child.SetProperty( CVSTR( "undef"), VJSONValue::sUndefined);

	VJSONArray		*object_children = new VJSONArray;
	object_children->Push( object_child);
	
	object.SetProperty( CVSTR( "children"), VJSONValue( object_children));
	
	ReleaseRefCountable( &object_children);
	
	VString s;
	VJSONWriter writer( JSON_WithQuotesIfNecessary);
	VError err = writer.StringifyValue( object, s);
	
	DebugMsg( s); 
	DebugMsg( "\n"); 
	
	VJSONWriter writer2( JSON_WithQuotesIfNecessary | JSON_PrettyFormatting);
	err = writer2.StringifyValue( object, s);

	DebugMsg( s); 
	DebugMsg( "\n"); 
	
	VJSONValue value;
	VJSONImporter importer(s);
	err = importer.Parse( value);
	VString s2;
	writer2.StringifyValue( value, s2);
	DebugMsg( s2); 
	DebugMsg( "\n"); 
	xbox_assert( s == s2);
	
	{
		VJSONImporter importer2( CVSTR( "[1,2,3,[4,5],6]"));
		err = importer2.Parse( value);
		value.GetString( s2);
		DebugMsg( s2); 
		DebugMsg( "\n");
	}

}


//---------------------------------------------------

EJSONStatus IJSONObject::IJSON_GetProperty( const VJSONObject *inOwner, const VString& inName, VJSONValue& outValue) const
{
	return EJSON_unhandled;
}


EJSONStatus IJSONObject::IJSON_SetProperty( VJSONObject *inOwner, const VString& inName, const VJSONValue& inValue)
{
	return EJSON_unhandled;
}


EJSONStatus IJSONObject::IJSON_RemoveProperty( VJSONObject *inOwner, const VString& inName)
{
	return EJSON_unhandled;
}


bool IJSONObject::IJSON_Clear( VJSONObject *inOwner)
{
	return true;
}


bool IJSONObject::IJSON_IsEmpty( const VJSONObject *inOwner) const
{
	return true;
}


EJSONStatus IJSONObject::IJSON_Stringify( const VJSONObject *inOwner, VJSONWriter& inWriter, VString& outString, VError *outError) const
{
	return EJSON_unhandled;
}


EJSONStatus IJSONObject::IJSON_Clone( const VJSONObject *inOwner, VJSONCloner& inCloner, VJSONValue& outValue, VError *outError) const
{
	return EJSON_unhandled;
}

//---------------------------------------------------


VJSONObject::VJSONObject( IJSONObject *inImplementation)
: fGraph( NULL)
, fImpl( RetainRef( inImplementation))
{
	VInterlocked::Increment( &sCount);
}


VJSONObject::~VJSONObject()
{
	ReleaseRef( &fGraph);
	ReleaseRef( &fImpl);
	VInterlocked::Decrement( &sCount);
}


sLONG VJSONObject::Release( const char* inDebugInfo) const
{
	if (VJSONGraph::IsPossiblyCyclic( fGraph))
		VJSONGraph::ReleaseObjectDependenciesIfNecessary( const_cast<VJSONObject*>( this));

	return IRefCountable::Release( inDebugInfo);
}


VJSONValue VJSONObject::GetProperty( const VString& inName) const
{
	VJSONValue value;
	EJSONStatus status = EJSON_unhandled;

	if (fImpl != NULL)
	{
		status = fImpl->IJSON_GetProperty( this, inName, value);
		if (status == EJSON_error)
			value.SetUndefined();
	}

	if (status == EJSON_unhandled)
	{
		MapType::const_iterator i = fMap.find( inName);
		if (i != fMap.end())
			value = i->second.first;
	}
	
	return value;
}


bool VJSONObject::SetProperty( const VString& inName, const VJSONValue& inValue)
{
	bool ok;
	EJSONStatus status = EJSON_unhandled;

	if (fImpl != NULL)
	{
		status = fImpl->IJSON_SetProperty( this, inName, inValue);
		ok = (status == EJSON_ok);
	}

	if (status == EJSON_unhandled)
	{
		try
		{
			ok = true;
			if (inValue.IsUndefined())
				fMap.erase( inName);
			else
			{
				std::pair<MapType::iterator,bool> i = fMap.insert( MapType::value_type( inName, std::pair<VJSONValue,size_t>(inValue,fMap.size())));
				if (!i.second)
				{
					i.first->second.first = inValue;
				}
					
				VJSONGraph::Connect( &fGraph, inValue);
			}
		}
		catch(...)
		{
			ok = false;
		}
	}
	
	return ok;
}


void VJSONObject::RemoveProperty( const VString& inName)
{
	EJSONStatus status = EJSON_unhandled;

	if (fImpl != NULL)
	{
		status = fImpl->IJSON_RemoveProperty( this, inName);
	}

	if (status == EJSON_unhandled)
	{
		MapType::const_iterator i = fMap.find( inName);
		if (i != fMap.end())
		{
			// decrement property indexes above the deleted one
			size_t index = i->second.second;
			for( MapType::iterator j = fMap.begin() ; j != fMap.end() ; ++j)
			{
				if (j->second.second > index)
					j->second.second -= 1;
			}
			fMap.erase( i);
		}
	}
}


void VJSONObject::Clear()
{
	if (fImpl != NULL)
	{
		bool ok = fImpl->IJSON_Clear( this);
	}

	fMap.clear();
}


bool VJSONObject::IsEmpty() const
{
	bool isEmpty = fMap.empty();
	
	if (isEmpty && (fImpl != NULL))
	{
		isEmpty = fImpl->IJSON_IsEmpty( this);
	}

	return isEmpty;
}


VError VJSONObject::GetString( VString& outString) const
{
	outString = VJSONValue::sObjectString;
	return VE_OK;
}


bool VJSONObject::DoStringify( VString& outString, VJSONWriter& inWriter, VError *outError) const
{
	EJSONStatus status = EJSON_unhandled;
	
	if (fImpl != NULL)
	{
		status = fImpl->IJSON_Stringify( this, inWriter, outString, outError);
	}

	return status != EJSON_unhandled;
}


VError VJSONObject::MergeWith(const VJSONObject* inOther, bool inPrivilegesSourceOnConflict)
{
	VError err = VE_OK;
	for (MapType::const_iterator cur = inOther->fMap.begin(), end = inOther->fMap.end(); (cur != end) && (err == VE_OK); ++cur)
	{
		if (inPrivilegesSourceOnConflict)
		{
			MapType::const_iterator found = fMap.find( cur->first);
			if (found == fMap.end())
				SetProperty(cur->first, cur->second.first);
		}
		else
			SetProperty(cur->first, cur->second.first);
	}
	return err;
}



VError VJSONObject::Clone( VJSONValue& outValue, VJSONCloner& inCloner) const
{
	VError err = VE_OK;
	EJSONStatus status = EJSON_unhandled;

	if (fImpl != NULL)
	{
		status = fImpl->IJSON_Clone( this, inCloner, outValue, &err);
	}
	
	if (status == EJSON_unhandled)
	{
		VJSONObject *clone = new VJSONObject();
		if (clone != NULL)
		{
			err = CloneProperties( inCloner, clone);
		}
		else
		{
			err = VE_MEMORY_FULL;
		}
		outValue.SetObject( clone);
		ReleaseRef( &clone);
	}

	return err;
}


VError VJSONObject::CloneProperties( VJSONCloner& inCloner, VJSONObject *inDestination) const
{
	VError err = VE_OK;
	if (inDestination != NULL)
	{
		MapType clonedMap( fMap);

		for( MapType::iterator i = clonedMap.begin() ; (i != clonedMap.end()) && (err == VE_OK) ; ++i)
		{
			if (i->second.first.IsObject())
			{
				VJSONObject *theOriginalObject = RetainRefCountable( i->second.first.GetObject());
				err = inCloner.CloneObject( theOriginalObject, i->second.first);
				VJSONGraph::Connect( &inDestination->fGraph, i->second.first);
				ReleaseRefCountable( &theOriginalObject);
			}
			else if (i->second.first.IsArray())
			{
				VJSONArray *theOriginalArray = RetainRefCountable( i->second.first.GetArray());
				err = theOriginalArray->Clone( i->second.first, inCloner);
				VJSONGraph::Connect( &inDestination->fGraph, i->second.first);
				ReleaseRefCountable( &theOriginalArray);
			}
		}
		
		if (err == VE_OK)
			inDestination->fMap.swap( clonedMap);
	}
	return err;
}


//---------------------------------------------------


VJSONPropertyConstOrderedIterator::VJSONPropertyConstOrderedIterator( const VJSONObject *inObject)
{
	fMappedValues.resize( inObject->fMap.size(), NULL);
	
	VectorOfMappedValueRef::iterator j = fMappedValues.begin();
	for( VJSONObject::MapType::const_iterator i = inObject->fMap.begin() ; i != inObject->fMap.end() ; ++i)
	{
		xbox_assert( i->second.second >= 0 && i->second.second < fMappedValues.size());
		fMappedValues[i->second.second] = &*i;
	}
	
	fIterator = fMappedValues.begin();
	fIterator_end = fMappedValues.end();
}


//---------------------------------------------------

VJSONArray::VJSONArray()
: fGraph( NULL)
{
	VInterlocked::Increment( &sCount);
}


VJSONArray::~VJSONArray()
{
	ReleaseRefCountable( &fGraph);
	VInterlocked::Decrement( &sCount);
}


sLONG VJSONArray::Release( const char* inDebugInfo) const
{
	if (VJSONGraph::IsPossiblyCyclic( fGraph))
		VJSONGraph::ReleaseArrayDependenciesIfNecessary( const_cast<VJSONArray*>( this));

	return IRefCountable::Release( inDebugInfo);
}


bool VJSONArray::Push( const VJSONValue& inValue)
{
	bool ok = true;
	try
	{
		// undefined values are legal as in JavaScript
		fVector.push_back( inValue);
		VJSONGraph::Connect( &fGraph, inValue);
	}
	catch(...)
	{
		ok = false;
	}
	return ok;
}


void VJSONArray::SetNth( size_t inIndex, const VJSONValue& inValue)
{
	if (testAssert( (inIndex > 0) && (inIndex <= fVector.size()) ))
	{
		fVector[inIndex-1] = inValue;
		VJSONGraph::Connect( &fGraph, inValue);
	}
}


bool VJSONArray::Resize( size_t inSize)
{
	bool ok = true;
	try
	{
		fVector.resize( inSize);
	}
	catch(...)
	{
		ok = false;
	}
	return ok;
}


void VJSONArray::Clear()
{
	fVector.clear();
}


VError VJSONArray::GetString( VString& outString) const
{
	VError err = VE_OK;

	if (fVector.empty())
	{
		outString.Clear();
	}
	else
	{
		VectorOfVString array;
		array.resize( fVector.size());
		VectorOfVString::iterator j = array.begin();
		for( VectorType::const_iterator i = fVector.begin() ; (i != fVector.end()) && (err == VE_OK) ; ++i, ++j)
		{
			err = i->GetString( *j);
		}
		err = outString.Join( array, ',') ? VE_OK : vThrowError( VE_STRING_ALLOC_FAILED);
	}
	return err;
}


VError VJSONArray::Clone( VJSONValue& outValue, VJSONCloner& inCloner) const
{
	VError err = VE_OK;

	VJSONArray *clone = new VJSONArray;
	if (clone != NULL)
	{
		VectorType clonedVector = fVector;

		for( VectorType::iterator i = clonedVector.begin() ; (i != clonedVector.end()) && (err == VE_OK) ; ++i)
		{
			if (i->IsObject())
			{
				VJSONObject *theOriginalObject = RetainRefCountable( i->GetObject());
				err = inCloner.CloneObject( theOriginalObject, *i);
				VJSONGraph::Connect( &clone->fGraph, *i);
				ReleaseRefCountable( &theOriginalObject);
			}
			else if (i->IsArray())
			{
				VJSONArray *theOriginalArray = RetainRefCountable( i->GetArray());
				err = theOriginalArray->Clone( *i, inCloner);
				VJSONGraph::Connect( &clone->fGraph, *i);
				ReleaseRefCountable( &theOriginalArray);
			}
		}
		
		if (err == VE_OK)
			clone->fVector.swap( clonedVector);
	}
	else
	{
		err = VE_MEMORY_FULL;
	}
	outValue.SetArray( clone);
	ReleaseRefCountable( &clone);

	return err;
}



//---------------------------------------------------

VJSONWriter::VJSONWriter( JSONOption inOptions)
: fOptions( inOptions)
, fLevel( 0)
, fIndentString( "\t")
{
	if ( (fOptions & JSON_PrettyFormatting) != 0)
		fIndentStringCurrentLevel += '\n';
}


VError VJSONWriter::StringifyValue( const VJSONValue& inValue, VString& outString)
{
	VError err = VE_OK;
	switch( inValue.GetType())
	{
		case JSON_undefined:
			{
				outString = VJSONValue::sUndefinedString;
				break;
			}

		case JSON_null:
			{
				outString = VJSONValue::sNullString;
				break;
			}

		case JSON_true:
			{
				outString = VJSONValue::sTrueString;
				break;
			}

		case JSON_false:
			{
				outString = VJSONValue::sFalseString;
				break;
			}
			
		case JSON_string:
			{
				VString s;
				err = inValue.GetString( s);
				s.GetJSONString( outString, GetOptions());
				break;
			}

		case JSON_date:
			{
				VTime dd;
				VString s;
				inValue.GetTime(dd);
				dd.GetJSONString(s);
				if ((GetOptions() & JSON_AllowDates) != 0)
				{
					outString = "\"!!" + s + "!!\"";
				}
				else
					outString = "\"" + s + "\"";
				break;
			}

		case JSON_number:
			{
				VReal r( inValue.GetNumber());
				err = r.GetJSONString( outString, GetOptions());
				break;
			}

		case JSON_array:
			{
				err = StringifyArray( inValue.GetArray(), outString);
				break;
			}

		case JSON_object:
			{
				err = StringifyObject( inValue.GetObject(), outString);
				break;
			}
		
		default:
			xbox_assert( false);
	}
	
	return err;
}


VError VJSONWriter::StringifyObject( const VJSONObject *inObject, VString& outString)
{
	if (inObject == NULL)
	{
		outString = VJSONValue::sUndefinedString;
		return VE_OK;
	}

	if (std::find( fStack.begin(), fStack.end(), inObject) != fStack.end())
	{
		return vThrowError( VE_JSON_STRINGIFY_CIRCULAR);
	}

	VError err = VE_OK;

	fStack.push_back( inObject);
	
	if (!inObject->DoStringify( outString, *this, &err))
	{
		if (inObject->fMap.empty())
		{
			outString = "{}";
		}
		else
		{
			IncrementLevel();

			VectorOfVString array;
			array.resize( inObject->fMap.size());

			VectorOfVString::iterator j = array.begin();
			for( VJSONPropertyConstOrderedIterator i( inObject) ; i.IsValid() && (err == VE_OK) ; ++i, ++j)
			{
				err = i.GetName().GetJSONString( *j, GetOptions());
				if (err == VE_OK)
				{
					InsertIndentString( *j);

					VString value;
					err = StringifyValue( i.GetValue(), value);
					if (err == VE_OK)
					{
						j->AppendUniChar( ':');
						if ( (fOptions & JSON_PrettyFormatting) != 0)
							j->AppendUniChar( ' ');
						j->AppendString( value);
					}
				}
					
			}

			DecrementLevel();

			if (err == VE_OK)
			{
				array.front().Insert( '{', 1);
				AppendIndentString( array.back());
				array.back().AppendUniChar( '}');
				err = outString.Join( array, ',') ? VE_OK : vThrowError( VE_STRING_ALLOC_FAILED);
			}
		}
	}

	fStack.pop_back();
	
	return err;
}


VError VJSONWriter::StringifyArray( const VJSONArray *inArray, VString& outString)
{
	VError err = VE_OK;
	
	if (inArray == NULL)
	{
		outString = VJSONValue::sUndefinedString;
	}
	else if (inArray->IsEmpty())
	{
		outString = "[]";
	}
	else
	{
		IncrementLevel();

		VectorOfVString array;
		size_t count = inArray->GetCount();

		try
		{
			array.resize( count);
		}
		catch(...)
		{
			err = vThrowError( VE_MEMORY_FULL);
		}

		VectorOfVString::iterator j = array.begin();
		for( size_t i = 0 ; (i < count) && (err == VE_OK) ; ++i, ++j)
		{
			err = StringifyValue( (*inArray)[i], *j);
			InsertIndentString( *j);
		}

		DecrementLevel();

		if (err == VE_OK)
		{
			array.front().Insert( '[', 1);
			AppendIndentString( array.back());
			array.back().AppendUniChar( ']');
			err = outString.Join( array, ',') ? VE_OK : vThrowError( VE_STRING_ALLOC_FAILED);
		}
	}
		
	return err;
}


VError VJSONWriter::StringifyValueToFile( VFile *inFile, const VJSONValue& inValue, JSONOption inOptions)
{
	if (!testAssert( inFile != NULL))
	{
		return VE_OK;
	}

	VString jsonString;
	VJSONWriter writer( inOptions);
	writer.StringifyValue( inValue, jsonString);
	return inFile->SetContentAsString( jsonString, VTC_UTF_8);
}


void VJSONWriter::IncrementLevel()
{
	++fLevel;
	if ( (fOptions & JSON_PrettyFormatting) != 0)
		fIndentStringCurrentLevel += fIndentString;
}


void VJSONWriter::DecrementLevel()
{
	--fLevel;
	if ( (fOptions & JSON_PrettyFormatting) != 0)
		fIndentStringCurrentLevel.Truncate( fIndentStringCurrentLevel.GetLength() - fIndentString.GetLength());
}


void VJSONWriter::InsertIndentString( VString& ioString)
{
	if ( (fOptions & JSON_PrettyFormatting) != 0)
		ioString.Insert( fIndentStringCurrentLevel, 1);
}


void VJSONWriter::AppendIndentString( VString& ioString)
{
	if ( (fOptions & JSON_PrettyFormatting) != 0)
		ioString.AppendString( fIndentStringCurrentLevel);
}


//---------------------------------------------------


VJSONCloner::VJSONCloner()
{
}


VError VJSONCloner::CloneObject( const VJSONObject *inObject, VJSONValue& outValue)
{
	VError err = VE_OK;
	
	if (inObject == NULL)
	{
		outValue.SetUndefined();
	}
	else
	{
		try
		{
			VJSONValue value;
			std::pair<MapOfValueByObject::iterator,bool> i = fClonedObjects.insert( MapOfValueByObject::value_type( inObject, value));
			if (i.second)
			{
				// not already cloned
				err = inObject->Clone( i.first->second, *this);
			}
			outValue = i.first->second;
		}
		catch(...)
		{
			outValue.SetUndefined();
			err = vThrowError( VE_MEMORY_FULL);
		}
	}
	return err;
}

