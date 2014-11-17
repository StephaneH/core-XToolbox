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
#include "VJSONTools.h"
#include "VErrorContext.h"
#include "VTask.h"
#include "VKernelBagKeys.h"



VErrorContext::VErrorContext(const VErrorContext& inContext)
{
	fIsSilent = false;
	fIsKeepingErrors = true;
	fFailedForMemory = false;
	if (inContext.fParametersForPushedErrors != NULL)
		fParametersForPushedErrors = inContext.fParametersForPushedErrors->Clone();
	else
		fParametersForPushedErrors = NULL;
	PushErrors( inContext);
}


VErrorContext::~VErrorContext()
{
	ReleaseRefCountable( &fParametersForPushedErrors);
}


VError VErrorContext::LoadFromBag( const VValueBag& inBag)
{
	VError err = VE_OK;
	const VBagArray *bags = inBag.GetElements( ErrorBagKeys::error);
	if (bags != NULL)
	{
		try
		{
			VIndex count = bags->GetCount();
			for( VIndex i = 1 ; (i <= count) && (err == VE_OK) ; ++i)
			{
				const VValueBag *bag = bags->GetNth( i);
				VErrorBase *error = new VErrorBase;
				if (error != NULL)
				{
					err = error->LoadFromBag( *bag);
					fStack.push_back( error);
					error->Release();
				}
			}
		}
		catch(...)
		{
			err = VE_MEMORY_FULL;
		}
	}
	return err;
}


VError VErrorContext::SaveToBag( VValueBag& ioBag) const
{
	for( VErrorStack::const_iterator i = fStack.begin() ; i != fStack.end() ; ++i)
	{
		VValueBag *bag = new VValueBag;
		if (bag != NULL)
		{
			(*i)->SaveToBag( *bag);
			ioBag.AddElement( ErrorBagKeys::error, bag);
		}
		ReleaseRefCountable( &bag);
	}
	return VE_OK;
}


void VErrorContext::SaveToJSONArray( VJSONValue& outArray) const
{
	if (fStack.empty())
	{
		outArray.SetUndefined();
	}
	else
	{
		VJSONArray *array = new VJSONArray;
		if (array != NULL)
		{
			for( VErrorStack::const_iterator i = fStack.begin() ; i != fStack.end() ; ++i)
			{
				VJSONObject *errorObject = new VJSONObject;
				if (errorObject != NULL)
				{
					VString description;
					(*i)->GetErrorDescription( description);
					errorObject->SetPropertyAsString( CVSTR( "message"), description);

					VError errCode = (*i)->GetError();
					sLONG errnum = ERRCODE_FROM_VERROR( errCode);
					OsType component = COMPONENT_FROM_VERROR( errCode);
					
					#if SMALLENDIAN
					ByteSwap( &component);
					#endif
					
					VString compname(&component, 4, VTC_US_ASCII);

					errorObject->SetPropertyAsString( CVSTR( "componentSignature"), compname);
					errorObject->SetPropertyAsNumber( CVSTR( "errCode"), errnum);
					
					array->Push( VJSONValue( errorObject));
				}
				ReleaseRefCountable( &errorObject);
			}
			outArray.SetArray( array);
		}
		else
		{
			outArray.SetUndefined();
		}
		ReleaseRefCountable( &array);
	}
}



VValueBag *VErrorContext::GetParametersForPushedErrors( bool inAllocateIfNecessary)
{
	if ( (fParametersForPushedErrors == NULL) && inAllocateIfNecessary)
		fParametersForPushedErrors = new VValueBag;
	return fParametersForPushedErrors;
}


bool VErrorContext::PushError( VErrorBase* inError)
{
	if (inError == NULL)
		return false;
	
	if (inError->GetError() == VE_MEMORY_FULL)
		fFailedForMemory = true;

	xbox_assert(inError->GetRefCount() > 0);

	bool pushed = false;
	try
	{
		inError->GetBag()->UnionClone( fParametersForPushedErrors);

		fStack.push_back( inError);
		pushed = true;
	}
	catch(...)
	{
	}
	return pushed;
}


void VErrorContext::Flush()
{
	fStack.clear();
	fFailedForMemory = false;
}


bool VErrorContext::DebugDisplay() const
{
	for( VErrorStack::const_iterator i = fStack.begin() ; i != fStack.end() ; ++i)
	{
		VString dump;
		VErrorBase*	error = i->Get();
		error->DumpToString( dump);
		DebugMsg( dump);
	}
	
	return !fStack.empty();
}


VErrorBase* VErrorContext::GetFirst() const
{
	return fStack.empty() ? NULL : fStack.front().Get();
}


VErrorBase* VErrorContext::GetLast() const
{
	return fStack.empty() ? NULL : fStack.back().Get();
}


VError VErrorContext::GetLastError() const
{
	VErrorBase*	error = GetLast();
	return (error == NULL) ? VE_OK : error->GetError();
}


VErrorBase* VErrorContext::Find( VError inError) const
{
	VErrorStack::const_iterator i = fStack.begin();
	for( ; (i != fStack.end()) && ((*i)->GetError() != inError) ; ++i)
		;
	
	return (i != fStack.end()) ? i->Get() : NULL;
}


bool VErrorContext::FindAny( VError inError,...) const
{
	bool oneFound = false;
	for( VErrorStack::const_iterator i = fStack.begin() ; (i != fStack.end()) && !oneFound ; ++i)
	{
		va_list argList;
		va_start(argList, inError);

		for( VError oneErr = inError ; oneErr != VE_OK ; oneErr = va_arg(argList, VError))
		{
			if (oneErr == (*i)->GetError())
			{
				oneFound = true;
				break;
			}
		}

		va_end(argList);
	}
	
	return oneFound;
}


void VErrorContext::PushErrors( const VErrorContext& inFromContext)
{
	try
	{
		for( VErrorStack::const_iterator i = inFromContext.fStack.begin() ; i != inFromContext.fStack.end() ; ++i)
		{
			(*i)->GetBag()->UnionClone( fParametersForPushedErrors);
			fStack.push_back( *i);
		}
	}
	catch(...)
	{
	}

	fFailedForMemory = inFromContext.fFailedForMemory;
}


//====================================================================================================================================


VErrorTaskContext::VErrorTaskContext()
: fRecycledCleanContext( new VErrorContext)
{
}


VErrorTaskContext::~VErrorTaskContext()
{
	xbox_assert(fStack.size() <= 1); // zero or one context (the current context)
	ReleaseRefCountable( &fRecycledCleanContext);
}


VErrorBase* VErrorTaskContext::GetLast() const
{
	VErrorBase *err = NULL;
	for( VectorOfVErrorContext::const_reverse_iterator i = fStack.rbegin() ; (i != fStack.rend()) && (err == NULL) ; ++i)
		err = (*i)->GetLast();
	
	return err;
}


VErrorBase* VErrorTaskContext::GetFirst() const
{
	VErrorBase *err = NULL;
	for( VectorOfVErrorContext::const_iterator i = fStack.begin() ; (i != fStack.end()) && (err == NULL) ; ++i)
		err = (*i)->GetFirst();
	
	return err;
}


void VErrorTaskContext::Flush()
{
	// only flush current context but reset last error
	if (!fStack.empty())
		fStack.back()->Flush();
}


bool VErrorTaskContext::PushError( VErrorBase* inError, bool *outSilent)
{
	bool isPushed = false;
	bool silent = false;
	
	if (inError != NULL)
	{
		try
		{
			if (fStack.empty())
			{
				VErrorContext *context = new VErrorContext;
				if (context)
				{
					fStack.push_back( context);
					isPushed = context->PushError( inError);
					context->Release();
				}
			}
			else
			{
				isPushed = fStack.back()->PushError( inError);
			}
			if (isPushed)
			{
				for( VectorOfVErrorContext::reverse_iterator i = fStack.rbegin() ; i != fStack.rend() ; ++i)
				{
					if ( (*i)->IsSilent())
					{
						silent = true;
						break;
					}

					if ( (*i)->IsErrorFiltered( inError->GetError()))
					{
						silent = true;
						// c'est le context courant qui devient silencieux
						fStack.back()->SetKeepingErrors( false);
						fStack.back()->SetSilent( true);
						break;
					}
				}
			}
		}
		catch(...)
		{
		}
	}
	
	if (outSilent)
		*outSilent = silent;
	
	return isPushed;
}


void VErrorTaskContext::PushErrorsFromContext( const VErrorContext* inErrorContext)
{
	if (inErrorContext != NULL)
	{
		for( VErrorStack::const_iterator i = inErrorContext->GetErrorStack().begin() ; i != inErrorContext->GetErrorStack().end() ; ++i)
			PushError( *i, NULL);
	}
}


void VErrorTaskContext::PushErrorsFromBag( const VValueBag& inBag)
{
	const VBagArray *bags = inBag.GetElements( ErrorBagKeys::error);
	if (bags != NULL)
	{
		try
		{
			VError err_load = VE_OK;
			VIndex count = bags->GetCount();
			for( VIndex i = 1 ; (i <= count) && (err_load == VE_OK) ; ++i)
			{
				const VValueBag *bag = bags->GetNth( i);
				VErrorBase *error = new VErrorBase;
				if (error != NULL)
				{
					err_load = error->LoadFromBag( *bag);
					if (err_load == VE_OK)
						PushError( error, NULL);
					error->Release();
				}
			}
		}
		catch(...)
		{
		}
	}
}


VErrorContext *VErrorTaskContext::PushNewContext( bool inKeepingErrors, bool inSilentContext)
{
	VErrorContext *context;

	try
	{
		if (fRecycledCleanContext != NULL)
		{
			context = fRecycledCleanContext;
			context->SetKeepingErrors( inKeepingErrors);
			context->SetSilent( inSilentContext);
			context->SetFailedForMemory( false);
			fRecycledCleanContext = NULL;
		}
		else
		{
			context = new VErrorContext( inKeepingErrors, inSilentContext);
		}

		fStack.push_back( context);
	}
	catch(...)
	{
		context = NULL;
	}
	
	
	return context;
}


void VErrorTaskContext::PopContext()
{
	if (!fStack.empty())
	{
		VErrorContext *context = fStack.back();

		if ( !context->IsEmpty() && context->IsKeepingErrors())
		{
			if (fStack.size() == 1)
			{
				VErrorContext *mergeContext = new VErrorContext( *context);
				if (mergeContext != NULL)
				{
					fStack.back().Adopt( mergeContext);
				}
				else
				{
					fStack.pop_back();
				}
			}
			else
			{
				fStack.pop_back();
				fStack.back()->PushErrors( *context);
			}
		}
		else
		{
			fStack.pop_back();
		}
	}
}


inline sWORD SwapWord(sWORD x) { return ((x & 0x00FF) << 8) | (x >> 8); };
inline uWORD SwapWord(uWORD x) { return ((x & 0x00FF) << 8) | (x >> 8); };

inline sLONG SwapLong(sLONG x) { return ( ((sLONG)(SwapWord((uWORD)(x & 0x0000FFFF))) << 16) | (sLONG)SwapWord((uWORD)(x >> 16))); };
inline uLONG SwapLong(uLONG x) { return ( ((uLONG)(SwapWord((uWORD)(x & 0x0000FFFF))) << 16) | (uLONG)SwapWord((uWORD)(x >> 16))); };

void VErrorTaskContext::BuildErrorStack(VValueBag& outBag)
{
	VErrorTaskContext *context = VTask::GetCurrent()->GetErrorContext( false);
	if (context)
	{
		VErrorContext combinedContext;
		context->GetErrors( combinedContext);

		const VErrorStack& errorstack = combinedContext.GetErrorStack();
		for( VErrorStack::const_iterator cur = errorstack.begin(), end = errorstack.end(); cur != end; cur++)
		{
			VErrorBase *err = *cur;
			BagElement errbag(outBag, "__ERROR");
			VString errdesc;
			err->GetErrorDescription(errdesc);
			errbag->SetString(L"message", errdesc);
			VError xerr = err->GetError();
			sLONG errnum = ERRCODE_FROM_VERROR(xerr);
			OsType component = COMPONENT_FROM_VERROR(xerr);
			component = SwapLong(component);
			VString compname(&component, 4, VTC_US_ASCII);
			errbag->SetString(L"componentSignature", compname);
			errbag->SetLong(L"errCode", errnum);
		}
	}
}


void VErrorTaskContext::RecycleContext( VErrorContext* inContext)
{
	// if the popped context is clean, let's recycle it
	if (inContext != NULL)
	{
		if ( (fRecycledCleanContext == NULL) && inContext->CanBeRecycled() )
			fRecycledCleanContext = inContext;
		else
			inContext->Release();
	}
}


void VErrorTaskContext::MergeAndFlushLastContext( bool inContextInitializedSilent)
{
	xbox_assert( fStack.size() >= 1);
	
	VErrorContext *backContext = fStack.back();
	if (!backContext->IsEmpty())
	{
		if (backContext->IsKeepingErrors())
		{
			if (fStack.size() > 1)
				fStack[fStack.size()-2]->PushErrors( *backContext);
			else
			{
				VErrorContext *context = new VErrorContext( *backContext);
				if (context != NULL)
				{
					fStack.insert( fStack.begin(), context);
					context->Release();
				}
			}
		}
		backContext->Flush();
		backContext->SetSilent( inContextInitializedSilent);
	}
}


bool VErrorTaskContext::IsErrorFiltered( VError inError) const
{
	VectorOfVErrorContext::const_iterator i = fStack.begin();
	for( ; (i != fStack.end()) && !(*i)->IsErrorFiltered( inError) ; ++i)
		;

	return i != fStack.end();
}


void VErrorTaskContext::GetErrors( VErrorContext& inContext) const
{
	for( VectorOfVErrorContext::const_iterator i = fStack.begin() ; i != fStack.end() ; ++i)
	{
		inContext.PushErrors( *(i->Get()));
	}
}


bool VErrorTaskContext::FailedForMemory() const
{
	if (fStack.empty())
		return false;
	else
		return fStack.back()->FailedForMemory();
}


VErrorBase *VErrorTaskContext::Find( VError inError) const
{
	VErrorBase *err = NULL;
	for( VectorOfVErrorContext::const_iterator i = fStack.begin() ; (err == NULL) && (i != fStack.end()) ; ++i)
	{
		err = (*i)->Find( inError);
	}
	return err;
}


VError VErrorTaskContext::GetLastError() const
{
	if (fStack.empty())
		return VE_OK;
	else
		return fStack.back()->GetLastError();
}


//====================================================================================================================================


StErrorContextInstaller::StErrorContextInstaller( VError inFirstFilteredError, VError inFilteredError, ...)
{
	// the context is created not silent.
	// it becomes silent the first time an error in the specified filter list is thrown.
	_Init( true, false);

	if (fContext != NULL)
	{
		fContext->AddErrorToFilter( inFirstFilteredError);

		va_list argList;
		va_start(argList, inFilteredError);

		for( VError oneErr = inFilteredError ; oneErr != VE_OK ; oneErr = va_arg(argList, VError))
		{
			fContext->AddErrorToFilter( oneErr);
		}

		va_end( argList);
	}
}

void StErrorContextInstaller::_Init( bool inKeepingErrors, bool inSilentContext)
{
	VTask *task = VTask::GetCurrent();
	fTaskContext = (task != NULL) ? task->GetErrorContext( true) : NULL;

	fContextInitializedSilent = inSilentContext;

	if (fTaskContext != NULL)
	{
		fContext = fTaskContext->PushNewContext( inKeepingErrors, inSilentContext);
	}
	else
	{
		fContext = new VErrorContext( inKeepingErrors, inSilentContext);
	}
}


StErrorContextInstaller::~StErrorContextInstaller()
{
	if (fContext != NULL)
	{
		if (fTaskContext != NULL)
		{
			xbox_assert( fContext == fTaskContext->GetLastContext());
			fTaskContext->PopContext();
			fTaskContext->RecycleContext( fContext);
		}
		else
		{
			fContext->Release();
		}
	}
}

