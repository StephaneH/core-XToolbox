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
#ifndef _MURMURHASH_H_
#define _MURMURHASH_H_

#include "VTypes.h"

BEGIN_TOOLBOX_NAMESPACE

// This is the 32-bit implementation for 64-bit hashes
XTOOLBOX_API uLONG8 MurmurHash64B ( const void * key, int len, unsigned int seed );
// This is the 64-bit implementation for 64-bit values
XTOOLBOX_API uLONG8 MurmurHash64A ( const void * key, int len, unsigned int seed );
// This is the 32-bit implementation for 32-bit values
XTOOLBOX_API unsigned int MurmurHash2 ( const void * key, int len, unsigned int seed );

// This is a simplified version of the 64-bit murmur hash function.  It doesn't
// take a seed (it uses a constant prime number instead), and it automatically
// picks the best murmur variant based on the target architecture.
XTOOLBOX_API uLONG8 SimpleMurmurHash64( const void *key, int len );

END_TOOLBOX_NAMESPACE

#endif // _MURMURHASH_H_