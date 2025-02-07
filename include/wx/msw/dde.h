/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/dde.h
// Purpose:     DDE class
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DDE_H_
#define _WX_DDE_H_

#include "wx/ipcbase.h"

/*
 * Mini-DDE implementation

   Most transactions involve a topic name and an item name (choose these
   as befits your application).

   A client can:

   - ask the server to execute commands (data) associated with a topic
   - request data from server by topic and item
   - poke data into the server
   - ask the server to start an advice loop on topic/item
   - ask the server to stop an advice loop

   A server can:

   - respond to execute, request, poke and advice start/stop
   - send advise data to client

   Note that this limits the server in the ways it can send data to the
   client, i.e. it can't send unsolicited information.
 *
 */

class WXDLLIMPEXP_FWD_BASE wxDDEServer;
class WXDLLIMPEXP_FWD_BASE wxDDEClient;

class WXDLLIMPEXP_BASE wxDDEConnection : public wxConnectionBase
{
public:
  wxDDEConnection(void *buffer, size_t size); // use external buffer
  wxDDEConnection(); // use internal buffer
  virtual ~wxDDEConnection();

  // implement base class pure virtual methods
  virtual const void *Request(const wxString& item,
                              size_t *size = NULL,
                              wxIPCFormat format = wxIPC_TEXT) override;
  virtual bool StartAdvise(const wxString& item) override;
  virtual bool StopAdvise(const wxString& item) override;
  virtual bool Disconnect() override;

protected:
  virtual bool DoExecute(const void *data, size_t size, wxIPCFormat format) override;
  virtual bool DoPoke(const wxString& item, const void *data, size_t size,
                      wxIPCFormat format) override;
  virtual bool DoAdvise(const wxString& item, const void *data, size_t size,
                        wxIPCFormat format) override;

public:
  wxString      m_topicName;
  wxDDEServer*  m_server;
  wxDDEClient*  m_client;

  WXHCONV       m_hConv;
  const void*   m_sendingData;
  int           m_dataSize;

  wxDECLARE_NO_COPY_CLASS(wxDDEConnection);
  wxDECLARE_DYNAMIC_CLASS(wxDDEConnection);
};

class WXDLLIMPEXP_BASE wxDDEServer : public wxServerBase
{
public:
    wxDDEServer();
    bool Create(const wxString& server_name) override;
    virtual ~wxDDEServer();

    virtual wxConnectionBase *OnAcceptConnection(const wxString& topic) override;

    // Find/delete wxDDEConnection corresponding to the HCONV
    wxDDEConnection *FindConnection(WXHCONV conv);
    bool DeleteConnection(WXHCONV conv);
    wxString& GetServiceName() { return m_serviceName; }
    const wxString& GetServiceName() const { return m_serviceName; }

    wxDDEConnectionList& GetConnections() { return m_connections; }
    const wxDDEConnectionList& GetConnections() const { return m_connections; }

protected:
    int       m_lastError;
    wxString  m_serviceName;
    wxDDEConnectionList m_connections;

    wxDECLARE_DYNAMIC_CLASS(wxDDEServer);
};

class WXDLLIMPEXP_BASE wxDDEClient: public wxClientBase
{
public:
    wxDDEClient();
    virtual ~wxDDEClient();

    bool ValidHost(const wxString& host) override;

    // Call this to make a connection. Returns NULL if cannot.
    virtual wxConnectionBase *MakeConnection(const wxString& host,
                                             const wxString& server,
                                             const wxString& topic) override;

    // Tailor this to return own connection.
    virtual wxConnectionBase *OnMakeConnection() override;

    // Find/delete wxDDEConnection corresponding to the HCONV
    wxDDEConnection *FindConnection(WXHCONV conv);
    bool DeleteConnection(WXHCONV conv);

    wxDDEConnectionList& GetConnections() { return m_connections; }
    const wxDDEConnectionList& GetConnections() const { return m_connections; }

protected:
    int       m_lastError;
    wxDDEConnectionList m_connections;

    wxDECLARE_DYNAMIC_CLASS(wxDDEClient);
};

void WXDLLIMPEXP_BASE wxDDEInitialize();
void WXDLLIMPEXP_BASE wxDDECleanUp();

#endif // _WX_DDE_H_
