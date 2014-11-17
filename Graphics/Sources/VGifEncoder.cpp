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
#include "V4DPictureIncludeBase.h"
#include "VGifEncoder.h"

VBitmapData* VQuantizer::Quantize( VBitmapData& source )
{
	VBitmapData* output;
	output=new VBitmapData(source.GetWidth(),source.GetHeight(),VBitmapData::VPixelFormat8bppIndexed);
	
	if ( !_singlePass )
		FirstPass ( source) ;
	
	std::vector<VColor>* palette=GetPalette();
	if(palette)
	{
		output->SetColorTable(*palette);
		delete palette;
	}
	SecondPass ( source , *output );
	return output;
	#if 0
	
	int	height = source.GetHeight() ;
	int width = source.GetWidth() ;

			
	Gdiplus::Rect	bounds( 0 , 0 , width , height ) ;
	Gdiplus::RectF	boundsf( 0.0 , 0.0 , 1.0*width ,1.0* height ) ;

	// First off take a 32bpp copy of the image
	Gdiplus::Bitmap*	copy= new Gdiplus::Bitmap(width ,height,PixelFormat32bppARGB);

	// And construct an 8bpp version
	Gdiplus::Bitmap*	output = new Gdiplus::Bitmap(width,height,PixelFormat8bppIndexed) ;

	// Now lock the bitmap into memory
	Gdiplus::Graphics g(copy );
			
	g.SetPageUnit(Gdiplus::UnitPixel);
	g.DrawImage( &source ,boundsf) ;
			
			// Define a pointer to the bitmap data
			Gdiplus::BitmapData	sourceData ;

			
			// Get the source image bits and lock into memory
			copy->LockBits ( &bounds , Gdiplus::ImageLockModeRead  , PixelFormat32bppARGB ,&sourceData) ;

			// Call the FirstPass function if not a single pass algorithm.
			// For something like an octree quantizer, this will run through
			// all image pixels, build a data structure, and create a palette.
			if ( !_singlePass )
				FirstPass ( sourceData , width , height ) ;

			// Then set the color palette on the output bitmap. I'm passing in the current palette 
			// as there's no way to construct a new, empty palette.
			Gdiplus::ColorPalette* palette=GetPalette();
			if(palette)
			{
				output->SetPalette(palette) ;
				free((void*)palette);
			}
			// Then call the second pass which actually does the conversion
			SecondPass ( sourceData , *output , width , height , bounds ) ;
			
			copy->UnlockBits ( &sourceData ) ;

			// Last but not least, return the output bitmap
			return output;
#endif
}
void VQuantizer::FirstPass ( VBitmapData& sourceData )
{
	// Define the source data pointers. The source row is a uBYTE to
	// keep addition of the stride value easier (as this is in bytes)
	
	sLONG*	pSourcePixel;

	// Loop through each row
	for ( int row = 0 ; row < sourceData.GetHeight() ; row++ )
	{
		// Set the source pixel to the first pixel in this row
		pSourcePixel = (sLONG*) sourceData.GetLinePtr(row);

		// And loop through each column
		for ( int col = 0 ; col < sourceData.GetWidth() ; col++ , pSourcePixel++ )
		{
			// Now I have the pixel, call the FirstPassQuantize function...
			InitialQuantizePixel ( (VBitmapData::_ARGBPix*)pSourcePixel ) ;
		}
		// Add the stride to the source row
	}
}
void VQuantizer::SecondPass ( VBitmapData& sourceData , VBitmapData& outputData )
{
	uBYTE*	pSourceRow = (uBYTE*)sourceData.GetPixelBuffer();
	sLONG*	pSourcePixel = (sLONG*) pSourceRow;
	sLONG*	pPreviousPixel = pSourcePixel;

	// Now define the destination data pointers
	uBYTE*	pDestinationRow = (uBYTE*) outputData.GetPixelBuffer();
	uBYTE*	pDestinationPixel = pDestinationRow ;
	
	uBYTE	pixelValue = QuantizePixel ( (VBitmapData::_ARGBPix*)pSourcePixel ) ;
	*pDestinationPixel = pixelValue ;
	
	for ( int row = 0 ; row < sourceData.GetHeight() ; row++ )
	{
		// Set the source pixel to the first pixel in this row
		pSourcePixel = (sLONG*) sourceData.GetLinePtr(row);
		// And set the destination pixel pointer to the first pixel in the row
		pDestinationPixel = (uBYTE*)outputData.GetLinePtr(row)  ;
		// Loop through each pixel on this scan line
		for ( int col = 0 ; col < sourceData.GetWidth() ; col++ , pSourcePixel++ , pDestinationPixel++ )
		{
			// Check if this is the same as the last pixel. If so use that value
			// rather than calculating it again. This is an inexpensive optimisation.
			if ( *pPreviousPixel != *pSourcePixel )
			{
				// Quantize the pixel
				pixelValue = QuantizePixel ( (VBitmapData::_ARGBPix*)pSourcePixel ) ;
				// And setup the previous pointer
				pPreviousPixel = pSourcePixel ;
			}

			// And set the pixel in the output
			*pDestinationPixel = pixelValue ;
		}
	}
	
	#if 0
	Gdiplus::BitmapData	outputData ;

	// Lock the output bitmap into memory
	output.LockBits ( &bounds , Gdiplus::ImageLockModeWrite , PixelFormat8bppIndexed , &outputData) ;

	// Define the source data pointers. The source row is a uBYTE to
	// keep addition of the stride value easier (as this is in bytes)
	uBYTE*	pSourceRow = (uBYTE*)sourceData.Scan0;
	sLONG*	pSourcePixel = (sLONG*)pSourceRow ;
	sLONG*	pPreviousPixel = pSourcePixel ;

	// Now define the destination data pointers
	uBYTE*	pDestinationRow = (uBYTE*) outputData.Scan0;
	uBYTE*	pDestinationPixel = pDestinationRow ;

	// And convert the first pixel, so that I have values going into the loop
	uBYTE	pixelValue = QuantizePixel ( (VBitmapData::_ARGBPix*)pSourcePixel ) ;

	// Assign the value of the first pixel
	*pDestinationPixel = pixelValue ;

	// Loop through each row
	for ( int row = 0 ; row < height ; row++ )
	{
		// Set the source pixel to the first pixel in this row
		pSourcePixel = (sLONG*) pSourceRow ;

		// And set the destination pixel pointer to the first pixel in the row
		pDestinationPixel = pDestinationRow ;

		// Loop through each pixel on this scan line
		for ( int col = 0 ; col < width ; col++ , pSourcePixel++ , pDestinationPixel++ )
		{
			// Check if this is the same as the last pixel. If so use that value
			// rather than calculating it again. This is an inexpensive optimisation.
			if ( *pPreviousPixel != *pSourcePixel )
			{
				// Quantize the pixel
				pixelValue = QuantizePixel ( (VBitmapData::_ARGBPix*)pSourcePixel ) ;
				// And setup the previous pointer
				pPreviousPixel = pSourcePixel ;
			}

			// And set the pixel in the output
			*pDestinationPixel = pixelValue ;
		}
		// Add the stride to the source row
		pSourceRow += sourceData.Stride ;
		// And to the destination row
		pDestinationRow += outputData.Stride ;
	}

		// Ensure that I unlock the output bits
		output.UnlockBits ( &outputData ) ;
	#endif
}
void VQuantizer::InitialQuantizePixel ( VBitmapData::_ARGBPix* /*inPix*/)
{

}
uBYTE VQuantizer::QuantizePixel ( VBitmapData::_ARGBPix* /*inPix*/ )
{
	return 0;
}
std::vector<VColor>* VQuantizer::GetPalette ( )
{
	return 0;
}

/************************************************************************************/
// NeuQuant Neural-Net Quantization Algorithm
// ------------------------------------------
//
// Copyright (c) 1994 Anthony Dekker
//
// NEUQUANT Neural-Net quantization algorithm by Anthony Dekker, 1994.
// See "Kohonen neural networks for optimal colour quantization"
// in "Network: Computation in Neural Systems" Vol. 5 (1994) pp 351-367.
// for a discussion of the algorithm.
//
// Any party obtaining a copy of these files from the author, directly or
// indirectly, is granted, free of charge, a full and unrestricted irrevocable,
// world-wide, paid up, royalty-free, nonexclusive right and license to deal
// in this software and documentation files (the "Software"), including without
// limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons who receive
// copies from any such party to do so, with the only requirement being
// that this copyright notice remain intact.

///////////////////////////////////////////////////////////////////////
// History
// -------
// January 2001: Adaptation of the Neural-Net Quantization Algorithm
//               for the FreeImage 2 library
//               Author: Hervé Drolon (drolon@infonie.fr)
// March 2004:   Adaptation for the FreeImage 3 library (port to big endian processors)
//               Author: Hervé Drolon (drolon@infonie.fr)
// April 2004:   Algorithm rewritten as a C++ class. 
//               Fixed a bug in the algorithm with handling of 4-uBYTE boundary alignment.
//               Author: Hervé Drolon (drolon@infonie.fr)
///////////////////////////////////////////////////////////////////////

// Four primes near 500 - assume no image has a length so large
// that it is divisible by all four primes
// ==========================================================

#define prime1		499
#define prime2		491
#define prime3		487
#define prime4		503

// ----------------------------------------------------------------

VNNQuantizer::VNNQuantizer(int PaletteSize,int sampling)
:VQuantizer( false)
{
	fsampling=sampling;
	netsize = PaletteSize;
	maxnetpos = netsize - 1;
	initrad = netsize < 8 ? 1 : (netsize >> 3);
	initradius = (initrad * radiusbias);

	network = NULL;

	network = (pixel *)malloc(netsize * sizeof(pixel));
	bias = (int *)malloc(netsize * sizeof(int));
	freq = (int *)malloc(netsize * sizeof(int));
	radpower = (int *)malloc(initrad * sizeof(int));

	if( !network || !bias || !freq || !radpower ) {
		if(network) 
			free(network);
		if(bias) 
			free(bias);
		if(freq) 
			free(freq);
		if(radpower) 
			free(radpower);
		radpower=0;
		freq=0;
		bias=0;
		network=0;
	}
}

VNNQuantizer::~VNNQuantizer()
{
	if(network) free(network);
	if(bias) free(bias);
	if(freq) free(freq);
	if(radpower) free(radpower);
}

///////////////////////////////////////////////////////////////////////////
// Initialise network in range (0,0,0) to (255,255,255) and set parameters
// -----------------------------------------------------------------------

void VNNQuantizer::initnet() {
	int i, *p;

	for (i = 0; i < netsize; i++) {
		p = network[i];
		p[FI_RGBA_BLUE] = p[FI_RGBA_GREEN] = p[FI_RGBA_RED] = (i << (netbiasshift+8))/netsize;
		freq[i] = intbias/netsize;	/* 1/netsize */
		bias[i] = 0;
	}
}

///////////////////////////////////////////////////////////////////////////////////////	
// Unbias network to give uBYTE values 0..255 and record position i to prepare for sort
// ------------------------------------------------------------------------------------

void VNNQuantizer::unbiasnet() {
	int i, j, temp;

	for (i = 0; i < netsize; i++) {
		for (j = 0; j < 3; j++) {
			// OLD CODE: network[i][j] >>= netbiasshift; 
			// Fix based on bug report by Juergen Weigert jw@suse.de
			temp = (network[i][j] + (1 << (netbiasshift - 1))) >> netbiasshift;
			if (temp > 255) temp = 255;
			network[i][j] = temp;
		}
		network[i][3] = i;			// record colour no 
	}
}

//////////////////////////////////////////////////////////////////////////////////
// Insertion sort of network and building of netindex[0..255] (to do after unbias)
// -------------------------------------------------------------------------------

void VNNQuantizer::inxbuild() {
	int i,j,smallpos,smallval;
	int *p,*q;
	int previouscol,startpos;

	previouscol = 0;
	startpos = 0;
	for (i = 0; i < netsize; i++) {
		p = network[i];
		smallpos = i;
		smallval = p[FI_RGBA_GREEN];			// index on g
		// find smallest in i..netsize-1
		for (j = i+1; j < netsize; j++) {
			q = network[j];
			if (q[FI_RGBA_GREEN] < smallval) {	// index on g
				smallpos = j;
				smallval = q[FI_RGBA_GREEN];	// index on g
			}
		}
		q = network[smallpos];
		// swap p (i) and q (smallpos) entries
		if (i != smallpos) {
			j = q[FI_RGBA_BLUE];  q[FI_RGBA_BLUE]  = p[FI_RGBA_BLUE];  p[FI_RGBA_BLUE]  = j;
			j = q[FI_RGBA_GREEN]; q[FI_RGBA_GREEN] = p[FI_RGBA_GREEN]; p[FI_RGBA_GREEN] = j;
			j = q[FI_RGBA_RED];   q[FI_RGBA_RED]   = p[FI_RGBA_RED];   p[FI_RGBA_RED]   = j;
			j = q[3];   q[3] = p[3];   p[3] = j;
		}
		// smallval entry is now in position i
		if (smallval != previouscol) {
			netindex[previouscol] = (startpos+i)>>1;
			for (j = previouscol+1; j < smallval; j++)
				netindex[j] = i;
			previouscol = smallval;
			startpos = i;
		}
	}
	netindex[previouscol] = (startpos+maxnetpos)>>1;
	for (j = previouscol+1; j < 256; j++)
		netindex[j] = maxnetpos; // really 256
}

///////////////////////////////////////////////////////////////////////////////
// Search for BGR values 0..255 (after net is unbiased) and return colour index
// ----------------------------------------------------------------------------

int VNNQuantizer::inxsearch(int b, int g, int r) {
	int i, j, dist, a, bestd;
	int *p;
	int best;

	bestd = 1000;		// biggest possible dist is 256*3
	best = -1;
	i = netindex[g];	// index on g
	j = i-1;			// start at netindex[g] and work outwards

	while ((i < netsize) || (j >= 0)) {
		if (i < netsize) {
			p = network[i];
			dist = p[FI_RGBA_GREEN] - g;				// inx key
			if (dist >= bestd)
				i = netsize;	// stop iter
			else {
				i++;
				if (dist < 0)
					dist = -dist;
				a = p[FI_RGBA_BLUE] - b;
				if (a < 0)
					a = -a;
				dist += a;
				if (dist < bestd) {
					a = p[FI_RGBA_RED] - r;
					if (a<0)
						a = -a;
					dist += a;
					if (dist < bestd) {
						bestd = dist;
						best = p[3];
					}
				}
			}
		}
		if (j >= 0) {
			p = network[j];
			dist = g - p[FI_RGBA_GREEN];			// inx key - reverse dif
			if (dist >= bestd)
				j = -1;	// stop iter
			else {
				j--;
				if (dist < 0)
					dist = -dist;
				a = p[FI_RGBA_BLUE] - b;
				if (a<0)
					a = -a;
				dist += a;
				if (dist < bestd) {
					a = p[FI_RGBA_RED] - r;
					if (a<0)
						a = -a;
					dist += a;
					if (dist < bestd) {
						bestd = dist;
						best = p[3];
					}
				}
			}
		}
	}
	return best;
}

///////////////////////////////
// Search for biased BGR values
// ----------------------------

int VNNQuantizer::contest(int b, int g, int r) {
	// finds closest neuron (min dist) and updates freq
	// finds best neuron (min dist-bias) and returns position
	// for frequently chosen neurons, freq[i] is high and bias[i] is negative
	// bias[i] = gamma*((1/netsize)-freq[i])

	int i,dist,a,biasdist,betafreq;
	int bestpos,bestbiaspos,bestd,bestbiasd;
	int *p,*f, *n;

	bestd = ~(((int) 1)<<31);
	bestbiasd = bestd;
	bestpos = -1;
	bestbiaspos = bestpos;
	p = bias;
	f = freq;

	for (i = 0; i < netsize; i++) {
		n = network[i];
		dist = n[FI_RGBA_BLUE] - b;
		if (dist < 0)
			dist = -dist;
		a = n[FI_RGBA_GREEN] - g;
		if (a < 0)
			a = -a;
		dist += a;
		a = n[FI_RGBA_RED] - r;
		if (a < 0)
			a = -a;
		dist += a;
		if (dist < bestd) {
			bestd = dist;
			bestpos = i;
		}
		biasdist = dist - ((*p)>>(intbiasshift-netbiasshift));
		if (biasdist < bestbiasd) {
			bestbiasd = biasdist;
			bestbiaspos = i;
		}
		betafreq = (*f >> betashift);
		*f++ -= betafreq;
		*p++ += (betafreq << gammashift);
	}
	freq[bestpos] += beta;
	bias[bestpos] -= betagamma;
	return bestbiaspos;
}

///////////////////////////////////////////////////////
// Move neuron i towards biased (b,g,r) by factor alpha
// ---------------------------------------------------- 

void VNNQuantizer::altersingle(int alpha, int i, int b, int g, int r) {
	int *n;

	n = network[i];				// alter hit neuron
	n[FI_RGBA_BLUE]	 -= (alpha * (n[FI_RGBA_BLUE]  - b)) / initalpha;
	n[FI_RGBA_GREEN] -= (alpha * (n[FI_RGBA_GREEN] - g)) / initalpha;
	n[FI_RGBA_RED]   -= (alpha * (n[FI_RGBA_RED]   - r)) / initalpha;
}

////////////////////////////////////////////////////////////////////////////////////
// Move adjacent neurons by precomputed alpha*(1-((i-j)^2/[r]^2)) in radpower[|i-j|]
// ---------------------------------------------------------------------------------

void VNNQuantizer::alterneigh(int rad, int i, int b, int g, int r) {
	int j, k, lo, hi, a;
	int *p, *q;

	lo = i - rad;   if (lo < -1) lo = -1;
	hi = i + rad;   if (hi > netsize) hi = netsize;

	j = i+1;
	k = i-1;
	q = radpower;
	while ((j < hi) || (k > lo)) {
		a = (*(++q));
		if (j < hi) {
			p = network[j];
			p[FI_RGBA_BLUE]  -= (a * (p[FI_RGBA_BLUE] - b)) / alpharadbias;
			p[FI_RGBA_GREEN] -= (a * (p[FI_RGBA_GREEN] - g)) / alpharadbias;
			p[FI_RGBA_RED]   -= (a * (p[FI_RGBA_RED] - r)) / alpharadbias;
			j++;
		}
		if (k > lo) {
			p = network[k];
			p[FI_RGBA_BLUE]  -= (a * (p[FI_RGBA_BLUE] - b)) / alpharadbias;
			p[FI_RGBA_GREEN] -= (a * (p[FI_RGBA_GREEN] - g)) / alpharadbias;
			p[FI_RGBA_RED]   -= (a * (p[FI_RGBA_RED] - r)) / alpharadbias;
			k--;
		}
	}
}

/////////////////////
// Main Learning Loop
// ------------------

/**
 Get a pixel sample at position pos. Handle 4-uBYTE boundary alignment.
 @param pos pixel position in a WxHx3 pixel buffer
 @param b blue pixel component
 @param g green pixel component
 @param r red pixel component
*/

void VNNQuantizer::FirstPass ( VBitmapData& sourceData )
{
	initnet();
	learn(sourceData,fsampling);
	unbiasnet();
}

void VNNQuantizer::getSample(VBitmapData& sourceData, sLONG pos, int *b, int *g, int *r) {
	// get equivalent pixel coordinates 
	// - assume it's a 24-bit image -

	sLONG rowByte24 = sourceData.GetWidth()*3;	//we need to assume image is 24 bits 
												//in order to get correct x & y pixel pos 
												//as passed pos is virtually based on 24bits image - see lengthCount in learn - as seen by scan algorithm
	int x = (pos % rowByte24)/3;
	int y = pos / rowByte24;

	uBYTE *bits=(uBYTE*)sourceData.GetPixelBuffer();

	//but here we use real image row byte size to get the proper px data
	VBitmapData::_ARGBPix* pix=	((VBitmapData::_ARGBPix*)(bits + (y *sourceData.GetRowByte())))+ x ; 

	*b = pix->variant.col.B << netbiasshift;
	*g = pix->variant.col.G << netbiasshift;
	*r = pix->variant.col.R << netbiasshift;
}

void VNNQuantizer::learn(VBitmapData& sourceData,int sampling_factor) {
	int i, j, b, g, r;
	int radius, rad, alpha, step, delta, samplepixels;
	int alphadec; // biased by 10 bits
	sLONG pos, lengthcount;

	xbox_assert(sourceData.GetPixelFormatSize() == 32);

	// image size as viewed by the scan algorithm
	lengthcount = sourceData.GetHeight() * sourceData.GetWidth() * 3;

	// number of samples used for the learning phase
	samplepixels = lengthcount / (3 * sampling_factor);

	// decrease learning rate after delta pixel presentations
	delta = samplepixels / ncycles;
	if(delta == 0) {
		// avoid a 'divide by zero' error with very small images
		delta = 1;
	}

	// initialize learning parameters
	alphadec = 30 + ((sampling_factor - 1) / 3);
	alpha = initalpha;
	radius = initradius;
	
	rad = radius >> radiusbiasshift;
	if (rad <= 1) rad = 0;
	for (i = 0; i < rad; i++) 
		radpower[i] = alpha*(((rad*rad - i*i)*radbias)/(rad*rad));
	
	// initialize pseudo-random scan
	if ((lengthcount % prime1) != 0)
		step = 3*prime1;
	else {
		if ((lengthcount % prime2) != 0)
			step = 3*prime2;
		else {
			if ((lengthcount % prime3) != 0) 
				step = 3*prime3;
			else
				step = 3*prime4;
		}
	}
	
	i = 0;		// iteration
	pos = 0;	// pixel position

	while (i < samplepixels) {
		
		if (pos+3 > lengthcount) //we ensure we do not overflow px buffer
			break;
		
		// get next learning sample
		getSample(sourceData,pos, &b, &g, &r);

		// find winning neuron
		j = contest(b, g, r);

		// alter winner
		altersingle(alpha, j, b, g, r);

		// alter neighbours 
		if (rad) alterneigh(rad, j, b, g, r);

		// next sample
		pos += step;
		while (pos >= lengthcount) pos -= lengthcount;
	
		i++;
		if (i % delta == 0) {	
			// decrease learning rate and also the neighborhood
			alpha -= alpha / alphadec;
			radius -= radius / radiusdec;
			rad = radius >> radiusbiasshift;
			if (rad <= 1) rad = 0;
			for (j = 0; j < rad; j++) 
				radpower[j] = alpha * (((rad*rad - j*j) * radbias) / (rad*rad));
		}
	}
	
}

//////////////
// Quantizer
// -----------
uBYTE VNNQuantizer::QuantizePixel ( VBitmapData::_ARGBPix* inPix ) 
{
	return inxsearch(inPix->variant.col.B, inPix->variant.col.G, inPix->variant.col.R);
}
std::vector<VColor>* VNNQuantizer::GetPalette ()
{
	std::vector<VColor>* palette=new std::vector<VColor>;
	for (int j = 0; j < netsize; j++) 
	{
		VColor col((uBYTE)network[j][FI_RGBA_RED],(uBYTE)network[j][FI_RGBA_GREEN],(uBYTE)network[j][FI_RGBA_BLUE]);
		
		/*uBYTE blue  = (BYTE)network[j][FI_RGBA_BLUE];
		uBYTE green = (BYTE)network[j][FI_RGBA_GREEN];
		uBYTE red	= (BYTE)network[j][FI_RGBA_RED];
		
		
		palette->Entries[j]=col.MakeARGB(255,red,green,blue);
		*/
		palette->push_back(col);
		
	}
	inxbuild();
	return palette;
}

// ******************* OctreeQuantizer **************************//
VOctreeQuantizer::VOctreeQuantizer ( int maxColors , int maxColorBits )
:VQuantizer(false)
{

	if ( maxColors <= 255 )
	{
		if ( ( maxColorBits >= 1 ) && ( maxColorBits <= 8 ) )
		{
			_octree = new VOctree ( maxColorBits  ) ;

			_maxColors = maxColors ;
		}
	}
}
VOctreeQuantizer::~VOctreeQuantizer()
{
	if(_octree)
		delete _octree;
}
void VOctreeQuantizer::InitialQuantizePixel ( VBitmapData::_ARGBPix* pixel )
{
	_octree->AddColor ( pixel ) ;
}

uBYTE VOctreeQuantizer::QuantizePixel ( VBitmapData::_ARGBPix* pixel )
{
	uBYTE	paletteIndex = (uBYTE)_maxColors ;	// The color at [_maxColors] is set to transparent

	// Get the palette index if this non-transparent
	if ( pixel->variant.col.A > 0 )
		paletteIndex = (uBYTE)_octree->GetPaletteIndex ( pixel ) ;

	return paletteIndex ;
}

std::vector<VColor>* VOctreeQuantizer::GetPalette ()
{
	std::vector<VColor> *result=new std::vector<VColor>;
	// First off convert the octree to _maxColors colors
	PaletteList	*pal = _octree->Palletize ( _maxColors - 1 ) ;
/*
	// Then convert the palette based on those colors
	for ( int index = 0 ; index < palette->size() ; index++ )
		original.Entries[index] = (*palette)[index].variant.ARGB ;

	// Add the transparent color
	original.Entries[_maxColors] = 0 ;
*/
	int nbentry=(int)pal->size();
	for (int j = 0; j < nbentry; j++) 
	{
		result->push_back(VColor((*pal)[j].variant.col.R,(*pal)[j].variant.col.G,(*pal)[j].variant.col.B,(*pal)[j].variant.col.A));
	}
	_maxColors=nbentry;
	result->push_back(VColor(0,0,0,0));
	return result ;
}

int VOctree::mask[8]={ 0x80 , 0x40 , 0x20 , 0x10 , 0x08 , 0x04 , 0x02 , 0x01 };

VOctree::VOctree( int maxColorBits )
{
	_maxColorBits = maxColorBits ;
	_leafCount = 0 ;
	_reducibleNodes.resize(9,0) ;
	_root = new VOctreeNode ( 0 , _maxColorBits , this ) ; 
	_previousColor.variant.ARGB=0 ;
	_previousNode = NULL ;
}

PaletteList* VOctree::Palletize ( int colorCount )
{
	while ( GetLeaves() > colorCount )
		Reduce ( ) ;

	// Now palettize the nodes
	PaletteList *palette = new PaletteList() ;
	int			paletteIndex = 0 ;
	_root->ConstructPalette ( palette , paletteIndex ) ;

	// And return the palette
	return palette ;
}

void VOctree::AddColor ( VBitmapData::_ARGBPix* pixel )
{
	// Check if this request is for the same color as the last
	if ( _previousColor.variant.ARGB == pixel->variant.ARGB )
	{
		// If so, check if I have a previous node setup. This will only ocurr if the first color in the image
		// happens to be black, with an alpha component of zero.
		if ( NULL == _previousNode )
		{
			_previousColor = *pixel;
			_root->AddColor ( pixel , _maxColorBits , 0 , this ) ;
		}
		else
		{	// Just update the previous node
			_previousNode->Increment ( pixel ) ;
		}
	}
	else
	{
		_previousColor = *pixel ;
		_root->AddColor ( pixel , _maxColorBits , 0 , this ) ;
	}
}

void VOctree::Reduce ( )
{
	int	index ;

	// Find the deepest level containing at least one reducible node
	for ( index = _maxColorBits - 1 ; ( index > 0 ) && ( NULL == _reducibleNodes[index] ) ; index-- ) ;

	// Reduce the node most recently added to the list at level 'index'
	VOctreeNode	*node = _reducibleNodes[index] ;
	_reducibleNodes[index] = node->GetNextReducible() ;

	// Decrement the leaf count after reducing the node
	_leafCount -= node->Reduce ( ) ;

	// And just in case I've reduced the last color to be added, and the next color to
	// be added is the same, invalidate the previousNode...
	_previousNode = NULL ;
}

void VOctree::TrackPrevious ( VOctreeNode* node )
{
	_previousNode = node ;
}

int VOctree::GetPaletteIndex ( VBitmapData::_ARGBPix* pixel )
{
	return _root->GetPaletteIndex ( pixel , 0 ) ;
}

VOctreeNode::VOctreeNode ( int level , int colorBits , VOctree* octree )
{
	// Construct the new node
	_leaf = ( level == colorBits ) ;

	_red = _green = _blue = 0 ;
	_pixelCount = 0 ;

	// If a leaf, increment the leaf count
	if ( _leaf )
	{
		octree->SetLeaves(octree->GetLeaves()+1) ;
		_nextReducible = NULL ;
		_children.clear();
	}
	else
	{
		// Otherwise add this to the reducible nodes
		_nextReducible = octree->GetReducibleNodes(level) ;
		octree->SetReducibleNodes(level,this );
		_children.resize(8,0);
	}
}
VOctreeNode::~VOctreeNode()
{
	
}
void VOctreeNode::AddColor ( VBitmapData::_ARGBPix* pixel , int colorBits , int level , VOctree* octree )
{
	// Update the color information if this is a leaf
	if ( _leaf )
	{
		Increment ( pixel ) ;
		// Setup the previous node
		octree->TrackPrevious ( this ) ;
	}
	else
	{
		// Go to the next level down in the tree
		int	shift = 7 - level ;
		int index = ( ( pixel->variant.col.R & VOctree::mask[level] ) >> ( shift - 2 ) ) |
			( ( pixel->variant.col.G & VOctree::mask[level] ) >> ( shift - 1 ) ) |
			( ( pixel->variant.col.B & VOctree::mask[level] ) >> ( shift ) ) ;

		VOctreeNode*	child = _children[index] ;

		if ( NULL == child )
		{
			// Create a new child node & store in the array
			child = new VOctreeNode ( level + 1 , colorBits , octree ) ; 
			_children[index] = child ;
		}

		// Add the color to the child node
		child->AddColor ( pixel , colorBits , level + 1 , octree ) ;
	}
}

int VOctreeNode::Reduce ( )
{
	_red = _green = _blue = 0 ;
	int	children = 0 ;

	// Loop through all children and add their information to this node
	for ( int index = 0 ; index < 8 ; index++ )
	{
		if ( NULL != _children[index] )
		{
			_red += _children[index]->_red ;
			_green += _children[index]->_green ;
			_blue += _children[index]->_blue ;
			_pixelCount += _children[index]->_pixelCount ;
			++children ;
			_children[index] = NULL ;
		}
	}

	// Now change this to a leaf node
	_leaf = true ;

	// Return the number of nodes to decrement the leaf count by
	return ( children - 1 ) ;
}

void VOctreeNode::ConstructPalette ( PaletteList* palette , int& paletteIndex )
{
	if ( _leaf )
	{
		// Consume the next palette index
		_paletteIndex = paletteIndex++ ;

		// And set the color of the palette entry
		VBitmapData::_ARGBPix col;
		col.variant.col.A=0;
		col.variant.col.R=_red / _pixelCount;
		col.variant.col.B=_blue / _pixelCount;
		col.variant.col.G=_green / _pixelCount;
		palette->push_back ( col ) ;
	}
	else
	{
		// Loop through children looking for leaves
		for ( int index = 0 ; index < 8 ; index++ )
		{
			if ( NULL != _children[index] )
				_children[index]->ConstructPalette ( palette , paletteIndex ) ;
		}
	}
}

int VOctreeNode::GetPaletteIndex ( VBitmapData::_ARGBPix* pixel , int level )
{
	int	paletteIndex = _paletteIndex ;

	if ( !_leaf )
	{
		int	shift = 7 - level ;
		int index = ( ( pixel->variant.col.R & VOctree::mask[level] ) >> ( shift - 2 ) ) |
			( ( pixel->variant.col.G & VOctree::mask[level] ) >> ( shift - 1 ) ) |
			( ( pixel->variant.col.B & VOctree::mask[level] ) >> ( shift ) ) ;
		if ( NULL != _children[index] )
			paletteIndex = _children[index]->GetPaletteIndex ( pixel , level + 1 ) ;
		else
			paletteIndex=-1;
		}
	return paletteIndex ;
}

void VOctreeNode::Increment ( VBitmapData::_ARGBPix* pixel )
{
	_pixelCount++ ;
	_red += pixel->variant.col.R;
	_green += pixel->variant.col.G;
	_blue += pixel->variant.col.B;
}


/***********************************************************
* gif encoder
***********************************************************/

#define MAXCODE(n_bits)        (((code_int) 1 << (n_bits)) - 1)
#define HashTabOf(i)       htab[i]
#define CodeTabOf(i)    codetab[i]

uLONG xGifEncoder::masks[17] = { 0x0000, 0x0001, 0x0003, 0x0007, 0x000F,
                                  0x001F, 0x003F, 0x007F, 0x00FF,
                                  0x01FF, 0x03FF, 0x07FF, 0x0FFF,
                                  0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };

xGifEncoder::xGifEncoder()
{
	Init();
}
xGifEncoder::~xGifEncoder()
{
}
void xGifEncoder::Init()
{
	fBitmapData=0;
	Width = 0;
	Height = 0;
	curx = 0;
	cury = 0;
	CountDown = 0;
	Pass = 0;
	Interlace = 0;
	a_count = 0;
	cur_accum = 0;
	cur_bits = 0;
	g_init_bits = 0;
	ClearCode = 0;
	EOFCode = 0;
	free_ent = 0;
	clear_flg = 0;
	offset = 0;
	in_count = 1;
	out_count = 0;	
	hsize = HSIZE;
	n_bits = 0;
	maxbits = GIFBITS;
	maxcode = 0;
	maxmaxcode = (code_int)1 << GIFBITS;
}

short xGifEncoder::GIFEncode(VStream& outStream,VBitmapData* inBitmapData, sLONG GInterlace, sLONG Background, sLONG Transparent)
{
	fBitmapData=inBitmapData;
	const std::vector<VColor>& colortable=inBitmapData->GetColorTable(); 
	sLONG B;
    sLONG RWidth, RHeight;
    sLONG LeftOfs, TopOfs;
    sLONG Resolution;
    sLONG ColorMapSize;
    sLONG InitCodeSize;
    sLONG i;
	sLONG BitsPerPixel;
	VColor rgbp;
	short ctsize;
		
    Interlace = GInterlace;
	ctsize = (short)colortable.size();
		
	BitsPerPixel = 0;
		
	if ( ctsize <= 2 )
	    BitsPerPixel = 1;
	else if ( ctsize <= 4 )
	    BitsPerPixel = 2;
	else if ( ctsize <= 8 )
	    BitsPerPixel = 3;
	else if ( ctsize <= 16 )
	     BitsPerPixel = 4;
	else if ( ctsize <= 32 )
	    BitsPerPixel = 5;
	else if ( ctsize <= 64 )
	    BitsPerPixel = 6;
	else if ( ctsize <= 128 )
	    BitsPerPixel = 7;
	else if ( ctsize <= 256 )
		BitsPerPixel = 8;

    ColorMapSize = 1<<BitsPerPixel;
Transparent=-1;
	if (Transparent>=0)
	{
		i = *((unsigned char*)fBitmapData->GetPixelBuffer()); // grosse magouille L.R le 13/1/97
		rgbp = colortable[i];
		if ( (rgbp.GetRed()==0xff) && (rgbp.GetBlue()==0xff) && (rgbp.GetGreen()==0xff))
		{
			Transparent = i;
		}
		else
		{
			for( i=0; i<ctsize; ++i ) 
			{
				rgbp = colortable[i];
				if ( (rgbp.GetRed()==0xff) && (rgbp.GetBlue()==0xff) && (rgbp.GetGreen()==0xff))
				{
					Transparent = i;
					break;
				}
		    }
		}
	}

        RWidth = Width = fBitmapData->GetWidth();
        RHeight = Height = fBitmapData->GetHeight();
        LeftOfs = TopOfs = 0;

        Resolution = BitsPerPixel;

        /*
         * Calculate number of bits we are expecting
         */
		CountDown = (sLONG) Width * (sLONG) Height;

        /*
         * Indicate which pass we are on (if interlace)
         */
        Pass = 0;

        /*
         * The initial code size
         */
        if( BitsPerPixel <= 1 )
			InitCodeSize = 2;
        else
			InitCodeSize = BitsPerPixel;

        /*
         * Set up the current x and y position
         */
        curx = cury = 0;

        /*
         * Write the Magic header
         */
        outStream.PutCString((Transparent < 0 ? "GIF87a" : "GIF89a"));
		
        /*
         * Write out the screen width and height
         */
		outStream.PutWord(RWidth);
		outStream.PutWord(RHeight);
 
        /*
         * Indicate that there is a global colour map
         */
        B = 0x80;       /* Yes, there is a color map */

        /*
         * OR in the resolution
         */
        B |= (Resolution - 1) << 5;

        /*
         * OR in the Bits per Pixel
         */
        B |= (BitsPerPixel - 1);

        /*
         * Write it out
         */
		outStream.PutByte( B );

        /*
         * Write out the Background colour
         */
        outStream.PutByte( Background );

        /*
         * Byte of 0's (future expansion)
         */
        outStream.PutByte( 0 );

        /*
         * Write out the Global Colour Map
         */
        for( i=0; i<ctsize; ++i ) 
		{
			outStream.PutByte( colortable[i].GetRed());
			outStream.PutByte( colortable[i].GetGreen());
			outStream.PutByte( colortable[i].GetBlue());
        }
        for(i=0;i<ColorMapSize-ctsize;i++)
        {
			outStream.PutByte( 0);
			outStream.PutByte( 0);
			outStream.PutByte( 0);
        }

	/*
	 * Write out extension for transparent colour index, if necessary.
	 */
	if ( Transparent >= 0 ) 
	{
	    outStream.PutByte( '!' );
	    outStream.PutByte( (unsigned char)0xf9);
	    outStream.PutByte( 4);
	    outStream.PutByte( 1);
	    outStream.PutByte( 0);
	    outStream.PutByte( 0);
	    outStream.PutByte( (unsigned char) Transparent);
	    outStream.PutByte( 0);
	}

    /*
    * Write an Image separator
    */
    outStream.PutByte( ',');

    /*
    * Write the Image header
    */

    outStream.PutWord( LeftOfs);
    outStream.PutWord( TopOfs);
    outStream.PutWord( Width);
    outStream.PutWord( Height);

	/*
	* Write out whether or not the image is interlaced
	*/
	if( Interlace )
		outStream.PutByte( 0x40);
	else
		outStream.PutByte( 0x00);

	/*
	* Write out the initial code size
	*/
	outStream.PutByte(InitCodeSize);

	/*
	* Go and actually compress the data
	*/
	compress( outStream, InitCodeSize+1 );

	/*
	* Write out a Zero-length packet (to end the series)
	*/
	outStream.PutByte( 0);

	/*
	* Write the GIF file terminator
	*/
	outStream.PutByte( ';');
			
	return(0);
}

void xGifEncoder::compress(VStream& outStream, sLONG init_bits)
{
	register sLONG fcode;
    register code_int i /* = 0 */;
    register sLONG c;
    register code_int ent;
    register code_int disp;
    register code_int hsize_reg;
    register sLONG hshift;

    /*
     * Set up the globals:  g_init_bits - initial number of bits
     *                      g_outfile   - pointer to output file
     */
    g_init_bits = init_bits;

    /*
     * Set up the necessary values
     */
    offset = 0;
    out_count = 0;
    clear_flg = 0;
    in_count = 1;
    maxcode = MAXCODE(n_bits = g_init_bits);

    ClearCode = (1 << (init_bits - 1));
    EOFCode = ClearCode + 1;
    free_ent = ClearCode + 2;

    char_init();

    ent = GIFNextPixel(*fBitmapData);

    hshift = 0;
	for (fcode = (sLONG) hsize; fcode < 65536L; fcode *= 2L)
        ++hshift;
    hshift = 8 - hshift;                /* set hash code range bound */

    hsize_reg = hsize;
    cl_hash((count_int) hsize_reg);            /* clear hash table */

    output(outStream, (code_int)ClearCode );

    while ( (c = GIFNextPixel(*fBitmapData)) != EOF ) {  

        ++in_count;

		fcode = (sLONG) (((sLONG) c << maxbits) + ent);
        i = (((code_int)c << hshift) ^ ent);    /* xor hashing */

        if ( HashTabOf (i) == fcode ) {
            ent = CodeTabOf (i);
            continue;
		}
		else if ((sLONG) HashTabOf(i) < 0)      /* empty slot */
            goto nomatch;
        disp = hsize_reg - i;           /* secondary hash (after G. Knott) */
        if ( i == 0 )
            disp = 1;
probe:
        if ( (i -= disp) < 0 )
            i += hsize_reg;

        if ( HashTabOf (i) == fcode ) {
            ent = CodeTabOf (i);
            continue;
        }
		if ((sLONG) HashTabOf(i) > 0)
            goto probe;
nomatch:
        output (outStream, (code_int) ent );
        ++out_count;
        ent = c;
        if ( free_ent < maxmaxcode ) { 
            CodeTabOf (i) = free_ent++; /* code -> hashtable */
            HashTabOf (i) = fcode;
        } else
                cl_block(outStream);
    }
    /*
     * Put out the final code.
     */
    output(outStream, (code_int)ent );
    ++out_count;
    output(outStream, (code_int) EOFCode );
}

void xGifEncoder::output(VStream& outStream, code_int code)
{
    cur_accum &= masks[ cur_bits ];

    if( cur_bits > 0 )
		cur_accum |= ((sLONG) code << cur_bits);
    else
        cur_accum = code;

    cur_bits += n_bits;

    while( cur_bits >= 8 ) {
        char_out(outStream,  (uBYTE)(cur_accum & 0xff) );
        cur_accum >>= 8;
        cur_bits -= 8;
    }

    /*
     * If the next entry is going to be too big for the code size,
     * then increase it, if possible.
     */
   if ( free_ent > maxcode || clear_flg ) 
   {
           if( clear_flg ) {

                maxcode = MAXCODE (n_bits = g_init_bits);
                clear_flg = 0;

            } else {

                ++n_bits;
                if ( n_bits == maxbits )
                    maxcode = maxmaxcode;
                else
                    maxcode = MAXCODE(n_bits);
            }
        }

    if( code == EOFCode ) 
	{
        /*
         * At EOF, write the rest of the buffer.
         */
        while( cur_bits > 0 ) 
		{
           char_out(outStream,  (uBYTE)(cur_accum & 0xff) );
           cur_accum >>= 8;
           cur_bits -= 8;
        }
		
        flush_char(outStream);
    }
}

void xGifEncoder::flush_char(VStream& outStream)
{
	if( a_count > 0 ) 
	{
		outStream.PutByte(a_count );
		outStream.PutBytes(accum,a_count );
		a_count = 0;
	}
}

void xGifEncoder::char_out(VStream& outStream, uBYTE c)
{
	accum[ a_count++ ] = c;
	if( a_count >= 254 )
		flush_char(outStream);
}

void xGifEncoder::cl_hash(count_int inhsize)          /* reset code table */                     
{
        register count_int *htab_p = htab+inhsize;

		register sLONG i;
		register sLONG m1 = -1;

        i = inhsize - 16;
        do {                            /* might use Sys V memset(3) here */
                *(htab_p-16) = m1;
                *(htab_p-15) = m1;
                *(htab_p-14) = m1;
                *(htab_p-13) = m1;
                *(htab_p-12) = m1;
                *(htab_p-11) = m1;
                *(htab_p-10) = m1;
                *(htab_p-9) = m1;
                *(htab_p-8) = m1;
                *(htab_p-7) = m1;
                *(htab_p-6) = m1;
                *(htab_p-5) = m1;
                *(htab_p-4) = m1;
                *(htab_p-3) = m1;
                *(htab_p-2) = m1;
                *(htab_p-1) = m1;
                htab_p -= 16;
        } while ((i -= 16) >= 0);

        for ( i += 16; i > 0; --i )
                *--htab_p = m1;
}


void xGifEncoder::cl_block (VStream& outStream)
{
	cl_hash ((count_int) hsize );
	free_ent = ClearCode + 2;
	clear_flg = 1;

	output(outStream, (code_int)ClearCode );
}


xGifEncoder::code_int xGifEncoder::GIFNextPixel(VBitmapData& inData)
{
	uBYTE* pixbuff;
	code_int r;

	if( CountDown == 0 )
		return (EOF);
	--CountDown;
	
	pixbuff=(uBYTE*)inData.GetLinePtr(cury);
	r= *(pixbuff+curx);
	++curx;
	
	if (curx >= Width)
	{
		curx = 0;
		++cury;
		
	}
	return (r);
}


