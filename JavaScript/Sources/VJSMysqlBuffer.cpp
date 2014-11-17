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

#include "VJavaScriptPrecompiled.h"

#include "VJSMysqlBuffer.h"
#include "VJSContext.h"
#include "VJSGlobalClass.h"
#include "VJSRuntime_blob.h"

USING_TOOLBOX_NAMESPACE

VJSMysqlBufferObject::VJSMysqlBufferObject ()
{
    fState = -2;
    fToAdvance = 0;
    fRead = 0;
    fReaded = 0;
    fOffsetRead = 0;
    eof = false;
    fpos = 0;
}

VJSMysqlBufferObject::~VJSMysqlBufferObject()
{
    for ( uLONG i = 0; i < fBuffer.size(); i++ )
    {
        fBuffer[i]->Release();
    }
}

void VJSMysqlBufferObject::AddBufferSimple ( VJSBufferObject *targetBuffer )
{
    targetBuffer->Retain();
    fBuffer.push_back ( targetBuffer );
}

Boolean VJSMysqlBufferObject::GoNext ( uLONG len )
{
    //GoNext purpose is to advance to the current position to "len" next bytes
    //if we reach the end of the buffer and it still remain some bytes in this case
    //we set fToAdvance to the number of bytes to advance and we return false
    if ( fOffset <= fBufferLength - len )
    {
        fOffset += len;
        return true;
    }

    fToAdvance = fOffset - fBufferLength + len;
    return false;
}

Boolean VJSMysqlBufferObject::ReadNext ( VJSBufferObject *inBuffer )
{
    //fToRead determines the number of bytes that we shall read
    //we read byte per byte and we incremente the values and decremente fToread
    //  - if we success to read all the desired bytes we set fToRead to 0 and return true
    //  - else return false
    while ( fToRead > fReaded )
    {

        if ( fOffset <= fBufferLength - 1 )
        {
            fRead |= ( ( uBYTE * ) ( inBuffer->GetDataPtr () ) ) [fOffset] << ( 8 * fReaded );
            fReaded++;
            fOffset++;
        }
        else
        {
            break;
        }
    }

    if ( fToRead == fReaded )
    {
        fReaded = 0;
        return true;
    }
    return false;
}

Boolean VJSMysqlBufferObject::AddBuffer ( VJSBufferObject *inBuffer )
{
    inBuffer->Retain();
    fBuffer.push_back ( inBuffer );
    Boolean continueReading = true;
    fOffset = 0;
    fBufferLength = inBuffer->GetDataSize();
    /*
    -2 =>
    */

    while ( continueReading )
    {
        switch ( fState )
        {
            /*
            Here we read the packet length in the three first bytes
            and the packet number in the fourth byte
            */
        case -2 :
            fToAdvance = 0;
            SetToRead ( 3 );
            if ( ReadNext ( inBuffer ) )
            {

                fState ++;
                fPackLength = fRead;
                fToAdvance = 1;
            }
            else
            {
                continueReading = false;
            }
            break;

        case -1 :
            if ( GoNext ( fToAdvance ) )
            {
                fState ++;
            }
            else
            {
                continueReading = false;
            }
            break;

        case 0 :
            SetToRead ( 1 );
            if ( ReadNext ( inBuffer ) )
            {
                fState ++;
                if ( fRead == 0 )
                {
                    fState = 10;
                    fOffsetRead = 0;
                }
                else if ( fRead == 255 )
                {
                    fState = 20;
                    fToAdvance = fPackLength - 2;
                }
                else
                {
                    fState = 31;
                }
            }
            else
            {
                continueReading = false;
            }
            break;

        case 12 :
        case 20 :
        case 38 :
        case 41 :
        case 46 :
        case 60 :
        case 15 :
        case 18 :
            if ( GoNext ( fToAdvance ) )
            {
                fState ++;
            }
            else
            {
                continueReading = false;
            }
            break;

        case 10 :
        case 13 :
        case 47 :
            SetToRead ( 1 );
            if ( ReadNext ( inBuffer ) )
            {
                fState ++;
            }
            else
            {
                continueReading = false;
            }
            break;

        case 16 :
            SetToRead ( 2 );
            fState ++;
            break;

        case 17 :
            if ( ReadNext ( inBuffer ) )
            {
                fToAdvance = fPackLength - fOffsetRead - 5;
                fState ++;
                if ( !eof )
                {
                    eof = ( ( fRead & 8 ) == 0 );
                }
            }
            else
            {
                continueReading = false;
            }
            break;

        case 19 :
            if ( eof )
            {
                return false;
            }
            fState = -2;
            break;

        case 21 :
            return false;
        case 11 :
        case 14 :
        case 31 :
            if ( fRead < 251 )
            {
                fToAdvance = 0;
            }
            else if ( fRead == 251 )
            {
                fToAdvance = 0;
            }
            else if ( fRead == 252 )
            {
                fToAdvance = 2;
            }
            else if ( fRead == 253 )
            {
                fToAdvance = 3;
            }
            else if ( fRead == 254 )
            {
                fToAdvance = 8;
            }
            fOffsetRead += fToAdvance;
            fState ++;
            break;

        case 32:
            if ( fOffsetRead == 0 )
            {
                fState += 2;
            }
            else
            {
                SetToRead ( fToAdvance );
                fState ++;
            }
            break;

        case 33 :
        case 36 :
        case 44 :
        case 62 :
            if ( ReadNext ( inBuffer ) )
            {
                fState ++;
            }
            else
            {
                continueReading = false;
            }
            break;

        case 34:
            fOffsetRead = fRead;
            fState ++;
            break;

        case 35:
            SetToRead ( 3 );
            fState ++;
            break;

        case 37:
            fToAdvance = fRead + 1;
            fState ++;
            break;

        case 39:
            fOffsetRead --;
            if ( fOffsetRead == 0 )
            {
                fState ++;
            }
            else
            {
                fState -= 4;
            }
            break;

        case 40:
            fToAdvance = 9;
            fState ++;
            break;

        case 42:
            fRowCount = 0;
            fRowsStart.push_back ( std::pair<uLONG, uLONG> ( fBuffer.size() - 1, fOffset ) );
            fState ++;
            break;

        case 43:
            SetToRead ( 3 );
            fState ++;
            break;

        case 45:
            if ( fRead == 5 )
            {
                fToAdvance = 1;
                fState ++;
            }
            else
            {
                fToAdvance = fRead + 1;
                fRowCount ++;
                fState  = 50;
            }
            break;

        case 50 :

            if ( GoNext ( fToAdvance ) )
            {
                fState  = 43;
            }
            else
            {
                continueReading = false;
            }
            break;

        case 48 :
            if ( fRead == 254 )
            {
                fToAdvance = 2;
                fState = 60;
            }
            else
            {
                fToAdvance = 4;
                fRowCount ++;
                fState = 50;
            }
            break;

        case 61 :
            SetToRead ( 2 );
            fState ++;
            break;

        case 63 :
            fRowsCount.push_back ( fRowCount );
            if ( fOffset > 9 )
            {
                fRowsEnd.push_back ( std::pair<uLONG, uLONG> ( fBuffer.size() - 1, fOffset - 10 ) );
            }
            else
            {
                fRowsEnd.push_back ( std::pair<uLONG, uLONG> ( fBuffer.size() - 2, fBuffer.size() - 9 + fOffset ) );
            }

            if ( ( fRead & 8 ) == 0 )
            {
                return false;
            }
            else
            {
                fState = -2;
            }

            if ( eof )
            {
                return false;
            }
            break;
        }
    }

    return true;
}


void VJSMysqlBufferObject::InitList()
{
    fOffset = 0;
    fpos = 0;
    fBufferLength = fBuffer[fpos]->GetDataSize();
    fOffsetRead = 0;
    fCoordIter = -1;
}


uLONG VJSMysqlBufferObject::GetRowsCount()
{
    return fRowsCount[fCoordIter];
}


VJSArray VJSMysqlBufferObject::Fetch ( VJSContext &inContext, sLONG inCount, uLONG inFieldCount, VJSArray &titles, VJSArray &types )
{
    //read VJSMysqlBufferObject::AdvanceFetch(sLONG inCount) first
    VJSArray fetchedRows ( inContext );
    fetchedRows.Clear();

    VJSObject row ( inContext );
    VString title;
    sLONG type;

    if ( inCount == -1 )
    {
        inCount = fRowsCount[fCoordIter];
    }

    if ( inCount != 0 )
    {
        if ( inCount > ( fRowsCount[fCoordIter] - fRowPosition ) )
        {
            inCount = fRowsCount[fCoordIter] - fRowPosition;
        }

        for ( uLONG i = 0; i < inCount ; i ++ )
        {
            row.MakeEmpty();
            Advance ( 4 );
            for ( uLONG t = 0; t < inFieldCount; t ++ )
            {
                titles.GetValueAt ( t ).GetString ( title );
                types.GetValueAt ( t ).GetLong ( &type );
                sLONG lcb = ReadLCB();
                if ( lcb >= 0 )
                {
                    VString value;

                    if ( type == eFIELD_TYPE_BIT )
                    {
                        //bit type returns a non printables characters so
                        //the lcb string will be empty a special processing is done
                        //here
                        uBYTE b = ReadUInt8();
                        row.SetProperty ( title, b != 0, JS4D::PropertyAttributeDontDelete );
                    }
                    else if ( type == eFIELD_TYPE_TINY_BLOB ||
                              type == eFIELD_TYPE_MEDIUM_BLOB ||
                              type == eFIELD_TYPE_LONG_BLOB ||
                              type == eFIELD_TYPE_BLOB )
                    {
                        if ( lcb >= 0 )
                        {
                            uBYTE *buffer = ( uBYTE * ) ::malloc ( lcb );
                            sLONG count = lcb;
                            sLONG index = 0;

                            while ( true )
                            {
								sLONG length = fBuffer[fpos]->GetDataSize() - fOffset;

								uBYTE *bufferPtr = (uBYTE*)fBuffer[fpos]->GetDataPtr();
								bufferPtr += fOffset;

                                if ( count < length )
                                {
									::memcpy ( buffer + index, bufferPtr, count );
                                    fOffset += count;
                                    break;
                                }
                                else
                                {
									::memcpy ( buffer + index, bufferPtr, length );
                                    count -= length;
                                    index += length;
                                    ++fpos;
									fOffset = 0;
                                }
                            }

                            row.SetProperty ( title, VJSBlob::NewInstance ( inContext, buffer, lcb, CVSTR( "")), JS4D::PropertyAttributeDontDelete );

                            ::free ( buffer );
                        }
                        else
                        {
                            row.SetNullProperty ( title );
                        }
                    }
                    else
                    {
                        value = ReadLCBString ( lcb );
                        FormatPropertyValue ( inContext, row, value, title, type );
                    }

                }
                else
                {
                    row.SetNullProperty ( title );
                }
            }

            fetchedRows.PushValue ( row );
        }

        fRowPosition += inCount;
    }

    return fetchedRows;
}


void VJSMysqlBufferObject::AdvanceFetch ( sLONG inCount )
{
    // Advance Fetch means, skip the next inCount rows
    // if inCount = -1 we should skip all rows
    if ( inCount == -1 )
    {
        inCount = fRowsCount[fCoordIter];
    }

    if ( inCount != 0 )
    {
        if ( inCount <= ( fRowsCount[fCoordIter] - fRowPosition ) )
        {
            // if the number of rows to skip is less than rest of no skiped rows
            for ( uLONG i = 0; i < inCount ; i ++ )
            {
                Advance ( ReadUInt24LE() + 1 ); //next row
            }

            fRowPosition += inCount;

        }
        else
        {
            // if the number of rows to skip is more than the rest of non skiped rows
            //-->       then we skip only the rest

            for ( uLONG i = 0; i < fRowsCount[fCoordIter] - fRowPosition ; i ++ )
            {
                Advance ( ReadUInt24LE() + 1 );
            }

            fRowPosition = fRowsCount[fCoordIter];

        }
    }

}


void VJSMysqlBufferObject::PrepareSelectFetch()
{
    if ( fCoordIter >= 0 )
    {
        //if this is not the first time we must iterate to the next statement
        fCoordIter++;
    }
    else
    {
        //before fetching results fCoordIter is set to -1
        fCoordIter = 0;
    }
    //the buffer position which contain the start of rows data in buffers list
    fpos = fRowsStart[fCoordIter].first;
    //Byte position start in the selected buffer
    fOffset = fRowsStart[fCoordIter].second;
    //row position is set to 0 because we didn't start the fetch
    fRowPosition = 0;
}


void VJSMysqlBufferObject::SaveRowsPosition()
{
    uLONG rowsCount = 0;
    //save the rows data start position
    fRowsStart.push_back ( std::pair<uLONG, uLONG> ( fpos, fOffset ) );

    //see IsRow function first
    while ( IsRow() )   //if the next packet is not an EOF
    {
        rowsCount++;
    }
    //if the next packet is an EOF
    //we save the position at the end of RowsData
    fRowsEnd.push_back ( std::pair<uLONG, uLONG> ( fpos, fOffset ) );
    //we save the number of rows of the current statement
    fRowsCount.push_back ( rowsCount );
}


void VJSMysqlBufferObject::SkipRowsData ()
{
    if ( fCoordIter >= 0 )
    {
        //if this is not the first time we must iterate to the next statement
        fCoordIter++;
    }
    else
    {
        //before fetching results fCoordIter is set to -1
        fCoordIter = 0;
    }
    //the buffer position which contain the start of rows data in buffers list
    fpos = fRowsEnd[fCoordIter].first;

    //Byte position start in the selected buffer
    fOffset = fRowsEnd[fCoordIter].second + 1;

    if ( fRowsStart.size() - fCoordIter == 1 )
    {
        fCoordIter = -1;
    }

    //because SkipRowsData just before the conncatenation which use fCoordIter for the fetch.
    //if we reach the last statement we reset fCoordIter to -1
}


uWORD VJSMysqlBufferObject::GetSize()
{
    return fBuffer.size();
}

uLONG VJSMysqlBufferObject::GetSelectResultCount()
{
    //return fRowsList.size();
    return ( uLONG ) 0;
}


uLONG VJSMysqlBufferObject::GetOffsetRead()
{
    uLONG offsetRead = fOffsetRead;
    fOffsetRead = 0;
    return offsetRead;
}

uBYTE VJSMysqlBufferObject::ReadNextHex()
{
    if ( fOffset <= fBufferLength - 1 )
    {
        return ( ( uBYTE * ) ( fBuffer[fpos]->GetDataPtr () ) ) [fOffset];
    }
    else
    {
        fpos++;
        fOffset = 0;
        fBufferLength = fBuffer[fpos]->GetDataSize();
        return ( ( uBYTE * ) ( fBuffer[fpos]->GetDataPtr() ) ) [fOffset];
    }
}

Boolean VJSMysqlBufferObject::HasNext()
{
    return ( Boolean ) ( fRowPosition < fRowsCount[fCoordIter] );
    //(Boolean)!(fpos == fRowsEnd[fCoordIter].first && fOffset >= fRowsEnd[fCoordIter].second);
}

Boolean VJSMysqlBufferObject::IsRow()
{
    if (
        //if it is the last 9 bytes of a buffer
        fOffset == fBufferLength - 9
        //and if if the 5th field is 254 ie it is EOF
        && ( ( uBYTE * ) ( fBuffer[fpos]->GetDataPtr() ) ) [fBufferLength - 5] == 0xfe
        //and we have exactly 5 bytes (because an EOF contains 5 bytes)
        && ( ( uBYTE * ) ( fBuffer[fpos]->GetDataPtr() ) ) [fBufferLength - 9] == 5
    )
    {
        return false;
    }

    Advance ( ReadUInt24LE() + 1 );
    return true;
}

void VJSMysqlBufferObject::Advance ( uLONG len )
{
    //to position the fOffset to the next len bytes,
    //if the the number of bytes aren't enough we go to the next buffer and continue advancing
    if ( fOffset <= fBufferLength - len )
    {
        fOffset += len;
    }
    else
    {
        fpos++;
        fOffset += len - fBufferLength;
        fBufferLength = fBuffer[fpos]->GetDataSize();
    }
}

uBYTE VJSMysqlBufferObject::ReadUInt8()
{
    //read one byte
    //although we are at the buffer 's end we pointe to the next buffer and and at the start of this buffer fOffset = 0
    if ( fOffset <= fBufferLength - 1 )
    {
        return ( ( uBYTE * ) ( fBuffer[fpos]->GetDataPtr () ) ) [fOffset++];
    }
    else
    {
        fpos++;
        fOffset = 0;
        fBufferLength = fBuffer[fpos]->GetDataSize();
        return ( ( uBYTE * ) ( fBuffer[fpos]->GetDataPtr() ) ) [fOffset++];
    }

}

uWORD VJSMysqlBufferObject::ReadUInt16LE()
{
    uWORD l = ReadUInt8();
    uWORD b = ReadUInt8();
    uWORD result = l | ( b << 8 );
    return result;
}

uLONG VJSMysqlBufferObject::ReadUInt24LE()
{
    uLONG l = ReadUInt8();
    uLONG b = ReadUInt16LE();
    uLONG result = l | ( b << 8 );
    return result;
}

uLONG VJSMysqlBufferObject::ReadUInt32LE()
{
    uLONG l = ReadUInt16LE();
    uLONG b = ReadUInt16LE();
    uLONG result = l | ( b << 16 );
    return result;
}

uLONG8 VJSMysqlBufferObject::ReadUInt64LE()
{
    uLONG8 l = ReadUInt32LE();
    uLONG8 b = ReadUInt32LE();
    uLONG8 result = l | ( b << 32 );
    return result;
}

sLONG VJSMysqlBufferObject::ReadLCB ()
{
    //Read Length Coded Binary Number
    uBYTE firstCoded = ReadUInt8();
    if ( firstCoded < 251 )
    {
        return ( uLONG ) firstCoded;
    }
    else if ( firstCoded == 251 )
    {
        //The 251 is for null value so to distinguish between a NULL value
        //and an empty string value we return -1 here
        return -1;
    }
    else if ( firstCoded == 252 )
    {
        return ( uLONG ) ReadUInt16LE();
    }
    else if ( firstCoded == 253 )
    {
        return ReadUInt24LE();
    }
    else if ( firstCoded == 254 )
    {
        return ReadUInt64LE();
    }
    else
    {
        return 0;
    }
}

uLONG VJSMysqlBufferObject::ReadLCBPlus()
{
    //Read Length Coded Binary Number and add it to fOffsetRead ,ReadLCBPlus is used only with OK
    //packet to know how many bytes have been read, to verify if the message field(optional) exists or not
    uLONG lcb;
    uBYTE firstCoded = ReadUInt8();
    fOffsetRead++;
    if ( firstCoded < 251 )
    {
        lcb = ( uLONG ) firstCoded;
    }
    else if ( firstCoded == 251 )
    {
        lcb = 0;
    }
    else if ( firstCoded == 252 )
    {
        lcb = ( uLONG ) ReadUInt16LE();
        fOffsetRead += 2;
    }
    else if ( firstCoded == 253 )
    {
        lcb = ReadUInt24LE();
        fOffsetRead += 3;
    }
    else
    {
        lcb = ReadUInt64LE();
        fOffsetRead += 8;
    }

    return lcb;
}


VString VJSMysqlBufferObject::ReadString ( uLONG bytesLength )
{
    //read String of bytesLength bytes (the argument)
    CharSet encoding = VTC_UTF_8;
    VString   decodedString;

    if ( fOffset + bytesLength <= fBufferLength )
    {
        //if the string bytes are in one buffer
        fBuffer[fpos]->ToString ( encoding, fOffset, fOffset + bytesLength , &decodedString );
    }
    else
    {
        //if the string bytes are shared between many buffers
        VString   subDecodedString;
        uLONG stillToRead;
        //how much bytes we still need to complete reading all string bytes
        stillToRead = bytesLength + fOffset - fBufferLength;
        decodedString = "";

        fBuffer[fpos]->ToString ( encoding, fOffset, fBufferLength, &subDecodedString );
        decodedString.AppendString ( subDecodedString );

        while ( stillToRead != 0 )
        {
            fpos++;
            fOffset = 0;
            fBufferLength = fBuffer[fpos]->GetDataSize();

            if ( stillToRead <= fBufferLength )
            {
                //here, because one "string bytes" may will be shared in not only 2 buffers but even more
                fBuffer[fpos]->ToString ( encoding, fOffset, stillToRead, &subDecodedString );
                fOffset += stillToRead;
                decodedString.AppendString ( subDecodedString );
                break;

            }
            else
            {
                //if we read the full buffer and we haven't achieved yet reading the whole string
                fBuffer[fpos]->ToString ( encoding, fOffset, fBufferLength, &subDecodedString );
                stillToRead -= fBufferLength;
                decodedString.AppendString ( subDecodedString );
            }
        }
    }

    return decodedString;
}


VString VJSMysqlBufferObject::ReadLCBString ( uLONG lcb )
{
    //the same analyse as the ReadLCBString above, expect that the string that
    //we gonna read is written in the value of the Length Coded Binary bytes
    //read String writen in "Length Coded Binary" bytes
    VString decodedString;
    CharSet encoding = VTC_UTF_8;

    if ( fOffset + lcb <= fBufferLength )
    {
        fBuffer[fpos]->ToString ( encoding, fOffset, fOffset + lcb , &decodedString );
        fOffset += lcb;
    }
    else
    {
        VString   subDecodedString;
        uLONG stillToRead;
        stillToRead = lcb + fOffset - fBufferLength;
        //decodedString = "";
        fBuffer[fpos]->ToString ( encoding, fOffset, fBufferLength, &subDecodedString );
        decodedString.AppendString ( subDecodedString );
        while ( stillToRead != 0 )
        {
            fpos++;
            fOffset = 0;
            fBufferLength = fBuffer[fpos]->GetDataSize();
            if ( stillToRead <= fBufferLength )
            {
                fBuffer[fpos]->ToString ( encoding, fOffset, stillToRead, &subDecodedString );
                fOffset += stillToRead;
                decodedString.AppendString ( subDecodedString );
                break;
            }
            else
            {
                fBuffer[fpos]->ToString ( encoding, fOffset, fBufferLength, &subDecodedString );
                stillToRead -= fBufferLength;
                decodedString.AppendString ( subDecodedString );

            }
        }
    }
    return decodedString;
}

void VJSMysqlBufferObject::FormatPropertyValue ( VJSContext &inContext, VJSObject &row, VString &inValue, VString &inTitle, uLONG inType )
{
    if ( inType == eFIELD_TYPE_TINY ||
            inType == eFIELD_TYPE_SHORT ||
            inType == eFIELD_TYPE_LONG ||
            inType == eFIELD_TYPE_INT24 ||
            inType == eFIELD_TYPE_YEAR )
    {
        row.SetProperty ( inTitle, inValue.GetLong8(), JS4D::PropertyAttributeDontDelete );
    }
    else if ( inType == eFIELD_TYPE_LONGLONG )
    {
        sLONG8 lVal = inValue.GetLong8();
        //according to this link javascript numbers are between
        //-9007199254740992 and 9007199254740992 inclusive
        //http://www.irt.org/script/1031.htm

        if ( lVal >= -9007199254740992LL && lVal <= 9007199254740992LL )
        {
            row.SetProperty ( inTitle, lVal, JS4D::PropertyAttributeDontDelete );
        }
        else
        {
            row.SetProperty ( inTitle, inValue, JS4D::PropertyAttributeDontDelete );
        }
    }
    else if ( inType == eFIELD_TYPE_FLOAT ||
              inType == eFIELD_TYPE_DOUBLE )
    {
        row.SetProperty ( inTitle, inValue.GetReal(), JS4D::PropertyAttributeDontDelete );
    }
    else if ( inType == eFIELD_TYPE_NULL )
    {
        row.SetNullProperty ( inTitle );
    }

    else if ( inType == eFIELD_TYPE_DATE ||
              inType == eFIELD_TYPE_NEWDATE ||
              inType == eFIELD_TYPE_DATETIME ||
              inType == eFIELD_TYPE_TIMESTAMP )
    {
        VJSValue value ( inContext );
        VTime time;
        inValue.GetTime ( time );
        value.SetTime ( time );
        row.SetProperty ( inTitle, value, JS4D::PropertyAttributeDontDelete );
    }
    //else if( inType == eFIELD_TYPE_TIME )
    //{
    //	VJSValue value ( inContext );
    //	VDuration duration;
    //	value.SetDuration(duration);
    //	row.SetProperty ( inTitle, value, JS4D::PropertyAttributeDontDelete );
    //}

    else if ( inType == eFIELD_TYPE_SET )
    {
        VectorOfVString v;
        VJSArray stringArray ( inContext );
        inValue.GetSubStrings ( ",", v );
        for ( uLONG i = 0; i < v.size(); ++i )
        {
            stringArray.PushString ( v[i] );
        }
        row.SetProperty ( inTitle, stringArray, JS4D::PropertyAttributeDontDelete );
    }
    else
    {
        row.SetProperty ( inTitle, inValue, JS4D::PropertyAttributeDontDelete );
    }
}


void VJSMysqlBufferClass::Finalize ( const VJSParms_finalize &inParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    inMysqlBuffer->Release();
}


void VJSMysqlBufferClass::Initialize ( const VJSParms_initialize &inParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    inMysqlBuffer->Retain();
}

void VJSMysqlBufferClass::Construct ( VJSParms_construct &ioParms )
{
    VJSMysqlBufferObject *inMysqlBuffer = new VJSMysqlBufferObject();

    if ( inMysqlBuffer == NULL )
    {
        vThrowError ( VE_UNKNOWN_ERROR );
        return;
    }

    ioParms.ReturnConstructedObject<VJSMysqlBufferClass> ( inMysqlBuffer );
}


void VJSMysqlBufferClass:: _addBuffer ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    xbox_assert ( inMysqlBuffer != NULL );

    VJSObject targetObject ( ioParms.GetContext() );
    if ( !ioParms.CountParams() || !ioParms.GetParamObject ( 1, targetObject ) || !targetObject.IsInstanceOf ( "Buffer" ) )
    {
        vThrowError ( VE_INVALID_PARAMETER );
        return;
    }

    VJSBufferObject *targetBuffer;
    targetBuffer = targetObject.GetPrivateData<VJSBufferClass>();
    xbox_assert ( targetBuffer != NULL );

    //if the second argument doesn't exist or it is equal to 0 we shall analyze
    //the buffer as a general case

    //queryT = 2nd argument
    //      queryT = 0 -> we shall analyze all buffers bytes by byte because end is not known
    //      queryT = 1 -> we shall stop buffers concatenation after the first statement analysis
    //      queryT = 2 -> we shall just analyze the 9 bytes for every buffer received until EOF packet
    //      queryT = 3 -> we shall concatenate just the first buffer

    if ( ioParms.CountParams() == 2 )
    {
        sLONG queryType;
        if ( !ioParms.GetLongParam ( 2, &queryType ) )
        {
            vThrowError ( VE_INVALID_PARAMETER );
            return;
        }
        if ( queryType == 1 )
        {
            inMysqlBuffer->eof = true;
        }
        else if ( queryType == 2 )
        {
            inMysqlBuffer->AddBufferSimple ( targetBuffer );
            uBYTE *vBuffer = ( uBYTE * ) ( targetBuffer->GetDataPtr() );
            if (
                //this field signals that this packet is EOF packet
                vBuffer[targetBuffer->GetDataSize() - 5] == 0xfe
                //number of usefull data is 5(then an EOF packet)
                && vBuffer[targetBuffer->GetDataSize() - 9] == 5
                //no more info from the server
                && ( ( ( vBuffer[targetBuffer->GetDataSize() - 2] + ( vBuffer[targetBuffer->GetDataSize() - 1] << 8 ) ) & 8 ) == 0 )
            )
            {
                ioParms.ReturnBool ( false );
                return;
            }
            ioParms.ReturnBool ( true );
            return;
        }
        else if ( queryType == 3 )
        {
            inMysqlBuffer->AddBufferSimple ( targetBuffer );
            ioParms.ReturnBool ( false );
            return;
        }
    }
    ioParms.ReturnBool ( inMysqlBuffer->AddBuffer ( targetBuffer ) == TRUE );
}


void VJSMysqlBufferClass:: _readNextHex ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    ioParms.ReturnNumber ( inMysqlBuffer->ReadNextHex() );
}


void VJSMysqlBufferClass:: _isRow ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    ioParms.ReturnBool ( inMysqlBuffer->IsRow() == TRUE );
}


void VJSMysqlBufferClass:: _hasNext ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    ioParms.ReturnBool ( inMysqlBuffer->HasNext() == TRUE );
}

void VJSMysqlBufferClass:: _getSize ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    ioParms.ReturnNumber ( inMysqlBuffer->GetSize() );
}

void VJSMysqlBufferClass:: _fetch ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    sLONG inCount;
    sLONG fieldCount;
    VJSContext inContext ( ioParms.GetContext() );
    VJSArray titles ( inContext );
    VJSArray types ( inContext );

    if (    ioParms.CountParams() != 4
            ||  !ioParms.IsNumberParam ( 1 )
            ||  !ioParms.IsNumberParam ( 2 )
            ||  !ioParms.GetLongParam ( 1, &inCount ) // the number of fetched rows(gonna to fetch)
            ||  !ioParms.GetLongParam ( 2, &fieldCount ) // the number of columns
            ||  !ioParms.GetParamArray ( 3, titles ) // the array of columns name
            ||  !ioParms.GetParamArray ( 4, types ) )   // array of the  types of each column
    {

        vThrowError ( VE_INVALID_PARAMETER );
        return;

    }

    ioParms.ReturnValue ( inMysqlBuffer->Fetch ( inContext, inCount, ( uLONG ) fieldCount, titles, types ) );
}


void VJSMysqlBufferClass:: _advanceFetch ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    sLONG inCount;

    if ( ioParms.CountParams() != 1 ||   !ioParms.IsNumberParam ( 1 ) ||    !ioParms.GetLongParam ( 1, &inCount ) )
    {
        vThrowError ( VE_INVALID_PARAMETER );
        return;
    }
    inMysqlBuffer->AdvanceFetch ( inCount );
}


void VJSMysqlBufferClass:: _getSelectResultCount ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    ioParms.ReturnNumber ( inMysqlBuffer->GetSelectResultCount() );
}

void VJSMysqlBufferClass:: _getRowsCount ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    ioParms.ReturnNumber ( inMysqlBuffer->GetRowsCount() );
}

void VJSMysqlBufferClass:: _getOffsetRead ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    ioParms.ReturnNumber ( inMysqlBuffer->GetOffsetRead() );
}

void VJSMysqlBufferClass:: _initList ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    inMysqlBuffer->InitList();
}

void VJSMysqlBufferClass:: _readUInt8 ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    ioParms.ReturnNumber ( inMysqlBuffer->ReadUInt8() );
}

void VJSMysqlBufferClass:: _readUInt16LE ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    ioParms.ReturnNumber ( inMysqlBuffer->ReadUInt16LE() );
}

void VJSMysqlBufferClass:: _readUInt24LE ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    ioParms.ReturnNumber ( inMysqlBuffer->ReadUInt24LE() );
}

void VJSMysqlBufferClass:: _readUInt32LE ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    ioParms.ReturnNumber ( inMysqlBuffer->ReadUInt32LE() );
}

void VJSMysqlBufferClass:: _readUInt64LE ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    ioParms.ReturnNumber ( inMysqlBuffer->ReadUInt64LE() );
}

void VJSMysqlBufferClass:: _readLCB ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    sLONG l = inMysqlBuffer->ReadLCB();
    if ( l < 0 )
    {
        l = 0;
    }
    ioParms.ReturnNumber ( l );
}

void VJSMysqlBufferClass:: _readLCBPlus ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    ioParms.ReturnNumber ( inMysqlBuffer->ReadLCBPlus() );
}

void VJSMysqlBufferClass:: _readLCBString ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    sLONG l = inMysqlBuffer->ReadLCB();
    if ( l >= 0 )
    {
        ioParms.ReturnString ( inMysqlBuffer->ReadLCBString ( l ) );
    }
    else
    {
        ioParms.ReturnNullValue();
    }
}

void VJSMysqlBufferClass:: _readLCBStringPlus ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    ioParms.ReturnString ( inMysqlBuffer->ReadLCBString ( inMysqlBuffer->ReadLCBPlus() ) );
}

void VJSMysqlBufferClass:: _readString ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    sLONG bytesLength;
    if ( (ioParms.CountParams() != 1) || (!ioParms.IsNumberParam ( 1 )) || (!ioParms.GetLongParam ( 1, &bytesLength )) )
    {
        vThrowError ( VE_INVALID_PARAMETER );
        return;
    }

    ioParms.ReturnString ( inMysqlBuffer->ReadString ( bytesLength ) );
}


void VJSMysqlBufferClass:: _advance ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    sLONG bytesLength;
    if ( (ioParms.CountParams() != 1) || (!ioParms.IsNumberParam ( 1 )) || (!ioParms.GetLongParam (1, &bytesLength)) )
    {
        vThrowError ( VE_INVALID_PARAMETER );
        return;
    }

    inMysqlBuffer->Advance ( bytesLength );
}

void VJSMysqlBufferClass:: _prepareSelectFetch ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    inMysqlBuffer->PrepareSelectFetch();
}

void VJSMysqlBufferClass:: _saveRowsPosition ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    inMysqlBuffer->SaveRowsPosition();
}

void VJSMysqlBufferClass:: _skipRowsData ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer )
{
    inMysqlBuffer->SkipRowsData ();
}

void VJSMysqlBufferClass::GetDefinition ( ClassDefinition &outDefinition )
{
    static inherited::StaticFunction functions[] =
    {
        { "addBuffer",				js_callStaticFunction<_addBuffer>, JS4D::PropertyAttributeDontDelete },
        { "init",					js_callStaticFunction<_initList>, JS4D::PropertyAttributeDontDelete },
        { "isRow",					js_callStaticFunction<_isRow>, JS4D::PropertyAttributeDontDelete },
        { "hasNext",				js_callStaticFunction<_hasNext>, JS4D::PropertyAttributeDontDelete },
        { "readNextHex",			js_callStaticFunction<_readNextHex>, JS4D::PropertyAttributeDontDelete },
        { "fetch",					js_callStaticFunction<_fetch>, JS4D::PropertyAttributeDontDelete },
        { "getSize",				js_callStaticFunction<_getSize>, JS4D::PropertyAttributeDontDelete },
        { "getSelectResultCount",	js_callStaticFunction<_getSelectResultCount>, JS4D::PropertyAttributeDontDelete },
        { "getRowsCount",			js_callStaticFunction<_getRowsCount>, JS4D::PropertyAttributeDontDelete },
        { "getOffsetRead",			js_callStaticFunction<_getOffsetRead>, JS4D::PropertyAttributeDontDelete },
        { "prepareSelectFetch",		js_callStaticFunction<_prepareSelectFetch>, JS4D::PropertyAttributeDontDelete },
        { "readUInt8",				js_callStaticFunction<_readUInt8>, JS4D::PropertyAttributeDontDelete },
        { "readUInt16LE",			js_callStaticFunction<_readUInt16LE>, JS4D::PropertyAttributeDontDelete },
        { "readUInt24LE",			js_callStaticFunction<_readUInt24LE>, JS4D::PropertyAttributeDontDelete },
        { "readUInt32LE",			js_callStaticFunction<_readUInt32LE>, JS4D::PropertyAttributeDontDelete },
        { "readUInt64LE",			js_callStaticFunction<_readUInt64LE>, JS4D::PropertyAttributeDontDelete },
        { "readString",				js_callStaticFunction<_readString>, JS4D::PropertyAttributeDontDelete },
        { "readLCBString",			js_callStaticFunction<_readLCBString>, JS4D::PropertyAttributeDontDelete },
        { "readLCBStringPlus",		js_callStaticFunction<_readLCBStringPlus>, JS4D::PropertyAttributeDontDelete },
        { "readLCB",				js_callStaticFunction<_readLCB>, JS4D::PropertyAttributeDontDelete },
        { "readLCBPlus",			js_callStaticFunction<_readLCBPlus>, JS4D::PropertyAttributeDontDelete },
        { "saveRowsPosition",		js_callStaticFunction<_saveRowsPosition>, JS4D::PropertyAttributeDontDelete },
        { "advance",				js_callStaticFunction<_advance>, JS4D::PropertyAttributeDontDelete },
        { "advanceFetch",			js_callStaticFunction<_advanceFetch>, JS4D::PropertyAttributeDontDelete },
        { "skipRowsData",			js_callStaticFunction<_skipRowsData>, JS4D::PropertyAttributeDontDelete },
        { 0, 0, 0}
    };

    outDefinition.className         = "MysqlBuffer";
    outDefinition.initialize        = js_initialize<Initialize>;
    outDefinition.finalize          = js_finalize<Finalize>;
    outDefinition.staticFunctions   = functions;
}
