/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/dde.cpp
// Purpose:     DDE classes
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"


#if wxUSE_IPC

#ifndef WX_PRECOMP
    #include "wx/utils.h"
    #include "wx/app.h"
    #include "wx/hashmap.h"
    #include "wx/module.h"
#endif

#include "wx/dde.h"
#include "wx/intl.h"
#include "wx/buffer.h"
#include "wx/strconv.h"

#include "wx/msw/private.h"

#include <string.h>
#include <ddeml.h>

// ----------------------------------------------------------------------------
// macros and constants
// ----------------------------------------------------------------------------

#if wxUSE_UNICODE
    #define DDE_CP      CP_WINUNICODE
#else
    #define DDE_CP      CP_WINANSI
#endif

#define GetHConv()       ((HCONV)m_hConv)

// default timeout for DDE operations (5sec)
#define DDE_TIMEOUT     5000

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

static wxDDEConnection *DDEFindConnection(HCONV hConv);
static void DDEDeleteConnection(HCONV hConv);
static wxDDEServer *DDEFindServer(const wxString& s);

extern "C" HDDEDATA EXPENTRY
_DDECallback(UINT wType,
             UINT wFmt,
             HCONV hConv,
             HSZ hsz1,
             HSZ hsz2,
             HDDEDATA hData,
             ULONG_PTR lData1,
             ULONG_PTR lData2);

// Add topic name to atom table before using in conversations
static HSZ DDEAddAtom(const wxString& string);
static HSZ DDEGetAtom(const wxString& string);

// string handles
static HSZ DDEAtomFromString(const wxString& s);
static wxString DDEStringFromAtom(HSZ hsz);
static void DDEFreeString(HSZ hsz);

// error handling
static wxString DDEGetErrorMsg(UINT error);
static void DDELogError(const wxString& s, UINT error = DMLERR_NO_ERROR);

// ----------------------------------------------------------------------------
// global variables
// ----------------------------------------------------------------------------

WX_DECLARE_STRING_HASH_MAP( HSZ, wxAtomMap );

static DWORD DDEIdInst = 0L;
static wxDDEConnection *DDECurrentlyConnecting = NULL;
static wxAtomMap wxAtomTable;

#include "wx/listimpl.cpp"

WX_DEFINE_LIST(wxDDEClientList)
WX_DEFINE_LIST(wxDDEServerList)
WX_DEFINE_LIST(wxDDEConnectionList)

static wxDDEClientList wxDDEClientObjects;
static wxDDEServerList wxDDEServerObjects;

static bool DDEInitialized = false;

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// A module to allow DDE cleanup without calling these functions
// from app.cpp or from the user's application.

class wxDDEModule : public wxModule
{
public:
    wxDDEModule() {}
    bool OnInit() override { return true; }
    void OnExit() override { wxDDECleanUp(); }

private:
    wxDECLARE_DYNAMIC_CLASS(wxDDEModule);
};

// ----------------------------------------------------------------------------
// wxWin macros
// ----------------------------------------------------------------------------

wxIMPLEMENT_DYNAMIC_CLASS(wxDDEServer, wxServerBase);
wxIMPLEMENT_DYNAMIC_CLASS(wxDDEClient, wxClientBase);
wxIMPLEMENT_DYNAMIC_CLASS(wxDDEConnection, wxConnectionBase);
wxIMPLEMENT_DYNAMIC_CLASS(wxDDEModule, wxModule);

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// initialization and cleanup
// ----------------------------------------------------------------------------

extern void wxDDEInitialize()
{
    if ( !DDEInitialized )
    {
        // Should insert filter flags
        UINT rc = DdeInitialize(&DDEIdInst, _DDECallback, APPCLASS_STANDARD, 0L);
        if ( rc != DMLERR_NO_ERROR )
        {
            DDELogError(wxT("Failed to initialize DDE"), rc);
        }
        else
        {
            DDEInitialized = true;
        }
    }
}

void wxDDECleanUp()
{
    // deleting them later won't work as DDE won't be initialized any more
    wxASSERT_MSG( wxDDEServerObjects.empty() &&
                    wxDDEClientObjects.empty(),
                    wxT("all DDE objects should be deleted by now") );

    wxAtomTable.clear();

    if ( DDEIdInst != 0 )
    {
        DdeUninitialize(DDEIdInst);
        DDEIdInst = 0;
    }
}

// ----------------------------------------------------------------------------
// functions working with the global connection list(s)
// ----------------------------------------------------------------------------

// Global find connection
static wxDDEConnection *DDEFindConnection(HCONV hConv)
{
    wxDDEServerList::compatibility_iterator serverNode = wxDDEServerObjects.GetFirst();
    wxDDEConnection *found = NULL;
    while (serverNode && !found)
    {
        wxDDEServer *object = serverNode->GetData();
        found = object->FindConnection((WXHCONV) hConv);
        serverNode = serverNode->GetNext();
    }

    if (found)
    {
        return found;
    }

    wxDDEClientList::compatibility_iterator clientNode = wxDDEClientObjects.GetFirst();
    while (clientNode && !found)
    {
        wxDDEClient *object = clientNode->GetData();
        found = object->FindConnection((WXHCONV) hConv);
        clientNode = clientNode->GetNext();
    }
    return found;
}

// Global delete connection
static void DDEDeleteConnection(HCONV hConv)
{
    wxDDEServerList::compatibility_iterator serverNode = wxDDEServerObjects.GetFirst();
    bool found = false;
    while (serverNode && !found)
    {
        wxDDEServer *object = serverNode->GetData();
        found = object->DeleteConnection((WXHCONV) hConv);
        serverNode = serverNode->GetNext();
    }
    if (found)
    {
        return;
    }

    wxDDEClientList::compatibility_iterator clientNode = wxDDEClientObjects.GetFirst();
    while (clientNode && !found)
    {
        wxDDEClient *object = clientNode->GetData();
        found = object->DeleteConnection((WXHCONV) hConv);
        clientNode = clientNode->GetNext();
    }
}

// Find a server from a service name
static wxDDEServer *DDEFindServer(const wxString& s)
{
    wxDDEServerList::compatibility_iterator node = wxDDEServerObjects.GetFirst();
    wxDDEServer *found = NULL;
    while (node && !found)
    {
        wxDDEServer *object = node->GetData();

        if (object->GetServiceName() == s)
        {
            found = object;
        }
        else
        {
            node = node->GetNext();
        }
    }

    return found;
}

// ----------------------------------------------------------------------------
// wxDDEServer
// ----------------------------------------------------------------------------

wxDDEServer::wxDDEServer()
{
    wxDDEInitialize();

    wxDDEServerObjects.Append(this);
}

bool wxDDEServer::Create(const wxString& server)
{
    m_serviceName = server;

    HSZ hsz = DDEAtomFromString(server);

    if ( !hsz )
    {
        return false;
    }


    bool success = (DdeNameService(DDEIdInst, hsz, (HSZ) NULL, DNS_REGISTER)
        != NULL);

    if (!success)
    {
        DDELogError(wxString::Format(_("Failed to register DDE server '%s'"),
            server.c_str()));
    }

    DDEFreeString(hsz);

    return success;
}

wxDDEServer::~wxDDEServer()
{
    if ( !m_serviceName.empty() )
    {
        HSZ hsz = DDEAtomFromString(m_serviceName);

        if (hsz)
        {
            if ( !DdeNameService(DDEIdInst, hsz,
                (HSZ) NULL, DNS_UNREGISTER) )
            {
                DDELogError(wxString::Format(
                    _("Failed to unregister DDE server '%s'"),
                    m_serviceName.c_str()));
            }

            DDEFreeString(hsz);
        }
    }

    wxDDEServerObjects.DeleteObject(this);

    wxDDEConnectionList::compatibility_iterator node = m_connections.GetFirst();
    while (node)
    {
        wxDDEConnection *connection = node->GetData();
        wxDDEConnectionList::compatibility_iterator next = node->GetNext();
        connection->OnDisconnect(); // May delete the node implicitly
        node = next;
    }

    // If any left after this, delete them
    node = m_connections.GetFirst();
    while (node)
    {
        wxDDEConnection *connection = node->GetData();
        wxDDEConnectionList::compatibility_iterator next = node->GetNext();
        delete connection;
        node = next;
    }
}

wxConnectionBase *wxDDEServer::OnAcceptConnection(const wxString& /* topic */)
{
    return new wxDDEConnection;
}

wxDDEConnection *wxDDEServer::FindConnection(WXHCONV conv)
{
    wxDDEConnectionList::compatibility_iterator node = m_connections.GetFirst();
    wxDDEConnection *found = NULL;
    while (node && !found)
    {
        wxDDEConnection *connection = node->GetData();
        if (connection->m_hConv == conv)
            found = connection;
        else node = node->GetNext();
    }
    return found;
}

// Only delete the entry in the map, not the actual connection
bool wxDDEServer::DeleteConnection(WXHCONV conv)
{
    wxDDEConnectionList::compatibility_iterator node = m_connections.GetFirst();
    while (node)
    {
        wxDDEConnection *connection = node->GetData();
        if (connection->m_hConv == conv)
        {
            m_connections.Erase(node);
            return true;
        }
        else
        {
            node = node->GetNext();
        }
    }
    return false;
}

// ----------------------------------------------------------------------------
// wxDDEClient
// ----------------------------------------------------------------------------

wxDDEClient::wxDDEClient()
{
    wxDDEInitialize();

    wxDDEClientObjects.Append(this);
}

wxDDEClient::~wxDDEClient()
{
    wxDDEClientObjects.DeleteObject(this);
    wxDDEConnectionList::compatibility_iterator node = m_connections.GetFirst();
    while (node)
    {
        wxDDEConnection *connection = node->GetData();
        delete connection;  // Deletes the node implicitly (see ~wxDDEConnection)
        node = m_connections.GetFirst();
    }
}

bool wxDDEClient::ValidHost(const wxString& /* host */)
{
    return true;
}

wxConnectionBase *wxDDEClient::MakeConnection(const wxString& WXUNUSED(host),
                                              const wxString& server,
                                              const wxString& topic)
{
    HSZ hszServer = DDEAtomFromString(server);

    if ( !hszServer )
    {
        return NULL;
    }


    HSZ hszTopic = DDEAtomFromString(topic);

    if ( !hszTopic )
    {
        DDEFreeString(hszServer);
        return NULL;
    }


    HCONV hConv = ::DdeConnect(DDEIdInst, hszServer, hszTopic,
        (PCONVCONTEXT) NULL);

    DDEFreeString(hszServer);
    DDEFreeString(hszTopic);


    if ( !hConv )
    {
        DDELogError( wxString::Format(
            _("Failed to create connection to server '%s' on topic '%s'"),
            server.c_str(), topic.c_str()) );
    }
    else
    {
        wxDDEConnection *connection = (wxDDEConnection*) OnMakeConnection();
        if (connection)
        {
            connection->m_hConv = (WXHCONV) hConv;
            connection->m_topicName = topic;
            connection->m_client = this;
            m_connections.Append(connection);
            return connection;
        }
    }

    return NULL;
}

wxConnectionBase *wxDDEClient::OnMakeConnection()
{
    return new wxDDEConnection;
}

wxDDEConnection *wxDDEClient::FindConnection(WXHCONV conv)
{
    wxDDEConnectionList::compatibility_iterator node = m_connections.GetFirst();
    wxDDEConnection *found = NULL;
    while (node && !found)
    {
        wxDDEConnection *connection = node->GetData();
        if (connection->m_hConv == conv)
            found = connection;
        else node = node->GetNext();
    }
    return found;
}

// Only delete the entry in the map, not the actual connection
bool wxDDEClient::DeleteConnection(WXHCONV conv)
{
    wxDDEConnectionList::compatibility_iterator node = m_connections.GetFirst();
    while (node)
    {
        wxDDEConnection *connection = node->GetData();
        if (connection->m_hConv == conv)
        {
            m_connections.Erase(node);
            return true;
        }
        else node = node->GetNext();
    }
    return false;
}

// ----------------------------------------------------------------------------
// wxDDEConnection
// ----------------------------------------------------------------------------

wxDDEConnection::wxDDEConnection(void *buffer, size_t size)
     : wxConnectionBase(buffer, size)
{
    m_client = NULL;
    m_server = NULL;

    m_hConv = 0;
    m_sendingData = NULL;
}

wxDDEConnection::wxDDEConnection()
     : wxConnectionBase()
{
    m_hConv = 0;
    m_sendingData = NULL;
    m_server = NULL;
    m_client = NULL;
}

wxDDEConnection::~wxDDEConnection()
{
    Disconnect();
    if (m_server)
        m_server->GetConnections().DeleteObject(this);
    else
        m_client->GetConnections().DeleteObject(this);
}

// Calls that CLIENT can make
bool wxDDEConnection::Disconnect()
{
    if ( !GetConnected() )
        return true;

    DDEDeleteConnection(GetHConv());

    bool ok = DdeDisconnect(GetHConv()) != 0;
    if ( !ok )
    {
        DDELogError(wxT("Failed to disconnect from DDE server gracefully"));
    }

    SetConnected( false );  // so we don't try and disconnect again

    return ok;
}

bool
wxDDEConnection::DoExecute(const void *data, size_t size, wxIPCFormat format)
{
    wxCHECK_MSG( format == wxIPC_TEXT ||
                 format == wxIPC_UTF8TEXT ||
                 format == wxIPC_UNICODETEXT,
                 false,
                 wxT("wxDDEServer::Execute() supports only text data") );

    wxMemoryBuffer buffer;
    LPBYTE realData = NULL;
    size_t realSize = 0;
    wxMBConv *conv = NULL;

    // Windows only supports either ANSI or UTF-16 format depending on the
    // build, so we need to convert the data if it doesn't use it already
#if wxUSE_UNICODE
    if ( format == wxIPC_TEXT )
    {
        conv = &wxConvLibc;
    }
    else if ( format == wxIPC_UTF8TEXT )
    {
        conv = &wxConvUTF8;
    }
    else // no conversion necessary for wxIPC_UNICODETEXT
    {
        realData = const_cast<BYTE*>(static_cast<const BYTE*>(data));
        realSize = size;
    }

    if ( conv )
    {
        const char * const text = (const char *)data;
        const size_t len = size;

        realSize = conv->ToWChar(NULL, 0, text, len);
        if ( realSize == wxCONV_FAILED )
            return false;

        realData = (LPBYTE)buffer.GetWriteBuf(realSize*sizeof(wchar_t));
        if ( !realData )
            return false;

        realSize = conv->ToWChar((wchar_t *)realData, realSize, text, len);
        if ( realSize == wxCONV_FAILED )
            return false;

        // We need to pass the size of the buffer to DdeClientTransaction() and
        // not the length of the string.
        realSize *= sizeof(wchar_t);
    }
#else // !wxUSE_UNICODE
    if ( format == wxIPC_UNICODETEXT )
    {
        conv = &wxConvLibc;
    }
    else if ( format == wxIPC_UTF8TEXT )
    {
        // we could implement this in theory but it's not obvious how to pass
        // the format information and, basically, why bother -- just use
        // Unicode build
        wxFAIL_MSG( wxT("UTF-8 text not supported in ANSI build") );

        return false;
    }
    else // don't convert wxIPC_TEXT
    {
        realData = (LPBYTE)data;
        realSize = size;
    }

    if ( conv )
    {
        const wchar_t * const wtext = (const wchar_t *)data;
        const size_t len = size/sizeof(wchar_t);

        realSize = conv->FromWChar(NULL, 0, wtext, len);
        if ( realSize == wxCONV_FAILED )
            return false;

        realData = (LPBYTE)buffer.GetWriteBuf(realSize);
        if ( !realData )
            return false;

        realSize = conv->FromWChar((char*)realData, realSize, wtext, len);
        if ( realSize == wxCONV_FAILED )
            return false;
    }
#endif // wxUSE_UNICODE/!wxUSE_UNICODE

    DWORD result;
    bool ok = DdeClientTransaction(realData,
                                   realSize,
                                   GetHConv(),
                                   NULL,
                                   // MSDN: if the transaction specified by
                                   // the wType parameter does not pass data
                                   // or is XTYP_EXECUTE, wFmt should be zero.
                                   0,
                                   XTYP_EXECUTE,
                                   DDE_TIMEOUT,
                                   &result) != 0;

    if ( !ok )
    {
        DDELogError(wxT("DDE execute request failed"));
    }

    return ok;
}

const void *wxDDEConnection::Request(const wxString& item, size_t *size, wxIPCFormat format)
{
    DWORD result;

    HSZ atom = DDEGetAtom(item);

    HDDEDATA returned_data = DdeClientTransaction(NULL, 0,
                                                  GetHConv(),
                                                  atom, format,
                                                  XTYP_REQUEST,
                                                  DDE_TIMEOUT,
                                                  &result);
    if ( !returned_data )
    {
        DDELogError(wxT("DDE data request failed"));

        return NULL;
    }

    DWORD len = DdeGetData(returned_data, NULL, 0, 0);

    void *data = GetBufferAtLeast(len);
    wxASSERT_MSG(data != NULL,
                 wxT("Buffer too small in wxDDEConnection::Request") );
    (void) DdeGetData(returned_data, (LPBYTE)data, len, 0);

    (void) DdeFreeDataHandle(returned_data);

    if (size)
        *size = (size_t)len;

    return data;
}

bool wxDDEConnection::DoPoke(const wxString& item, const void *data, size_t size, wxIPCFormat format)
{
    DWORD result;

    HSZ item_atom = DDEGetAtom(item);
    bool ok = DdeClientTransaction(const_cast<BYTE*>(static_cast<const BYTE*>(data)),
                                   size,
                                   GetHConv(),
                                   item_atom, format,
                                   XTYP_POKE,
                                   DDE_TIMEOUT,
                                   &result) != 0;
    if ( !ok )
    {
        DDELogError(_("DDE poke request failed"));
    }

    return ok;
}

bool wxDDEConnection::StartAdvise(const wxString& item)
{
    DWORD result;
    HSZ atom = DDEGetAtom(item);

    bool ok = DdeClientTransaction(NULL, 0,
                                   GetHConv(),
                                   atom, CF_TEXT,
                                   XTYP_ADVSTART,
                                   DDE_TIMEOUT,
                                   &result) != 0;
    if ( !ok )
    {
        DDELogError(_("Failed to establish an advise loop with DDE server"));
    }

    return ok;
}

bool wxDDEConnection::StopAdvise(const wxString& item)
{
    DWORD result;
    HSZ atom = DDEGetAtom(item);

    bool ok = DdeClientTransaction(NULL, 0,
                                   GetHConv(),
                                   atom, CF_TEXT,
                                   XTYP_ADVSTOP,
                                   DDE_TIMEOUT,
                                   &result) != 0;
    if ( !ok )
    {
        DDELogError(_("Failed to terminate the advise loop with DDE server"));
    }

    return ok;
}

// Small helper function converting the data from the format used by the given
// conversion to UTF-8.
static
wxCharBuffer ConvertToUTF8(const wxMBConv& conv, const void* data, size_t size)
{
    return wxConvUTF8.cWC2MB(conv.cMB2WC((const char*)data, size, NULL));
}

// Calls that SERVER can make
bool wxDDEConnection::DoAdvise(const wxString& item,
                               const void *data,
                               size_t size,
                               wxIPCFormat format)
{
    HSZ item_atom = DDEGetAtom(item);
    HSZ topic_atom = DDEGetAtom(m_topicName);

    // Define the buffer which, if it's used at all, needs to stay alive until
    // after DdePostAdvise() call which uses it.
    wxCharBuffer buf;

    // As we always use CF_TEXT for XTYP_ADVSTART, we have to use wxIPC_TEXT
    // here, even if a different format was specified for this value. Of
    // course, this can only be done for just a few of the formats, so check
    // that we have one of them here.
    switch ( format )
    {
        case wxIPC_TEXT:
        case wxIPC_OEMTEXT:
        case wxIPC_UTF8TEXT:
        case wxIPC_PRIVATE:
            // Use the data directly.
            m_sendingData = data;
            m_dataSize = size;
            break;

        case wxIPC_UTF16TEXT:
        case wxIPC_UTF32TEXT:
            // We need to convert the data to UTF-8 as UTF-16 or 32 would
            // appear as mojibake in the client when received as CF_TEXT.
            buf = format == wxIPC_UTF16TEXT
                    ? ConvertToUTF8(wxMBConvUTF16(), data, size)
                    : ConvertToUTF8(wxMBConvUTF32(), data, size);

            m_sendingData = buf.data();
            m_dataSize = buf.length();
            break;

        case wxIPC_INVALID:
        case wxIPC_BITMAP:
        case wxIPC_METAFILE:
        case wxIPC_SYLK:
        case wxIPC_DIF:
        case wxIPC_TIFF:
        case wxIPC_DIB:
        case wxIPC_PALETTE:
        case wxIPC_PENDATA:
        case wxIPC_RIFF:
        case wxIPC_WAVE:
        case wxIPC_ENHMETAFILE:
        case wxIPC_FILENAME:
        case wxIPC_LOCALE:
            wxFAIL_MSG( "Unsupported IPC format for Advise()" );
            return false;
    }

    bool ok = DdePostAdvise(DDEIdInst, topic_atom, item_atom) != 0;
    if ( !ok )
    {
        DDELogError(_("Failed to send DDE advise notification"));
    }

    return ok;
}

// ----------------------------------------------------------------------------
// _DDECallback
// ----------------------------------------------------------------------------

#define DDERETURN HDDEDATA

HDDEDATA EXPENTRY
_DDECallback(UINT wType,
             UINT wFmt,
             HCONV hConv,
             HSZ hsz1,
             HSZ hsz2,
             HDDEDATA hData,
             ULONG_PTR WXUNUSED(lData1),
             ULONG_PTR WXUNUSED(lData2))
{
    switch (wType)
    {
        case XTYP_CONNECT:
            {
                wxString topic = DDEStringFromAtom(hsz1),
                         srv = DDEStringFromAtom(hsz2);
                wxDDEServer *server = DDEFindServer(srv);
                if (server)
                {
                    wxDDEConnection *connection =
                        (wxDDEConnection*) server->OnAcceptConnection(topic);
                    if (connection)
                    {
                        connection->m_server = server;
                        server->GetConnections().Append(connection);
                        connection->m_hConv = 0;
                        connection->m_topicName = topic;
                        DDECurrentlyConnecting = connection;
                        return (DDERETURN)(DWORD)true;
                    }
                }
                break;
            }

        case XTYP_CONNECT_CONFIRM:
            {
                if (DDECurrentlyConnecting)
                {
                    DDECurrentlyConnecting->m_hConv = (WXHCONV) hConv;
                    DDECurrentlyConnecting = NULL;
                    return (DDERETURN)(DWORD)true;
                }
                break;
            }

        case XTYP_DISCONNECT:
            {
                wxDDEConnection *connection = DDEFindConnection(hConv);
                if (connection)
                {
                    connection->SetConnected( false );
                    if (connection->OnDisconnect())
                    {
                        DDEDeleteConnection(hConv);  // Delete mapping: hConv => connection
                        return (DDERETURN)(DWORD)true;
                    }
                }
                break;
            }

        case XTYP_EXECUTE:
            {
                wxDDEConnection *connection = DDEFindConnection(hConv);

                if (connection)
                {
                    DWORD len = DdeGetData(hData, NULL, 0, 0);

                    void *data = connection->GetBufferAtLeast(len);
                    wxASSERT_MSG(data != NULL,
                                 wxT("Buffer too small in _DDECallback (XTYP_EXECUTE)") );

                    DdeGetData(hData, (LPBYTE)data, len, 0);

                    DdeFreeDataHandle(hData);

                    // XTYP_EXECUTE can be used for text only and the text is
                    // always in ANSI format for ANSI build and Unicode format
                    // in Unicode build
                    #if wxUSE_UNICODE
                        wFmt = wxIPC_UNICODETEXT;
                    #else
                        wFmt = wxIPC_TEXT;
                    #endif

                    if ( connection->OnExecute(connection->m_topicName,
                                               data,
                                               (int)len,
                                               (wxIPCFormat)wFmt) )
                    {
                        return (DDERETURN)(DWORD)DDE_FACK;
                    }
                }

                return (DDERETURN)DDE_FNOTPROCESSED;
            }

        case XTYP_REQUEST:
            {
                wxDDEConnection *connection = DDEFindConnection(hConv);

                if (connection)
                {
                    wxString item_name = DDEStringFromAtom(hsz2);

                    size_t user_size = wxNO_LEN;
                    const void *data = connection->OnRequest(connection->m_topicName,
                                                             item_name,
                                                             &user_size,
                                                             (wxIPCFormat)wFmt);
                    if (data)
                    {
                        if (user_size == wxNO_LEN)
                        {
                            switch (wFmt)
                            {
                                case wxIPC_TEXT:
                                case wxIPC_UTF8TEXT:
                                    user_size = strlen((const char*)data) + 1;  // includes final NUL
                                    break;
                                case wxIPC_UNICODETEXT:
                                    user_size = (wcslen((const wchar_t*)data) + 1) * sizeof(wchar_t);  // includes final NUL
                                    break;
                                default:
                                    user_size = 0;
                            }
                        }

                        HDDEDATA handle = DdeCreateDataHandle(DDEIdInst,
                                                              const_cast<BYTE*>(static_cast<const BYTE*>(data)),
                                                              user_size,
                                                              0,
                                                              hsz2,
                                                              wFmt,
                                                              0);
                        return (DDERETURN)handle;
                    }
                }
                break;
            }

        case XTYP_POKE:
            {
                wxDDEConnection *connection = DDEFindConnection(hConv);

                if (connection)
                {
                    wxString item_name = DDEStringFromAtom(hsz2);

                    DWORD len = DdeGetData(hData, NULL, 0, 0);

                    void *data = connection->GetBufferAtLeast(len);
                    wxASSERT_MSG(data != NULL,
                                 wxT("Buffer too small in _DDECallback (XTYP_POKE)") );

                    DdeGetData(hData, (LPBYTE)data, len, 0);

                    DdeFreeDataHandle(hData);

                    connection->OnPoke(connection->m_topicName,
                                       item_name,
                                       data,
                                       (int)len,
                                       (wxIPCFormat) wFmt);

                    return (DDERETURN)DDE_FACK;
                }
                else
                {
                    return (DDERETURN)DDE_FNOTPROCESSED;
                }
            }

        case XTYP_ADVSTART:
            {
                wxDDEConnection *connection = DDEFindConnection(hConv);

                if (connection)
                {
                    wxString item_name = DDEStringFromAtom(hsz2);

                    return (DDERETURN)connection->
                                OnStartAdvise(connection->m_topicName, item_name);
                }

                break;
            }

        case XTYP_ADVSTOP:
            {
                wxDDEConnection *connection = DDEFindConnection(hConv);

                if (connection)
                {
                    wxString item_name = DDEStringFromAtom(hsz2);

                    return (DDERETURN)connection->
                        OnStopAdvise(connection->m_topicName, item_name);
                }

                break;
            }

        case XTYP_ADVREQ:
            {
                wxDDEConnection *connection = DDEFindConnection(hConv);

                if (connection && connection->m_sendingData)
                {
                    HDDEDATA data = DdeCreateDataHandle
                                    (
                                        DDEIdInst,
                                        const_cast<BYTE*>(static_cast<const BYTE*>(connection->m_sendingData)),
                                        connection->m_dataSize,
                                        0,
                                        hsz2,
                                        wFmt,
                                        0
                                    );

                    connection->m_sendingData = NULL;

                    return (DDERETURN)data;
                }

                break;
            }

        case XTYP_ADVDATA:
            {
                wxDDEConnection *connection = DDEFindConnection(hConv);

                if (connection)
                {
                    wxString item_name = DDEStringFromAtom(hsz2);

                    DWORD len = DdeGetData(hData, NULL, 0, 0);

                    char* const data = (char *)connection->GetBufferAtLeast(len);
                    wxASSERT_MSG(data != NULL,
                                 wxT("Buffer too small in _DDECallback (XTYP_ADVDATA)") );

                    DdeGetData(hData, (LPBYTE)data, len, 0);

                    DdeFreeDataHandle(hData);

                    // We always get data in CF_TEXT format, but it could
                    // actually be UTF-8, so try recovering the original format
                    // if possible.
                    if ( wFmt != CF_TEXT )
                    {
                        wxLogDebug("Unexpected format %02x in XTYP_ADVDATA", wFmt);
                        return (DDERETURN)DDE_FNOTPROCESSED;
                    }

                    wxIPCFormat format;
                    if ( wxConvUTF8.ToWChar(NULL, 0, data, len) != wxCONV_FAILED )
                        format = wxIPC_UTF8TEXT;
                    else
                        format = wxIPC_TEXT;

                    if ( connection->OnAdvise(connection->m_topicName,
                                              item_name,
                                              data,
                                              (int)len,
                                              format) )
                    {
                        return (DDERETURN)(DWORD)DDE_FACK;
                    }
                }

                return (DDERETURN)DDE_FNOTPROCESSED;
            }
    }

    return (DDERETURN)0;
}

// ----------------------------------------------------------------------------
// DDE strings and atoms
// ----------------------------------------------------------------------------

// Atom table stuff
static HSZ DDEAddAtom(const wxString& str)
{
    HSZ atom = DDEAtomFromString(str);
    wxAtomTable[str] = atom;
    return atom;
}

static HSZ DDEGetAtom(const wxString& str)
{
    wxAtomMap::iterator it = wxAtomTable.find(str);

    if (it != wxAtomTable.end())
        return it->second;

    return DDEAddAtom(str);
}

/* atom <-> strings
The returned handle has to be freed by the caller (using
(static) DDEFreeString).
*/
static HSZ DDEAtomFromString(const wxString& s)
{
    wxASSERT_MSG( DDEIdInst, wxT("DDE not initialized") );

    HSZ hsz = DdeCreateStringHandle(DDEIdInst, wxMSW_CONV_LPTSTR(s), DDE_CP);
    if ( !hsz )
    {
        DDELogError(_("Failed to create DDE string"));
    }

    return hsz;
}

static wxString DDEStringFromAtom(HSZ hsz)
{
    // all DDE strings are normally limited to 255 bytes
    static const size_t len = 256;

    wxString s;
    (void)DdeQueryString(DDEIdInst, hsz, wxStringBuffer(s, len), len, DDE_CP);

    return s;
}

static void DDEFreeString(HSZ hsz)
{
    // DS: Failure to free a string handle might indicate there's
    // some other severe error.
    bool ok = (::DdeFreeStringHandle(DDEIdInst, hsz) != 0);
    wxASSERT_MSG( ok, wxT("Failed to free DDE string handle") );
    wxUnusedVar(ok);
}

// ----------------------------------------------------------------------------
// error handling
// ----------------------------------------------------------------------------

static void DDELogError(const wxString& s, UINT error)
{
    if ( !error )
    {
        error = DdeGetLastError(DDEIdInst);
    }

    wxLogError(s + wxT(": ") + DDEGetErrorMsg(error));
}

static wxString DDEGetErrorMsg(UINT error)
{
    wxString err;
    switch ( error )
    {
        case DMLERR_NO_ERROR:
            err = _("no DDE error.");
            break;

        case DMLERR_ADVACKTIMEOUT:
            err = _("a request for a synchronous advise transaction has timed out.");
            break;
        case DMLERR_BUSY:
            err = _("the response to the transaction caused the DDE_FBUSY bit to be set.");
            break;
        case DMLERR_DATAACKTIMEOUT:
            err = _("a request for a synchronous data transaction has timed out.");
            break;
        case DMLERR_DLL_NOT_INITIALIZED:
            err = _("a DDEML function was called without first calling the DdeInitialize function,\nor an invalid instance identifier\nwas passed to a DDEML function.");
            break;
        case DMLERR_DLL_USAGE:
            err = _("an application initialized as APPCLASS_MONITOR has\nattempted to perform a DDE transaction,\nor an application initialized as APPCMD_CLIENTONLY has \nattempted to perform server transactions.");
            break;
        case DMLERR_EXECACKTIMEOUT:
            err = _("a request for a synchronous execute transaction has timed out.");
            break;
        case DMLERR_INVALIDPARAMETER:
            err = _("a parameter failed to be validated by the DDEML.");
            break;
        case DMLERR_LOW_MEMORY:
            err = _("a DDEML application has created a prolonged race condition.");
            break;
        case DMLERR_MEMORY_ERROR:
            err = _("a memory allocation failed.");
            break;
        case DMLERR_NO_CONV_ESTABLISHED:
            err = _("a client's attempt to establish a conversation has failed.");
            break;
        case DMLERR_NOTPROCESSED:
            err = _("a transaction failed.");
            break;
        case DMLERR_POKEACKTIMEOUT:
            err = _("a request for a synchronous poke transaction has timed out.");
            break;
        case DMLERR_POSTMSG_FAILED:
            err = _("an internal call to the PostMessage function has failed. ");
            break;
        case DMLERR_REENTRANCY:
            err = _("reentrancy problem.");
            break;
        case DMLERR_SERVER_DIED:
            err = _("a server-side transaction was attempted on a conversation\nthat was terminated by the client, or the server\nterminated before completing a transaction.");
            break;
        case DMLERR_SYS_ERROR:
            err = _("an internal error has occurred in the DDEML.");
            break;
        case DMLERR_UNADVACKTIMEOUT:
            err = _("a request to end an advise transaction has timed out.");
            break;
        case DMLERR_UNFOUND_QUEUE_ID:
            err = _("an invalid transaction identifier was passed to a DDEML function.\nOnce the application has returned from an XTYP_XACT_COMPLETE callback,\nthe transaction identifier for that callback is no longer valid.");
            break;
        default:
            err.Printf(_("Unknown DDE error %08x"), error);
    }

    return err;
}

#endif
  // wxUSE_IPC
