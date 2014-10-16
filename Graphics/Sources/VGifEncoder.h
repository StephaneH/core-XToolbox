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
#ifndef __GifEncoder__
#define __GifEncoder__

BEGIN_TOOLBOX_NAMESPACE

/********************************/
// quantizer
/********************************/



typedef std::vector<VBitmapData::_ARGBPix> PaletteList;

class XTOOLBOX_API VQuantizer
{
	public:
	
	VQuantizer( bool singlePass )
	{
		_singlePass = singlePass ;
	}
	virtual ~VQuantizer()
	{

	}
	VBitmapData* Quantize( VBitmapData& source );
	virtual void FirstPass ( VBitmapData& sourceData );
	virtual void SecondPass ( VBitmapData& sourceData , VBitmapData& output);
	virtual void InitialQuantizePixel ( VBitmapData::_ARGBPix* inPix);		
	virtual uBYTE QuantizePixel ( VBitmapData::_ARGBPix* inPix ) ;	
	virtual std::vector<VColor>* GetPalette () ;
	private:
	bool _singlePass;
};

/**
  NEUQUANT Neural-Net quantization algorithm by Anthony Dekker
*/

// ----------------------------------------------------------------
// Constant definitions
// ----------------------------------------------------------------

/** number of colours used: 
	for 256 colours, fixed arrays need 8kb, plus space for the image
*/
//static const int netsize = 256;

/**@name network definitions */
//@{
//static const int maxnetpos = (netsize - 1);
/// bias for colour values
static const int netbiasshift = 4;
/// no. of learning cycles
static const int ncycles = 100;
//@}

/**@name defs for freq and bias */
//@{
/// bias for fractions
static const int intbiasshift = 16;
static const int intbias = (((int)1) << intbiasshift);
/// gamma = 1024
static const int gammashift = 10;
// static const int gamma = (((int)1) << gammashift);
/// beta = 1 / 1024
static const int betashift = 10;
static const int beta = (intbias >> betashift);
static const int betagamma = (intbias << (gammashift-betashift));
//@}

/**@name defs for decreasing radius factor */
//@{
/// for 256 cols, radius starts
//static const int initrad = (netsize >> 3);
/// at 32.0 biased by 6 bits
static const int radiusbiasshift = 6;
static const int radiusbias = (((int)1) << radiusbiasshift);
/// and decreases by a 
//static const int initradius	= (initrad * radiusbias);
// factor of 1/30 each cycle
static const int radiusdec = 30;
//@}

/**@name defs for decreasing alpha factor */
//@{
/// alpha starts at 1.0
static const int alphabiasshift = 10;
static const int initalpha = (((int)1) << alphabiasshift);
//@}

/**@name radbias and alpharadbias used for radpower calculation */
//@{
static const int radbiasshift = 8;
static const int radbias = (((int)1) << radbiasshift);
static const int alpharadbshift = (alphabiasshift+radbiasshift);
static const int alpharadbias = (((int)1) << alpharadbshift);	
//@}

#define FI_RGBA_RED				2
#define FI_RGBA_GREEN			1
#define FI_RGBA_BLUE			0
#define FI_RGBA_ALPHA			3
#define FI_RGBA_RED_MASK		0x00FF0000
#define FI_RGBA_GREEN_MASK		0x0000FF00
#define FI_RGBA_BLUE_MASK		0x000000FF
#define FI_RGBA_ALPHA_MASK		0xFF000000
#define FI_RGBA_RED_SHIFT		16
#define FI_RGBA_GREEN_SHIFT		8
#define FI_RGBA_BLUE_SHIFT		0
#define FI_RGBA_ALPHA_SHIFT		24


class VNNQuantizer : public VQuantizer
{
private:
VNNQuantizer& operator=(const VNNQuantizer&){assert(false);return *this;}
protected:
	/**@name image parameters */
	//@{
	/// pointer to input dib
	//FIBITMAP *dib_ptr;
	/// image width
	int img_width;
	/// image height
	int img_height;
	/// image line length
	int img_line;
	//@}

	/**@name network parameters */
	//@{

	int netsize, maxnetpos, initrad, initradius;

	/// BGRc
	typedef int pixel[4];
	/// the network itself
	pixel *network;

	/// for network lookup - really 256
	int netindex[256];

	/// bias array for learning
	int *bias;
	/// freq array for learning
	int *freq;
	/// radpower for precomputation
	int *radpower;
	//@}
	int fsampling;
protected:
	/// Initialise network in range (0,0,0) to (255,255,255) and set parameters
	void initnet();	

	/// Unbias network to give byte values 0..255 and record position i to prepare for sort
	void unbiasnet();

	/// Insertion sort of network and building of netindex[0..255] (to do after unbias)
	void inxbuild();

	/// Search for BGR values 0..255 (after net is unbiased) and return colour index
	int inxsearch(int b, int g, int r);

	/// Search for biased BGR values
	int contest(int b, int g, int r);
	
	/// Move neuron i towards biased (b,g,r) by factor alpha
	void altersingle(int alpha, int i, int b, int g, int r);

	/// Move adjacent neurons by precomputed alpha*(1-((i-j)^2/[r]^2)) in radpower[|i-j|]
	void alterneigh(int rad, int i, int b, int g, int r);

	/** Main Learning Loop
	@param sampling_factor sampling factor in [1..30]
	*/
	void learn(VBitmapData& sourceData,int sampling_factor);

	/// Get a pixel sample at position pos. Handle 4-byte boundary alignment.
	void getSample(VBitmapData& sourceData,long pos, int *b, int *g, int *r);


public:
	virtual void FirstPass ( VBitmapData& sourceData);
	virtual uBYTE QuantizePixel ( VBitmapData::_ARGBPix* inPix ) ;
	virtual std::vector<VColor>* GetPalette ();
	/// Constructor
	VNNQuantizer(int PaletteSize,int sampling);
	
	/// Destructor
	~VNNQuantizer();
};

class XTOOLBOX_API VOctreeNode
{
	private:
	VOctreeNode(const VOctreeNode&){assert(false);}
	VOctreeNode& operator=(const VOctreeNode&){assert(false);return *this;}
	public:
	VOctreeNode ( int level , int colorBits ,class VOctree* octree );
	virtual ~VOctreeNode();
	
	void AddColor ( VBitmapData::_ARGBPix* pixel , int colorBits , int level , class VOctree* octree );
	VOctreeNode* GetNextReducible(){return _nextReducible;}
	void SetNextReducible(VOctreeNode* in){_nextReducible=in;}
	int Reduce ( );
	void ConstructPalette ( PaletteList* palette , int &paletteIndex );
	int GetPaletteIndex ( VBitmapData::_ARGBPix* pixel , int level );
	void Increment ( VBitmapData::_ARGBPix* pixel );
	private:
	bool						_leaf ;
	int							_pixelCount ;
	int							_red ;
	int							_green ;
	int							_blue ;
	std::vector<VOctreeNode*>	_children ;
	VOctreeNode*					_nextReducible;
	int							_paletteIndex ;
};

class XTOOLBOX_API VOctree
{
	private:
	VOctree ( const VOctree&){assert(false);}
	VOctree& operator=(const VOctree&){assert(false);return *this;}
	public:
	VOctree ( int maxColorBits );
	virtual ~VOctree ( )
	{
		;
	}
	
	void AddColor ( VBitmapData::_ARGBPix* pixel );
	void Reduce ( );
	int GetLeaves(){return _leafCount;}
	void SetLeaves(int inVal){_leafCount=inVal;}
	void TrackPrevious ( VOctreeNode* node );
	int GetPaletteIndex ( VBitmapData::_ARGBPix* pixel );
	PaletteList* Palletize ( int colorCount );
	VOctreeNode* GetReducibleNodes(int index){return _reducibleNodes[index];}
	void SetReducibleNodes(int index,VOctreeNode* inNode){_reducibleNodes[index]=inNode;}
	static int mask[8];
	private:
	
	VOctreeNode*			_root;
	int					_leafCount;
	std::vector<VOctreeNode*>	_reducibleNodes;
	int							_maxColorBits;
	VOctreeNode*					_previousNode;
	VBitmapData::_ARGBPix					_previousColor;
};

class XTOOLBOX_API VOctreeQuantizer : public VQuantizer
{
	private:
	VOctreeQuantizer& operator=(const VOctreeQuantizer&){assert(false);return *this;}
	VOctreeQuantizer(const VOctreeQuantizer&):VQuantizer(false){assert(false);}
	public:
	VOctreeQuantizer ( int maxColors , int maxColorBits );
	virtual ~VOctreeQuantizer();
	
	virtual void InitialQuantizePixel ( VBitmapData::_ARGBPix* pixel );
	virtual uBYTE QuantizePixel ( VBitmapData::_ARGBPix* pixel );
	virtual std::vector<VColor>* GetPalette () ;
	private:

	VOctree*			_octree;
	int				_maxColors ;
};

/*************************************************/
// gif encoder
/*************************************************/

#define HSIZE 5003
#define GIFBITS 12

class XTOOLBOX_API xGifEncoder : public VObject
{
	private:
	typedef unsigned char char_type;
	typedef long code_int;
	typedef long count_int;
	xGifEncoder(const xGifEncoder&)
	{
		assert(false);
	}
	xGifEncoder& operator=(const xGifEncoder&){assert(false);return *this;}
	public:
	xGifEncoder();
	virtual ~xGifEncoder();

	short GIFEncode(VStream& outStream,VBitmapData* inBitmapData, sLONG GInterlace, sLONG Background, sLONG Transparent);

	private:
	
	void flush_char(VStream& outStream);
	void char_out(VStream& outStream, uBYTE c);
	
	void cl_hash(count_int inhsize);
	void output(VStream& outStream, code_int code);
	void cl_block (VStream& outStream);
	void compress(VStream& outStream, sLONG init_bits);
	void char_init(){a_count = 0;}
	xGifEncoder::code_int GIFNextPixel(VBitmapData& inData);

	void Init();

	static unsigned long masks[17];

	sLONG Width;
	sLONG Height;
	sLONG curx;
	sLONG cury;
	sLONG CountDown;
	sLONG Pass;
	sLONG Interlace;
	sLONG a_count;
	uLONG cur_accum;
	sLONG cur_bits;
	sLONG g_init_bits;
	sLONG ClearCode;
	sLONG EOFCode;
	long free_ent;
	sLONG clear_flg;
	sLONG offset;
	sLONG in_count;
	sLONG out_count;	
	long hsize;
	sLONG n_bits;
	sLONG maxbits;
	long maxcode;
	long maxmaxcode;

	count_int htab[HSIZE];
	unsigned short codetab[HSIZE];
	uBYTE accum[256];
	VBitmapData* fBitmapData;
};

END_TOOLBOX_NAMESPACE
#endif