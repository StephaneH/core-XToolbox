
#include "VKernelIPCPrecompiled.h"
#include "VCommandLineParserExtension.h"
#include <ctype.h>
#include <stdlib.h> // _strtoui64 or strtoul
#include <cstddef>
#include <locale>
#if !VERSIONWIN
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif



BEGIN_TOOLBOX_NAMESPACE

namespace Extension
{
namespace CommandLineParser
{


	namespace // anonymous
	{

		template<class T> struct StringToInteger {};

		template<> struct StringToInteger<sLONG>
		{
			static inline bool Perform(const char* const inBuffer, int inBase, sLONG& out)
			{
				char* pend = NULL;
				out = (sLONG) strtol(inBuffer, &pend, inBase);
				return !((pend == inBuffer || *pend != '\0' || errno == ERANGE));
			}
		};

		template<> struct StringToInteger<sLONG8>
		{
			static inline bool Perform(const char* const inBuffer, int inBase, sLONG8& out)
			{
				char* pend = NULL;
				#if VERSIONWIN
				out = _strtoi64(inBuffer, &pend, inBase);
				#else
				out = (sLONG8) strtoll(inBuffer, &pend, inBase);
				#endif
				return !((pend == inBuffer || *pend != '\0' || errno == ERANGE));
			}
		};

		template<> struct StringToInteger<uLONG>
		{
			static inline bool Perform(const char* const inBuffer, int inBase, uLONG& out)
			{
				char* pend = NULL;
				out = (uLONG) strtoul(inBuffer, &pend, inBase);
				return !((pend == inBuffer || *pend != '\0' || errno == ERANGE));
			}
		};

		template<> struct StringToInteger<uLONG8>
		{
			static inline bool Perform(const char* const inBuffer, int inBase, uLONG8& out)
			{
				char* pend = NULL;
				#if VERSIONWIN
				out = (uLONG8) _strtoui64(inBuffer, &pend, inBase);
				#else
				out = (uLONG8) strtoull(inBuffer, &pend, inBase);
				#endif
				return !((pend == inBuffer || *pend != '\0' || errno == ERANGE));
			}
		};



		template<class U>
		static inline bool Convert(const VString& inText, U& outResult)
		{
			enum { bufferMaxLen = 31, };

			// convenient alias to the input string
			const VString& text = inText;
			// length of the original string
			unsigned length     = (unsigned) text.GetLength();

			// trim from the left
			unsigned offset = 0;
			for (; offset < length; ++offset)
			{
				if (0 == isspace((int)text[offset]))
					break;
			}

			// trim from the right
			length -= offset;
			while (length != 0 && 0 != isspace((int) text[length - 1]))
				--length;

			if (length > 0 && length < bufferMaxLen)
			{
				char buffer[bufferMaxLen + 1];

				for (unsigned i = 0; i < length; ++i)
				{
					int c = text[i + offset];
					if ((c >= '0' && c <= '9') || (c == '-') || (c == '+'))
						buffer[i] = (char) c;
					else
						return false; // error
				}

				// ensure zero-terminated
				buffer[length] = '\0';

				// perform the conversion
				int base = 0; // automatic 10 or 16
				return StringToInteger<U>::Perform(buffer, base, outResult);
			}
			return false;
		}


	} // anonymous namespace




	bool Argument<sLONG>::Append(sLONG& out, const VString& inValue)
	{
		return Convert(inValue, out);
	}

	bool Argument<uLONG>::Append(uLONG& out, const VString& inValue)
	{
		return Convert(inValue, out);
	}

	bool Argument<sLONG8>::Append(sLONG8& out, const VString& inValue)
	{
		return Convert(inValue, out);
	}

	bool Argument<uLONG8>::Append(uLONG8& out, const VString& inValue)
	{
		return Convert(inValue, out);
	}







	namespace // anonymous
	{

		static inline void ResolveInputAliasFile(VFile*& inFile, const VFilePath& inOriginalPath)
		{
			if (VE_OK != VFile::ResolveAliasFile(&inFile, false))
			{
				XBOX::ReleaseRefCountable(&inFile);
				// reset to the original one
				inFile = new VFile(inOriginalPath);
			}
		}


		#if VERSIONWIN
		/*!
		** \brief Try to normalize a file path according its true nature from the filesystem
		**
		** \param[in,out] ioFilpath A file path
		** \return True if the filepath has been successfully resolved, false if not found (or empty -
		**   ioFilePath will remain untouched if this case)
		*/
		static bool NormalizePathAccordingToFileSystem(VFilePath& ioFilepath)
		{
			bool success = false;

			if (!ioFilepath.IsEmpty())
			{
				if (ioFilepath.IsFolder()) // `is folder` according VFilePath, e.g. with a final separator
				{
					// is a folder according to VFilePath - checking from the filesystem
					VFolder folder(ioFilepath);
					if (folder.Exists())
					{
						success = true;
					}
					else
					{
						// not found, a file maybe ?
						VString newAddr = ioFilepath.GetPath();
						newAddr.Validate(newAddr.GetLength() - 1);
						VFile file(newAddr);
						if (file.Exists())
						{
							ioFilepath.FromFullPath(newAddr);
							success = true;
						}
					}
				}
				else
				{
					// is a file according to VFilePath - checking from the filesystem
					VFile file(ioFilepath);
					if (file.Exists())
					{
						success = true;
					}
					else
					{
						// not found, a file maybe ?
						VString newAddr = ioFilepath.GetPath();
						newAddr += FOLDER_SEPARATOR;
						VFolder folder(newAddr);
						if (folder.Exists())
						{
							ioFilepath.FromFullPath(newAddr);
							success = true;
						}
					}
				}
			}
			return success;
		}
		#endif


		/*!
		** \brief Try to convert a string into a VFilePath according its true nature from the filesystem
		**
		** \param[in] inUrl An arbitrary file path
		** \param[in,out] ioFilpath A file path
		** \return True if the filepath has been successfully resolved, false if not found (or empty)
		**   but in all cases outFilePath will be correctly set
		*/
		static bool NormalizePathAccordingToFileSystem(const VString& inUrl, VFilePath& outFilePath)
		{
			bool success = false;

			if (!inUrl.IsEmpty())
			{
				#if !VERSIONWIN
				{
					// Canonicalize the input url, in removing all '.' and '..', and making the url absolute
					StStringConverter<char> utf8url(inUrl, VTC_UTF_8);
					char* newPath = realpath(utf8url.GetCPointer(), NULL);
					if (newPath != NULL)
					{
						// preparing the new url to use
						VString newUrl;
						newUrl.AppendBlock(newPath, strlen(newPath), VTC_UTF_8);

						// since we already have the filepath in UTF8 (amd we already know that the inode exists),
						// directly checking the true nature of the given url to add a final separator as
						// required by the toolbox to distinguish files and folders
						struct stat sb;
						if (0 == stat(newPath, &sb) && (S_IFDIR == (sb.st_mode & S_IFMT)))
							newUrl += '/';

						free(newPath);
						outFilePath.FromFullPath(newUrl, FPS_POSIX);
						success = true;
					}
					else
					{
						// impossible to resolve from the filesystem (not found, permission denied...)
						// let's continue, it might not be an error
						outFilePath.FromFullPath(inUrl, FPS_POSIX);
					}
				}
				#else
				{
					outFilePath.FromFullPath(inUrl, FPS_DEFAULT);
					success = NormalizePathAccordingToFileSystem(outFilePath);
				}
				#endif
			}
			else
				outFilePath.Clear();

			return success;
		}


	} // anonymous namespace






	bool Argument<VFilePath>::Append(VFilePath& out, const VString& inValue)
	{
		// the given url might not exists - no error should be given for an invalid input
		NormalizePathAccordingToFileSystem(inValue, out);
		return true;
	}



	bool Argument<VRefPtr<VFile> >::Append(VRefPtr<VFile>& out, const VString& inValue)
	{
		VFilePath filepath;
		if (NormalizePathAccordingToFileSystem(inValue, filepath) && filepath.IsFile())
		{
			// the file exists - already checked by NormalizePathAccordingToFileSystem
			VFile* file = new VFile(filepath);
			if (file->IsAliasFile())
			{
				ResolveInputAliasFile(file, filepath);

				if (file->Exists())
				{
					out.Adopt(file);
					return true;
				}
				delete file;
			}
			else
			{
				out.Adopt(file);
				return true;
			}
		}

		out.Clear();
		return false;
	}


	bool Argument<VRefPtr<VFolder> >::Append(VRefPtr<VFolder>& out, const VString& inValue)
	{
		VFilePath filepath;
		if (NormalizePathAccordingToFileSystem(inValue, filepath) && filepath.IsFolder())
		{
			// the folder exists - already checked by NormalizePathAccordingToFileSystem
			VFolder* folder = new VFolder(filepath);
			out.Adopt(folder);
			return true;
		}

		out.Clear();
		return false;
	}






} // namespace CommandLineParser
} // namespace Extension
END_TOOLBOX_NAMESPACE


