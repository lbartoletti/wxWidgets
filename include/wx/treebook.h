///////////////////////////////////////////////////////////////////////////////
// Name:        wx/treebook.h
// Purpose:     wxTreebook: wxNotebook-like control presenting pages in a tree
// Author:      Evgeniy Tarassov, Vadim Zeitlin
// Modified by:
// Created:     2005-09-15
// Copyright:   (c) 2005 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_TREEBOOK_H_
#define _WX_TREEBOOK_H_

#include "wx/defs.h"

#if wxUSE_TREEBOOK

#include "wx/bookctrl.h"
#include "wx/containr.h"
#include "wx/treebase.h"        // for wxTreeItemId
#include "wx/vector.h"

typedef wxWindow wxTreebookPage;

class WXDLLIMPEXP_FWD_CORE wxTreeCtrl;
class WXDLLIMPEXP_FWD_CORE wxTreeEvent;

// ----------------------------------------------------------------------------
// wxTreebook
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxTreebook : public wxNavigationEnabled<wxBookCtrlBase>
{
public:
    // Constructors and such
    // ---------------------

    // Default ctor doesn't create the control, use Create() afterwards
    wxTreebook()
    {
    }

    // This ctor creates the tree book control
    wxTreebook(wxWindow *parent,
               wxWindowID id,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = wxBK_DEFAULT,
               const wxString& name = wxEmptyString)
    {
        (void)Create(parent, id, pos, size, style, name);
    }

    // Really creates the control
    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxBK_DEFAULT,
                const wxString& name = wxEmptyString);


    // Page insertion operations
    // -------------------------

    // Notice that page pointer may be NULL in which case the next non NULL
    // page (usually the first child page of a node) is shown when this page is
    // selected

    // Inserts a new page just before the page indicated by page.
    // The new page is placed on the same level as page.
    virtual bool InsertPage(size_t pos,
                            wxWindow *page,
                            const wxString& text,
                            bool bSelect = false,
                            int imageId = NO_IMAGE) override;

    // Inserts a new sub-page to the end of children of the page at given pos.
    virtual bool InsertSubPage(size_t pos,
                               wxWindow *page,
                               const wxString& text,
                               bool bSelect = false,
                               int imageId = NO_IMAGE);

    // Adds a new page at top level after all other pages.
    virtual bool AddPage(wxWindow *page,
                         const wxString& text,
                         bool bSelect = false,
                         int imageId = NO_IMAGE) override;

    // Adds a new child-page to the last top-level page inserted.
    // Useful when constructing 1 level tree structure.
    virtual bool AddSubPage(wxWindow *page,
                            const wxString& text,
                            bool bSelect = false,
                            int imageId = NO_IMAGE);

    // Deletes the page and ALL its children. Could trigger page selection
    // change in a case when selected page is removed. In that case its parent
    // is selected (or the next page if no parent).
    virtual bool DeletePage(size_t pos) override;


    // Tree operations
    // ---------------

    // Gets the page node state -- node is expanded or collapsed
    virtual bool IsNodeExpanded(size_t pos) const;

    // Expands or collapses the page node. Returns the previous state.
    // May generate page changing events (if selected page
    // is under the collapsed branch, then parent is autoselected).
    virtual bool ExpandNode(size_t pos, bool expand = true);

    // shortcut for ExpandNode(pos, false)
    bool CollapseNode(size_t pos) { return ExpandNode(pos, false); }

    // get the parent page or wxNOT_FOUND if this is a top level page
    int GetPageParent(size_t pos) const;

    // the tree control we use for showing the pages index tree
    wxTreeCtrl* GetTreeCtrl() const { return (wxTreeCtrl*)m_bookctrl; }


    // Standard operations inherited from wxBookCtrlBase
    // -------------------------------------------------

    virtual bool SetPageText(size_t n, const wxString& strText) override;
    virtual wxString GetPageText(size_t n) const override;
    virtual int GetPageImage(size_t n) const override;
    virtual bool SetPageImage(size_t n, int imageId) override;
    virtual int SetSelection(size_t n) override { return DoSetSelection(n, SetSelection_SendEvent); }
    virtual int ChangeSelection(size_t n) override { return DoSetSelection(n); }
    virtual int HitTest(const wxPoint& pt, long *flags = NULL) const override;
    virtual bool DeleteAllPages() override;

protected:
    // Implementation of a page removal. See DeletPage for comments.
    wxTreebookPage *DoRemovePage(size_t pos) override;

    virtual void OnImagesChanged() override;

    // This subclass of wxBookCtrlBase accepts NULL page pointers (empty pages)
    virtual bool AllowNullPage() const override { return true; }
    virtual wxWindow *TryGetNonNullPage(size_t page) override;

    // event handlers
    void OnTreeSelectionChange(wxTreeEvent& event);
    void OnTreeNodeExpandedCollapsed(wxTreeEvent& event);

    // array of tree item ids corresponding to the page indices
    wxVector<wxTreeItemId> m_treeIds;

private:
    // The real implementations of page insertion functions
    // ------------------------------------------------------
    // All DoInsert/Add(Sub)Page functions add the page into :
    // - the base class
    // - the tree control
    // - update the index/TreeItemId corespondance array
    bool DoInsertPage(size_t pos,
                      wxWindow *page,
                      const wxString& text,
                      bool bSelect = false,
                      int imageId = NO_IMAGE);
    bool DoInsertSubPage(size_t pos,
                         wxWindow *page,
                         const wxString& text,
                         bool bSelect = false,
                         int imageId = NO_IMAGE);
    bool DoAddSubPage(wxWindow *page,
                         const wxString& text,
                         bool bSelect = false,
                         int imageId = NO_IMAGE);

    // Overridden methods used by the base class DoSetSelection()
    // implementation.
    void UpdateSelectedPage(size_t newsel) override;
    wxBookCtrlEvent* CreatePageChangingEvent() const override;
    void MakeChangedEvent(wxBookCtrlEvent &event) override;

    // Does the selection update. Called from page insertion functions
    // to update selection if the selected page was pushed by the newly inserted
    void DoUpdateSelection(bool bSelect, int page);


    // Operations on the internal private members of the class
    // -------------------------------------------------------
    // Returns the page TreeItemId for the page.
    // Or, if the page index is incorrect, a fake one (fakePage.IsOk() == false)
    wxTreeItemId DoInternalGetPage(size_t pos) const;

    // Linear search for a page with the id specified. If no page
    // found wxNOT_FOUND is returned. The function is used when we catch an event
    // from m_tree (wxTreeCtrl) component.
    int DoInternalFindPageById(wxTreeItemId page) const;

    // Updates page and wxTreeItemId correspondence.
    void DoInternalAddPage(size_t newPos, wxWindow *page, wxTreeItemId pageId);

    // Removes the page from internal structure.
    void DoInternalRemovePage(size_t pos)
        { DoInternalRemovePageRange(pos, 0); }

    // Removes the page and all its children designated by subCount
    // from internal structures of the control.
    void DoInternalRemovePageRange(size_t pos, size_t subCount);

    // Returns internal number of pages which can be different from
    // GetPageCount() while performing a page insertion or removal.
    size_t DoInternalGetPageCount() const { return m_treeIds.size(); }


    wxDECLARE_EVENT_TABLE();
    wxDECLARE_DYNAMIC_CLASS_NO_COPY(wxTreebook);
};


// ----------------------------------------------------------------------------
// treebook event class and related stuff
// ----------------------------------------------------------------------------

// wxTreebookEvent is obsolete and defined for compatibility only
#define wxTreebookEvent wxBookCtrlEvent
typedef wxBookCtrlEventFunction wxTreebookEventFunction;
#define wxTreebookEventHandler(func) wxBookCtrlEventHandler(func)


wxDECLARE_EXPORTED_EVENT( WXDLLIMPEXP_CORE, wxEVT_TREEBOOK_PAGE_CHANGED, wxBookCtrlEvent );
wxDECLARE_EXPORTED_EVENT( WXDLLIMPEXP_CORE, wxEVT_TREEBOOK_PAGE_CHANGING, wxBookCtrlEvent );
wxDECLARE_EXPORTED_EVENT( WXDLLIMPEXP_CORE, wxEVT_TREEBOOK_NODE_COLLAPSED, wxBookCtrlEvent );
wxDECLARE_EXPORTED_EVENT( WXDLLIMPEXP_CORE, wxEVT_TREEBOOK_NODE_EXPANDED, wxBookCtrlEvent );

#define EVT_TREEBOOK_PAGE_CHANGED(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_TREEBOOK_PAGE_CHANGED, winid, wxBookCtrlEventHandler(fn))

#define EVT_TREEBOOK_PAGE_CHANGING(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_TREEBOOK_PAGE_CHANGING, winid, wxBookCtrlEventHandler(fn))

#define EVT_TREEBOOK_NODE_COLLAPSED(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_TREEBOOK_NODE_COLLAPSED, winid, wxBookCtrlEventHandler(fn))

#define EVT_TREEBOOK_NODE_EXPANDED(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_TREEBOOK_NODE_EXPANDED, winid, wxBookCtrlEventHandler(fn))

// old wxEVT_COMMAND_* constants
#define wxEVT_COMMAND_TREEBOOK_PAGE_CHANGED     wxEVT_TREEBOOK_PAGE_CHANGED
#define wxEVT_COMMAND_TREEBOOK_PAGE_CHANGING    wxEVT_TREEBOOK_PAGE_CHANGING
#define wxEVT_COMMAND_TREEBOOK_NODE_COLLAPSED   wxEVT_TREEBOOK_NODE_COLLAPSED
#define wxEVT_COMMAND_TREEBOOK_NODE_EXPANDED    wxEVT_TREEBOOK_NODE_EXPANDED


#endif // wxUSE_TREEBOOK

#endif // _WX_TREEBOOK_H_
