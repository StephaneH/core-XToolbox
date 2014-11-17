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
#ifndef __VJSRuntime_blob__
#define __VJSRuntime_blob__

#include "JS4D.h"
#include "VJSClass.h"

BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VJSBlobData : public XBOX::VObject, public XBOX::IRefCountable
{
/*
	const data to implement copy on write optimisation.
*/
public:
			VJSBlobData():fDataPtr( NULL), fDataSize( 0)	{;}
			VJSBlobData( void *inDataPtr, XBOX::VSize inDataSize);

			const void*	GetDataPtr() const	{ return fDataPtr;}
			XBOX::VSize	GetDataSize() const	{ return fDataSize;}
			
private:
			VJSBlobData( const VJSBlobData&);
			VJSBlobData& operator=( const VJSBlobData&);
			~VJSBlobData();
			
			void*			fDataPtr;
			XBOX::VSize		fDataSize;
};


class XTOOLBOX_API VJSDataSlice : public XBOX::VObject, public XBOX::IRefCountable
{
public:
			VJSDataSlice( VJSBlobData *inBlobData, XBOX::VSize inStart, XBOX::VSize inSize)
				: fBlobData( RetainRefCountable( inBlobData))
				, fStart( inStart)
				, fSize( inSize)
				{}

			
			const void*		GetDataPtr() const	{ return (fBlobData == NULL) ? NULL : (char*) fBlobData->GetDataPtr() + fStart;}
			XBOX::VSize		GetDataSize() const	{ return fSize;}
			
			VJSBlobData*	GetBlobData() const	{ return fBlobData;}
			XBOX::VSize		GetStart() const	{ return fStart;}
			XBOX::VSize		GetSize() const		{ return fSize;}
			
private:
			VJSDataSlice( const VJSDataSlice&);
			VJSDataSlice& operator=( const VJSDataSlice&);
			~VJSDataSlice()
				{
					ReleaseRefCountable( &fBlobData);
				}

			VJSBlobData*	fBlobData;
			XBOX::VSize		fStart;
			XBOX::VSize		fSize;
};


class XTOOLBOX_API VJSBlobValue : public XBOX::VObject, public XBOX::IRefCountable
{
public:
			sLONG8				GetStart() const		{ return fStart;}
			sLONG8				GetSize() const			{ return fSizeIsMaxSize ? GetMaxSize() : fSize;}
			const VString&		GetContentType() const	{ return fContentType;}

	virtual	VJSBlobValue*		Slice( sLONG8 inStart, sLONG8 inEnd, const VString& inContentType) const = 0;

	virtual	VJSDataSlice*		RetainDataSlice() const = 0;
	virtual	void				SetData( VJSBlobData *inData);
	
	virtual	sLONG8				GetMaxSize() const;

	virtual	VError				CopyTo( XBOX::VFile *inDestination, bool inOverwrite);

protected:
								// use start = 0 and max size
								VJSBlobValue( const VString& inContentType);
								VJSBlobValue( sLONG8 inStart, sLONG8 inSize, const VString& inContentType);
	virtual						~VJSBlobValue();
		
private:
								VJSBlobValue( const VJSBlobValue&);
								VJSBlobValue& operator=( const VJSBlobValue&);

			sLONG8				fStart;
			sLONG8				fSize;
			bool				fSizeIsMaxSize;	// tells GetSize() not to return fSize but dynamically fetch implementation information (for file size)
			XBOX::VString		fContentType;
};


class XTOOLBOX_API VJSBlobValue_Slice : public VJSBlobValue
{
typedef VJSBlobValue inherited;
public:
		// copies a slice of VBlob parameter.
	static	VJSBlobValue_Slice*	Create( VJSBlobData *inBlobData, XBOX::VSize inBlobStart, XBOX::VSize inBlobSize, XBOX::VSize inSliceStart, XBOX::VSize inSliceSize, const XBOX::VString& inContentType);
	static	VJSBlobValue_Slice*	Create( const XBOX::VBlob& inBlob, const XBOX::VString& inContentType);

		// convenience creator that builds a slice with its own blob data.
		// inBlobData is copied.
	static	VJSBlobValue_Slice*	Create( const void* inBlobData, XBOX::VSize inBlobSize, const XBOX::VString& inContentType);

		// takes VBlob ownership
								VJSBlobValue_Slice( VJSBlobData *inBlobData, XBOX::VSize inStart, XBOX::VSize inSize, const VString& inContentType);
								~VJSBlobValue_Slice();
		
	virtual	VJSBlobValue_Slice*	Slice( sLONG8 inStart, sLONG8 inEnd, const VString& inContentType) const;

	virtual	VJSDataSlice*		RetainDataSlice() const;
	virtual	void				SetData( VJSBlobData *inData);

private:
		VJSBlobValue_Slice( const VJSBlobValue_Slice&);
		VJSBlobValue_Slice& operator=( const VJSBlobValue_Slice&);

			VJSDataSlice*		fSlice;
};


class XTOOLBOX_API VJSBlob : public VJSClass<VJSBlob, VJSBlobValue >
{
public:
	typedef VJSClass<VJSBlob, VJSBlobValue> inherited;

	static VJSObject		NewInstance( const VJSContext& inContext, void *inData, VSize inLength, const VString& inContentType);

	static	void			Initialize( const VJSParms_initialize& inParms, VJSBlobValue* inBlob);
	static	void			Finalize( const VJSParms_finalize& inParms, VJSBlobValue* inBlob);
	static	void			GetDefinition( ClassDefinition& outDefinition);
	static	void			Construct( VJSParms_construct &ioParms);

	static	void			do_Slice( VJSParms_callStaticFunction& ioParms, VJSBlobValue* inBlob);
	static	void			do_ToBuffer( VJSParms_callStaticFunction& ioParms, VJSBlobValue* inBlob);

	static	void			do_ToString( VJSParms_callStaticFunction& ioParms, VJSBlobValue* inBlob);
	static	void			do_WriteString( VJSParms_callStaticFunction& ioParms, VJSBlobValue* inBlob);

	static	void			do_GetSize( VJSParms_getProperty& ioParms, VJSBlobValue* inBlob);
	static	void			do_GetType( VJSParms_getProperty& ioParms, VJSBlobValue* inBlob);

	static	void			do_CopyTo( VJSParms_callStaticFunction& ioParms, VJSBlobValue* inBlob);
};

END_TOOLBOX_NAMESPACE

#endif