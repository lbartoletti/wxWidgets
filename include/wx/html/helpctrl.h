/////////////////////////////////////////////////////////////////////////////
// Name:        wx/html/helpctrl.h
// Purpose:     wxHtmlHelpController
// Notes:       Based on htmlhelp.cpp, implementing a monolithic
//              HTML Help controller class,  by Vaclav Slavik
// Author:      Harm van der Heijden and Vaclav Slavik
// Copyright:   (c) Harm van der Heijden and Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_HELPCTRL_H_
#define _WX_HELPCTRL_H_

#include "wx/defs.h"

#if wxUSE_WXHTML_HELP

#include "wx/helpbase.h"
#include "wx/html/helpfrm.h"

#define wxID_HTML_HELPFRAME   (wxID_HIGHEST + 1)

// This style indicates that the window is
// embedded in the application and must not be
// destroyed by the help controller.
#define wxHF_EMBEDDED                0x00008000

// Create a dialog for the help window.
#define wxHF_DIALOG                  0x00010000

// Create a frame for the help window.
#define wxHF_FRAME                   0x00020000

// Make the dialog modal when displaying help.
#define wxHF_MODAL                   0x00040000

class WXDLLIMPEXP_FWD_HTML wxHtmlHelpDialog;
class WXDLLIMPEXP_FWD_HTML wxHtmlHelpWindow;
class WXDLLIMPEXP_FWD_HTML wxHtmlHelpFrame;
class WXDLLIMPEXP_FWD_HTML wxHtmlHelpDialog;

class WXDLLIMPEXP_HTML wxHtmlHelpController : public wxHelpControllerBase // wxEvtHandler
{
    wxDECLARE_DYNAMIC_CLASS(wxHtmlHelpController);

public:
    wxHtmlHelpController(int style = wxHF_DEFAULT_STYLE, wxWindow* parentWindow = NULL);
    wxHtmlHelpController(wxWindow* parentWindow, int style = wxHF_DEFAULT_STYLE);

    virtual ~wxHtmlHelpController();

    void SetShouldPreventAppExit(bool enable);

    void SetTitleFormat(const wxString& format);
    void SetTempDir(const wxString& path) { m_helpData.SetTempDir(path); }
    bool AddBook(const wxString& book_url, bool show_wait_msg = false);
    bool AddBook(const wxFileName& book_file, bool show_wait_msg = false);

    bool Display(const wxString& x);
    bool Display(int id);
    bool DisplayContents() override;
    bool DisplayIndex();
    bool KeywordSearch(const wxString& keyword,
                       wxHelpSearchMode mode = wxHELP_SEARCH_ALL) override;

    wxHtmlHelpWindow* GetHelpWindow() { return m_helpWindow; }
    void SetHelpWindow(wxHtmlHelpWindow* helpWindow);

    wxHtmlHelpFrame* GetFrame() { return m_helpFrame; }
    wxHtmlHelpDialog* GetDialog() { return m_helpDialog; }

#if wxUSE_CONFIG
    void UseConfig(wxConfigBase *config, const wxString& rootpath = wxEmptyString);

    // Assigns config object to the Ctrl. This config is then
    // used in subsequent calls to Read/WriteCustomization of both help
    // Ctrl and it's wxHtmlWindow
    virtual void ReadCustomization(wxConfigBase *cfg, const wxString& path = wxEmptyString);
    virtual void WriteCustomization(wxConfigBase *cfg, const wxString& path = wxEmptyString);
#endif // wxUSE_CONFIG

    //// Backward compatibility with wxHelpController API

    virtual bool Initialize(const wxString& file, int WXUNUSED(server) ) override { return Initialize(file); }
    virtual bool Initialize(const wxString& file) override;
    virtual void SetViewer(const wxString& WXUNUSED(viewer), long WXUNUSED(flags) = 0) override {}
    virtual bool LoadFile(const wxString& file = wxT("")) override;
    virtual bool DisplaySection(int sectionNo) override;
    virtual bool DisplaySection(const wxString& section) override { return Display(section); }
    virtual bool DisplayBlock(long blockNo) override { return DisplaySection(blockNo); }
    virtual bool DisplayTextPopup(const wxString& text, const wxPoint& pos) override;

    virtual void SetFrameParameters(const wxString& titleFormat,
                               const wxSize& size,
                               const wxPoint& pos = wxDefaultPosition,
                               bool newFrameEachTime = false) override;
    /// Obtains the latest settings used by the help frame and the help
    /// frame.
    virtual wxFrame *GetFrameParameters(wxSize *size = NULL,
                               wxPoint *pos = NULL,
                               bool *newFrameEachTime = NULL) override;

    // Get direct access to help data:
    wxHtmlHelpData *GetHelpData() { return &m_helpData; }

    virtual bool Quit() override ;
    virtual void OnQuit() override {}

    void OnCloseFrame(wxCloseEvent& evt);

    // Make the help controller's frame 'modal' if
    // needed
    void MakeModalIfNeeded();

    // Find the top-most parent window
    wxWindow* FindTopLevelWindow();

protected:
    void Init(int style);

    virtual wxWindow* CreateHelpWindow();
    virtual wxHtmlHelpFrame* CreateHelpFrame(wxHtmlHelpData *data);
    virtual wxHtmlHelpDialog* CreateHelpDialog(wxHtmlHelpData *data);
    virtual void DestroyHelpWindow();

    wxHtmlHelpData      m_helpData;
    wxHtmlHelpWindow*   m_helpWindow;
#if wxUSE_CONFIG
    wxConfigBase *      m_Config;
    wxString            m_ConfigRoot;
#endif // wxUSE_CONFIG
    wxString            m_titleFormat;
    int                 m_FrameStyle;
    wxHtmlHelpFrame*    m_helpFrame;
    wxHtmlHelpDialog*   m_helpDialog;

    bool                m_shouldPreventAppExit;

    wxDECLARE_NO_COPY_CLASS(wxHtmlHelpController);
};

/*
 * wxHtmlModalHelp
 * A convenience class particularly for use on wxMac,
 * where you can only show modal dialogs from a modal
 * dialog.
 *
 * Use like this:
 *
 * wxHtmlModalHelp help(parent, filename, topic);
 *
 * If topic is empty, the help contents is displayed.
 */

class WXDLLIMPEXP_HTML wxHtmlModalHelp
{
public:
    wxHtmlModalHelp(wxWindow* parent, const wxString& helpFile, const wxString& topic = wxEmptyString,
        int style = wxHF_DEFAULT_STYLE | wxHF_DIALOG | wxHF_MODAL);
};

#endif // wxUSE_WXHTML_HELP

#endif // _WX_HELPCTRL_H_
