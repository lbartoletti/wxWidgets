///////////////////////////////////////////////////////////////////////////////
// Name:        wx/listbook.h
// Purpose:     wxListbook: wxListCtrl and wxNotebook combination
// Author:      Vadim Zeitlin
// Modified by:
// Created:     19.08.03
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_LISTBOOK_H_
#define _WX_LISTBOOK_H_

#include "wx/defs.h"

#if wxUSE_LISTBOOK

#include "wx/bookctrl.h"
#include "wx/containr.h"

class WXDLLIMPEXP_FWD_CORE wxListView;
class WXDLLIMPEXP_FWD_CORE wxListEvent;

wxDECLARE_EXPORTED_EVENT( WXDLLIMPEXP_CORE, wxEVT_LISTBOOK_PAGE_CHANGED,  wxBookCtrlEvent );
wxDECLARE_EXPORTED_EVENT( WXDLLIMPEXP_CORE, wxEVT_LISTBOOK_PAGE_CHANGING, wxBookCtrlEvent );

// wxListbook flags
#define wxLB_DEFAULT          wxBK_DEFAULT
#define wxLB_TOP              wxBK_TOP
#define wxLB_BOTTOM           wxBK_BOTTOM
#define wxLB_LEFT             wxBK_LEFT
#define wxLB_RIGHT            wxBK_RIGHT
#define wxLB_ALIGN_MASK       wxBK_ALIGN_MASK

// ----------------------------------------------------------------------------
// wxListbook
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxListbook : public wxNavigationEnabled<wxBookCtrlBase>
{
public:
    wxListbook() { }

    wxListbook(wxWindow *parent,
               wxWindowID id,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = 0,
               const wxString& name = wxEmptyString)
    {
        (void)Create(parent, id, pos, size, style, name);
    }

    // quasi ctor
    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = 0,
                const wxString& name = wxEmptyString);


    // overridden base class methods
    virtual bool SetPageText(size_t n, const wxString& strText) override;
    virtual wxString GetPageText(size_t n) const override;
    virtual int GetPageImage(size_t n) const override;
    virtual bool SetPageImage(size_t n, int imageId) override;
    virtual bool InsertPage(size_t n,
                            wxWindow *page,
                            const wxString& text,
                            bool bSelect = false,
                            int imageId = NO_IMAGE) override;
    virtual int SetSelection(size_t n) override { return DoSetSelection(n, SetSelection_SendEvent); }
    virtual int ChangeSelection(size_t n) override { return DoSetSelection(n); }
    virtual int HitTest(const wxPoint& pt, long *flags = NULL) const override;

    virtual bool DeleteAllPages() override;

    wxListView* GetListView() const { return (wxListView*)m_bookctrl; }

protected:
    virtual wxWindow *DoRemovePage(size_t page) override;

    virtual void OnImagesChanged() override;

    void UpdateSelectedPage(size_t newsel) override;

    wxBookCtrlEvent* CreatePageChangingEvent() const override;
    void MakeChangedEvent(wxBookCtrlEvent &event) override;

    // event handlers
    void OnListSelected(wxListEvent& event);
    void OnSize(wxSizeEvent& event);

private:
    // this should be called when we need to be relaid out
    void UpdateSize();

    // Get the correct wxListCtrl flags to use depending on our own flags.
    long GetListCtrlFlags(bool hasImages) const;

    // Update list control wxLC_ICON flag depending on whether we have images.
    void SyncListCtrlIconFlag(bool hasImages);


    wxDECLARE_EVENT_TABLE();
    wxDECLARE_DYNAMIC_CLASS_NO_COPY(wxListbook);
};

// ----------------------------------------------------------------------------
// listbook event class and related stuff
// ----------------------------------------------------------------------------

// wxListbookEvent is obsolete and defined for compatibility only (notice that
// we use #define and not typedef to also keep compatibility with the existing
// code which forward declares it)
#define wxListbookEvent wxBookCtrlEvent
typedef wxBookCtrlEventFunction wxListbookEventFunction;
#define wxListbookEventHandler(func) wxBookCtrlEventHandler(func)

#define EVT_LISTBOOK_PAGE_CHANGED(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_LISTBOOK_PAGE_CHANGED, winid, wxBookCtrlEventHandler(fn))

#define EVT_LISTBOOK_PAGE_CHANGING(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_LISTBOOK_PAGE_CHANGING, winid, wxBookCtrlEventHandler(fn))

// old wxEVT_COMMAND_* constants
#define wxEVT_COMMAND_LISTBOOK_PAGE_CHANGED    wxEVT_LISTBOOK_PAGE_CHANGED
#define wxEVT_COMMAND_LISTBOOK_PAGE_CHANGING   wxEVT_LISTBOOK_PAGE_CHANGING

#endif // wxUSE_LISTBOOK

#endif // _WX_LISTBOOK_H_
