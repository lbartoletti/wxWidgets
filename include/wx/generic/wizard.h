///////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/wizard.h
// Purpose:     declaration of generic wxWizard class
// Author:      Vadim Zeitlin
// Modified by: Robert Vazan (sizers)
// Created:     28.09.99
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_WIZARD_H_
#define _WX_GENERIC_WIZARD_H_

// ----------------------------------------------------------------------------
// wxWizard
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_CORE wxButton;
class WXDLLIMPEXP_FWD_CORE wxStaticBitmap;
class WXDLLIMPEXP_FWD_CORE wxWizardEvent;
class WXDLLIMPEXP_FWD_CORE wxBoxSizer;
class WXDLLIMPEXP_FWD_CORE wxWizardSizer;

class WXDLLIMPEXP_CORE wxWizard : public wxWizardBase
{
public:
    // ctor
    wxWizard() { Init(); }
    wxWizard(wxWindow *parent,
             int id = wxID_ANY,
             const wxString& title = wxEmptyString,
             const wxBitmapBundle& bitmap = wxBitmapBundle(),
             const wxPoint& pos = wxDefaultPosition,
             long style = wxDEFAULT_DIALOG_STYLE)
    {
        Init();
        Create(parent, id, title, bitmap, pos, style);
    }
    bool Create(wxWindow *parent,
             int id = wxID_ANY,
             const wxString& title = wxEmptyString,
             const wxBitmapBundle& bitmap = wxBitmapBundle(),
             const wxPoint& pos = wxDefaultPosition,
             long style = wxDEFAULT_DIALOG_STYLE);
    void Init();
    virtual ~wxWizard();

    // implement base class pure virtuals
    virtual bool RunWizard(wxWizardPage *firstPage) override;
    virtual wxWizardPage *GetCurrentPage() const override;
    virtual void SetPageSize(const wxSize& size) override;
    virtual wxSize GetPageSize() const override;
    virtual void FitToPage(const wxWizardPage *firstPage) override;
    virtual wxSizer *GetPageAreaSizer() const override;
    virtual void SetBorder(int border) override;

    /// set/get bitmap
    wxBitmap GetBitmap() const { return m_bitmap.GetBitmapFor(this); }
    void SetBitmap(const wxBitmapBundle& bitmap);

    // implementation only from now on
    // -------------------------------

    // is the wizard running?
    bool IsRunning() const { return m_page != NULL; }

    // show the prev/next page, but call TransferDataFromWindow on the current
    // page first and return false without changing the page if
    // TransferDataFromWindow() returns false - otherwise, returns true
    virtual bool ShowPage(wxWizardPage *page, bool goingForward = true);

    // do fill the dialog with controls
    // this is app-overridable to, for example, set help and tooltip text
    virtual void DoCreateControls();

    // Do the adaptation
    virtual bool DoLayoutAdaptation() override;

    // Set/get bitmap background colour
    void SetBitmapBackgroundColour(const wxColour& colour) { m_bitmapBackgroundColour = colour; }
    const wxColour& GetBitmapBackgroundColour() const { return m_bitmapBackgroundColour; }

    // Set/get bitmap placement (centred, tiled etc.)
    void SetBitmapPlacement(int placement) { m_bitmapPlacement = placement; }
    int GetBitmapPlacement() const { return m_bitmapPlacement; }

    // Set/get minimum bitmap width
    void SetMinimumBitmapWidth(int w) { m_bitmapMinimumWidth = w; }
    int GetMinimumBitmapWidth() const { return m_bitmapMinimumWidth; }

    // Tile bitmap
    static bool TileBitmap(const wxRect& rect, wxDC& dc, const wxBitmap& bitmap);

protected:
    // for compatibility only, doesn't do anything any more
    void FinishLayout() { }

    // Do fit, and adjust to screen size if necessary
    virtual void DoWizardLayout();

    // Resize bitmap if necessary
    virtual bool ResizeBitmap(wxBitmap& bmp);

    // was the dialog really created?
    bool WasCreated() const { return m_btnPrev != NULL; }

    // event handlers
    void OnCancel(wxCommandEvent& event);
    void OnBackOrNext(wxCommandEvent& event);
    void OnHelp(wxCommandEvent& event);

    void OnWizEvent(wxWizardEvent& event);

    void AddBitmapRow(wxBoxSizer *mainColumn);
    void AddStaticLine(wxBoxSizer *mainColumn);
    void AddBackNextPair(wxBoxSizer *buttonRow);
    void AddButtonRow(wxBoxSizer *mainColumn);

    // This function can be used as event handle for wxEVT_DPI_CHANGED event.
    void WXHandleDPIChanged(wxDPIChangedEvent& event);

    // the page size requested by user
    wxSize m_sizePage;

    // the dialog position from the ctor
    wxPoint m_posWizard;

    // wizard state
    wxWizardPage  *m_page;       // the current page or NULL
    wxWizardPage  *m_firstpage;  // the page RunWizard started on or NULL
    wxBitmapBundle m_bitmap;     // the default bitmap to show

    // wizard controls
    wxButton    *m_btnPrev,     // the "<Back" button
                *m_btnNext;     // the "Next>" or "Finish" button
    wxStaticBitmap *m_statbmp;  // the control for the bitmap

    // cached labels so their translations stay consistent
    wxString    m_nextLabel,
                m_finishLabel;

    // Border around page area sizer requested using SetBorder()
    int m_border;

    // Whether RunWizard() was called
    bool m_started;

    // Whether was modal (modeless has to be destroyed on finish or cancel)
    bool m_wasModal;

    // True if pages are laid out using the sizer
    bool m_usingSizer;

    // Page area sizer will be inserted here with padding
    wxBoxSizer *m_sizerBmpAndPage;

    // Actual position and size of pages
    wxWizardSizer *m_sizerPage;

    // Bitmap background colour if resizing bitmap
    wxColour    m_bitmapBackgroundColour;

    // Bitmap placement flags
    int         m_bitmapPlacement;

    // Minimum bitmap width
    int         m_bitmapMinimumWidth;

    friend class wxWizardSizer;

    wxDECLARE_DYNAMIC_CLASS(wxWizard);
    wxDECLARE_EVENT_TABLE();
    wxDECLARE_NO_COPY_CLASS(wxWizard);
};

#endif // _WX_GENERIC_WIZARD_H_
