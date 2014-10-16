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
#include "VGraphicsPrecompiled.h"
#include "VPattern.h"
#include "VRect.h"
#include "V4DPictureIncludeBase.h"
#if VERSIONMAC
#include <QuickTime/QuickTime.h>
#endif

using namespace std;

VFileBitmap::VFileBitmap(const VString &inPath)
{
	fPath = inPath;
	BuildBitmap();
}

VFileBitmap::VFileBitmap(const VFileBitmap &inFrom)
{
	fPath = inFrom.fPath;
	BuildBitmap();
}

void VFileBitmap::BuildBitmap()
{
#if VERSIONWIN
	//try to decode with WIC decoder first
	VFile file(fPath);
	fBitmap = VPictureCodec_WIC::Decode( file);

	//backfall to gdiplus decoder if failed
	if (fBitmap == NULL)
	{
		fBitmap = new Gdiplus::Bitmap(fPath.GetCPointer());
		fBitmap->SetResolution( 96.0f, 96.0f); //JQ 16/04/2012: fixed ACI0076335 - ensure dpi=96
	}
#endif
#if VERSIONMAC
	VFile file(fPath);
	fBitmap = VPictureCodec_ImageIO::Decode( file);
	
	/*
	fBitmap = NULL;
	FSRef fileRef;
	if (file.MAC_GetFSRef(&fileRef))
	{
		OSType	dataType;
		Handle	dataRef = NULL;
		ComponentResult	result = ::QTNewDataReferenceFromFSRef(&fileRef, 0, &dataRef, &dataType);
		assert(result == noErr);
	
		if (dataRef != NULL)
		{
			GraphicsImportComponent	component = NULL;
			result = ::GetGraphicsImporterForDataRefWithFlags(dataRef, dataType, &component, 0);
			assert(result == noErr);
		
			result = ::GraphicsImportCreateCGImage(component, &fBitmap, kGraphicsImportCreateCGImageUsingCurrentSettings);
			assert(result == noErr);
			
			::CloseComponent(component);
			::DisposeHandle(dataRef);
		}
	}
	*/
#endif
}

VFileBitmap::~VFileBitmap ()
{
#if VERSIONWIN
	delete fBitmap;
	fBitmap = NULL;
#endif
#if VERSIONMAC
	::CGImageRelease(fBitmap);
	fBitmap = NULL;
#endif
}

void VFileBitmap::GetBounds(VRect &outRect) const
{
	outRect.SetLeft(0);
	outRect.SetTop(0);
#if VERSIONWIN
	Gdiplus::SizeF size;
	fBitmap->GetPhysicalDimension(&size);
	outRect.SetWidth(size.Width);
	outRect.SetHeight(size.Height);
 #endif
 #if VERSIONMAC
	size_t w = CGImageGetWidth(fBitmap);
	size_t h = CGImageGetHeight(fBitmap);
	outRect.SetWidth(w);
	outRect.SetHeight(h);
 #endif
 
}

void VFileBitmap::GetPath(VString &outPath) const
{
	outPath = fPath;
}

//*********

VPattern::VPattern()
{
	fWrapMode = ePatternWrapTile;
}


VPattern::VPattern(const VPattern& /*inOriginal*/)
{
}


VPattern::~VPattern()
{
}


void VPattern::ApplyToContext(VGraphicContext* inContext) const
{
	if (inContext == NULL) return;
	
	//JQ 04/11/2011: oups release parent context - very important for Direct2D especially... (!!!)
	ContextRef contextNative = inContext->GetParentContext();
	DoApplyPattern( contextNative, inContext->GetFillRule());
	inContext->ReleaseParentContext( contextNative); //do something only for GDIPlus & Direct2D impls
}


void VPattern::DoApplyPattern(ContextRef /*inContext*/, FillRuleType) const
{
	// Override to apply the pattern to the given context
	// NULL means current port/context (whenever possible)
}


void VPattern::ReleaseFromContext(VGraphicContext* inContext) const
{
	if (inContext == NULL) return;
#if VERSIONWIN	
	xbox_assert(inContext->IsGDIImpl());
#endif
	DoReleasePattern(inContext->GetParentContext());
}


void VPattern::DoReleasePattern(ContextRef /*inContext*/) const
{
	// Override if you need to release the pattern
	// NULL means current port/context (whenever possible)
}


VError VPattern::ReadFromStream(VStream* /*inStream*/, sLONG /*inParam*/)
{
	return VE_OK;
}


VError VPattern::WriteToStream(VStream* /*inStream*/, sLONG /*inParam*/) const
{
	return VE_OK;
}


VPattern* VPattern::RetainStdPattern(PatternStyle inStyle)
{
	return new VStandardPattern(inStyle);
}


VPattern* VPattern::RetainPatternFromStream(VStream* inStream, OsType inKind)
{
	VPattern*	pattern = NULL;
	
	switch (inKind)
	{
		case VStandardPattern::Pattern_Kind:
			pattern = new VStandardPattern;
			break;
			
		case VRadialGradientPattern::Pattern_Kind:
			pattern = new VRadialGradientPattern;
			break;
			
		case VAxialGradientPattern::Pattern_Kind:
			pattern = new VAxialGradientPattern;
			break;
	}
	
	if (pattern != NULL)
		pattern->ReadFromStream(inStream);
		
	return pattern;
}


#pragma mark-

VStandardPattern::VStandardPattern(PatternStyle inStyle, const VColor* inColor)
{
	_Init();
	fPatternStyle = inStyle;
	
	if (inColor != NULL)
		fPatternColor = *inColor;
}


VStandardPattern::VStandardPattern(const VStandardPattern& inOriginal)
	: VPattern(inOriginal)
{
	_Init();
	fPatternStyle = inOriginal.fPatternStyle;
	fPatternColor = inOriginal.fPatternColor;
}


VStandardPattern::~VStandardPattern()
{
	_Release();
}


VStandardPattern& VStandardPattern::operator=(const VStandardPattern& inOriginal)
{
	_Release();
	
	fPatternStyle = inOriginal.fPatternStyle;
	fPatternColor = inOriginal.fPatternColor;
	return *this;
}


void VStandardPattern::SetPatternStyle(PatternStyle inStyle)
{
	if (fPatternStyle != inStyle)
	{
		fPatternStyle = inStyle;
		_Release();
	}
}


VError VStandardPattern::ReadFromStream(VStream* inStream, sLONG inParam)
{
	VPattern::ReadFromStream(inStream, inParam);
	
	uLONG	longValue;
	inStream->GetLong(longValue);
	SetPatternStyle((PatternStyle)longValue);
	
	fPatternColor.ReadFromStream(inStream);
	
	return inStream->GetLastError();
}


VError VStandardPattern::WriteToStream(VStream* inStream, sLONG inParam) const
{
	VPattern::WriteToStream(inStream, inParam);
	
	inStream->PutLong(fPatternStyle);
	fPatternColor.WriteToStream(inStream);
	
	return inStream->GetLastError();
}


void VStandardPattern::_Init()
{
	fPatternStyle = PAT_FILL_PLAIN;
	fPatternRef = NULL;
}


#pragma mark-


VGradientPattern::VGradientPattern()
{
	_Init();
}


VGradientPattern::VGradientPattern(GradientStyle inStyle, const VPoint& inStartPoint, const VPoint& inEndPoint, const VColor& inStartColor, const VColor& inEndColor)
	: fStartPoint(inStartPoint), fEndPoint(inEndPoint), fStartColor(inStartColor), fEndColor(inEndColor)
{
	_Init();
	fGradientStyle = inStyle;
}

//multicolor gradient constructor
VGradientPattern::VGradientPattern (GradientStyle inStyle, const VPoint& inStartPoint, const VPoint& inEndPoint, 
									const GradientStopVector& inGradStops, 
									const VAffineTransform& inTransform)
	: fStartPoint(inStartPoint), fEndPoint(inEndPoint)
{
	_Init();
	fGradientStyle = inStyle;
	SetGradientStops( inGradStops);
	fTransform = inTransform;
}


VGradientPattern::VGradientPattern(const VGradientPattern& inOriginal)
	: VPattern(inOriginal)
{
	_Init();
	
	fGradientStyle = inOriginal.fGradientStyle;
	fStartPoint = inOriginal.fStartPoint;
	fEndPoint = inOriginal.fEndPoint;
	fStartColor = inOriginal.fStartColor;
	fEndColor = inOriginal.fEndColor;
	
	fGradientStops = inOriginal.fGradientStops;
#if VERSIONMAC
	memcpy( fLUT, inOriginal.fLUT, sizeof(fLUT));
#endif
	
	fTransform = inOriginal.fTransform;
}


VGradientPattern::~VGradientPattern()
{
	_Release();
}


VGradientPattern& VGradientPattern::operator=(const VGradientPattern& inOriginal)
{
	_Release();
	
	fGradientStyle = inOriginal.fGradientStyle;
	fStartPoint = inOriginal.fStartPoint;
	fEndPoint = inOriginal.fEndPoint;
	fStartColor = inOriginal.fStartColor;
	fEndColor = inOriginal.fEndColor;

	fGradientStops = inOriginal.fGradientStops;
#if VERSIONMAC
	memcpy( fLUT, inOriginal.fLUT, sizeof(fLUT));
#endif	
	
	fTransform = inOriginal.fTransform;
	
	return *this;
}


void VGradientPattern::SetGradientStyle(GradientStyle inStyle)
{
	if (fGradientStyle != inStyle)
	{
		fGradientStyle = inStyle;
		_Release();
	}
}


void VGradientPattern::SetStartPoint(const VPoint& inPoint)
{
	if (fStartPoint != inPoint)
	{
		fStartPoint = inPoint;
		_Release();
	}
}


void VGradientPattern::SetEndPoint(const VPoint& inPoint)
{
	if (fEndPoint != inPoint)
	{
		fEndPoint = inPoint;
		_Release();
	}
}



/** define a gradient with more than 2 colors
@remarks
	each stop is defined by a pair <offset,color> where offset is [0-1] value 
	which is mapped to [start point-end point] position
	(so gradient can start farer than start point and finish before end point)
*/
void VGradientPattern::SetGradientStops( const GradientStopVector& inGradStops)
{
	if (inGradStops.size() >= 1)
	{
		fStartColor = inGradStops[0].second;
		fEndColor = inGradStops.back().second;
	}
	
	if (inGradStops.size() < 2)
	{
		//invalid: use default gradient
		SetGradientStyle( GRAD_STYLE_LINEAR);
		return;
	}
	else if (inGradStops.size() == 2 
		&&
		inGradStops[0].first == 0.0f
		&& 
		inGradStops[1].first == 1.0f)
	{
		//use standard gradient

		SetGradientStyle( GRAD_STYLE_LINEAR);
		return;
	}

	//use multicolor gradient

	if (&fGradientStops != &inGradStops)
		fGradientStops = inGradStops;
#if VERSIONWIN
	//ensure that first stop offset is 0.0 and last stop offset is 1.0 (gdiplus constraint)
	if (fGradientStops[0].first != 0.0f)
	{
		GradientStop stop = GradientStop(0.0f,fGradientStops[0].second);
		fGradientStops.insert( fGradientStops.begin(), stop);
	}
	if (fGradientStops.back().first != 1.0f)
	{
		GradientStop stop = GradientStop(1.0f,fGradientStops.back().second);
		fGradientStops.push_back( stop);
	}
#endif

#if VERSIONMAC
	//build internal lut from gradient stops
	GradientStopVector::const_iterator it = inGradStops.begin();
	int offsetPrev = -1;
	GReal colorPrev[4] = { 0,0,0,0};
	for (;it != inGradStops.end(); it++)
	{
		int offset = (*it).first*(GRADIENT_LUT_SIZE-1);
		VColor color = (*it).second;
		if (offset < 0)
			offset = 0;
		if (offset >= GRADIENT_LUT_SIZE)
			offset = GRADIENT_LUT_SIZE-1;
		if (offsetPrev == -1)
		{
			//fill lut up to first stop color offset with first stop color

			colorPrev[0] = color.GetRed() / 255.0f;
			colorPrev[1] = color.GetGreen() / 255.0f;
			colorPrev[2] = color.GetBlue() / 255.0f;
			colorPrev[3] = color.GetAlpha() / 255.0f;

			for (int i = 0; i <= offset; i++)
			{
				fLUT[i][0] = colorPrev[0];
				fLUT[i][1] = colorPrev[1];
				fLUT[i][2] = colorPrev[2];
				fLUT[i][3] = colorPrev[3];
			}
			offsetPrev = offset;
		}
		else
		{
			//linearly interpolate colors from previous stop color to actual stop color
			//on gradient lut sub-range

			GReal colorPacked[4];
			colorPacked[0] = color.GetRed() / 255.0f;
			colorPacked[1] = color.GetGreen() / 255.0f;
			colorPacked[2] = color.GetBlue() / 255.0f;
			colorPacked[3] = color.GetAlpha() / 255.0f;

			if (offset == offsetPrev)
			{
				fLUT[offset][0] = colorPacked[0]; 
				fLUT[offset][1] = colorPacked[1]; 
				fLUT[offset][2] = colorPacked[2]; 
				fLUT[offset][3] = colorPacked[3]; 
			}
			else
			{
				GReal denom = offset-offsetPrev;
				for (int i = offsetPrev+1; i <= offset; i++)
				{
					GReal frac = (i-offsetPrev)/denom;
					fLUT[i][0] = colorPrev[0]*(1.0f-frac) + colorPacked[0]*frac;
					fLUT[i][1] = colorPrev[1]*(1.0f-frac) + colorPacked[1]*frac;
					fLUT[i][2] = colorPrev[2]*(1.0f-frac) + colorPacked[2]*frac;
					fLUT[i][3] = colorPrev[3]*(1.0f-frac) + colorPacked[3]*frac;
				}
			}

			offsetPrev = offset;
			colorPrev[0] = colorPacked[0]; 
			colorPrev[1] = colorPacked[1]; 
			colorPrev[2] = colorPacked[2]; 
			colorPrev[3] = colorPacked[3]; 
		}
	}
	if (offsetPrev < GRADIENT_LUT_SIZE-1)
	{
		//fill lut after last stop color offset with last stop color
		for (int i = offsetPrev+1; i <= GRADIENT_LUT_SIZE-1; i++)
		{
			fLUT[i][0] = colorPrev[0];
			fLUT[i][1] = colorPrev[1];
			fLUT[i][2] = colorPrev[2];
			fLUT[i][3] = colorPrev[3];
		}
	}
#endif

	//set gradient style if it is not set yet
	if (!(fGradientStyle == GRAD_STYLE_LUT 
		  ||
		  fGradientStyle == GRAD_STYLE_LUT_FAST))
		SetGradientStyle( GRAD_STYLE_LUT);
}


void VGradientPattern::ComputeValues(const GReal* inValue, GReal* outValue)
{		
#if VERSIONMAC
	if (fGradientStyle == GRAD_STYLE_LUT)
	{
		//linear interpolate color from neighbouring colors in LUT

		xbox_assert(*inValue >= 0.0f && *inValue <= 1.0f);

		GReal frac = (*inValue)*(GRADIENT_LUT_SIZE-1);
		int index		= (int)floor(frac);
		int indexNext	= (int)ceil(frac);
		frac -= index;

		GReal *colorPrev = &(fLUT[index][0]);
		GReal *colorNext = &(fLUT[indexNext][0]);

		outValue[0] = colorPrev[0]*(1.0f-frac) + colorNext[0]*frac;
		outValue[1] = colorPrev[1]*(1.0f-frac) + colorNext[1]*frac;
		outValue[2] = colorPrev[2]*(1.0f-frac) + colorNext[2]*frac;
		outValue[3] = colorPrev[3]*(1.0f-frac) + colorNext[3]*frac;
	}
	else if (fGradientStyle == GRAD_STYLE_LUT_FAST)
	{
		//pick nearest color from LUT

		xbox_assert(*inValue >= 0.0f && *inValue <= 1.0f);

		int index = (int)((*inValue)*(GRADIENT_LUT_SIZE-1));
		GReal *color = &(fLUT[index][0]);
		outValue[0] = *color++;
		outValue[1] = *color++;
		outValue[2] = *color++;
		outValue[3] = *color;
	}
	else
		DoComputeValues(inValue, outValue); 
#endif
}


VError VGradientPattern::ReadFromStream(VStream* inStream, sLONG inParam)
{
	VPattern::ReadFromStream(inStream, inParam);
	
	_Release();
	
	fStartColor.ReadFromStream(inStream);
	fEndColor.ReadFromStream(inStream);
	
	Real	realValue;
	inStream->GetReal(realValue);
	fStartPoint.SetX((GReal) realValue);
	inStream->GetReal(realValue);
	fStartPoint.SetY((GReal) realValue);
	inStream->GetReal(realValue);
	fEndPoint.SetX((GReal) realValue);
	inStream->GetReal(realValue);
	fEndPoint.SetY((GReal) realValue);
	
	uLONG	longValue;
	inStream->GetLong(longValue);
	SetGradientStyle((GradientStyle)longValue);
	
	if (fGradientStyle == GRAD_STYLE_LUT
		||
		fGradientStyle == GRAD_STYLE_LUT_FAST)
	{
		sLONG count = inStream->GetLong();
		if (count > 0)
		{
			fGradientStops.clear();
			for (int i = 0; i < count; i++)
			{
				VError error;

				SmallReal offset;
				error = inStream->GetSmallReal( offset);
				if (error != VE_OK)
				{
					SetGradientStyle(GRAD_STYLE_LINEAR);
					return inStream->GetLastError();
				}

				VColor color;
				error = color.ReadFromStream( inStream);
				if (error != VE_OK)
				{
					SetGradientStyle(GRAD_STYLE_LINEAR);
					return inStream->GetLastError();
				}

				fGradientStops.push_back( GradientStop(offset,color));
			}
			SetGradientStops( fGradientStops);
		}
		else
			SetGradientStyle(GRAD_STYLE_LINEAR);
	}
	return inStream->GetLastError();
}


VError VGradientPattern::WriteToStream(VStream* inStream, sLONG inParam) const
{
	VPattern::WriteToStream(inStream, inParam);

	fStartColor.WriteToStream(inStream);
	fEndColor.WriteToStream(inStream);
	inStream->PutReal(fStartPoint.GetX());
	inStream->PutReal(fStartPoint.GetY());
	inStream->PutReal(fEndPoint.GetX());
	inStream->PutReal(fEndPoint.GetY());
	inStream->PutLong(fGradientStyle);
	
	if (fGradientStyle == GRAD_STYLE_LUT
		||
		fGradientStyle == GRAD_STYLE_LUT_FAST)
	{
		inStream->PutLong( (sLONG)fGradientStops.size());

		GradientStopVector::const_iterator it = fGradientStops.begin();
		for (;it != fGradientStops.end(); it++)
		{
			inStream->PutSmallReal( (*it).first);
			((*it).second).WriteToStream( inStream);
		}
	}

	return inStream->GetLastError();
}


void VGradientPattern::_Init()
{
	fGradientStyle = GRAD_STYLE_LINEAR;
	fGradientRef = NULL;
	fWrapMode = ePatternWrapClamp;
}


#pragma mark-

VAxialGradientPattern::VAxialGradientPattern()
{
	_Init();
	_ComputeFactors();
}


VAxialGradientPattern::VAxialGradientPattern(GradientStyle inStyle, const VPoint& inStartPoint, const VPoint& inEndPoint, const VColor& inStartColor, const VColor& inEndColor)
	: VGradientPattern(inStyle, inStartPoint, inEndPoint, inStartColor, inEndColor)
{
	_Init();
	fBilinear = false;
	_ComputeFactors();
}


VAxialGradientPattern::VAxialGradientPattern(GradientStyle inStyle, const VPoint& inStartPoint, const VPoint& inCentralPoint, const VPoint& inEndPoint, const VColor& inStartColor, const VColor& inCentralColor, const VColor& inEndColor)
	: VGradientPattern(inStyle, inStartPoint, inEndPoint, inStartColor, inEndColor), fCentralPoint(inCentralPoint), fCentralColor(inCentralColor)
{
	_Init();
	fBilinear = true;
	_ComputeFactors();
}


//multicolor gradient constructor 
VAxialGradientPattern::VAxialGradientPattern (GradientStyle inStyle, const VPoint& inStartPoint, const VPoint& inEndPoint, 
											  const GradientStopVector& inGradStops, const VAffineTransform& inTransform)
	: VGradientPattern(inStyle, inStartPoint, inEndPoint, inGradStops, inTransform)
{
	_Init();
	fBilinear = false;
	_ComputeFactors();
}


VAxialGradientPattern::VAxialGradientPattern(const VAxialGradientPattern& inOriginal)
	: VGradientPattern(inOriginal)
{
	_Init();
	fBilinear = inOriginal.fBilinear;
	fCentralPoint = inOriginal.fCentralPoint;
	fCentralColor = inOriginal.fCentralColor;
	_ComputeFactors();
}


VAxialGradientPattern::~VAxialGradientPattern()
{
}


VAxialGradientPattern& VAxialGradientPattern::operator=(const VAxialGradientPattern& inOriginal)
{
	VGradientPattern::operator=(inOriginal);
	
	fBilinear = inOriginal.fBilinear;
	fCentralPoint = inOriginal.fCentralPoint;
	fCentralColor = inOriginal.fCentralColor;

	_ComputeFactors();

	return *this;
}


void VAxialGradientPattern::DoComputeValues(const GReal* inValue, GReal* outValue)
{
	outValue[0] = (*inValue) * fSpreadFactor[0] + fStartComponent[0];
	outValue[1] = (*inValue) * fSpreadFactor[1] + fStartComponent[1];
	outValue[2] = (*inValue) * fSpreadFactor[2] + fStartComponent[2];
	outValue[3] = (*inValue) * fSpreadFactor[3] + fStartComponent[3];
}


void VAxialGradientPattern::_ComputeFactors()
{
	if (fBilinear)
	{
		//build gradient stops
	
		fBilinear = false;
		
		GReal	gradientDeltaX = fEndPoint.GetX() - fStartPoint.GetX();
		GReal	gradientDeltaY = fEndPoint.GetY() - fStartPoint.GetY();
		GReal	centerDeltaX = fCentralPoint.GetX() - fStartPoint.GetX();
		GReal	centerDeltaY = fCentralPoint.GetY() - fStartPoint.GetY();
	    // JM 171006 SmallReal	gradientLength = sqrt(gradientDeltaX * gradientDeltaX - gradientDeltaY * gradientDeltaY);
		// JM 171006 fCentralValue = sqrt(centerDeltaX * centerDeltaX - centerDeltaY * centerDeltaY) / gradientLength;
		GReal	gradientLength = sqrt(fabs(gradientDeltaX * gradientDeltaX + gradientDeltaY * gradientDeltaY));
	    fCentralValue = sqrt(fabs(centerDeltaX * centerDeltaX + centerDeltaY * centerDeltaY)) / gradientLength;
		if (fCentralValue == 0.0f)
			fCentralValue = 0.00001f; //prevent exception
	    
		GradientStopVector stops;
		stops.push_back(GradientStop(0.0f, fStartColor));
		stops.push_back(GradientStop(fCentralValue, fCentralColor));
		stops.push_back(GradientStop(1.0f, fEndColor));
		SetGradientStops(stops);
		return;
	}
#if VERSIONMAC
	if (fGradientStyle == GRAD_STYLE_LUT 
		||
		fGradientStyle == GRAD_STYLE_LUT_FAST)
		return;

	GReal	startComponent[4];
	GReal	endComponent[4];
	    
	startComponent[0] = fStartColor.GetRed() / (GReal) 255;
	startComponent[1] = fStartColor.GetGreen() / (GReal) 255;
	startComponent[2] = fStartColor.GetBlue() / (GReal) 255;
	startComponent[3] = fStartColor.GetAlpha() / (GReal) 255;
	
	endComponent[0] = fEndColor.GetRed() / (GReal) 255;
	endComponent[1] = fEndColor.GetGreen() / (GReal) 255;
	endComponent[2] = fEndColor.GetBlue() / (GReal) 255;
	endComponent[3] = fEndColor.GetAlpha() / (GReal) 255;
	
	for (sLONG index = 0; index < 4; index++)
	{
		fSpreadFactor[index] = (endComponent[index] - startComponent[index]);
		fStartComponent[index] = startComponent[index];
	}
#endif
}


VError VAxialGradientPattern::ReadFromStream(VStream* inStream, sLONG inParam)
{
	VGradientPattern::ReadFromStream(inStream, inParam);
	
	fCentralColor.ReadFromStream(inStream);
	
	Real	realValue;
	inStream->GetReal(realValue);
	fCentralPoint.SetX((GReal) realValue);
	inStream->GetReal(realValue);
	fCentralPoint.SetY((GReal) realValue);
	
	sWORD	wordValue;
	inStream->GetWord(wordValue);
	fBilinear = wordValue;
	
	_ComputeFactors();
	
	return inStream->GetLastError();
}


VError VAxialGradientPattern::WriteToStream(VStream* inStream, sLONG inParam) const
{
	VGradientPattern::WriteToStream(inStream, inParam);

	fCentralColor.WriteToStream(inStream);
	inStream->PutReal(fCentralPoint.GetX());
	inStream->PutReal(fCentralPoint.GetY());
	inStream->PutWord(fBilinear);
	
	return inStream->GetLastError();
}


void VAxialGradientPattern::_Init()
{
	fBilinear = false;
	fWrapMode = ePatternWrapClamp;
}


#pragma mark-

VRadialGradientPattern::VRadialGradientPattern()
{
	_Init();
	_ComputeFactors();
}


VRadialGradientPattern::VRadialGradientPattern(GradientStyle inStyle, const VPoint& inStartPoint, const VPoint& inEndPoint, const VColor& inStartColor, const VColor& inEndColor, GReal inStartRadius, GReal inEndRadius)
	: VGradientPattern(inStyle, inStartPoint, inEndPoint, inStartColor, inEndColor)
{
	_Init();
	fStartRadiusX = inStartRadius;
	fStartRadiusY = inStartRadius;

	fEndRadiusX = inEndRadius;
	fEndRadiusY = inEndRadius;
	
	_ComputeFactors();
}


//multicolor circular gradient constructor
VRadialGradientPattern::VRadialGradientPattern (GradientStyle inStyle, const VPoint& inStartPoint, const VPoint& inEndPoint, 
												GReal inStartRadius, GReal inEndRadius, 
												const GradientStopVector& inGradStops, const VAffineTransform& inTransform)
	: VGradientPattern(inStyle, inStartPoint, inEndPoint, inGradStops, inTransform)
{
	_Init();
	fStartRadiusX = inStartRadius;
	fStartRadiusY = inStartRadius;
	
	fEndRadiusX = inEndRadius;
	fEndRadiusY = inEndRadius;
	
	_ComputeFactors();
}




VRadialGradientPattern::VRadialGradientPattern(GradientStyle inStyle, const VPoint& inStartPoint, const VPoint& inEndPoint, const VColor& inStartColor, const VColor& inEndColor, GReal inStartRadiusX, GReal inStartRadiusY, GReal inEndRadiusX, GReal inEndRadiusY)
	: VGradientPattern(inStyle, inStartPoint, inEndPoint, inStartColor, inEndColor)
{
	_Init();
	fStartRadiusX = inStartRadiusX;
	fStartRadiusY = inStartRadiusY;

	fEndRadiusX = inEndRadiusX;
	fEndRadiusY = inEndRadiusY;
	
	_ComputeFactors();
}


//multicolor ellipsoid gradient constructor
VRadialGradientPattern::VRadialGradientPattern (GradientStyle inStyle, const VPoint& inStartPoint, const VPoint& inEndPoint, 
												GReal inStartRadiusX, GReal inStartRadiusY, GReal inEndRadiusX, GReal inEndRadiusY, 
												const GradientStopVector& inGradStops, const VAffineTransform& inTransform)
	: VGradientPattern(inStyle, inStartPoint, inEndPoint, inGradStops, inTransform)
{
	_Init();
	fStartRadiusX = inStartRadiusX;
	fStartRadiusY = inStartRadiusY;
	
	fEndRadiusX = inEndRadiusX;
	fEndRadiusY = inEndRadiusY;
	
    fEndRadiusTiling	= 0;	

	_ComputeFactors();
}



VRadialGradientPattern::VRadialGradientPattern(const VRadialGradientPattern& inOriginal)
	: VGradientPattern(inOriginal)
{
	_Init();

	*this = inOriginal;
	
	_ComputeFactors();
}


VRadialGradientPattern::~VRadialGradientPattern()
{
}


VRadialGradientPattern& VRadialGradientPattern::operator=(const VRadialGradientPattern& inOriginal)
{
	VGradientPattern::operator=(inOriginal);
	
	fStartRadiusX = inOriginal.fStartRadiusX;
	fStartRadiusY = inOriginal.fStartRadiusY;

	fEndRadiusX = inOriginal.fEndRadiusX;
	fEndRadiusY = inOriginal.fEndRadiusY;
	
    fEndRadiusTiling	= inOriginal.fEndRadiusTiling;	
	fStartPointTiling	= inOriginal.fStartPointTiling;
	fEndPointTiling		= inOriginal.fEndPointTiling;  
  	
	return *this;
}

void VRadialGradientPattern::_ComputeFactors()
{
	//ensure that focal point lies inside exterior radius
	//(otherwise gradient drawing is undeterminate)
	if (fEndRadiusX != 0 && fEndRadiusY != 0 && (fStartPoint.x != fEndPoint.x || fStartPoint.y != fEndPoint.y))
	{
		//map focal to circle coordinate space with center at end point and radius equal to fEndRadiusX
		GReal fx = fStartPoint.x-fEndPoint.x;
		GReal fy = (fStartPoint.y-fEndPoint.y)*fEndRadiusX/fEndRadiusY;
		
		if ((fx*fx+fy*fy) > fEndRadiusX*fEndRadiusX*(GReal) 0.9999*(GReal) 0.9999) //radius*0.9999 in order to avoid math discrepancy errors
		{
			//set focal to intersection with circle
			GReal angle = atan2(fy,fx);
			fy = sin(angle)*fEndRadiusX*(GReal) 0.9999;
			fx = cos(angle)*fEndRadiusX*(GReal) 0.9999;
			
			//map focal from circle coordinate space to user coordinate space 
			fStartPoint.x = fx+fEndPoint.x;
			fStartPoint.y = fy*fEndRadiusY/fEndRadiusX+fEndPoint.y;
		}
	}
	
	//here end point and radius need to be adjusted for reflect and repeat wrapping modes
	if (GetWrapMode() != ePatternWrapClamp)
	{	
		VPoint startPoint	= fStartPoint;
		VPoint endPoint		= fEndPoint;	
	
		//map radius and focal coordinates from ellipsoid coordinate space (user space) to circular coordinate space centered on end point
		//(more easy to work with circular coordinate space)
		GReal endRadius = xbox::Max( fEndRadiusX, fEndRadiusY);
	
		if (startPoint != endPoint
			&&
			fEndRadiusX != fEndRadiusY
			&&
			fEndRadiusX != 0
			&& 
			fEndRadiusY != 0)
		{
			VPoint focal = startPoint-endPoint;
			if (fEndRadiusX >= fEndRadiusY)
				focal.y *= fEndRadiusX/fEndRadiusY;
			else
				focal.x *= fEndRadiusY/fEndRadiusX;
		
			startPoint = endPoint + focal;
		}
	
		//if focal is not centered and as radius is enlarged for tiling wrapping modes, 
		//we must modify center point to ensure ratio (dist focal to radius)/radius keep the same
		//otherwise drawing will not be correct
		if (startPoint != endPoint && endRadius != 0)
		{
			VPoint vFocalToCenter = endPoint-startPoint;
			GReal distFocalToCenter = vFocalToCenter.GetLength();
			if (distFocalToCenter > endRadius)
				distFocalToCenter = endRadius;
			GReal distFocalToRadius = endRadius-distFocalToCenter;
			
			vFocalToCenter.Normalize();
			endRadius *= kPATTERN_GRADIENT_TILE_MAX_COUNT;
			endPoint += vFocalToCenter*(endRadius-distFocalToRadius*kPATTERN_GRADIENT_TILE_MAX_COUNT);
			
			//ensure that focal still lies in exterior circle :
			//because of math discrepancy errors (especially near end radius border) we need to check that to avoid Quartz2D artifacts
			
			//map focal to circle coordinate space with center at new end point
			GReal fx = startPoint.x-endPoint.x;
			GReal fy = startPoint.y-endPoint.y;
			
			while ((fx*fx+fy*fy) > endRadius*endRadius*(GReal) 0.9999*(GReal) 0.9999)	//radius*0.9999 to deal with math discrepancy errors (value 0.9999 determined by experimentation)
				endRadius *= (GReal) 1.01; 
		}
		else
			//focal is centered: only enlarge radius
			endRadius *= kPATTERN_GRADIENT_TILE_MAX_COUNT;
		
		//update tiling datas
		fEndRadiusTiling = endRadius;
		
		if (endPoint != fEndPoint)
		{
			//adjust new end point position (to take account ellipsoid shape)
			if (fEndRadiusX >= fEndRadiusY)
			{
				GReal ratio = fEndRadiusY/fEndRadiusX;
				
				startPoint -= endPoint;
				
				//map new end point from circular coordinate space centered on last end point to user space
				//(this will be new origin for circular coordinate space)
				endPoint -= fEndPoint;
				endPoint.y *= ratio;
				fEndPointTiling = fEndPoint+endPoint;
				
#if VERSIONMAC
				startPoint += fEndPointTiling;	//keep start point in circular coordinate space whom origin is equal to end point
												//(transform to user space is delayed to gradient transform)
#else
				//map start point to user space (ellipsoid coordinate space)
				startPoint.y *= ratio;
				startPoint += fEndPointTiling;
#endif
				fStartPointTiling = startPoint;
			}
			else
			{
				GReal ratio = fEndRadiusX/fEndRadiusY;
				
				startPoint -= endPoint;
				
				//map new end point from circular coordinate space centered on last end point to user space
				//(this will be new origin for circular coordinate space)
				endPoint -= fEndPoint;
				endPoint.x *= ratio;
				fEndPointTiling = fEndPoint+endPoint;
				
#if VERSIONMAC
				startPoint += fEndPointTiling;	//keep start point in circular coordinate space whom origin is equal to end point
												//(transform to user space is delayed to gradient transform)
#else
				//map start point to user space (ellipsoid coordinate space)
				startPoint.x *= ratio;
				startPoint += fEndPointTiling;
#endif
				
				fStartPointTiling = startPoint;
			}
		}
		else
		{
				fEndPointTiling = endPoint;
				fStartPointTiling = startPoint;
		}
		
#if VERSIONMAC	
		//compute radial gradient transform matrix
		//remark: we must remap input coordinates if the radial gradient is ellipsoid and not strictly circular
		
		fMat.MakeIdentity();
		if (fEndRadiusX != fEndRadiusY && fEndRadiusX != 0 && fEndRadiusY != 0)
		{
			fMat.Translate( -fEndPointTiling.GetX(), -fEndPointTiling.GetY(), VAffineTransform::MatrixOrderAppend);
			if (fEndRadiusX > fEndRadiusY)
				fMat.Scale( 1, fEndRadiusY/fEndRadiusX, VAffineTransform::MatrixOrderAppend); 
			else
				fMat.Scale( fEndRadiusX/fEndRadiusY, 1, VAffineTransform::MatrixOrderAppend);
			fMat.Translate( fEndPointTiling.GetX(), fEndPointTiling.GetY(), VAffineTransform::MatrixOrderAppend);
		}
#endif
	}
#if VERSIONMAC	
	else
	{
		//compute radial gradient transform matrix
		//remark: we must remap input coordinates if the radial gradient is ellipsoid and not strictly circular
		
		fMat.MakeIdentity();
		if (fEndRadiusX != fEndRadiusY && fEndRadiusX != 0 && fEndRadiusY != 0)
		{
			fMat.Translate( -fEndPoint.GetX(), -fEndPoint.GetY(), VAffineTransform::MatrixOrderAppend);
			if (fEndRadiusX > fEndRadiusY)
				fMat.Scale( 1, fEndRadiusY/fEndRadiusX, VAffineTransform::MatrixOrderAppend); 
			else
				fMat.Scale( fEndRadiusX/fEndRadiusY, 1, VAffineTransform::MatrixOrderAppend);
			fMat.Translate( fEndPoint.GetX(), fEndPoint.GetY(), VAffineTransform::MatrixOrderAppend);
		}
	}

	//concat with user transform if any (append in order to allow gradient shearing for instance)
	if (!fTransform.IsIdentity())
		fMat *= fTransform;
	
	if (fGradientStyle == GRAD_STYLE_LUT 
		||
		fGradientStyle == GRAD_STYLE_LUT_FAST)
		return;

	GReal	startComponent[4];
	GReal	endComponent[4];
	
	//compute radial gradient factors
	
	startComponent[0] = fStartColor.GetRed() / 255.0f;
	startComponent[1] = fStartColor.GetGreen() / 255.0f;
	startComponent[2] = fStartColor.GetBlue() / 255.0f;
	startComponent[3] = fStartColor.GetAlpha() / 255.0f;
		
	endComponent[0] = fEndColor.GetRed() / 255.0f;
	endComponent[1] = fEndColor.GetGreen() / 255.0f;
	endComponent[2] = fEndColor.GetBlue() / 255.0f;
	endComponent[3] = fEndColor.GetAlpha() / 255.0f;
		
	for (sLONG index = 0; index < 4; index++)
	{
		fSpreadFactor[index] = (endComponent[index] - startComponent[index]);
		fStartComponent[index] = startComponent[index];
	}
#endif
}


void VRadialGradientPattern::DoComputeValues(const GReal* inValue, GReal* outValue)
{
#if VERSIONMAC
	outValue[0] = (*inValue) * fSpreadFactor[0] + fStartComponent[0];
	outValue[1] = (*inValue) * fSpreadFactor[1] + fStartComponent[1];
	outValue[2] = (*inValue) * fSpreadFactor[2] + fStartComponent[2];
	outValue[3] = (*inValue) * fSpreadFactor[3] + fStartComponent[3];
#endif
}


VError VRadialGradientPattern::ReadFromStream(VStream* inStream, sLONG inParam)
{
	VGradientPattern::ReadFromStream(inStream, inParam);
	
	fStartRadiusX = (GReal) inStream->GetReal();
	fStartRadiusY = (GReal) inStream->GetReal();

	fEndRadiusX = (GReal) inStream->GetReal();
	fEndRadiusY = (GReal) inStream->GetReal();
	
	_ComputeFactors();
	return inStream->GetLastError();
}


VError VRadialGradientPattern::WriteToStream(VStream* inStream, sLONG inParam) const
{
	VGradientPattern::WriteToStream(inStream, inParam);

	inStream->PutReal(fStartRadiusX);
	inStream->PutReal(fStartRadiusY);

	inStream->PutReal(fEndRadiusX);
	inStream->PutReal(fEndRadiusY);
	
	return inStream->GetLastError();
}


void VRadialGradientPattern::_Init()
{
	fStartRadiusX = (GReal) 0.0;
	fStartRadiusY = (GReal) 0.0;

	fEndRadiusX = (GReal) 0.0;
	fEndRadiusY = (GReal) 0.0;

	fWrapMode = ePatternWrapClamp;
}