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

#include "VChromeDbgHdlPage.h"
#include "VRemoteDebugPilot.h" // for GetAbsoluteUrl function

USING_TOOLBOX_NAMESPACE

#if GUY_DUMP_DEBUG_MSG
#define DEBUG_MSG(x)	DebugMsg((x))
#else
#define DEBUG_MSG(x)
#endif


XBOX::VString*					VChromeDbgHdlPage::sSupportedCSSProperties = NULL;

extern VLogHandler*				sPrivateLogHandler;

#define WKA_USE_UNIFIED_DBG

//#define K_EVALUATING_STATE		(1 << 3)
//#define K_LOOKING_UP_STATE		(1 << 5)
#define K_WAITING_CS_STATE		(1 << 7)


#define K_METHOD_STR		"{\"method\":"
#define K_PARAMS_STR		"\"params\":"
#define K_ID_STR			"\"id\":"
#define K_EXPRESSION_STR	"\"expression\":\""
#define K_ORDINAL_STR		"\\\"ordinal\\\":"

const XBOX::VString			K_DQUOT_STR("\"");

const XBOX::VString			K_UNKNOWN_CMD("UNKNOWN_CMD");

const XBOX::VString			K_TIM_SET_INC_MEM_DET("Timeline.setIncludeMemoryDetails");
const XBOX::VString			K_TIM_SUP_FRA_INS("Timeline.supportsFrameInstrumentation");

const XBOX::VString			K_DBG_PAU("Debugger.pause");
const XBOX::VString			K_DBG_STE_OVE("Debugger.stepOver");
const XBOX::VString			K_DBG_STE_INT("Debugger.stepInto");
const XBOX::VString			K_DBG_STE_OUT("Debugger.stepOut");
const XBOX::VString			K_DBG_RES("Debugger.resume");

const XBOX::VString			K_DBG_PAU_STR("Debugger.paused");
const XBOX::VString			K_DBG_RES_STR("{\"method\":\"Debugger.resumed\"}");

const XBOX::VString			K_DBG_CAU_REC("Debugger.causesRecompilation");
const XBOX::VString			K_DBG_SUP_SEP_SCR_COM_AND_EXE("Debugger.supportsSeparateScriptCompilationAndExecution");
const XBOX::VString			K_DBG_SUP_NAT_BRE("Debugger.supportsNativeBreakpoints");
const XBOX::VString			K_DBG_SET_OVE_MES("Debugger.setOverlayMessage");

const XBOX::VString			K_PRO_CAU_REC("Profiler.causesRecompilation");
const XBOX::VString			K_PRO_IS_SAM("Profiler.isSampling");
const XBOX::VString			K_PRO_HAS_HEA_PRO("Profiler.hasHeapProfiler");

const XBOX::VString			K_NET_ENA("Network.enable");
const XBOX::VString			K_NET_CAN_CLE_BRO_CAC("Network.canClearBrowserCache");
const XBOX::VString			K_NET_CAN_CLE_BRO_COO("Network.canClearBrowserCookies");

//const XBOX::VString			K_WOR_SET_WOR_INS_ENA("Worker.setWorkerInspectionEnabled");
const XBOX::VString			K_WOR_ENA("Worker.enable");
const XBOX::VString			K_WOR_CAN_INS_WOR("Worker.canInspectWorkers");

const XBOX::VString			K_DBG_CAN_SET_SCR_SOU("Debugger.canSetScriptSource");
const XBOX::VString			K_DBG_ENA("Debugger.enable");
const XBOX::VString			K_PRO_ENA("Profiler.enable");
const XBOX::VString			K_CON_ENA("Console.enable");
const XBOX::VString			K_CON_DIS("Console.disable");
const XBOX::VString			K_INS_ENA("Inspector.enable");
const XBOX::VString			K_DAT_ENA("Database.enable");
const XBOX::VString			K_DOM_STO_ENA("DOMStorage.enable");
const XBOX::VString			K_CSS_ENA("CSS.enable");
const XBOX::VString			K_CSS_GET_SUP_PRO("CSS.getSupportedCSSProperties");
const XBOX::VString			K_DBG_SET_PAU_ON_EXC("Debugger.setPauseOnExceptions");
const XBOX::VString			K_DOM_GET_DOC("DOM.getDocument");
const XBOX::VString			K_DOM_HID_HIG("DOM.hideHighlight");
const XBOX::VString			K_DOM_REQ_CHI_NOD("DOM.requestChildNodes");
const XBOX::VString			K_DOM_HIG_NOD("DOM.highlightNode");
const XBOX::VString			K_DOM_HIG_FRA("DOM.highlightFrame");
const XBOX::VString			K_CSS_GET_COM_STY_FOR_NOD("CSS.getComputedStyleForNode");
const XBOX::VString			K_CSS_GET_INL_STY_FOR_NOD("CSS.getInlineStylesForNode");
const XBOX::VString			K_CSS_GET_MAT_STY_FOR_NOD("CSS.getMatchedStylesForNode");
const XBOX::VString			K_CON_ADD_INS_NOD("Console.addInspectedNode");
const XBOX::VString			K_RUN_ENA("Runtime.enable");
const XBOX::VString			K_RUN_EVA("Runtime.evaluate");
const XBOX::VString			K_RUN_REL_OBJ_GRO("Runtime.releaseObjectGroup");
const XBOX::VString			K_DOM_SET_INS_MOD_ENA("DOM.setInspectModeEnabled");
const XBOX::VString			K_CON_MES_CLE("Console.messagesCleared");
const XBOX::VString			K_DOM_PUS_NOD_BY_PAT_TO_FRO_END("DOM.pushNodeByPathToFrontend");
const XBOX::VString			K_NET_GET_RES_BOD("Network.getResponseBody");
const XBOX::VString			K_DBG_SET_BRE_BY_URL("Debugger.setBreakpointByUrl");
const XBOX::VString			K_DBG_REM_BRE("Debugger.removeBreakpoint");
const XBOX::VString			K_APP_CAC_ENA("ApplicationCache.enable");
const XBOX::VString			K_APP_CAC_GET_FRA_WIT_MAN("ApplicationCache.getFramesWithManifests");
const XBOX::VString			K_DBG_GET_SCR_SOU("Debugger.getScriptSource");
const XBOX::VString			K_DBG_EVA_ON_CAL_FRA("Debugger.evaluateOnCallFrame");
const XBOX::VString			K_RUN_GET_PRO("Runtime.getProperties");
const XBOX::VString			K_DOM_STO_ADD_DOM_STO("DOMStorage.addDOMStorage");

const XBOX::VString			K_PAG_ENA("Page.enable");
const XBOX::VString			K_PAG_GET_RES_TRE("Page.getResourceTree");
const XBOX::VString			K_PAG_REL("Page.reload");
const XBOX::VString			K_PAG_GET_RES_CON("Page.getResourceContent");
const XBOX::VString			K_PAG_CAN_OVE_DEV_MET("Page.canOverrideDeviceMetrics");
const XBOX::VString			K_PAG_CAN_OVE_GEO_LOC("Page.canOverrideGeolocation");
const XBOX::VString			K_PAG_CAN_OVE_DEV_ORI("Page.canOverrideDeviceOrientation");
const XBOX::VString			K_PAG_SET_TOU_EMU_ENA("Page.setTouchEmulationEnabled");
const XBOX::VString			K_PAG_CAN_SHO_FPS_COU("Page.canShowFPSCounter");
const XBOX::VString			K_PAG_CAN_CON_PAI("Page.canContinuouslyPaint");

const XBOX::VString			K_LINE_FEED_STR("\\n");

const XBOX::VString			K_EMPTY_FILENAME("empty-filename");

#define K_NODE_ID_STR				"\"nodeId\":"
#define K_BACKSLASHED_ID_STR		"\\\"id\\\":"
#define K_SCRIPTID_ID_STR			"\"scriptId\":\""

#define K_FILE_STR					"file://"

#define K_ID_FMT_STR				"},\"id\":"


const XBOX::VString			K_RESULT_TRUE_STR("\"result\":true");
const XBOX::VString			K_RESULT_FALSE_STR("\"result\":false");

const XBOX::VString			K_EMPTY_STR("");
const XBOX::VString			K_SEND_RESULT_STR1("{\"result\":{");
const XBOX::VString			K_SEND_RESULT_STR2("},\"id\":");

#define K_MAX_NODE_ID_SIZE		(16)

#define K_FRAME_FACTOR			(100)
#define K_FRAME_ID_VALUE		"100"
#define K_LOADER_ID_VALUE		"200"
static sLONG					s_node_id = 1000;


const XBOX::VString				K_APP_CAC_GET_FRA_WIT_MAN_STR1("\"frameIds\":[]");

const XBOX::VString				K_APP_CAC_ENA_STR1("{\"method\":\"ApplicationCache.networkStateUpdated\",\"params\":{\"isNowOnline\":true}}");

const XBOX::VString				K_DBG_SET_BRE_BY_URL_STR1("{\"result\":{\"breakpointId\":\"");
const XBOX::VString				K_DBG_SET_BRE_BY_URL_STR2("\",\"locations\":[]},\"id\":");

const XBOX::VString				K_LINE_1("function myWrite(test) {\\n");
const XBOX::VString				K_LINE_2("\\t\\n\\tvar test1 = 123;\\n");
const XBOX::VString				K_LINE_3("\\t\\n\\ttest2 = \\\"GH\\\";\\n");
const XBOX::VString				K_LINE_4("\\tdocument.write('\\u003Cb\\u003EHello World \'+test+\' \'+test2+\' \'+test1+\' !!!!!\\u003C/b\\u003E\');\\n");
const XBOX::VString				K_LINE_5("\\n}\\n\\n");
const XBOX::VString				K_LINE_6("myWrite(3);\\n\\n");


const XBOX::VString				K_NET_GET_RES_BOD_STR1("{\"result\":{\"body\":\"\\u003C!DOCTYPE HTML PUBLIC \\\"-//W3C//DTD HTML 4.01 Transitional//EN\\\" \\\"http://www.w3.org/TR/html4/loose.dtd\\\"\\u003E\\u003C!--DVID=00000745--\\u003E\n\\u003CHTML\\u003E\n\\u003Chead\\u003E\n\\u003Ctitle\\u003E\nHello World\n\\u003C/title\\u003E\n\\u003C/HEAD\\u003E\n\\u003Cbody\\u003E\n\\u003Cscript type=\\\"text/javascript\\\"\\u003E\n\n");
const XBOX::VString				K_NET_GET_RES_BOD_STR2("\n\\u003C/script\\u003E\n\\u003C/body\\u003E\n\\u003C/html\\u003E\n\n\",\"base64Encoded\":false},\"id\":");


const XBOX::VString				K_DOM_PUS_NOD_BY_PAT_TO_FRO_END_STR1("{\"result\":{\"nodeId\":");

const XBOX::VString				K_PAG_GET_RES_TRE_STR1A("{\"result\":{\"frameTree\":{\"frame\":{\"id\":\"");
const XBOX::VString				K_PAG_GET_RES_TRE_STR1B("\",\"loaderId\":\"" K_LOADER_ID_VALUE "\",\"url\":\"");
const XBOX::VString				K_PAG_GET_RES_TRE_STR1C("\",\"securityOrigin\":\"");
//const XBOX::VString				K_PAG_GET_RES_TRE_STR1C("\",\"mimeType\":\"text/html\",\"securityOrigin\":\"");
const XBOX::VString				K_PAG_GET_RES_TRE_STR1D("\"},\"resources\":[");
const XBOX::VString				K_PAG_GET_RES_TRE_STR1E("]}},\"id\":");


const XBOX::VString				K_PAG_REL_AFT_BKPT_STR7A1("{\"method\":\"Network.requestWillBeSent\",\"params\":{\"requestId\":\"30.16\",\"frameId\":\"" K_FRAME_ID_VALUE "\",\"loaderId\":\"" K_LOADER_ID_VALUE "\",\"documentURL\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR7A2("\",\"request\":{\"url\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR7A3("\",\"method\":\"GET\",\"headers\":{\"User-Agent\":\"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/535.11 (KHTML, like Gecko) Chrome/17.0.963.56 Safari/535.11\",\"Accept\":\"*/*\"}},\"timestamp\":1331300225.473645,\"initiator\":{\"type\":\"parser\",\"url\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR7A4("\",\"lineNumber\":");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR7B("},\"stackTrace\":[]}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR8("{\"method\":\"Network.loadingFinished\",\"params\":{\"requestId\":\"30.15\",\"timestamp\":1331300225.463939}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR9A("{\"method\":\"Network.responseReceived\",\"params\":{\"requestId\":\"30.16\",\"frameId\":\"" K_FRAME_ID_VALUE "\",\"loaderId\":\"" K_LOADER_ID_VALUE "\",\"timestamp\":1331300225.475966,\"type\":\"Script\",\"response\":{\"url\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR9B("\",\"status\":0,\"statusText\":\"\",\"mimeType\":\"application/javascript\",\"connectionReused\":false,\"connectionId\":0,\"fromDiskCache\":false,\"timing\":{\"requestTime\":1331300225.473833,\"proxyStart\":-1,\"proxyEnd\":-1,\"dnsStart\":-1,\"dnsEnd\":-1,\"connectStart\":-1,\"connectEnd\":-1,\"sslStart\":-1,\"sslEnd\":-1,\"sendStart\":0,\"sendEnd\":0,\"receiveHeadersEnd\":0},\"headers\":{},\"requestHeaders\":{}}}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR10("{\"method\":\"Network.dataReceived\",\"params\":{\"requestId\":\"30.16\",\"timestamp\":1331300225.476164,\"dataLength\":154,\"encodedDataLength\":0}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR11A("{\"method\":\"Debugger.scriptParsed\",\"params\":{\"scriptId\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR11B("\",\"url\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR11C("\",\"startLine\":0,\"startColumn\":0,\"endLine\":11,\"endColumn\":1}}");	

const XBOX::VString				K_PAG_REL_AFT_BKPT_STR12A1("{\"method\":\"Debugger.breakpointResolved\",\"params\":{\"breakpointId\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR12A2(":6:0\",\"location\":{\"scriptId\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR12A3("\",\"lineNumber\":");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR12B(",\"columnNumber\":1}}}");


const XBOX::VString				K_DOM_REQ_CHI_NOD_STR1("{\"method\":\"DOM.setChildNodes\",\"params\":{\"parentId\":");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR2(",\"nodes\":[{\"nodeId\":");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR3(",\"nodeType\":1,\"nodeName\":\"SCRIPT\",\"localName\":\"script\",\"nodeValue\":\"\",\"childNodeCount\":1,\"children\":[{\"nodeId\":");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR4A(",\"nodeType\":3,\"nodeName\":\"\",\"localName\":\"\",\"nodeValue\":\"\\");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR4B("ndocument.write('Hello World !!!!!/b');\\");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR4C("ndocument.write('Hello World 2!!!!!/b');\\");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR4D("ndocument.write('Hello World 3!!!!!/b');\\");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR4E("n\"}],\"attributes\":[\"type\",\"text/javascript\"]},{\"nodeId\":");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR5(",\"nodeType\":1,\"nodeName\":\"B\",\"localName\":\"b\",\"nodeValue\":\"\",\"childNodeCount\":1,\"children\":[{\"nodeId\":");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR6(",\"nodeType\":3,\"nodeName\":\"\",\"localName\":\"\",\"nodeValue\":\"Hello World !!!!!\"}],\"attributes\":[]},{\"nodeId\":");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR7(",\"nodeType\":1,\"nodeName\":\"B\",\"localName\":\"b\",\"nodeValue\":\"\",\"childNodeCount\":1,\"children\":[{\"nodeId\":");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR8(",\"nodeType\":3,\"nodeName\":\"\",\"localName\":\"\",\"nodeValue\":\"Hello World 2!!!!!\"}],\"attributes\":[]},{\"nodeId\":");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR9(",\"nodeType\":1,\"nodeName\":\"B\",\"localName\":\"b\",\"nodeValue\":\"\",\"childNodeCount\":1,\"children\":[{\"nodeId\":");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR10(",\"nodeType\":3,\"nodeName\":\"\",\"localName\":\"\",\"nodeValue\":\"Hello World 3!!!!!\"}],\"attributes\":[]}]}}");

const XBOX::VString				K_GET_DOC_STR1("{\"result\":{\"root\":{\"nodeId\":");
const XBOX::VString				K_GET_DOC_STR2(",\"nodeType\":9,\"nodeName\":\"#document\",\"localName\":\"\",\"nodeValue\":\"\",\"childNodeCount\":3,\"children\":[{\"nodeId\":");
const XBOX::VString				K_GET_DOC_STR3(",\"nodeType\":10,\"nodeName\":\"html\",\"localName\":\"\",\"nodeValue\":\"\",\"publicId\":\"-//W3C//DTD HTML 4.01 Transitional//EN\",\"systemId\":\"http://www.w3.org/TR/html4/loose.dtd\",\"internalSubset\":\"\"},{\"nodeId\":");
const XBOX::VString				K_GET_DOC_STR4(",\"nodeType\":8,\"nodeName\":\"\",\"localName\":\"\",\"nodeValue\":\"DVID=00000745\"},{\"nodeId\":");
const XBOX::VString				K_GET_DOC_STR5(",\"nodeType\":1,\"nodeName\":\"HTML\",\"localName\":\"html\",\"nodeValue\":\"\",\"childNodeCount\":2,\"children\":[{\"nodeId\":");
const XBOX::VString				K_GET_DOC_STR6(",\"nodeType\":1,\"nodeName\":\"HEAD\",\"localName\":\"head\",\"nodeValue\":\"\",\"childNodeCount\":1,\"attributes\":[]},{\"nodeId\":");
const XBOX::VString				K_GET_DOC_STR7A(",\"nodeType\":1,\"nodeName\":\"BODY\",\"localName\":\"body\",\"nodeValue\":\"\",\"childNodeCount\":4,\"attributes\":[]}],\"attributes\":[]}],\"documentURL\":\"");
const XBOX::VString				K_GET_DOC_STR7B("\",\"xmlVersion\":\"\"}},\"id\":");

const XBOX::VString				K_DBG_GET_SCR_SOU_STR1("{\"result\":{\"scriptSource\":\"");


VChromeDbgHdlPage::VTracesContainer::VTracesContainer()
{
	fStartIndex = 0;
	fEndIndex = 0;
	for( sLONG idx = 0; idx < K_BAGS_ARRAY_SIZE; idx++ )
	{
		fBags[idx] = NULL;
	}
}

VChromeDbgHdlPage::VTracesContainer::~VTracesContainer()
{
	fLock.Lock();
	for( sLONG idx = 0; idx < K_BAGS_ARRAY_SIZE; idx++ )
	{
		if (fBags[idx])
		{
			fBags[idx]->Release();
			fBags[idx] = NULL;
		}
	}
	fLock.Unlock();
}

sLONG VChromeDbgHdlPage::VTracesContainer::Next(sLONG inIndex)
{
	sLONG	nextIndex = inIndex + 1;
	if (nextIndex >= K_BAGS_ARRAY_SIZE)
	{
		nextIndex = 0;
	}
	return nextIndex;
}

void VChromeDbgHdlPage::VTracesContainer::Put(const XBOX::VValueBag* inTraceBag )
{
	fLock.Lock();
	sLONG	nextIndex = Next(fEndIndex);

	if (nextIndex == fStartIndex)
	{
		inTraceBag->Release();
		//fMessagesLost = true;
	}
	else
	{
		fBags[fEndIndex] = inTraceBag;
		fEndIndex = nextIndex;
	}

	fLock.Unlock();
}
const XBOX::VValueBag* VChromeDbgHdlPage::VTracesContainer::Get()
{
	const XBOX::VValueBag*	value = NULL;
	//sLONG	nbRead;
	if (fStartIndex != fEndIndex)
	{
		fLock.Lock();
		sLONG curtIndex = fStartIndex;
		value = fBags[curtIndex];
		fBags[curtIndex] = NULL;
		curtIndex = Next(curtIndex);
		fStartIndex = curtIndex;
		fLock.Unlock();
	}
	return value;
}

VChromeDbgHdlPage::VChromeDbgHdlPage(const XBOX::VString& inSolutionName,VTracesContainer* intracesContainer,bool inV8Mode) :
	fWS(NULL),
	fSolutionName(inSolutionName),
	fFileName(K_EMPTY_FILENAME),
	fSem(1),
	fFifo(16),
	fOutFifo(16),
	fState(STOPPED_STATE),
	fConsoleEnabled(false),
	fV8Mode(inV8Mode)
{
	fPageNb = -1;
	fTracesContainer = intracesContainer;
	fTracesContainer->Retain();
}

VChromeDbgHdlPage::~VChromeDbgHdlPage()
{

	if (fWS)
	{
		ReleaseRefCountable(&fWS);
	}
	ReleaseRefCountable(&fTracesContainer);
}

#define K_CSSPROPS_FILENAME		CVSTR("supportedCSSProperties.txt")
void VChromeDbgHdlPage::Init(sLONG inPageNb,CHTTPServer* inHTTPSrv,IWAKDebuggerSettings* inBrkpts)
{
	if (!sSupportedCSSProperties)
	{
		XBOX::VFolder*		resourcesFolder = VProcess::Get()->RetainFolder(VProcess::eFS_Resources);
		if (testAssert(resourcesFolder != NULL))
		{
			VFile			cssPropsFile(*resourcesFolder,K_CSSPROPS_FILENAME);
			if (testAssert(cssPropsFile.Exists() == true))
			{
				VString			fileContent;
				if (testAssert(cssPropsFile.GetContentAsString(fileContent,VTC_UTF_8) == VE_OK))
				{
					sSupportedCSSProperties = new VString(fileContent);
				}
			}
			ReleaseRefCountable(&resourcesFolder);
		}
	}
	xbox_assert(sSupportedCSSProperties != NULL);
	xbox_assert(fWS == NULL);
	fWS = inHTTPSrv->NewHTTPWebsocketServerHandler();
	xbox_assert(fWS != NULL);

	fFileName = K_EMPTY_FILENAME;
	fPageNb = inPageNb;
}



/*
   checks the syntax of the command requested by the client. Syntax is:
	{"method":"name_of_method",["params":{"name_of_param":"value_of_param},]"id":"number"}
*/
XBOX::VError VChromeDbgHdlPage::CheckInputData()
{

	XBOX::VError	err;

	err = VE_OK;

	fMsgVString  = VString(fMsgData);

	VJSONValue	jsonMsg;

	err = VJSONImporter::ParseString( fMsgVString, jsonMsg );
	if (!err)
	{
		VJSONValue	jsonMethod = jsonMsg.GetProperty("method");
		VJSONValue	jsonId = jsonMsg.GetProperty("id");
		if ( jsonMethod.IsString() &&  jsonId.IsNumber() && ( (err = jsonMethod.GetString(fMethod)) == VE_OK) )
		{
			fId = "";
			fId.AppendLong(jsonId.GetNumber());
			fJsonParamsValue = jsonMsg.GetProperty("params");
			if ( fJsonParamsValue.GetType() != JSON_undefined)
			{
				if (!fJsonParamsValue.IsObject())
				{
					DEBUG_MSG("VChromeDbgHdlPage::CheckInputData params (when exist) should be a js object!!!\n");
					err = VE_INVALID_PARAMETER;
				}
			}
		}
		else
		{
#if GUY_DUMP_DEBUG_MSG

			DebugMsg("Messages should start with '%s' and 'id'!!!!\n",K_METHOD_STR);

#endif
			err = VE_INVALID_PARAMETER;
		}
	}
	return err;
}

const VString K_PROTOCOL_STR("http://");



XBOX::VError VChromeDbgHdlPage::SendMsg(const XBOX::VString& inValue)
{
	VError					err;
	VSize					size;
	VString					trace;
	VString					subString;
	VIndex					firstIdx;
	VIndex					lastIdx;
	VIndex					nbChars;

	firstIdx = 1;
	lastIdx = inValue.GetLength();
	err = VE_OK;
	while( !err && (firstIdx <= lastIdx) )
	{
		nbChars = lastIdx - firstIdx + 1;
		if (nbChars > 1024)
		{
			nbChars = 1024;
		}
		inValue.GetSubString(firstIdx,nbChars,subString);
		firstIdx += nbChars;

#if GUY_DUMP_DEBUG_MSG

		DebugMsg(" treat '%S' ... resp-><%S>\n",&fMethod,&subString);

#endif

		VStringConvertBuffer	buffer(subString,VTC_UTF_8);
		size = buffer.GetSize();
		err = fWS->WriteMessage( buffer.GetCPointer(), size, (firstIdx > lastIdx) );

		trace = CVSTR("VChromeDbgHdlPage::SendMsg WriteMessage err=");
		trace.AppendLong8(err);
		trace += ", size:";
		trace.AppendLong8(size);
		trace += ", val <";
		trace += subString;
		trace += ">";
		sPrivateLogHandler->Put( (err != VE_OK ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),trace);
	}
	return err;
}

XBOX::VError VChromeDbgHdlPage::SendResult(const XBOX::VString& inValue, const XBOX::VString& inRequestId )
{
	VString				tmpStr;

	tmpStr = K_SEND_RESULT_STR1;
	tmpStr += inValue;
	tmpStr += K_SEND_RESULT_STR2;
	tmpStr += inRequestId;
	tmpStr += "}";

	return SendMsg(tmpStr);

}
XBOX::VError VChromeDbgHdlPage::SendResult(const XBOX::VString& inValue)
{
	return SendResult(inValue,fId);
}

XBOX::VError VChromeDbgHdlPage::SendDbgPaused(const XBOX::VString& inStr)
{
	XBOX::VError		err;
	XBOX::VString		resp;

	resp = K_METHOD_STR;
	resp += K_DQUOT_STR;
	resp += K_DBG_PAU_STR;

	resp += K_DQUOT_STR;
	resp += inStr;

	err = SendMsg(resp);
	return err;
}
const VString	K_SCRIPT_ID_STR("\"location\":{\"scriptId\":\"");
XBOX::VError VChromeDbgHdlPage::TreatPageReload(
				const XBOX::VString&			inStr,
				const XBOX::VString&			inExcStr,
				const XBOX::VString&			inExcInfos)
{
	XBOX::VError		err = VE_OK;
	XBOX::VString		resp;

	VString trace("VChromeDbgHdlPage::TreatPageReload called");
	sPrivateLogHandler->Put( WAKDBG_INFO_LEVEL,trace);
	
	if (!err && inExcStr.GetLength() > 0)
	{
		resp = "{\"method\":\"Console.messageAdded\",\"params\":{\"message\":{\"source\":\"javascript\",\"level\":\"error\",\"text\":\"";
		resp += inExcStr;
		resp += "\",\"type\":\"log\"";
		if (inExcInfos.GetLength())
		{
			resp += ",";
			if (fV8Mode)
			{
				VString	jsonStr;
				inExcInfos.GetJSONString(jsonStr);
				resp += jsonStr;
			}
			else
			{
				resp += inExcInfos;
			}
		}
		resp += "}}}";
		err = SendMsg(resp);
	}

	VIndex					idx = 1;
	if (!err)
	{
		std::map< XBOX::VString, VCallstackDescription >::iterator	itDesc = fCallstackDescription.begin();
		while( itDesc != fCallstackDescription.end() )
		{
			(*itDesc).second.fSourceSent = false;
			++itDesc;
		}
	}
	while( !err && ((idx = inStr.Find(K_SCRIPT_ID_STR,idx)) > 0) && (idx <= inStr.GetLength()) )
	{
		VString				absFileName;
		VectorOfVString		tmpData;
		VIndex				endIndex;
		VString				scriptIDStr;

		idx += K_SCRIPT_ID_STR.GetLength();
		endIndex = inStr.Find(CVSTR("\""),idx);
		if (endIndex)
		{
			inStr.GetSubString(idx,endIndex-idx,scriptIDStr);
			std::map< XBOX::VString, VCallstackDescription >::iterator	itDesc = fCallstackDescription.find(scriptIDStr);
			
			if (testAssert(itDesc != fCallstackDescription.end()))
			{
				absFileName = "emptyFileName";
				VRemoteDebugPilot::GetAbsoluteUrl((*itDesc).second.fFileName,fSolutionName,absFileName);
				if (!err)
				{
					// only declare each file once to the remote dbgr
					if ( (*itDesc).second.fSourceSent )
					{
						continue;
					}
					(*itDesc).second.fSourceSent = true;
				}
			}
			if (!err)
			{
				resp = K_PAG_REL_AFT_BKPT_STR7A1;
				resp += absFileName;
				resp += K_PAG_REL_AFT_BKPT_STR7A2;
				resp += absFileName;
				resp += K_PAG_REL_AFT_BKPT_STR7A3;
				resp += absFileName;
				resp += K_PAG_REL_AFT_BKPT_STR7A4;
				resp.AppendLong(fPageNb);
				resp += K_PAG_REL_AFT_BKPT_STR7B;
			}
			/*if (!l_err)
			{
				l_err = SendMsg(resp);
				resp = K_PAG_REL_AFT_BKPT_STR8;
			}*/
			if (!err)
			{
				err = SendMsg(resp);
				resp = K_PAG_REL_AFT_BKPT_STR9A;
				resp += absFileName;
				resp += K_PAG_REL_AFT_BKPT_STR9B;
			}
			if (!err)
			{
				err = SendMsg(resp);
				resp = K_PAG_REL_AFT_BKPT_STR10;
			}
			if (!err)
			{
				err = SendMsg(resp);
				resp = K_PAG_REL_AFT_BKPT_STR11A;
				resp += scriptIDStr;
				resp += K_PAG_REL_AFT_BKPT_STR11B;
				resp += absFileName;
				resp += K_PAG_REL_AFT_BKPT_STR11C;
				if (!err)
				{
					err = SendMsg(resp);
				}
			}
		}
	}

	if (!err)
	{
		err = SendDbgPaused(inStr);
	}

	return err;
}

void VChromeDbgHdlPage::Log( const VString& inLoggerID, EMessageLevel inLevel, const VString& inMessage, VString* outFormattedMessage)
{
	StStringConverter<char> loggerConverter( VTC_StdLib_char);
	StStringConverter<char> messageConverter( VTC_StdLib_char);
	Log( loggerConverter.ConvertString( inLoggerID), inLevel, messageConverter.ConvertString( inMessage), outFormattedMessage);
}


void VChromeDbgHdlPage::Log( const char* inLoggerID, EMessageLevel inLevel, const char* inMessage, VString* outFormattedMessage)
{

	if (true)//ShouldLog(inLevel))
	{
		bool done = false;
		char szTime[512];
		time_t now = ::time( NULL);
		::strftime( szTime, sizeof( szTime),"%Y-%m-%d %X", ::localtime( &now));

		if (outFormattedMessage != NULL)
		{
			outFormattedMessage->Clear();

			/*void *buffer = ::malloc( ::strlen( inMessage) + 256);
			if (buffer != NULL)
			{
				::sprintf( (char*)buffer, "%s [%s] %s - %s\n", szTime, inLoggerID, GetMessageLevelName(inLevel), inMessage);

				outFormattedMessage->FromCString( (char*)buffer);
				fOutput->AppendString( (char*)buffer);
				done = true;

				::free( buffer);
			}*/
		}

		if (!done)
		{
			XBOX::VString		message(inMessage);
			XBOX::VString		jsonMessage;
			message.GetJSONString(jsonMessage);
			XBOX::VError		err = VE_OK;
			XBOX::VString		resp;
			resp = "{\"method\":\"Console.messageAdded\",\"params\":{\"message\":{\"source\":\"javascript\",\"level\":\"log\",\"text\":\"";
			resp += jsonMessage;
			resp += "\",\"type\":\"log\"";
			resp += "}}}";
			err = SendMsg(resp);
		}

	}

}
XBOX::VError VChromeDbgHdlPage::TreatTraces()
{
	//fTracesLock.Lock();

	const VValueBag*	traceBag = NULL;

	if (fState == STOPPED_STATE)
	{
		/*while ( (traceBag = fTracesContainer->Get()) != NULL )
		//for( std::vector<const VValueBag*>::size_type idx = 0; idx < fTraces.size(); idx++ )
		{
			//fTraces[idx]->Release();
			traceBag->Release();
		}*/
	}
	else
	{
		if (fConsoleEnabled)
		{
			while ( (traceBag = fTracesContainer->Get()) != NULL )
			//for( std::vector<const VValueBag*>::size_type idx = 0; idx < fTraces.size(); idx++ )
			{
				VString message;
				ILoggerBagKeys::message.Get( traceBag, message);

				VError errorCode = VE_OK;
				ILoggerBagKeys::error_code.Get( traceBag, errorCode);

				EMessageLevel level = ILoggerBagKeys::level.Get(traceBag);
				VString loggerID;
				if (!ILoggerBagKeys::source.Get( traceBag, loggerID))
				{
					VProcess::Get()->GetProductName(loggerID);
					/* OsType componentSignature = 0;
					if (!ILoggerBagKeys::component_signature.Get( inMessage, componentSignature))
						componentSignature = COMPONENT_FROM_VERROR( errorCode);

					if (componentSignature != 0)
					{
						loggerID.AppendUniChar( '.').AppendOsType( componentSignature);
					}*/
				}
				Log( loggerID, level, message, NULL);
				traceBag->Release();
			}
		}
	}
	//fTraces.clear();
	
	//fTracesLock.Unlock();

	return VE_OK;
}



XBOX::VError VChromeDbgHdlPage::TreatMsg(XBOX::VSemaphore* inSem)
{		
	XBOX::VError		l_err;
	XBOX::VString		l_cmd;
	XBOX::VString		l_resp;
	bool				l_found;
	sBYTE*				l_id;
	ChrmDbgMsg_t		l_msg;

	l_err = VE_OK;

	l_cmd = K_UNKNOWN_CMD;
	l_found = false;
	l_err = CheckInputData();

	VString l_trace("VChromeDbgHdlPage::TreatMsg Method=");
	l_trace += fMethod;
	//if (fParams)
	{
		l_trace += " ,";
		VString l_params = "params=<";
		l_params += fMsgVString;
		l_params += ">";
		l_trace += l_params;
	}
	sPrivateLogHandler->Put( (l_err != VE_OK ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),l_trace);

	if (!l_err)
	{
#if GUY_DUMP_DEBUG_MSG
		DebugMsg("VChromeDbgHdlPage::TreatMsg '%S'\n",&fMsgVString);
#endif
	}
	else
	{
		DEBUG_MSG("VChromeDbgHdlPage::TreatMsg CheckInputData pb !!!!\n");
	}

	l_cmd = K_APP_CAC_GET_FRA_WIT_MAN;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_APP_CAC_GET_FRA_WIT_MAN_STR1);
	}
#if 1
	l_cmd = K_APP_CAC_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	

		l_resp = K_APP_CAC_ENA_STR1;
		l_err = SendMsg(l_resp);
		if (!l_err)
		{
			l_err = SendResult(K_EMPTY_STR);
		}
	}

	l_cmd = K_DBG_REM_BRE;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {

		VJSONValue	jsonBreakpointId = fJsonParamsValue.GetProperty("breakpointId");
		VString		bkptStr;

		if (jsonBreakpointId.IsString() && (jsonBreakpointId.GetString(bkptStr) == VE_OK) )
		{
			VIndex		lineIndex = bkptStr.Find(K_PROTOCOL_STR);
			if (lineIndex > 0)
			{
				lineIndex =  bkptStr.Find(":",K_PROTOCOL_STR.GetLength()+1);
			}
			if (lineIndex > 0)
			{
				VString		relUrl;
				VString		absUrl;
				bkptStr.GetSubString(1,lineIndex-1,absUrl);
				VRemoteDebugPilot::GetRelativeUrl(absUrl,relUrl);
				VIndex		lineEndIndex = bkptStr.Find(":",lineIndex+1);
				VString		lineStr;
				bkptStr.GetSubString(lineIndex+1,lineEndIndex-1-lineIndex,lineStr);
		
				unsigned int	lineNb;
				lineNb = lineStr.GetLong();

				bool		breakpointSet = false;
				//VString		vstringFilename(tmpStr);
				CallstackDescriptionMap::iterator	itDesc = fCallstackDescription.begin();
				//l_err = VE_INVALID_PARAMETER;
				while( itDesc != fCallstackDescription.end() )
				{
					if ( (*itDesc).second.fFileName == relUrl )
					{
						intptr_t	sourceId = (*itDesc).first.GetLong8();
						VChromeDebugHandler::RemoveBreakPoint(fCtxId,sourceId,lineNb);
						//l_err = VE_OK;
						breakpointSet = true;
						break;
					}
					++itDesc;
				}
				if (!breakpointSet)
				{
					VChromeDebugHandler::RemoveBreakPointByUrl(relUrl,lineNb);
				}
				if (!l_err)
				{
					// we answer as success since wakanda is not able to return a status regarding bkpt handling
					l_err = SendResult(K_EMPTY_STR);
				}

			}
		}
	}

	l_cmd = K_DBG_EVA_ON_CAL_FRA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {

		l_err = VE_INVALID_PARAMETER;
		//DEBUG_MSG"VChromeDbgHdlPage::TreatMsg fMsgVString=< %S >\n",&fMsgVString);

		VJSONValue	jsonExpr; 
		VJSONValue	jsonCallFrameId;
		VString		jsonCallFrameStr;
		VJSONValue	jsonCallFrameIdValue;

		if (fJsonParamsValue.GetType() == JSON_object)
		{
			jsonExpr = fJsonParamsValue.GetProperty("expression"); 
			jsonCallFrameId = fJsonParamsValue.GetProperty("callFrameId");

			if ( jsonExpr.IsString() && jsonCallFrameId.IsString() && (jsonCallFrameId.GetString(jsonCallFrameStr) == VE_OK) )
			{
				if (VJSONImporter::ParseString( jsonCallFrameStr, jsonCallFrameIdValue ) == VE_OK )
				{
					VJSONValue	jsonOrdinal;
					VString		expression;
					if (jsonCallFrameIdValue.IsObject())
					{
						jsonOrdinal = jsonCallFrameIdValue.GetProperty("ordinal");
						if ( (jsonExpr.GetString(expression) == VE_OK) && jsonOrdinal.IsNumber() )
						{
							l_msg.data.Msg.fLineNumber = (sLONG)jsonOrdinal.GetNumber();


#if GUY_DUMP_DEBUG_MSG
							DebugMsg("VChromeDbgHdlPage::TreatMsg requiring value of '%S'\n",&expression);
#endif

							l_msg.type = SEND_CMD_MSG;
							l_msg.data.Msg.fType = WAKDebuggerServerMessage::SRV_EVALUATE_MSG;
							l_msg.data.Msg.fSrcId = fSrcId;
							l_msg.data.Msg.fRequestId = (intptr_t)fId.GetLong();
							l_msg.data.Msg.fString[expression.ToBlock(l_msg.data.Msg.fString,K_MAX_FILESIZE-1,VTC_UTF_8,false,false)] = 0;
							
							l_err = fOutFifo.Put(l_msg);
							if (testAssert( l_err == VE_OK ))
							{
								//fInternalState |= K_EVALUATING_STATE;
							}
						}
					}
				}
			}

		}
		if (l_err)
		{
			Eval("\"result\":{\"value\":{\"type\":\"string\",\"value\":\"error\"}",fId);
		}
	}

	l_cmd = K_DBG_SET_BRE_BY_URL;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		
		sLONG8		lineNb = 0;
		sLONG8		colNb = 0;
		bool		breakpointSet = false;
		VString		vFilenameUrl;
		VString		relativeUrl;

		VJSONValue	jsonBreakpointUrl = fJsonParamsValue.GetProperty("url");
		VJSONValue	jsonBreakpointLineNumber = fJsonParamsValue.GetProperty("lineNumber");

		if (jsonBreakpointUrl.IsString() && (jsonBreakpointUrl.GetString(vFilenameUrl) == VE_OK) &&
				jsonBreakpointLineNumber.IsNumber() )
		{
			VRemoteDebugPilot::GetRelativeUrl(vFilenameUrl,relativeUrl);
			lineNb = jsonBreakpointLineNumber.GetNumber();
			VJSONValue	jsonBreakpointColumnNumber = fJsonParamsValue.GetProperty("columnNumber");
			if (jsonBreakpointColumnNumber.IsNumber())
			{
				colNb = jsonBreakpointColumnNumber.GetNumber();
			}

			CallstackDescriptionMap::iterator	itDesc = fCallstackDescription.begin();
			while( itDesc != fCallstackDescription.end() )
			{
				if ( (*itDesc).second.fFileName == relativeUrl )
				{
					intptr_t	sourceId = (*itDesc).first.GetLong8();
					VChromeDebugHandler::AddBreakPoint(fCtxId,sourceId,lineNb);
					breakpointSet = true;
					break;
				}
				++itDesc;
			}
			if (!breakpointSet)
			{
				VChromeDebugHandler::AddBreakPointByUrl(relativeUrl,lineNb);
			}
		}
		if (!l_err)
		{
			l_resp = K_DBG_SET_BRE_BY_URL_STR1;
			l_resp += vFilenameUrl;
			l_resp += ":";
			l_resp.AppendLong(lineNb);
			l_resp += ":";
			l_resp.AppendLong(colNb);
			l_resp += K_DBG_SET_BRE_BY_URL_STR2;
			l_resp += fId;
			l_resp += "}";
			// we answer as success since wakanda is not able to return a status regarding bkpt handling
			l_err = SendMsg(l_resp);
		}
	}

	/*l_cmd = K_NET_GET_RES_BOD;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		l_resp = K_NET_GET_RES_BOD_STR1;
		l_resp += K_LINE_1;
		l_resp += K_LINE_2;
		l_resp += K_LINE_3;
		l_resp += K_LINE_4;
		l_resp += K_LINE_5;
		l_resp += K_LINE_6;
		l_resp += K_NET_GET_RES_BOD_STR2;
		l_resp += fId;
		l_resp += "}";
		l_err = SendMsg(l_resp);
	}*/
	
	l_cmd = K_PAG_GET_RES_CON;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {

		VJSONValue		jsonValueUrl = fJsonParamsValue.GetProperty("url");
		VString			sourceUrl;
		l_err = jsonValueUrl.GetString(sourceUrl);

		l_resp = "{\"result\":{\"content\":\"";
		if (l_err)
		{
			l_resp += "// source not found \\n";
		}
		else
		{
			VString	relativeUrl;
			VRemoteDebugPilot::GetRelativeUrl(sourceUrl,relativeUrl);
			{
				VectorOfVString		sourceData;
				VChromeDebugHandler::GetSourceFromUrl(fCtxId,relativeUrl,sourceData);
				for( VectorOfVString::size_type idx=0; idx < sourceData.size(); idx++ )
				{
					l_resp += sourceData[idx];
					l_resp += K_LINE_FEED_STR;
				}
			}
		}
		l_resp += "\",\"base64Encoded\":false},\"id\":";
		l_resp += fId;
		l_resp += "}";
		l_err = SendMsg(l_resp);
	}

	l_cmd = K_DOM_SET_INS_MOD_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_DOM_PUS_NOD_BY_PAT_TO_FRO_END;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_resp = K_DOM_PUS_NOD_BY_PAT_TO_FRO_END_STR1;
		l_resp.AppendLong(fBodyNodeId);
		l_resp += K_ID_FMT_STR;
		l_resp += fId;
		l_resp += "}";
		l_err = SendMsg(l_resp);
	}
	
	l_cmd = K_PAG_REL;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		// page reload from CHrome not handled
		//xbox_assert(false);
		l_err = VE_UNKNOWN_ERROR;
		return l_err;
	}

	l_cmd = K_DBG_GET_SCR_SOU;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		VJSONValue		jsonScriptId = fJsonParamsValue.GetProperty("scriptId");
		VString			scriptIdStr;

		if ( !jsonScriptId.IsString() || (jsonScriptId.GetString(scriptIdStr) != VE_OK) )
		{
			DebugMsg("VChromeDbgHdlPage::TreatMsg NO scriptId param found for %S\n",&fMethod);
			l_err = VE_INVALID_PARAMETER;
		}

		l_resp = K_DBG_GET_SCR_SOU_STR1;

		if (!l_err)
		{
			std::map< XBOX::VString, VCallstackDescription>::iterator	itDesc = fCallstackDescription.find(scriptIdStr);
			if (itDesc != fCallstackDescription.end())
			{
				for(	VectorOfVString::iterator itCodeLine = (*itDesc).second.fSourceCode.begin();
						(itCodeLine != (*itDesc).second.fSourceCode.end());
						++itCodeLine			)
				{
					l_resp += *itCodeLine;
					l_resp += K_LINE_FEED_STR;
				}
			}
			else
			{
				l_resp += CVSTR(" NO SOURCE AVAILABLE ");
				l_resp += K_LINE_FEED_STR;
			}
		}

		l_resp += "\"},\"id\":";
		l_resp += fId;
		l_resp += "}";
		l_err = SendMsg(l_resp);
	}
	l_cmd = K_RUN_EVA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) )
	{
		// GH TBC
		l_resp = "{\"result\":{\"result\":{\"type\":\"undefined\"},\"wasThrown\":false";
		l_resp += K_ID_FMT_STR;
		l_resp += fId;
		l_resp += "}";
		l_err = SendMsg(l_resp);
	}
	
	l_cmd = K_RUN_GET_PRO;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) )
	{
		VJSONValue		jsonObjectId = fJsonParamsValue.GetProperty("objectId");
		VString			objectIdStr;

		if ( jsonObjectId.IsString() && (jsonObjectId.GetString(objectIdStr) == VE_OK) )
		{
			VJSONValue	objectIdValue;
			if (VJSONImporter::ParseString( objectIdStr, objectIdValue ) == VE_OK )
			{
				VJSONValue		jsonId = objectIdValue.GetProperty("id");
				if (jsonId.IsNumber())
				{
					l_msg.data.Msg.fObjRef = jsonId.GetNumber();
					l_msg.type = SEND_CMD_MSG;
					l_msg.data.Msg.fType = WAKDebuggerServerMessage::SRV_LOOKUP_MSG;
					l_msg.data.Msg.fRequestId = (intptr_t)fId.GetLong();
					l_err = fOutFifo.Put(l_msg);
					xbox_assert( l_err == VE_OK );
				}
			}
		}
	}

	l_cmd = K_DOM_HIG_FRA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_DOM_HIG_NOD;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);

	}

	l_cmd = K_RUN_REL_OBJ_GRO;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_DBG_SET_OVE_MES;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}
#if 0
	l_cmd = K_DOM_REQ_CHI_NOD;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_id = strstr(fParams,K_NODE_ID_STR);
		if (!l_id)
		{
			DEBUG_MSG"VChromeDbgHdlPage::TreatMsg NO %s param found for %S\n",K_NODE_ID_STR,&fMethod);
			l_err = VE_INVALID_PARAMETER;
		}
		if (!l_err && (atoi(l_id+strlen(K_NODE_ID_STR)) != fBodyNodeId) )
		{
			DEBUG_MSG"VChromeDbgHdlPage::TreatMsg INCORRECT BODY node id for %S\n",&fMethod);
			l_err = VE_INVALID_PARAMETER;
		}
		if (!l_err)
		{
			l_resp = K_DOM_REQ_CHI_NOD_STR1;
			l_resp.AppendLong(fBodyNodeId);
			l_resp += K_DOM_REQ_CHI_NOD_STR2;
			l_resp.AppendLong(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR3;
			l_resp.AppendLong(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR4A;
			//l_resp += K_DOM_REQ_CHI_NOD_STR4B;
			//l_resp += K_DOM_REQ_CHI_NOD_STR4C;
			//l_resp += K_DOM_REQ_CHI_NOD_STR4D;
			l_resp += "n";
			l_resp+= "\\";
			l_resp += K_DOM_REQ_CHI_NOD_STR4E;
			l_resp.AppendLong(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR5;
			l_resp.AppendLong(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR6;
			l_resp.AppendLong(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR7;
			l_resp.AppendLong(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR8;
			l_resp.AppendLong(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR9;
			l_resp.AppendLong(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR10;

			l_err = SendMsg(l_resp);

			if (!l_err)
			{
				l_err = SendResult(K_EMPTY_STR);
			}
		}

	}
#endif

	l_cmd = K_CON_ADD_INS_NOD;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		l_err = SendResult(K_EMPTY_STR);
	}
#if 0
	l_cmd = K_CSS_GET_MAT_STY_FOR_NOD;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_id = strstr(fParams,K_NODE_ID_STR);
		if (!l_id)
		{
			DEBUG_MSG"VChromeDbgHdlPage::TreatMsg NO %s param found for %S\n",K_NODE_ID_STR,&fMethod);
			l_err = VE_INVALID_PARAMETER;
		}
		if (!l_err && (atoi(l_id+strlen(K_NODE_ID_STR)) != fBodyNodeId) )
		{
			DEBUG_MSG"VChromeDbgHdlPage::TreatMsg INCORRECT BODY node id for %S\n",&fMethod);
			l_err = VE_INVALID_PARAMETER;
		}
		if (!l_err)
		{
			l_resp.AppendBlock(_binary_getMatchedStylesForNode,sizeof(_binary_getMatchedStylesForNode),VTC_UTF_8);
			l_resp += fId;
			l_resp += "}";
			l_err = SendMsg(l_resp);
		}
	}

	l_cmd = K_CSS_GET_INL_STY_FOR_NOD;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_id = strstr(fParams,K_NODE_ID_STR);
		if (!l_id)
		{
			DEBUG_MSG"VChromeDbgHdlPage::TreatMsg NO %s param found for %S\n",K_NODE_ID_STR,&fMethod);
			l_err = VE_INVALID_PARAMETER;
		}
		if (!l_err && (atoi(l_id+strlen(K_NODE_ID_STR)) != fBodyNodeId) )
		{
			DEBUG_MSG"VChromeDbgHdlPage::TreatMsg INCORRECT BODY node id for %S\n",&fMethod);
			l_err = VE_INVALID_PARAMETER;
		}
		if (!l_err)
		{
			l_resp.AppendBlock(_binary_getInlineStylesForNode,sizeof(_binary_getInlineStylesForNode),VTC_UTF_8);
			l_resp += fId;
			l_resp += "}";
			l_err = SendMsg(l_resp);
		}
	}
#endif
/*
	l_cmd = K_CSS_GET_COM_STY_FOR_NOD;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_id = strstr(fParams,K_NODE_ID_STR);
		if (!l_id)
		{
			DEBUG_MSG"NO %s param found for %S\n",K_NODE_ID_STR,&fMethod);
			l_err = VE_INVALID_PARAMETER;
		}
		if (!l_err && (atoi(l_id+strlen(K_NODE_ID_STR)) != fBodyNodeId) )
		{
			DEBUG_MSG"INCORRECT BODY node id for %S\n",&fMethod);
			l_err = VE_INVALID_PARAMETER;
		}
		if (!l_err)
		{
			l_resp.AppendBlock(_binary_getComputedStyleForNode,sizeof(_binary_getComputedStyleForNode),VTC_UTF_8);
			l_resp += fId;
			l_resp += "}";
			l_err = SendMsg(l_resp);
		}
	}*/
	
	l_cmd = K_DOM_GET_DOC;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {

		l_resp = K_GET_DOC_STR1;
		l_resp.AppendLong(s_node_id++);
		l_resp += K_GET_DOC_STR2;
		l_resp.AppendLong(s_node_id++);
		l_resp += K_GET_DOC_STR3;
		l_resp.AppendLong(s_node_id++);
		l_resp += K_GET_DOC_STR4;
		l_resp.AppendLong(s_node_id++);
		l_resp += K_GET_DOC_STR5;
		l_resp.AppendLong(s_node_id++);
		l_resp += K_GET_DOC_STR6;
		fBodyNodeId = s_node_id;
		l_resp.AppendLong(s_node_id++);
		l_resp += K_GET_DOC_STR7A;
		l_resp += fFileName;
		l_resp += K_GET_DOC_STR7B;
		l_resp += fId;
		l_resp += "}";

		l_err = SendMsg(l_resp);
	}

	l_cmd = K_DOM_HID_HIG;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_DBG_SET_PAU_ON_EXC;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_TIM_SET_INC_MEM_DET;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_CSS_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_CSS_GET_SUP_PRO;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_resp += (*sSupportedCSSProperties);
		//l_resp.AppendBlock(_binary_getSupportedCSSProperties,sizeof(_binary_getSupportedCSSProperties),VTC_UTF_8);
		l_resp += fId;
		l_resp += "}";
		l_err = SendMsg(l_resp);

	}	
	
	l_cmd = K_DOM_STO_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}
	
	l_cmd = K_PAG_SET_TOU_EMU_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}
	
	l_cmd = K_RUN_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_DAT_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_INS_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		l_err = SendResult(K_EMPTY_STR);
		if (!l_err)
		{
			fState = CONNECTED_STATE;
			inSem->Unlock();
		}
	}

	l_cmd = K_CON_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		l_err = SendResult(K_EMPTY_STR);
		fConsoleEnabled = true;
	}

	l_cmd = K_CON_DIS;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		l_err = SendResult(K_EMPTY_STR);
		fConsoleEnabled = false;
	}
	l_cmd = K_PRO_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_DBG_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_DBG_CAN_SET_SCR_SOU;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_TRUE_STR);
	}

	/*l_cmd = K_WOR_SET_WOR_INS_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}*/

	l_cmd = K_WOR_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_WOR_CAN_INS_WOR;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_TRUE_STR);
	}

	l_cmd = K_NET_CAN_CLE_BRO_CAC;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_TRUE_STR);
	}

	l_cmd = K_NET_CAN_CLE_BRO_COO;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_TRUE_STR);
	}

	l_cmd = K_PAG_GET_RES_TRE;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		VectorOfVString		filenameVector;
		VChromeDebugHandler::GetAllJSFileNames(filenameVector);

		l_resp = K_PAG_GET_RES_TRE_STR1A;
		l_resp.AppendLong(fPageNb*K_FRAME_FACTOR);
		l_resp += K_PAG_GET_RES_TRE_STR1B;
		l_resp += K_PROTOCOL_STR;
		l_resp += fSolutionName;
		l_resp += K_PAG_GET_RES_TRE_STR1C;
		l_resp += K_PROTOCOL_STR;
		l_resp += fSolutionName;
		l_resp += K_PAG_GET_RES_TRE_STR1D;
		for( VectorOfVString::size_type idx = 0; idx < filenameVector.size(); idx++ )
		{
			if (idx > 0)
			{
				l_resp += ",";
			}
			VString	absUrl;
			VRemoteDebugPilot::GetAbsoluteUrl(filenameVector[idx],fSolutionName,absUrl);
			l_resp += "{\"url\":\"";
			l_resp += absUrl;
			l_resp += "\",\"type\":\"Script\",\"mimeType\":\"application/javascript\"}";
		}
		l_resp += K_PAG_GET_RES_TRE_STR1E;
		l_resp += fId;
		l_resp += "}";
		l_err = SendMsg(l_resp);
	}
	
	l_cmd = K_PAG_CAN_SHO_FPS_COU;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult("\"show\":false");
	}
	
	l_cmd = K_PAG_CAN_CON_PAI;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult("\"value\":false");
	}

	l_cmd = K_PAG_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_NET_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_PRO_HAS_HEA_PRO;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_TRUE_STR);
	}

	l_cmd = K_TIM_SUP_FRA_INS;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_TRUE_STR);
	}

	l_cmd = K_PAG_CAN_OVE_DEV_MET;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_TRUE_STR);
	}

	l_cmd = K_PAG_CAN_OVE_GEO_LOC;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_TRUE_STR);
	}

	l_cmd = K_PAG_CAN_OVE_DEV_ORI;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_TRUE_STR);
	}

	l_cmd = K_PRO_IS_SAM;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_TRUE_STR);
	}

	l_cmd = K_PRO_CAU_REC;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		l_err = SendResult(K_RESULT_FALSE_STR);
	}

	l_cmd = K_DBG_SUP_NAT_BRE;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_TRUE_STR);
	}

	l_cmd = K_DBG_RES;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
		if (!l_err)
		{
			l_resp = K_DBG_RES_STR;
			l_err = SendMsg(l_resp);
		}
		l_msg.type = SEND_CMD_MSG;
		l_msg.data.Msg.fType = WAKDebuggerServerMessage::SRV_CONTINUE_MSG;
		l_err = fOutFifo.Put(l_msg);
		xbox_assert( l_err == VE_OK );
	}
	l_cmd = K_DBG_STE_INT;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
		if (!l_err)
		{
			l_resp = K_DBG_RES_STR;
			l_err = SendMsg(l_resp);
		}
		l_msg.type = SEND_CMD_MSG;
		l_msg.data.Msg.fType = WAKDebuggerServerMessage::SRV_STEP_INTO_MSG;
		l_err = fOutFifo.Put(l_msg);
		xbox_assert( l_err == VE_OK );
	}
	l_cmd = K_DBG_STE_OUT;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
		if (!l_err)
		{
			l_resp = K_DBG_RES_STR;
			l_err = SendMsg(l_resp);
		}
		l_msg.type = SEND_CMD_MSG;
		l_msg.data.Msg.fType = WAKDebuggerServerMessage::SRV_STEP_OUT_MSG;
		l_err = fOutFifo.Put(l_msg);
		xbox_assert( l_err == VE_OK );
	}
	l_cmd = K_DBG_STE_OVE;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
		if (!l_err)
		{
			l_resp = K_DBG_RES_STR;
			l_err = SendMsg(l_resp);
		}
		l_msg.type = SEND_CMD_MSG;
		l_msg.data.Msg.fType = WAKDebuggerServerMessage::SRV_STEP_OVER_MSG;
		l_err = fOutFifo.Put(l_msg);
		xbox_assert( l_err == VE_OK );
	}
	l_cmd = K_DBG_CAU_REC;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_FALSE_STR);
	}
	l_cmd = K_DBG_SUP_SEP_SCR_COM_AND_EXE;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_TRUE_STR);
	}
	l_cmd = K_DBG_PAU;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}
	if (!l_found)
	{
		DebugMsg("VChromeDbgHdlPage::TreatMsg Method='%S' UNKNOWN!!!\n",&fMethod);

		xbox_assert(false);
		return VE_OK;// --> pour l'instant on renvoit OK  !!! VE_INVALID_PARAMETER;
	}
#endif
	return l_err;

}

XBOX::VError VChromeDbgHdlPage::Start(OpaqueDebuggerContext inCtxId)
{
	VError		err = VE_OK;
	if (fState != STOPPED_STATE)
	{
		err = VE_INVALID_PARAMETER;
	}
	if (!err)
	{
		fFifo.Reset();
		fOutFifo.Reset();
		fState = STARTING_STATE;
		fCtxId = inCtxId;
	}
	return err;
}

XBOX::VError VChromeDbgHdlPage::Stop()
{
	ChrmDbgMsg_t		pageMsg;

	pageMsg.type = STOP_MSG;

	return fFifo.Put(pageMsg);
}

XBOX::VError VChromeDbgHdlPage::TreatBreakpointReached(ChrmDbgMsg_t& inMsg)
{
	VString				tmpStr;
	VError				err = VE_INVALID_PARAMETER;

	if (fInternalState & K_WAITING_CS_STATE)
	{
		xbox_assert(false);
		return err;
	}
	fLineNb = (sLONG)inMsg.data.Msg.fLineNumber;
					
	tmpStr = VString("VChromeDbgHdlPage::TreatBreakpointReached page=");
	tmpStr.AppendLong(fPageNb);
	tmpStr += " BRKPT_REACHED_MSG fLineNb=";
	tmpStr.AppendLong(fLineNb);
	sPrivateLogHandler->Put(WAKDBG_INFO_LEVEL,tmpStr);

	fFileName = inMsg.data._urlStr;
	VURL::Decode( fFileName );
	VIndex				idx;
	/*idx = fFileName.FindUniChar( L'/', fFileName.GetLength(), true );
	if ( (idx  > 1) && (idx < fFileName.GetLength()) )
	{
		VString	l_tmp_str;
		fFileName.GetSubString(idx+1,fFileName.GetLength()-idx,l_tmp_str);
		fFileName = l_tmp_str;
	}*/
	//fSource = inMsg.data._dataVectStr;
	fExcStr = inMsg.data._excStr;
	ChrmDbgMsg_t	msg;
	msg.type = SEND_CMD_MSG;
	msg.data.Msg.fType = WAKDebuggerServerMessage::SRV_GET_CALLSTACK_MSG;
	msg.data.Msg.fWithExceptionInfos = (fExcStr.GetLength() > 0);
	err = fOutFifo.Put(msg);
	if (testAssert( (err == VE_OK) ))
	{
		fInternalState |= K_WAITING_CS_STATE;
	}
	return err;
}

XBOX::VError VChromeDbgHdlPage::SendEvaluateResult(const XBOX::VString& inResult,const XBOX::VString& inRequestId)
{
	VError		err = VE_INVALID_PARAMETER;
	VString		resp = inResult;
	VString		hdr = CVSTR("{\"value\":");
	VString		sub;

	//if (resp.GetLength() > hdr.GetLength())
	if (resp.Find(hdr) == 1)
	{
		resp.GetSubString(hdr.GetLength()+1,resp.GetLength()-hdr.GetLength(),sub);
	}
	else
	{
		hdr = CVSTR("\"result\":{\"value\":");
		if (resp.Find(hdr) == 1)
		{
			resp.GetSubString(hdr.GetLength()+1,resp.GetLength()-hdr.GetLength(),sub);
		}
		else
		{
			sub = inResult;
		}
	}
	resp = CVSTR("\"result\":");
	resp += sub;
	//resp += inResult;
	err = SendResult(resp,inRequestId);
	return err;
}

XBOX::VError VChromeDbgHdlPage::TreatEvaluate(ChrmDbgMsg_t& inMsg)
{
	VError				err = VE_INVALID_PARAMETER;

	/*if (!(fInternalState & K_EVALUATING_STATE))
	{
		xbox_assert(false);
		return err;
	}*/
	err = SendEvaluateResult(inMsg.data._dataStr,inMsg.data._requestId);
	//fInternalState &= ~K_EVALUATING_STATE;

	return err;
}


XBOX::VError VChromeDbgHdlPage::TreatLookup(ChrmDbgMsg_t& inMsg)
{
	VError				err = VE_INVALID_PARAMETER;

	/*if (!(fInternalState & K_LOOKING_UP_STATE))
	{
		xbox_assert(false);
		return err;
	}*/

	VString		resp = inMsg.data._dataStr;
	//resp += fLookupId;
	//resp += "}";
	err = SendMsg(resp);
	//fInternalState &= ~K_LOOKING_UP_STATE;

	return err;

}

XBOX::VError VChromeDbgHdlPage::TreatWS(IHTTPResponse* ioResponse,XBOX::VSemaphore* inSem)
{
	XBOX::VError		err = VE_OK;
	ChrmDbgMsg_t		msg;
	bool				msgFound;
	VString				tmpStr;
	VString				originalTaskName;

	if (!testAssert(fState == STARTING_STATE))
	{
		// the page should be released after use (STOPPED_STATE), one should pass here only on STARTING_STATE
		DebugMsg("VChromeDbgHdlPage::TreatWS bad STATE=%d\n",fState);
		err = VE_INVALID_PARAMETER;
	}
	else
	{
		if (!testAssert(fSem.TryToLock()))
		{
			err = VE_INVALID_PARAMETER;
			DEBUG_MSG("VChromeDbgHdlPage::TreatWS already in use\n");
		}
	}

	if (err)
	{
		inSem->Unlock();
		return err;
	}

	fTask = VTask::GetCurrent();
	fTask->GetName(originalTaskName);
	VString		taskName("ChromeDbgHdlPage ");
	taskName.AppendLong(fPageNb);
	fTask->SetName(taskName);
	fInternalState = 0;
	err = fWS->TreatNewConnection(ioResponse,false);
	xbox_assert(err == VE_OK);
	tmpStr = CVSTR("VChromeDbgHdlPage::TreatWS TreatNewConnection status=");
	tmpStr.AppendLong8(err);
	sPrivateLogHandler->Put( (err != VE_OK ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),tmpStr);
	
	fFileName = K_EMPTY_FILENAME;
	fOffset = 0;

	while(!err)
	{
		fMsgLen = K_MAX_SIZE - 1 - fOffset;

		// read the messages coming from the (debugging) browser
		err = fWS->ReadMessage(fMsgData+fOffset,fMsgLen,fIsMsgTerminated, 0);

		fMsgData[K_MAX_SIZE-1] = 0;

		// only display this trace on error (otherwise there are lots of msgs due to polling
		if (err)
		{
			tmpStr = CVSTR("VChromeDbgHdlPage::TreatWS ReadMessage status=");
			tmpStr.AppendLong8(err);
			tmpStr += " len=";
			tmpStr.AppendLong(fMsgLen);
			tmpStr += " fIsMsgTerminated=";
			tmpStr += (fIsMsgTerminated ? "1" : "0" );
			sPrivateLogHandler->Put( (err != VE_OK ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),tmpStr);
		}
		else
		{
			fMsgData[fOffset+fMsgLen] = 0;
		}

		if (!err)
		{
			// when nothing requested by the browser
			if (!fMsgLen)
			{
				// read the messages coming from the WAKSrv pilot
				err = fFifo.Get(msg,msgFound,200);
				
				if (err || msgFound)
				{
					tmpStr = VString("VChromeDbgHdlPage::TreatWS page=");
					tmpStr.AppendLong(fPageNb);
					tmpStr += " ,fFifo.get status=";
					tmpStr.AppendLong8(err);
					tmpStr += ", msg_found=";
					tmpStr += (msgFound ? "1" : "0" );
					tmpStr += ", type=";
					tmpStr.AppendLong8(msg.type);
					sPrivateLogHandler->Put( (err != VE_OK ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),tmpStr);
				}

				if (testAssert( err == VE_OK ))
				{
					if (!msgFound)
					{
						msg.type = NO_MSG;
					}
					switch(msg.type)
					{
					case NO_MSG:
						break;

					case STOP_MSG:
						err = VE_INVALID_PARAMETER;
						break;

					case EVAL_MSG:
						err = TreatEvaluate(msg);
						xbox_assert(err == VE_OK);
						break;

					case LOOKUP_MSG:
						err = TreatLookup(msg);
						xbox_assert(err == VE_OK);
						break;

					case ABORT_MSG:
						msg.type = SEND_CMD_MSG;
						msg.data.Msg.fType = WAKDebuggerServerMessage::SRV_ABORT;
						err = fOutFifo.Put(msg);
						if (err !=  VE_OK)
						{
							VTask::Sleep(1000);
							err = fOutFifo.Put(msg);
						}
						//err = VE_USER_ABORT;
						break;

					case BRKPT_REACHED_MSG:
						err = TreatBreakpointReached(msg);
						xbox_assert(err == VE_OK);
						break;

					case CALLSTACK_MSG:
						if (fInternalState & K_WAITING_CS_STATE)
						{
							fExcInfosStr = msg.data._excInfosStr;
							fCallstackDescription = msg.data._callstackDesc;
							err = TreatPageReload(msg.data._dataStr,fExcStr,fExcInfosStr);
							xbox_assert(err == VE_OK);
							fInternalState &= ~K_WAITING_CS_STATE;
						}
						else
						{
							xbox_assert(false);
							err = VE_INVALID_PARAMETER;
						}
						break;

					//case SEND_TRACES_MSG:
					//	err = TreatTraces(msg.data._values);
					//	break;

					default:
						DebugMsg("VChromeDbgHdlPage::TreatWS bad msg=%d\n",msg.type);
						tmpStr = CVSTR("VChromeDbgHdlPage::TreatWS bad msg=");
						tmpStr.AppendLong8(msg.type);
						sPrivateLogHandler->Put(WAKDBG_ERROR_LEVEL,tmpStr);
						xbox_assert(false);
					}
					TreatTraces();
				}
				continue;
			}
			//printf("ReadMessage len=%d term=%d\n",fMsgLen,fIsMsgTerminated);
			fOffset += fMsgLen;
			if (fIsMsgTerminated)
			{
				err = TreatMsg(inSem);
				if (err)
				{
					tmpStr = CVSTR("VChromeDbgHdlPage::TreatWS TreatMsg pb...closing");
					sPrivateLogHandler->Put(WAKDBG_ERROR_LEVEL,tmpStr);
					DebugMsg("VChromeDbgHdlPage::TreatWS TreatMsg pb...closing\n");
				}
				fOffset = 0;
			}
		}
		else
		{
			DebugMsg("VChromeDbgHdlPage::TreatWS ReadMessage ERR!!!\n");
		}
	}
	if (err && (fState < CONNECTED_STATE))
	{
		inSem->Unlock();
	}
	/*if (err != VE_USER_ABORT)
	{
		// we send a 'continue' msg so that the context will be continued (in case the user a closed the chrm debugging window without WAKsrv being aware)
		msg.type = SEND_CMD_MSG;
		msg.data.Msg.fType = WAKDebuggerServerMessage::SRV_CONTINUE_MSG;
		fOutFifo.Put(msg);
	}*/
	if (err && (err != VE_USER_ABORT))
	{
		msg.type = SEND_CMD_MSG;
		msg.data.Msg.fType = WAKDebuggerServerMessage::SRV_CONNEXION_INTERRUPTED;
		fOutFifo.Put(msg);
	}
	fWS->Close();
	fState = STOPPED_STATE;
	fSem.Unlock();
	
	fTask->SetName(originalTaskName);
	fTask = NULL;
	return err;
}

XBOX::VError VChromeDbgHdlPage::Lookup(const XBOX::VString& inLookupData)
{
	ChrmDbgMsg_t		pageMsg;

	pageMsg.type = LOOKUP_MSG;
	pageMsg.data._dataStr = inLookupData;

	VError	err = fFifo.Put(pageMsg);
	bool res = (err == VE_OK);
	VString traceStr = VString("VChromeDbgHdlPage::Lookup (SEND_LOOKUP_MSG) fFifo.put status=");
	traceStr += (res ? "1" : "0" );
	sPrivateLogHandler->Put( (!res ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),traceStr);
	return err;
}

XBOX::VError VChromeDbgHdlPage::Eval(const XBOX::VString& inEvalData,const XBOX::VString& inRequestId)
{
	ChrmDbgMsg_t		pageMsg;

	pageMsg.type = EVAL_MSG;
	pageMsg.data._dataStr = inEvalData;
	pageMsg.data._requestId = inRequestId;

	VError	err = fFifo.Put(pageMsg);
	bool res = (err == VE_OK);
	VString traceStr = VString("VChromeDbgHdlPage::Eval (SEND_EVAL_MSG) fFifo.put status=");
	traceStr += (res ? "1" : "0" );
	sPrivateLogHandler->Put( (!res ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),traceStr);
	return err;
}

XBOX::VError VChromeDbgHdlPage::Callstack(	const XBOX::VString&				inCallstackData,
											const XBOX::VString&				inExceptionInfosStr,
											const CallstackDescriptionMap&		inCallstackDesc)
{
	ChrmDbgMsg_t		pageMsg;

	pageMsg.type = CALLSTACK_MSG;
	pageMsg.data._dataStr = inCallstackData;
	pageMsg.data._excInfosStr = inExceptionInfosStr;
	pageMsg.data._callstackDesc = inCallstackDesc;

	VError	err = fFifo.Put(pageMsg);
	bool res = (err == VE_OK);
	VString traceStr = VString("VChromeDbgHdlPage::Callstack (SEND_CALLSTACK_MSG) fFifo.put status=");
	traceStr += (res ? "1" : "0" );
	sPrivateLogHandler->Put( (!res ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),traceStr);
	return err;
}


XBOX::VError VChromeDbgHdlPage::BreakpointReached(
										int								inLineNb,
										const XBOX::VString&			inUrlStr,
										const XBOX::VString&			inExceptionStr)	
{
	ChrmDbgMsg_t		pageMsg;
					
	pageMsg.type = BRKPT_REACHED_MSG;
	pageMsg.data.Msg.fLineNumber = inLineNb;
	pageMsg.data._urlStr = inUrlStr;
	pageMsg.data._excStr = inExceptionStr;

	VError	err = fFifo.Put(pageMsg);
	bool res = (err == VE_OK);
	VString traceStr = VString("VChromeDbgHdlPage::BreakpointReached (BREAKPOINT_REACHED_MSG) fFifo.put status=");
	traceStr += (res ? "1" : "0" );
	sPrivateLogHandler->Put( (!res ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),traceStr);
	return err;
}

XBOX::VError VChromeDbgHdlPage::Abort()
{
	ChrmDbgMsg_t		pageMsg;
	VError				err;

	pageMsg.type = ABORT_MSG;
	err = fFifo.Put(pageMsg);
	if (!testAssert(err == VE_OK))
	{
		VTask::Sleep(1000);
		err = fFifo.Put(pageMsg);
	}
	return err;
}

WAKDebuggerServerMessage* VChromeDbgHdlPage::WaitFrom()
{
	WAKDebuggerServerMessage*		msg;
	ChrmDbgMsg_t					pageMsg;
	bool							msgFound;

	msg = new WAKDebuggerServerMessage();

	if (testAssert( msg != NULL ))
	{
		msg->fType = WAKDebuggerServerMessage::SRV_CONTINUE_MSG;
	}
	else
	{
		return NULL;
	}

#if 1

	fOutFifo.Get(pageMsg,msgFound,0);
	if (!msgFound)
	{
		pageMsg.type = NO_MSG;
	}
	if (pageMsg.type == SEND_CMD_MSG)
	{
		memcpy( msg, &pageMsg.data.Msg, sizeof(*msg) );
	}
	else
	{
		xbox_assert(false);
	}
	return msg;

#endif
}

