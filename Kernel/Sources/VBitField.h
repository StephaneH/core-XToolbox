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
#ifndef __VBitField__
#define __VBitField__

BEGIN_TOOLBOX_NAMESPACE

/** @brief VBitField is a utility template to manipulate a fixed size bit field **/
template<int nbBits>
struct VBitField
{
	uLONG	fData[(nbBits+31)/32];

	VBitField&	operator=( uLONG inValue)
	{
		fData[0] = inValue;
		for( int i = sizeof( fData)/4 - 1 ; i > 0 ; --i)
			fData[i] = 0;

		return *this;
	}

	VBitField&	operator|=( uLONG inValue)
	{
		fData[0] |= inValue;
		return *this;
	}

	VBitField&	operator|=( const VBitField& inValue)
	{
		for( int i = sizeof( fData)/4 - 1 ; i >= 0 ; --i)
			fData[i] |= inValue.fData[i];

		return *this;
	}

	VBitField&	operator&=( const VBitField& inValue)
	{
		for( int i = sizeof( fData)/4 - 1 ; i >= 0 ; --i)
			fData[i] &= inValue.fData[i];

		return *this;
	}

	VBitField&	operator^=( const VBitField& inValue)
	{
		for( int i = sizeof( fData)/4 - 1 ; i >= 0 ; --i)
			fData[i] ^= inValue.fData[i];

		return *this;
	}
	
	VBitField	operator~() const
	{
		VBitField result=*this;
		for( int i = sizeof( result)/4 - 1 ; i >= 0 ; --i)
			result.fData[i] = ~result.fData[i];

		return result;
	}

	void Fill( uLONG inValue)
	{
		for( int i = sizeof( fData)/4 - 1 ; i >= 0 ; --i)
			fData[i] = inValue;
	}


	bool Test( int inBitIndex) const
	{
		if (testAssert(inBitIndex >= 0 && inBitIndex < (sizeof( fData)/4)*32))
			return (fData[inBitIndex/32] & (1 << (inBitIndex % 32))) != 0;
		
		return false;
	}

	void Set( int inBitIndex)
	{
		if (testAssert(inBitIndex >= 0 && inBitIndex < (sizeof( fData)/4)*32))
			fData[inBitIndex/32] |= (1 << (inBitIndex % 32));
	}

	void Clear( int inBitIndex)
	{
		if (testAssert(inBitIndex >= 0 && inBitIndex < (sizeof( fData)/4)*32))
			fData[inBitIndex/32] &= ~(1 << (inBitIndex % 32));
	}

	uLONG Capacity() const
	{
		return (sizeof( fData)/4)*32;
	}
};


template<int nbBits>
VBitField<nbBits>	operator&( const VBitField<nbBits>& inLeftValue, const VBitField<nbBits>& inRightValue)
{
	VBitField<nbBits> r;

	for( int i = sizeof( r.fData)/4 - 1 ; i >= 0 ; --i)
		r.fData[i] = inLeftValue.fData[i] & inRightValue.fData[i];

	return r;
}

template<int nbBits>
VBitField<nbBits>	operator&( const VBitField<nbBits>& inLeftValue, uLONG inRightValue) {return inLeftValue & VBitField<nbBits>( inRightValue);}
template<int nbBits>
VBitField<nbBits>	operator&( uLONG inLeftValue, const VBitField<nbBits>& inRightValue) {return VBitField<nbBits>(inLeftValue) & inRightValue;}


template<int nbBits>
VBitField<nbBits>	operator|( const VBitField<nbBits>& inLeftValue, const VBitField<nbBits>& inRightValue)
{
	VBitField<nbBits> r;

	for( int i = sizeof( r.fData)/4 - 1 ; i >= 0 ; --i)
		r.fData[i] = inLeftValue.fData[i] | inRightValue.fData[i];

	return r;
}

template<int nbBits>
VBitField<nbBits>	operator|( const VBitField<nbBits>& inLeftValue, uLONG inRightValue) {return inLeftValue | VBitField<nbBits>( inRightValue);}
template<int nbBits>
VBitField<nbBits>	operator|( uLONG inLeftValue, const VBitField<nbBits>& inRightValue) {return VBitField<nbBits>(inLeftValue) | inRightValue;}

END_TOOLBOX_NAMESPACE


#endif
