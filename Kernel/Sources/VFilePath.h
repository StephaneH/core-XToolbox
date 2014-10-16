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
#ifndef __VFilePath__
#define __VFilePath__

#include "Kernel/Sources/VString.h"


BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VFile;
class VFolder;

class XTOOLBOX_API VFilePath : public VObject
{
public:
								VFilePath():fIsValid( false)	{;}
								VFilePath( const VFilePath& inFullPath);
	explicit					VFilePath( const VString& inFullPath, FilePathStyle inFullPathStyle = FPS_DEFAULT);
								VFilePath( const VFilePath& inBaseFolder, const VString& inRelativPath, FilePathStyle inRelativPathStyle = FPS_DEFAULT);

	virtual	CharSet				DebugDump( char *inTextBuffer, sLONG& inBufferSize) const;

			void				FromFilePath( const VFilePath& inFilePath);
			void				FromFullPath( const VString& inFullPath, FilePathStyle inFullPathStyle = FPS_DEFAULT);
			
	// always use base folder for FPS_DEFAULT
	// If given FPS_POSIX and a relativePath that appears to be a full path, inBaseFolder is not used.
			void				FromRelativePath( const VFilePath& inBaseFolder, const VString& inRelativPath, FilePathStyle inFullPathStyle = FPS_DEFAULT);
			void				FromRelativePath( const VFolder& inBaseFolder, const VString& inRelativPath, FilePathStyle inRelativPathStyle = FPS_DEFAULT);
	
	// returns full path with current platform conventions ('\' on windows, ':' on mac)
			void				GetPath( VString& outFullPath) const;
			const VString&		GetPath() const				{ return fFullPath;}

	/*
		If this path is a in inBaseFolder, outRelativePath returns the relative path in default path style and the function returns true.
		else the function returns false and outRelativeFullPath returns the full path.
		WARNING: although some file system (linux) are diacritcal, the base folder match is not.
	
		c:\toto\file.htm relative to c:\toto\ -> file.html
		c:\toto\tata\file.htm relative to c:\toto\ -> tata\file.html
		macintosh hd:toto:tata:file.htm relative to macintosh hd:toto: -> tata:file.html
	*/
			bool				GetRelativePath( const VFilePath& inBaseFolder, VString& outRelativeOrFullPath) const;
			
	/*
		Same as GetRelativePath but the relative or full path returned is in posix notation.

		c:\toto\file.htm relative to c:\toto\ -> file.html
		c:\toto\tata\file.htm relative to c:\toto\ -> tata/file.html
	*/
			bool				GetRelativePosixPath( const VFilePath& inBaseFolder, VString& outRelativeOrFullPath) const;

	// returns full path using Unix conventions ('/')
			void				GetPosixPath( VString& outFullPath) const;
		
			bool				IsEmpty() const				{ return fFullPath.IsEmpty();}
			bool				IsValid() const				{ return fIsValid;}

			// on peut construire un VFile avec
			bool				IsFile() const;
			
			// on peut construire un VFolder avec
			bool				IsFolder() const;
	
			// get folder or file's name
			void				GetName( VString& outName) const;

			/*
				change the filename if it's a file
				change the foldername if it's a folder
				-> use SetFileName if you don't want to change folder names
			*/
			bool				SetName( const VString& inName);
	
			// get file's name or "" if a folder
			void				GetFileName( VString& outName, bool inWithExtension = true) const;

			/** @brief If it's a folder, append the file name. If it's a file, change the file name 
				@param inWithExtension If false, the extension will be preserved if it exists. No effect if the path is a folder. */
			bool				SetFileName( const VString& inName, bool inWithExtension = true);

			// get folder's name or "" if a file
			void				GetFolderName( VString& outName, bool inWithExtension = true) const;

			/** @brief If it's a folder, change the folder name. No effect if the path is a file.
				@param inWithExtension If false, the extension will be preserved if it exists. */
			bool				SetFolderName( const VString& inName, bool inWithExtension = true);
			
			/** @brief must be a file or a folder (no dot) */
			void				GetExtension( VString& outExtension) const;
			
			/** @brief tells if the path ends with specified extension (no dot)
			Must be a file or a folder.
			Warning: some file system are diacritical, some are not. VFile::MatchExtension knows that. */
			bool				MatchExtension( const VString& inExtension, bool inDiacritical) const;

			// must be a file or a folder
			bool				SetExtension( const VString& inExtension);

			/*
				Que l'on pointe sur un fichier ou un dossier,
				on retourne le dossier contenant
				
				c:\toto\file.htm -> c:\toto\
				c:\toto\ -> c:\
				
				retourne false et chaine vide si l'operation echoue (pas de parent par exemple)
			*/
			bool				GetParent( VFilePath& outParentFullPath) const;
			bool				HasParent() const;

			/*
				Si on pointe sur un fichier, on retourne son dossier.
				si on pointe sur un dossier, on le retourne.
				
				c:\toto\file.htm -> c:\toto\
				c:\toto\ -> c:\toto\
			*/
			bool				GetFolder( VFilePath& outFolderPath) const;

			/*
				up one level.

				Que l'on pointe sur un fichier ou un dossier,
				on pointe le dossier contenant
				
				c:\toto\file.htm -> c:\toto\
				c:\toto\ -> c:\
			*/
			VFilePath&			ToParent();

			/*
				down one level to point to a folder.
				
				the file path MUST be a valid folder.
			*/
			VFilePath&			ToSubFolder( const VString& inFolderName);

			/*
				down one level to point to a file.
				
				the file path MUST be a valid folder.
			*/
			VFilePath&			ToSubFile( const VString& inFileName);
	
			/*
				Si on pointe sur un fichier, on se positionne sur son dossier.
				si on pointe sur un dossier, on ne fait rien.
				
				c:\toto\file.htm -> c:\toto\
				c:\toto\ -> c:\toto\
			*/
			VFilePath&			ToFolder();

			/*
				return true if this is child of inOther.
				inOther must be a folder path.
			*/
			bool				IsChildOf( const VFilePath& inOther) const;
			
			/*
				returns true if this path can be expressed relatively to the other
			*/
			bool				IsSameOrChildOf( const VFilePath& inOther) const;

			/*
				Return the max depth of the current path
			*/
			uLONG				GetDepth();

			void				RemoveBlanks();
			
			void				Clear();
			
			VFilePath&			operator=( const VFilePath& inOther)	{ FromFilePath( inOther); return *this;}

			/* should be platform or disk partition type dependent */
			bool				operator != (const VFilePath& inOther) const { return fFullPath != inOther.GetPath(); }
			bool				operator == (const VFilePath& inOther) const { return fFullPath == inOther.GetPath(); }
			bool				operator < ( const VFilePath &inOther) const { return fFullPath < inOther.GetPath(); }

private:

			bool				_GetParent( VString& ioParentPath) const;
			bool				_GetFolder( VString& ioFolderPath) const;
			bool				_GetRelativePath( const VFilePath& inBaseFolder, VString& outRelativeOrFullPath) const;
	
	static	bool				_ResolveRelative( VString& ioFullPath);
	static	void				_RemoveBlanks( VString& ioPath);
	static	bool				_ResolveStep( VString& inPath, sLONG inFirstChar, sLONG inNextPos, bool& isOK);
	static	bool				_CheckInvalidChars( const VString& inPath);
	
			VString				fFullPath;
	mutable	bool				fIsValid;
};

END_TOOLBOX_NAMESPACE

#endif
