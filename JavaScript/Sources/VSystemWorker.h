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
#ifndef __V_SYSTEM_WORKER__
#define __V_SYSTEM_WORKER__

#include "Kernel/Sources/VFilePath.h"
#include "Kernel/Sources/VString.h"

BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VSystemWorker : public VObject, public IRefCountable
{
public:

	static	VSystemWorker*			Create( const VString &inName, const VFilePath &inPath, VError *outError = NULL);
	
			const VString&			GetName() const						{	return fName;	}
			const VFilePath&		GetPath() const						{	return fPath;	}

private: 
									VSystemWorker( const VString &inName, const VFilePath &inPath);
	virtual							~VSystemWorker() { ; }
									VSystemWorker( const VSystemWorker&);
			VSystemWorker&			operator=(const VSystemWorker&);
			
			VString					fName;
			VFilePath				fPath;
};


class XTOOLBOX_API VSystemWorkerNamespace : public VObject, public IRefCountable
{
public:
									VSystemWorkerNamespace( VSystemWorkerNamespace *inParent = NULL);
	
			void					RegisterSystemWorker( VSystemWorker *inSystemWorker);
			//VError					RegisterSystemWorker( const VString& inName, const VFilePath& inPath);
			void					UnregisterSystemWorker( const VString& inName);
			
			VSystemWorker*			RetainSystemWorker( const VString& inName);
			//VFolder*				RetainSystemWorkerRootFolder( const VString& inName);

			VError					LoadFromDefinitionFile( VFile *inFile, VFileSystemNamespace & inFSNamespace);

			//typedef	std::map<VString, VRefPtr<VFileSystem> >	OrderedMapOfFileSystem;
			//VError					RetainMapOfFileSystems(OrderedMapOfFileSystem& outMap, EFSNOptions inOptions = eFSN_Default);

			// Init application level file systems common to all applications - Studio, Server.
			VError					InitAppFileSystems ( VFileSystemNamespace* inFSNamespace );

			VError					GetPath ( VString const & inName, VFilePath & outPath );

private:
	virtual							~VSystemWorkerNamespace();
									VSystemWorkerNamespace( const VSystemWorkerNamespace&);
			VSystemWorkerNamespace&	operator=( const VSystemWorkerNamespace&);

	typedef	unordered_map_VString<VRefPtr<VSystemWorker> >	MapOfSystemWorkers;
	
			VCriticalSection		fMutex;
			MapOfSystemWorkers		fMap;
			VSystemWorkerNamespace*	fParent;
};


END_TOOLBOX_NAMESPACE

#endif
