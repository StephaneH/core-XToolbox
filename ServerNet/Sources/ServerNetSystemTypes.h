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
#ifndef __SNET_SYST_TYPES__
#define __SNET_SYST_TYPES__


BEGIN_TOOLBOX_NAMESPACE


#if VERSIONMAC || VERSION_LINUX

	typedef sLONG Socket;
	const Socket kBAD_SOCKET=-1;

	class XBsdTCPSocket;
	typedef XBsdTCPSocket XTCPSock;

	class XBsdAcceptIterator;
	typedef XBsdAcceptIterator XTCPAcceptIterator;

	class XBsdNetAddr;
	typedef XBsdNetAddr XNetAddr;

	class XBsdUDPSocket;
	typedef XBsdUDPSocket XUDPSock;

#elif VERSIONWIN

	typedef SOCKET Socket;
	const Socket kBAD_SOCKET=INVALID_SOCKET;		

	class XWinTCPSocket;
	typedef XWinTCPSocket XTCPSock;

	class XWinAcceptIterator;
	typedef XWinAcceptIterator XTCPAcceptIterator;

	class XWinNetAddr;
	typedef XWinNetAddr XNetAddr;

	class XWinUDPSocket;
	typedef XWinUDPSocket XUDPSock;
	
#endif


END_TOOLBOX_NAMESPACE


#endif
