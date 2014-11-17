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
/*
 Type Declaration Dictionary Keys 
 The following keys are used in type declarations
 COPY OF THE STANDARD MAC OS X UTType.h HEADER
*/

#ifndef _4DUTTYPE_H_
#define _4DUTTYPE_H_

BEGIN_TOOLBOX_NAMESPACE

/*
 *  kUTExportedTypeDeclarationsKey4D
 *  
 *  Availability:
 *    Mac OS X:         in version 10.3 and later in ApplicationServices.framework
 *    CarbonLib:        not available in CarbonLib 1.x
 *    Non-Carbon CFM:   not available
 */
const VString kUTExportedTypeDeclarationsKey4D("UTExportedTypeDeclarations");

/*
 *  kUTImportedTypeDeclarationsKey4D
 *  
 *  Availability:
 *    Mac OS X:         in version 10.3 and later in ApplicationServices.framework
 *    CarbonLib:        not available in CarbonLib 1.x
 *    Non-Carbon CFM:   not available
 */
const VString kUTImportedTypeDeclarationsKey4D("UTImportedTypeDeclarations");

/*
 *  kUTTypeIdentifierKey4D
 *  
 *  Availability:
 *    Mac OS X:         in version 10.3 and later in ApplicationServices.framework
 *    CarbonLib:        not available in CarbonLib 1.x
 *    Non-Carbon CFM:   not available
 */
const VString kUTTypeIdentifierKey4D("UTTypeIdentifier");

/*
 *  kUTTypeTagSpecificationKey4D
 *  
 *  Availability:
 *    Mac OS X:         in version 10.3 and later in ApplicationServices.framework
 *    CarbonLib:        not available in CarbonLib 1.x
 *    Non-Carbon CFM:   not available
 */
const VString kUTTypeTagSpecificationKey4D("UTTypeTagSpecification");

/*
 *  kUTTypeConformsToKey4D
 *  
 *  Availability:
 *    Mac OS X:         in version 10.3 and later in ApplicationServices.framework
 *    CarbonLib:        not available in CarbonLib 1.x
 *    Non-Carbon CFM:   not available
 */
const VString kUTTypeConformsToKey4D("UTTypeConformsTo");

/*
 *  kUTTypeDescriptionKey4D
 *  
 *  Availability:
 *    Mac OS X:         in version 10.3 and later in ApplicationServices.framework
 *    CarbonLib:        not available in CarbonLib 1.x
 *    Non-Carbon CFM:   not available
 */
const VString kUTTypeDescriptionKey4D("UTTypeDescription");
/*
 *  kUTTypeIconFileKey4D
 *  
 *  Availability:
 *    Mac OS X:         in version 10.3 and later in ApplicationServices.framework
 *    CarbonLib:        not available in CarbonLib 1.x
 *    Non-Carbon CFM:   not available
 */
const VString kUTTypeIconFileKey4D("UTTypeIconFile");

/*
 *  kUTTypeReferenceURLKey4D
 *  
 *  Availability:
 *    Mac OS X:         in version 10.3 and later in ApplicationServices.framework
 *    CarbonLib:        not available in CarbonLib 1.x
 *    Non-Carbon CFM:   not available
 */
const VString kUTTypeReferenceURLKey4D("UTTypeReferenceURL");

/*
 *  kUTTypeVersionKey4D
 *  
 *  Availability:
 *    Mac OS X:         in version 10.3 and later in ApplicationServices.framework
 *    CarbonLib:        not available in CarbonLib 1.x
 *    Non-Carbon CFM:   not available
 */
const VString kUTTypeVersionKey4D("UTTypeVersion");


/*
 Type Tag Classes
 
 The following constant strings identify tag classes for use 
 when converting uniform type identifiers to and from
 equivalent tags.
 */
/*
 *  kUTTagClassFilenameExtension
 *  
 *  Availability:
 *    Mac OS X:         in version 10.3 and later in ApplicationServices.framework
 *    CarbonLib:        not available in CarbonLib 1.x
 *    Non-Carbon CFM:   not available
 */
const VString kUTTagClassFilenameExtension4D("public.filename-extension");

/*
 *  kUTTagClassMIMEType
 *  
 *  Availability:
 *    Mac OS X:         in version 10.3 and later in ApplicationServices.framework
 *    CarbonLib:        not available in CarbonLib 1.x
 *    Non-Carbon CFM:   not available
 */
const VString kUTTagClassMIMEType4D("public.mime-type");

/*
 *  kUTTagClassNSPboardType
 *  
 *  Availability:
 *    Mac OS X:         in version 10.3 and later in ApplicationServices.framework
 *    CarbonLib:        not available in CarbonLib 1.x
 *    Non-Carbon CFM:   not available
 */
//const VString kUTTagClassNSPboardType("UTTagClassNSPboardType");

/*
 *  kUTTagClassOSType
 *  
 *  Availability:
 *    Mac OS X:         in version 10.3 and later in ApplicationServices.framework
 *    CarbonLib:        not available in CarbonLib 1.x
 *    Non-Carbon CFM:   not available
 */
const VString kUTTagClassOSType4D("com.apple.ostype");


END_TOOLBOX_NAMESPACE

#endif
