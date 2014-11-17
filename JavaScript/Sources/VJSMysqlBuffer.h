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

#ifndef __VJSMYSQLBUFFER__
#define __VJSMYSQLBUFFER__

#include "VJSClass.h"
#include "VJSValue.h"
#include "VJSBuffer.h"

BEGIN_TOOLBOX_NAMESPACE

/*
The aim of the VJSMysqlBufferObject class is to provide an efficient way
of reading and retreiving results from a MySQL database.
*/

enum
{
    eFIELD_TYPE_DECIMAL      = 0x00,
    eFIELD_TYPE_TINY         = 0x01,
    eFIELD_TYPE_SHORT        = 0x02,
    eFIELD_TYPE_LONG         = 0x03,
    eFIELD_TYPE_FLOAT        = 0x04,
    eFIELD_TYPE_DOUBLE       = 0x05,
    eFIELD_TYPE_NULL         = 0x06,
    eFIELD_TYPE_TIMESTAMP    = 0x07,
    eFIELD_TYPE_LONGLONG     = 0x08,
    eFIELD_TYPE_INT24        = 0x09,
    eFIELD_TYPE_DATE         = 0x0a,
    eFIELD_TYPE_TIME         = 0x0b,
    eFIELD_TYPE_DATETIME     = 0x0c,
    eFIELD_TYPE_YEAR         = 0x0d,
    eFIELD_TYPE_NEWDATE      = 0x0e,
    eFIELD_TYPE_VARCHAR      = 0x0f,
    eFIELD_TYPE_BIT          = 0x10,
    eFIELD_TYPE_NEWDECIMAL   = 0xf6,
    eFIELD_TYPE_ENUM         = 0xf7,
    eFIELD_TYPE_SET          = 0xf8,
    eFIELD_TYPE_TINY_BLOB    = 0xf9,
    eFIELD_TYPE_MEDIUM_BLOB  = 0xfa,
    eFIELD_TYPE_LONG_BLOB    = 0xfb,
    eFIELD_TYPE_BLOB         = 0xfc,
    eFIELD_TYPE_VAR_STRING   = 0xfd,
    eFIELD_TYPE_STRING       = 0xfe,
    eFIELD_TYPE_GEOMETRY     = 0xff
};

class VJSMysqlBufferObject : public IRefCountable
{
public:

    //Default constructor
    VJSMysqlBufferObject();

    //Destructor
    virtual ~VJSMysqlBufferObject();

    /*
    Adds targetBuffer to the buffersObj List, and retains it.
    @param: targetBuffer as a pointer to VJSBufferObject
    @return: Boolean that determines if we there is still buffers to add or not
    */
    Boolean AddBuffer ( VJSBufferObject *targetBuffer );

    /*

    Adds targetBuffer to the buffersObj List without analysis
    @param: targetBuffer as a pointer to VJSBufferObject
    @return: void
    */
    void    AddBufferSimple ( VJSBufferObject *targetBuffer );


    /*
    Initializes MysqlBuffer element with:
    - Setting offset to 0.
    - Initializing the iterator with the first element of the buffersObj
    list.
    - Setting the iBufferLength to the length of the current iterator
    buffer.
    @return: void

    */
    void    InitList();

    /*
    prepares the select for fetch
    @return: void
    */
    void    PrepareSelectFetch();

    /*
    save rows data start and end position
    @return: void
    */
    void    SaveRowsPosition();

    /*
    TODO(no docs found about this)
    @return: void
    */
    void    __SaveRows ( VJSContext &inContext, sLONG fieldCount,  VJSArray titles, VJSArray types );

    /*
    advance in buffer with len places
    @param: len as uLONG
    @return: void
    */
    void    Advance ( uLONG len );

    /*
    Tests if the next bytes are for an EOF Packet (The end of a series of
    Field Packets),if not increments offset 4 times.
    @return: Boolean
    */
    Boolean IsRow();

    /*
    moves to the next result if any
    @return: Boolean
    */
    Boolean GoNext ( uLONG len );

    /*
    tells if there is next result to retreive
    @return: Boolean
    */
    Boolean HasNext();

    //a flag if w have reached the end
    Boolean eof;

    /*
    read fToRead bytes and return reading status
    @param: inBuffer as VJSBufferObject pointer
    @return: Boolean
    */
    Boolean ReadNext ( VJSBufferObject *inBuffer );

    /*
    get the select result count
    @return: uLONG
    */
    uLONG   GetSelectResultCount();

    /*
    get the rows count
    @return: uLONG
    */
    uLONG   GetRowsCount();

    /*
    getter for offsetRead value
    @return: uLONG
    */
    uLONG   GetOffsetRead();


    /*
    Gets the number of received buffers ( buffersObj list length).
    @return: uLONG
    */
    uWORD   GetSize();

    /*
    reads the next hex
    @return: uBYTE
    */
    uBYTE   ReadNextHex();

    /*
    Reads the next byte and increments offset.
    @return: uBYTE
    */
    inline uBYTE    ReadUInt8();

    /*
    Reads the next 2 bytes and increments offset.
    @return: uWORD
    */
    inline uWORD    ReadUInt16LE();

    /*
    Reads the next 3 bytes and increments offset.
    @return: uLONG
    */
    inline uLONG    ReadUInt24LE();

    /*
    Reads the next 4 bytes and increments offset.
    @return: uLONG
    */
    inline uLONG    ReadUInt32LE();

    /*
    Reads the next 8 bytes and increments offset.
    @return: uLONG
    */
    inline uLONG8   ReadUInt64LE();

    /*
    Decodes and returns a string from buffer data encoded with 'UTF-8' encoding,
    and beginning at specified offset and contains bytesLength bytes.
    @param: bytesLength as uLONG
    @return: VString
    */
    VString ReadString ( uLONG bytesLength );


    /*
    Decodes and returns the Length Coded String from buffer data encoded
    with 'UTF-8', beginning at specified offset.
    @param: lcb as uLONG
    @return: VString
    */
    VString ReadLCBString ( uLONG lcb );


    /*
    Decodes and returns the Length Coded String from buffer data encoded
    with 'UTF-8', beginning at specified offset.
    @return: uLONG
    */
    sLONG   ReadLCB();

    /*
    Read Length Coded Binary Number and add it to fOffsetRead ,ReadLCBPlus
    is used only with OK packet to know how many bytes have been read, to
    verify if the message field(optional) exists or not
    @return: uLONG
    */
    uLONG   ReadLCBPlus();

    /*
    fetch 'count' of rows
    @param: inContext as VJSContext reference
    @param: count as sLONG
    @param: inFieldCount as uLONG
    @param: titles as VJSArray
    @param: types as VJSArray
    @return: VJSArray
    */
    VJSArray Fetch ( VJSContext &inContext, sLONG count, uLONG inFieldCount, VJSArray &titles, VJSArray &types );

    /*
    advance fetch with inCount
    @param: inCount as sLONG
    @return: void
    */
    void     AdvanceFetch ( sLONG inCount );

    /*
    skip rows data from fetch
    @return: void
    */
    void     SkipRowsData();

private:

    void SetToRead ( uLONG inValue )
    {
        fToRead = inValue;
        fRead = 0;
        fReaded = 0;
    }

    //to format a property value according to its raw value and its mysql type
    void FormatPropertyValue ( VJSContext &inContext, VJSObject &row, VString &value, VString &title, uLONG type );

    //The position in current buffer at which to begin copying bytes.
    uLONG fOffset;
    //The byte length of the current (from the buffers std::list) buffer data
    sLONG fBufferLength;

    //Analysis state, used only in the addBuffer function
    sBYTE fState;
    //number of bytes to advance
    uLONG fToAdvance;
    //number of bytes to read
    uLONG fToRead;
    //number of bytes already read
    uLONG fReaded;
    //packet length
    uLONG fPackLength;
    //this variable is used in the OK packet to know current data size
    uLONG fOffsetRead;
    //the readen value
    uLONG fRead;
    //the row's position between rowsData used with fetch to ensure that we are not at the end packet.
    uLONG fRowPosition;
    //total number of rows
    uLONG fRowCount;
    //vector of buffers' pointers
    std::vector<VJSBufferObject *> fBuffer;
    //current buffer position
    uLONG  fpos ;

    std::vector <std::pair<uLONG, uLONG> > fRowsStart, fRowsEnd;
    //vector of the number of rows data of a ResultSet
    std::vector <uLONG> fRowsCount;
    //ResultSet statement position for iterating between multiple ResultSets
    sLONG fCoordIter;
};


class XTOOLBOX_API VJSMysqlBufferClass : public VJSClass<VJSMysqlBufferClass, VJSMysqlBufferObject>
{
public:

    typedef VJSClass<VJSMysqlBufferClass, VJSMysqlBufferObject> inherited;

    static void GetDefinition           ( ClassDefinition &outDefinition );
    static void Initialize              ( const VJSParms_initialize &inParms, VJSMysqlBufferObject *inMysqlBuffer );
    static void Finalize                ( const VJSParms_finalize &inParms,  VJSMysqlBufferObject *inMysqlBuffer );
    static void Construct               ( VJSParms_construct &inParms );

private:

    static void     _addBuffer ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );

    static void     _initList ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );
    static void     _initFetch ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );

    static void     _isRow ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );
    static void     _hasNext ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );

    static void     _prepareSelectFetch ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );
    static void     _getRowsCount ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );
    static void     _getSelectResultCount ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );
    static void     _getSize ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );

    static void     _getOffsetRead ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );
    static void     _readNextHex ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );


    static void     _readUInt8 ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );
    static void     _readUInt16LE ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );
    static void     _readUInt24LE ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );
    static void     _readUInt32LE ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );
    static void     _readUInt64LE ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );


    static void     _readString ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );
    static void     _saveRowsPosition ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );

    static void     _readLCBString ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );
    static void     _readLCBStringPlus ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );

    static void     _readLCB ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );
    static void     _readLCBPlus ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );
    static void     _advance ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );
    static void     _fetch ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );

    static void     _advanceFetch ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );

    static void     _skipRowsData ( VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer );

};
END_TOOLBOX_NAMESPACE
#endif
