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
#ifndef __VJSRuntime_Image__
#define __VJSRuntime_Image__

#include "JS4D.h"
#include "VJSClass.h"

BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API VJSImage : public XBOX::VJSClass<VJSImage, VJSPictureContainer>
{
public:
	typedef VJSClass<VJSImage, VJSPictureContainer>	inherited;

	static	void			Initialize( const XBOX::VJSParms_initialize& inParms, VJSPictureContainer* inPict);
	static	void			Finalize( const XBOX::VJSParms_finalize& inParms, VJSPictureContainer* inPict);
	static	void			GetProperty( XBOX::VJSParms_getProperty& ioParms, VJSPictureContainer* inPict);
	static	bool			SetProperty( XBOX::VJSParms_setProperty& ioParms, VJSPictureContainer* inPict);
	static	void			GetPropertyNames( XBOX::VJSParms_getPropertyNames& ioParms, VJSPictureContainer* inPict);
	static	void			GetDefinition( ClassDefinition& outDefinition);

	static void _getPath(XBOX::VJSParms_callStaticFunction& ioParms, VJSPictureContainer* inPict);  // File : getPath()
	static void _setPath(XBOX::VJSParms_callStaticFunction& ioParms, VJSPictureContainer* inPict);  // setPath(File : path, {string : mime/type})
	static void _saveMeta(XBOX::VJSParms_callStaticFunction& ioParms, VJSPictureContainer* inPict);  // saveMeta({metadata})
	static void _save(XBOX::VJSParms_callStaticFunction& ioParms, VJSPictureContainer* inPict);  // bool : save(File, string : mime type)

#if !VERSION_LINUX
	static void _thumbnail(XBOX::VJSParms_callStaticFunction& ioParms, VJSPictureContainer* inPict);  // Image : thumbnail(number : widht, number : heigth,  mode)
#endif

	static void _getSize( XBOX::VJSParms_getProperty& ioParms, VJSPictureContainer* inPict);
	static void _getWidth( XBOX::VJSParms_getProperty& ioParms, VJSPictureContainer* inPict);
	static void _getHeight( XBOX::VJSParms_getProperty& ioParms, VJSPictureContainer* inPict);
	static void _getMeta( XBOX::VJSParms_getProperty& ioParms, VJSPictureContainer* inPict);

};

END_TOOLBOX_NAMESPACE

#endif
