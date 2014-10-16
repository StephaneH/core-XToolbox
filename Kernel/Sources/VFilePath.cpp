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
#include "VFilePath.h"
#include "VFile.h"
#include "VFolder.h"
#include "VIntlMgr.h"
#include "VCollator.h"
#include "VURL.h"


VFilePath::VFilePath(const VFilePath& inOther)
: fFullPath( inOther.fFullPath)
, fIsValid( inOther.fIsValid)
{
}


VFilePath::VFilePath(const VString& inFullPath, FilePathStyle inFullPathStyle)
{
	FromFullPath( inFullPath, inFullPathStyle);
}


VFilePath::VFilePath(const VFilePath& inBaseFolder, const VString& inRelativePath, FilePathStyle inRelativePathStyle)
{
	FromRelativePath(inBaseFolder, inRelativePath, inRelativePathStyle);
}


void VFilePath::GetPath(VString& outFullPath) const
{
	outFullPath.FromString(fFullPath);
}


bool VFilePath::_GetRelativePath( const VFilePath& inBaseFolder, VString& outRelativeOrFullPath) const
{
	// warning: non diacritical
	sLONG matchedLength;
	VIndex pos = testAssert( inBaseFolder.IsFolder()) ? VIntlMgr::GetDefaultMgr()->GetCollator()->FindString( fFullPath.GetCPointer(), fFullPath.GetLength(), inBaseFolder.GetPath().GetCPointer(), inBaseFolder.GetPath().GetLength(), false, &matchedLength) : 0;

	if (pos == 1)
	{
		fFullPath.GetSubString( matchedLength + 1, fFullPath.GetLength() - matchedLength, outRelativeOrFullPath);
	}
	return (pos == 1);
}


bool VFilePath::GetRelativePath( const VFilePath& inBaseFolder, VString& outRelativeOrFullPath) const
{
	bool isRelative = _GetRelativePath( inBaseFolder, outRelativeOrFullPath);
	if (!isRelative)
	{
		outRelativeOrFullPath = fFullPath;
	}

	return isRelative;
}


bool VFilePath::GetRelativePosixPath( const VFilePath& inBaseFolder, VString& outRelativeOrFullPath) const
{
	bool isRelative = _GetRelativePath( inBaseFolder, outRelativeOrFullPath);

	if (!isRelative)
	{
		GetPosixPath( outRelativeOrFullPath);
	}
	else
	{
		outRelativeOrFullPath.ExchangeAll( FOLDER_SEPARATOR, CHAR_SOLIDUS);
	}

	return isRelative;
}


bool VFilePath::_ResolveRelative(VString& ioFullPath)
{
	bool isOK = true;
		
	// find top level
	// on windows skip server name and drive letter
	// on mac and windows check there's no leading separator
	
	sLONG pos = 1;
	#if VERSIONWIN
	if (ioFullPath.GetLength() >= 2) {
		// check UNC prefix
		if (ioFullPath[0] == FOLDER_SEPARATOR && ioFullPath[1] == FOLDER_SEPARATOR) {
			// relative analysis cant only begin after the share name
			pos = ioFullPath.FindUniChar(FOLDER_SEPARATOR, 3, false);
			if (pos <= 0)
				pos = 3;
			else if (pos == 3) {
				// three separators: no good
				isOK = false;
			} else
				pos = pos + 1;
	
		// check drive letter
		} else if (ioFullPath[1] == CHAR_COLON) {
			// check trailing separator
			if (ioFullPath.GetLength() == 2)
				ioFullPath.AppendUniChar(FOLDER_SEPARATOR);
			else if (ioFullPath[2] != FOLDER_SEPARATOR)
				isOK = false;
			else
				pos = 4;
		}
	}
	#endif
	
	#if !VERSION_LINUX
	// no leading FOLDER_SEPARATOR
	if (isOK && pos <= ioFullPath.GetLength()) {
		if (ioFullPath[ pos-1] == FOLDER_SEPARATOR)
			isOK = false;
	}
	#endif
	
	// starts relativ analysis
	if (isOK && pos <= ioFullPath.GetLength()) {
		_ResolveStep(ioFullPath, pos, ioFullPath.FindUniChar(FOLDER_SEPARATOR, pos, false), isOK);
	}
	
	#if !VERSION_LINUX
	// still no leading FOLDER_SEPARATOR
	if (isOK && pos <= ioFullPath.GetLength()) {
		if (ioFullPath[ pos-1] == FOLDER_SEPARATOR)
			isOK = false;
	}
	#endif
	
	return isOK;
}


void VFilePath::_RemoveBlanks(VString& ioPath)
{
	// remove leading blanks
	while(ioPath.GetLength() > 0)
	{
		if (!VIntlMgr::GetDefaultMgr()->IsSpace(ioPath[ 0]))
			break;
		ioPath.SubString(2, ioPath.GetLength()-1);
	}

	// remove trailing blanks
	while(ioPath.GetLength() > 0)
	{
		if (!VIntlMgr::GetDefaultMgr()->IsSpace(ioPath[ ioPath.GetLength()-1]))
			break;
		ioPath.SubString(1, ioPath.GetLength()-1);
	}
	
	sLONG pos = 0;
	while(pos < ioPath.GetLength())
	{
		pos = ioPath.FindUniChar(FOLDER_SEPARATOR, pos + 1, false);
		if (pos <= 0)
			break;
		
		// remove blanks after the separator
		sLONG i = pos + 1;
		while(i < ioPath.GetLength())
		{
			if (!VIntlMgr::GetDefaultMgr()->IsSpace(ioPath[ i-1]))
				break;
			ioPath.Remove(i, 1);
		}
		
		// remove blanks before the separator
		i = pos;
		while(--i > 0)
		{
			if (!VIntlMgr::GetDefaultMgr()->IsSpace(ioPath[ i-1]))
				break;
			ioPath.Remove(i, 1);
			--pos;
		}
	}
}


bool VFilePath::_ResolveStep(VString& inPath, sLONG inFirstChar, sLONG inNextPos, bool& ioOK)
{
bool isOneResolved = false;

	while(ioOK && (inNextPos > 0) ) {
		
		#if VERSIONMAC
		// look for :: sequences
		// hd:a::b -> hd:b
		// /hd/a/../b
		if ((inNextPos + 1 <= inPath.GetLength() ) && (inPath[ inNextPos] == FOLDER_SEPARATOR)) {
			inPath.Remove(inFirstChar, inNextPos-inFirstChar+2);
			isOneResolved = true;
			break;
		}
		#endif
		
		#if VERSIONWIN == 1 || VERSION_LINUX == 1
		// removes \.\ sequences
		while(   (inNextPos + 2 <= inPath.GetLength() )
			   && (inPath[inNextPos] == CHAR_FULL_STOP)
			   && (inPath[inNextPos + 1] == FOLDER_SEPARATOR))
		{
			inPath.Remove(inNextPos + 1, 2);
		}

		// Even if "//" sequences are accepted by Mac and Linux platforms or "\\\\" for Windows platform, 
		// reject them as bad programming practice.

		// breaks on invalid \\ sequence
		if (   (inNextPos + 1 <= inPath.GetLength() )
			 && (inPath[inNextPos] == FOLDER_SEPARATOR))
		{
			ioOK = false;
			break;
		}

		// look for \..\ sequence
		if (	(inNextPos + 3 <= inPath.GetLength() )
			 && (inPath[inNextPos] == CHAR_FULL_STOP)
			 && (inPath[inNextPos + 1] == CHAR_FULL_STOP)
			 && (inPath[inNextPos + 2] == FOLDER_SEPARATOR))
		{
			inPath.Remove(inFirstChar, inNextPos-inFirstChar+4);	// sc 16/07/2009 bug fix, was inPath.Remove(inFirstChar+1, inNextPos-inFirstChar+3)
			isOneResolved = true;
			break;
		}
		#endif

		if (inNextPos >= inPath.GetLength())
			break;

		if (_ResolveStep(inPath, inNextPos + 1, inPath.FindUniChar(FOLDER_SEPARATOR, inNextPos + 1), ioOK))		// sc 16/07/2009 bug fix, was _ResolveStep(inPath, inNextPos, inPath.FindUniChar(FOLDER_SEPARATOR, inNextPos + 1), ioOK)
			inNextPos = inPath.FindUniChar(FOLDER_SEPARATOR, inFirstChar);
		else
			inNextPos = 0;
	}
	return isOneResolved;
}


bool VFilePath::_CheckInvalidChars(const VString& /*inPath*/)
{
	bool isOK = true;
	#if VERSIONMAC
	// the finder maps ':' by '-' in filenames, anything else seems allowed
	//inPath.FindUniChar(inChar);
	#endif
	#if VERSIONWIN
	#endif
	return isOK;
}


void VFilePath::GetPosixPath( VString& outFullPath) const
{
	if (!fIsValid) 
		outFullPath.Clear();
	else
	{
		outFullPath = fFullPath;

		// on windows: c:\dfdfsdf\dd.fgg
		// on mac: macintosh hd:system:folder:dd
		// on unix: /user/bin/dd.fgg
#if VERSIONWIN
		UniChar *p = outFullPath.GetCPointerForWrite();
		for(sLONG i = outFullPath.GetLength() - 1 ;  i >= 0 ; --i)
		{
			if (p[i] == FOLDER_SEPARATOR)
				p[i] = CHAR_SOLIDUS;
		}
		// do not prepend with a '/' if a drive or a UNC server name is specified
		if ((outFullPath.GetLength()) < 2 
			|| ((outFullPath[1] != CHAR_COLON) && (outFullPath[0] != CHAR_SOLIDUS || outFullPath[1] != CHAR_SOLIDUS)) )
			outFullPath.Insert(CHAR_SOLIDUS, 1);
#elif VERSIONMAC
		//OR 10/10/2012 ACI0076393: do not assume all HFS paths are rooted in /Volumes/, let OS figure out where the path root/mount point is
		//Folder separator conversions between HFS and posix are also handled in VURL::Convert()
		VURL::Convert(outFullPath,XBOX::eURL_HFS_STYLE,XBOX::eURL_POSIX_STYLE,false);
#endif
	}
}


bool VFilePath::GetParent(VFilePath& outParentFullPath) const
{
	outParentFullPath.fFullPath.FromString(fFullPath);
	if (!fIsValid || !_GetParent(outParentFullPath.fFullPath))
		outParentFullPath.fFullPath.Clear();

	outParentFullPath.fIsValid = outParentFullPath.fFullPath.GetLength() > 0;
		
	return outParentFullPath.fIsValid;
}


VFilePath& VFilePath::ToParent()
{
	if (fIsValid)
	{
		fIsValid = _GetParent( fFullPath);
	}
	
	return *this;
}


VFilePath& VFilePath::ToFolder()
{
	if (fIsValid)
	{
		fIsValid = _GetFolder( fFullPath);
	}
	
	return *this;
}


bool VFilePath::IsChildOf( const VFilePath& inOther) const
{
	xbox_assert( inOther.IsFolder());
	return IsValid() && inOther.IsFolder() && fFullPath.BeginsWith( inOther.fFullPath, false) && (fFullPath != inOther.fFullPath);
}


bool VFilePath::IsSameOrChildOf( const VFilePath& inOther) const
{
	xbox_assert( inOther.IsFolder());
	return IsValid() && inOther.IsFolder() && fFullPath.BeginsWith( inOther.fFullPath, false);
}


uLONG VFilePath::GetDepth()
{
	uLONG depth = 0;
	if ( fIsValid )
	{
		depth = 1;
		XBOX::VString path(fFullPath);
		while ( _GetParent( path ) )
			depth++;
	}
	return depth;
}

void VFilePath::RemoveBlanks()
{
	_RemoveBlanks( fFullPath );
}

CharSet VFilePath::DebugDump(char *inTextBuffer, sLONG& inBufferSize) const
{
	return fFullPath.DebugDump(inTextBuffer, inBufferSize);
}


bool VFilePath::_GetParent(VString& ioParentPath) const
{
	sLONG len = ioParentPath.GetLength();
	if (len <= 1) {
		// ":" ou "a"
		ioParentPath.Clear(); // pas de parent
	} else {
		sLONG pos = ioParentPath.FindUniChar(FOLDER_SEPARATOR, ioParentPath.GetLength() -1, true); // on se fiche du dernier char
		if (pos > 0)
			ioParentPath.SubString(1, pos); // inclue le folder separator
		else
			ioParentPath.Clear(); // pas de parent
	}
	
	return ioParentPath.GetLength() > 0;
}


bool VFilePath::HasParent() const
{
	if (!fIsValid)
		return false;

	bool ok = fIsValid;
	VIndex len = fFullPath.GetLength();
	if (len <= 1)
	{
		// ":" ou "a"
		ok = false; // pas de parent
	}
	else
	{
		sLONG pos = fFullPath.FindUniChar(FOLDER_SEPARATOR, fFullPath.GetLength() -1, true); // on se fiche du dernier char
		if (pos > 0)
			ok = true;
		else
			ok = false; // pas de parent
	}
	
	return ok;
}


void VFilePath::FromFilePath(const VFilePath& inFilePath)
{
	inFilePath.GetPath(fFullPath);
	fIsValid = inFilePath.IsValid();
}


bool VFilePath::SetName(const VString& inName)
{
	xbox_assert(inName.FindUniChar(FOLDER_SEPARATOR) <= 0);

	if (!IsValid())
		return false;
	
	if (IsFile())
	{
		ToFolder();
		if (IsValid())
			fFullPath += inName;
	}
	else
	{
		ToParent();
		if (IsValid())
		{
			fFullPath += inName;
			fFullPath.AppendUniChar(FOLDER_SEPARATOR);
		}
	}
	return IsValid();
}


bool VFilePath::GetFolder( VFilePath& outFolderPath) const
{
	outFolderPath.fFullPath.FromString(fFullPath);
	if (!fIsValid || !_GetFolder(outFolderPath.fFullPath))
		outFolderPath.fFullPath.Clear();
	
	outFolderPath.fIsValid = outFolderPath.fFullPath.GetLength() > 0;
		
	return outFolderPath.fIsValid;
}


void VFilePath::Clear()
{
	fFullPath.Clear();
	fIsValid = false;
}


bool VFilePath::_GetFolder( VString& ioFolderPath) const
{
	sLONG pos = ioFolderPath.FindUniChar(FOLDER_SEPARATOR, ioFolderPath.GetLength(), true);
	if (pos > 0) {
		if (pos != ioFolderPath.GetLength()) {
			ioFolderPath.SubString(1, pos); // inclue le folder separator
		}
	} else
		ioFolderPath.Clear(); // pas de parent
	
	return ioFolderPath.GetLength() > 0;
}


bool VFilePath::SetExtension(const VString& inExtension)
{
	xbox_assert(inExtension.FindUniChar(CHAR_FULL_STOP) <= 0);
	xbox_assert(inExtension.FindUniChar(FOLDER_SEPARATOR) <= 0);

	sLONG pos = fFullPath.FindUniChar(CHAR_FULL_STOP, fFullPath.GetLength(), true);
	sLONG posMin = fFullPath.FindUniChar(FOLDER_SEPARATOR, fFullPath.GetLength(), true);

	// on ne doit pas aller plus loin qu'un folder separator
	if (posMin > pos)
		pos = -1;

	if (inExtension.IsEmpty())
	{
		// remove existing extension if any
		if (pos > 0)
			fFullPath.SubString( 1, pos-1);
	}
	else if (pos == fFullPath.GetLength())
	{
		// point en derniere position
		fFullPath += inExtension;
	}
	else if (pos > 0)
	{
		// extension non vide
		fFullPath.Replace(inExtension, pos + 1, fFullPath.GetLength()-pos);
	}
	else
	{
		// pas d'extension
		fFullPath.AppendUniChar(CHAR_FULL_STOP);
		fFullPath += inExtension;
	}
	return fIsValid;
}


void VFilePath::FromFullPath( const VString& inFullPath, FilePathStyle inFullPathStyle)
{
	fFullPath.FromString(inFullPath);
	if (inFullPathStyle == FPS_POSIX)
		VURL::Convert( fFullPath, eURL_POSIX_STYLE, eURL_NATIVE_STYLE, false);

	fIsValid = (fFullPath.GetLength() > 0) ? _ResolveRelative(fFullPath) : false;

}


void VFilePath::FromRelativePath(const VFolder& inBaseFolder, const VString& inRelativePath, FilePathStyle inRelativePathStyle)
{
	FromRelativePath( inBaseFolder.GetPath(), inRelativePath, inRelativePathStyle);
}


VFilePath& VFilePath::ToSubFolder( const VString& inFolderName)
{
	xbox_assert(inFolderName.FindUniChar( FOLDER_SEPARATOR) <= 0);

	if (testAssert( IsFolder()))
	{
		fFullPath += inFolderName;
		fFullPath.AppendUniChar( FOLDER_SEPARATOR);
	}
	
	return *this;
}


VFilePath& VFilePath::ToSubFile( const VString& inFileName)
{
	xbox_assert(inFileName.FindUniChar(FOLDER_SEPARATOR) <= 0);

	if (testAssert( IsFolder()))
	{
		fFullPath += inFileName;
	}
	
	return *this;
}


void VFilePath::GetFileName( VString& outName, bool inWithExtension) const
{
	/*
	returns an empty string if it's not a file
	*/
	if (!IsFile())
		outName.Clear();
	else
	{
		VIndex pos_start = fFullPath.FindUniChar( FOLDER_SEPARATOR, fFullPath.GetLength(), true);
		if (testAssert(pos_start > 0))
		{
			VIndex pos_end;
			if (!inWithExtension)
			{
				pos_end = fFullPath.FindUniChar( CHAR_FULL_STOP, fFullPath.GetLength(), true);
				if (pos_end <= pos_start)
					pos_end = fFullPath.GetLength() + 1;
			}
			else
			{
				pos_end = fFullPath.GetLength() + 1;
			}

			fFullPath.GetSubString( pos_start + 1, pos_end - pos_start - 1, outName);
		}
		else
		{
			// ne devrait pas arriver: un fichier a toujours un contenant
			outName.FromString( fFullPath);
		}
	}
}


bool VFilePath::SetFileName( const VString& inName, bool inWithExtension)
{
	xbox_assert(inName.FindUniChar(FOLDER_SEPARATOR) <= 0);

	if (!IsValid())
		return false;
	
	if (IsFolder())
	{
		fFullPath += inName;
	}
	else
	{
		// Preserve the extension if inWithExtension equals false
		VStr63 extension;
		if(!inWithExtension)
			GetExtension(extension);
		ToFolder();
		if (IsValid()){
			fFullPath += inName;
			if(!inWithExtension && extension.GetLength() > 0){
				fFullPath += CHAR_FULL_STOP;
				fFullPath += extension;
			}
		}
	}
	return IsValid();
}


void VFilePath::GetFolderName( VString& outName, bool inWithExtension) const
{
	/*
	returns an empty string if it's not a file
	*/
	if (!IsFolder())
		outName.Clear();
	else
	{
		// not the same code as GetFileName because of the trailing FOLDER_SEPARATOR
		VIndex pos_start = fFullPath.FindUniChar( FOLDER_SEPARATOR, fFullPath.GetLength() - 1, true);
		if (testAssert(pos_start > 0))
		{
			VIndex pos_end;
			if (!inWithExtension)
			{
				pos_end = fFullPath.FindUniChar( CHAR_FULL_STOP, fFullPath.GetLength(), true);
				if (pos_end <= pos_start)
					pos_end = fFullPath.GetLength();
			}
			else
			{
				pos_end = fFullPath.GetLength();
			}
			
			fFullPath.GetSubString( pos_start + 1, pos_end - pos_start - 1, outName);
		}
		else
		{
			// ne devrait pas arriver: un fichier a toujours un contenant
			outName.FromString( fFullPath);
		}
	}
}


bool VFilePath::SetFolderName( const VString& inName, bool inWithExtension)
{
	if (!IsValid() || !IsFolder())
		return false;
	
	// Preserve the extension if inWithExtension equals false
	VStr63 extension;
	if(!inWithExtension)
		GetExtension(extension);

	ToParent();
	if (IsValid())
	{
		fFullPath += inName;
		if(!inWithExtension && extension.GetLength() > 0)
		{
			fFullPath += CHAR_FULL_STOP;
			fFullPath += extension;
		}
		fFullPath.AppendUniChar(FOLDER_SEPARATOR);
	}

	return IsValid();
}


bool VFilePath::IsFolder() const
{
	if (!fIsValid || fFullPath.GetLength() == 0)
		return false;
	return fFullPath[ fFullPath.GetLength()-1] == FOLDER_SEPARATOR;
}


bool VFilePath::IsFile() const
{
	if (!fIsValid || fFullPath.GetLength() == 0)
		return false;
	return fFullPath[ fFullPath.GetLength()-1] != FOLDER_SEPARATOR;
}


void VFilePath::GetName(VString& outName) const
{
	// get file or folder name
	sLONG pos = fFullPath.FindUniChar(FOLDER_SEPARATOR, fFullPath.GetLength(), true);
	if (pos > 0) {
		VString temp(fFullPath);
		if (pos != fFullPath.GetLength()) {
			// file name
			temp.SubString(pos + 1, temp.GetLength()-pos);
		} else if (pos > 1) {
			// folder name
			sLONG pos0 = fFullPath.FindUniChar(FOLDER_SEPARATOR, pos-1, true);
			if (pos0 > 0)
				temp.SubString(pos0 + 1, pos-pos0-1);
			else
				temp.SubString(1, pos-1);
		} else {
			// it's ":"
			temp.Clear();
		}
		outName.FromString(temp);
	} else
		outName.FromString(fFullPath);
}


void VFilePath::GetExtension(VString& outExtension) const
{
	xbox_assert(IsValid());
	
	sLONG pos = fFullPath.FindUniChar(CHAR_FULL_STOP, fFullPath.GetLength(), true);
	sLONG posMin;
	if (IsFolder())
		posMin = (fFullPath.GetLength() > 1) ? fFullPath.FindUniChar(FOLDER_SEPARATOR, fFullPath.GetLength() - 1, true) : -1;
	else
		posMin = fFullPath.FindUniChar(FOLDER_SEPARATOR, fFullPath.GetLength(), true);

	// pas de point en derniere position
	// et on ne doit pas aller plus loin qu'un folder separator
	if ((pos > 0) && (pos != fFullPath.GetLength()) && (pos > posMin))
	{
		if (IsFolder())
			fFullPath.GetSubString(pos + 1, fFullPath.GetLength()-pos-1, outExtension);
		else
			fFullPath.GetSubString(pos + 1, fFullPath.GetLength()-pos, outExtension);
	} else
		outExtension.Clear();
}


bool VFilePath::MatchExtension( const VString& inExtension, bool inDiacritical) const
{
	xbox_assert( IsValid());
	VStr31 extension( CHAR_FULL_STOP);
	extension += inExtension;
	if (IsFolder())
		extension += FOLDER_SEPARATOR;
	return fFullPath.EndsWith( extension, inDiacritical);
}


void VFilePath::FromRelativePath( const VFilePath& inBaseFolder, const VString& inRelativePath, FilePathStyle inRelativePathStyle)
{
	if (inRelativePathStyle == FPS_SYSTEM)
	{
		inBaseFolder.GetPath( fFullPath);
		fFullPath += inRelativePath;
	}
	else if (testAssert( inRelativePathStyle == FPS_POSIX))
	{
		bool isFullPath = !inRelativePath.IsEmpty() && (inRelativePath[0] == '/');

		// VFilePath::GetPosixPath produces c:/folder/file
		// this is a convention accepted by xerces, php and others 
		#if VERSIONWIN
		if ( (inRelativePath.GetLength() > 1) && (inRelativePath[1] == ':') )
			isFullPath = true;
		#endif

		if (isFullPath)
		{
			// it's actually a full path
			fFullPath = inRelativePath;
			VURL::Convert( fFullPath, eURL_POSIX_STYLE, eURL_NATIVE_STYLE, false);
		}
		else
		{
			inBaseFolder.GetPath( fFullPath);
			VString relativePath( inRelativePath);
			VURL::Convert( relativePath, eURL_POSIX_STYLE, eURL_NATIVE_STYLE, true);
			fFullPath += relativePath;
		}
	}
	fIsValid = (fFullPath.GetLength() > 0) ? _ResolveRelative(fFullPath) : false;
}



