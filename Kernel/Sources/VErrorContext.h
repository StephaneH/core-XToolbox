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
#ifndef __VErrorContext__
#define __VErrorContext__

#include "Kernel/Sources/VError.h"

BEGIN_TOOLBOX_NAMESPACE

class VJSONArrayWriter;

/*!
	@class VErrorContext
	@abstract Error stack handling class
	@discussion
		Each VTask has its own VErrorTaskContext to gather errors.
		The error context may be pass along to another VTask while processing a user
		request that goes from one thread to another or from one machine to another.

		Before executing a user request (menu, button, key down), one should call
		VErrorContext::Flush() to clear the error stack, and VErrorContext::Display()
		once the request is completed.
*/

class XTOOLBOX_API VErrorContext : public VObject, public IRefCountable
{
friend class VErrorTaskContext;
public:
									VErrorContext(): fParametersForPushedErrors(NULL), fFailedForMemory(false), fIsSilent( false), fIsKeepingErrors( true) {}
									VErrorContext( bool inKeepingErrors, bool inSilent): fParametersForPushedErrors(NULL), fFailedForMemory(false), fIsSilent(inSilent), fIsKeepingErrors(inKeepingErrors) {}
									VErrorContext( const VErrorContext& inContext);
	virtual							~VErrorContext();

			/** @brief Push an error and returns true if was sucessfully pushed.
			**/
			bool					PushError( VErrorBase* inError);
			/** @brief push errors from another context **/
			void					PushErrors( const VErrorContext& inFromContext);

			bool					DebugDisplay() const;
			void					Flush();

			VErrorBase*				GetFirst() const;
			VErrorBase*				GetLast() const;
			VError					GetLastError() const;

			VErrorBase*				Find( VError inError) const;
			bool					FindAny( VError inError,...) const;

			// Error filter support
			void					AddErrorToFilter( VError inError)				{ fFilter.push_back( inError); }
			sLONG					CountFilteredErrors() const						{ return (sLONG) fFilter.size(); }
			bool					IsErrorFiltered( VError inError) const			{ return std::find( fFilter.begin(), fFilter.end(), inError) != fFilter.end(); }

			// Accessors
			const VErrorStack&		GetErrorStack () const							{ return fStack; }

			/** @brief errors from a silent context should not propagate **/
			bool					IsSilent() const								{ return fIsSilent;}
			void					SetSilent( bool inSilent)						{ fIsSilent = inSilent;}

			bool					IsKeepingErrors() const							{ return fIsKeepingErrors;}
			void					SetKeepingErrors( bool inKeepingErrors)			{ fIsKeepingErrors = inKeepingErrors;}

			bool					FailedForMemory() const							{ return fFailedForMemory; };
			void					SetFailedForMemory( bool inFailedForMemory)		{ fFailedForMemory = inFailedForMemory;}

			/** @brief a context is empty is it has no error **/
			bool					IsEmpty() const									{ return fStack.empty(); }

			/** @brief tells if a context can be recycled **/
			bool					CanBeRecycled() const							{ return fStack.empty() && fFilter.empty() && (GetRefCount() == 1) && (fParametersForPushedErrors == NULL); }

			/** @brief loading and saving withing a bag **/
			VError					LoadFromBag( const VValueBag& inBag);
			VError					SaveToBag( VValueBag& ioBag) const;

			/** @brief saving errors into JSON array or json_undefined if no error **/
			void					SaveToJSONArray( VJSONValue& outArray) const;

			/** @brief context parameters are replicated into any error pushed inside the context **/
			VValueBag*				GetParametersForPushedErrors( bool inAllocateIfNecessary);
	const	VValueBag*				GetParametersForPushedErrors() const			{ return fParametersForPushedErrors;}

private:
			VErrorStack				fStack;
			std::vector<VError>		fFilter;
			VValueBag*				fParametersForPushedErrors;		// parameters
			bool					fFailedForMemory;
			bool					fIsSilent;
			bool					fIsKeepingErrors;
};


typedef std::vector<VRefPtr<VErrorContext> >	VectorOfVErrorContext;

/*!
	@class VErrorTaskContext
	@abstract Task related error context stack
	@discussion
*/

class VValueBag;

class XTOOLBOX_API VErrorTaskContext : public VObject, public IRefCountable
{
public:
									VErrorTaskContext();
	virtual							~VErrorTaskContext();

			/** @brief Pushes an error and returns true if was sucessfully pushed.
				outSilent returns true if at least one context in the stack is silent.
				Throwing an error filtered by a context will make the context silent.
			**/
			bool					PushError( VErrorBase* inError, bool *outSilent);

			// push errors from another context.
			// Does nothing if inErrorContext is NULL or empty.
			void					PushErrorsFromContext( const VErrorContext* inErrorContext);

			// push errors stored in bag
			void					PushErrorsFromBag( const VValueBag& inBag);

	// Error contexts support
			VErrorContext*			GetLastContext() const	{ return fStack.empty() ? NULL : fStack.back();}

			VErrorContext*			PushNewContext( bool inKeepingErrors, bool inSilentContext);
			void					PopContext();
			void					MergeAndFlushLastContext( bool inContextInitializedSilent);

			VErrorBase*				GetFirst() const;
			VErrorBase*				GetLast() const;
			VError					GetLastError() const;

			VErrorBase*				Find( VError inError) const;

			bool					IsErrorFiltered( VError inError) const;
			void					Flush();

			// combine all contexts
			void					GetErrors( VErrorContext& inContext) const;

			bool					FailedForMemory() const;

			// optim: to avoid new/delete of VErrorContext, we try to keep popped contexts for reuse
			void					RecycleContext( VErrorContext *inContext);

	static	void					BuildErrorStack(VValueBag& outBag);

private:
			VErrorTaskContext( const VErrorTaskContext& /*inContext*/) {};

			VErrorContext*			fRecycledCleanContext;
			VectorOfVErrorContext	fStack;
};


/*!
	@class StErrorContextInstaller
	@abstract Automatic handling class for error contexts
	@discussion
		Constructor installs a clean error context, resetting LastError to VE_OK.
		Destructor deinstall the context and, if we don't keep the errors, reset lasterror to what it was.
*/

class XTOOLBOX_API StErrorContextInstaller
{
public:
			/** @brief if inKeepingErrors is true, all errors are pushed in previous context by ~StErrorContextInstaller **/
			/** @brief if inKeepingErrors is false, all errors dropped. **/
			// inSilentContext means no debug window is shown when an error is thrown.
			explicit				StErrorContextInstaller()												{ _Init( true, false);}
			explicit				StErrorContextInstaller( bool inKeepingErrors)							{ _Init( inKeepingErrors, !inKeepingErrors);}
			explicit				StErrorContextInstaller( bool inKeepingErrors, bool inSilentContext)	{ _Init( inKeepingErrors, inSilentContext);}

			/** @brief By default, errors are kept but if any error in the list is thrown, all errors are dopped by ~StErrorContextInstaller **/
			// the error list must be terminated with VE_OK.
			explicit				StErrorContextInstaller( VError inFirstFilteredError, VError inFilteredError, ...);
									~StErrorContextInstaller();

			VErrorContext*			GetContext() const								{ return fContext; }

			VError					GetLastError() const							{ return fContext->GetLastError(); }
			void					Flush()											{ fContext->Flush(); }
			void					MergeAndFlush()									{ fTaskContext->MergeAndFlushLastContext( fContextInitializedSilent); }

									operator VError() const							{ return GetLastError(); }

private:
			void					_Init( bool inKeepingErrors, bool inSilentContext);

			VErrorTaskContext*		fTaskContext;
			VErrorContext*			fContext;
			bool					fContextInitializedSilent;
};



END_TOOLBOX_NAMESPACE

#endif
