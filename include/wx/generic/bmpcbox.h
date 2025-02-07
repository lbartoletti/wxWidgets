/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/bmpcbox.h
// Purpose:     wxBitmapComboBox
// Author:      Jaakko Salli
// Modified by:
// Created:     Aug-30-2006
// Copyright:   (c) Jaakko Salli
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_BMPCBOX_H_
#define _WX_GENERIC_BMPCBOX_H_


#define wxGENERIC_BITMAPCOMBOBOX     1

#include "wx/odcombo.h"

// ----------------------------------------------------------------------------
// wxBitmapComboBox: a wxComboBox that allows images to be shown
// in front of string items.
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxBitmapComboBox : public wxOwnerDrawnComboBox,
                                         public wxBitmapComboBoxBase
{
public:

    // ctors and such
    wxBitmapComboBox() : wxOwnerDrawnComboBox(), wxBitmapComboBoxBase()
    {
        Init();
    }

    wxBitmapComboBox(wxWindow *parent,
                     wxWindowID id = wxID_ANY,
                     const wxString& value = wxEmptyString,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     int n = 0,
                     const wxString choices[] = NULL,
                     long style = 0,
                     const wxValidator& validator = wxDefaultValidator,
                     const wxString& name = wxASCII_STR(wxBitmapComboBoxNameStr))
        : wxOwnerDrawnComboBox(),
          wxBitmapComboBoxBase()
    {
        Init();

        (void)Create(parent, id, value, pos, size, n,
                     choices, style, validator, name);
    }

    wxBitmapComboBox(wxWindow *parent,
                     wxWindowID id,
                     const wxString& value,
                     const wxPoint& pos,
                     const wxSize& size,
                     const wxArrayString& choices,
                     long style,
                     const wxValidator& validator = wxDefaultValidator,
                     const wxString& name = wxASCII_STR(wxBitmapComboBoxNameStr));

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxString& value,
                const wxPoint& pos,
                const wxSize& size,
                int n,
                const wxString choices[],
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxASCII_STR(wxBitmapComboBoxNameStr));

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxString& value,
                const wxPoint& pos,
                const wxSize& size,
                const wxArrayString& choices,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxASCII_STR(wxBitmapComboBoxNameStr));

    virtual ~wxBitmapComboBox();

    virtual wxString GetStringSelection() const override;

    // Adds item with image to the end of the combo box.
    int Append(const wxString& item, const wxBitmapBundle& bitmap = wxBitmapBundle());
    int Append(const wxString& item, const wxBitmapBundle& bitmap, void *clientData);
    int Append(const wxString& item, const wxBitmapBundle& bitmap, wxClientData *clientData);

    // Inserts item with image into the list before pos. Not valid for wxCB_SORT
    // styles, use Append instead.
    int Insert(const wxString& item, const wxBitmapBundle& bitmap, unsigned int pos);
    int Insert(const wxString& item, const wxBitmapBundle& bitmap,
               unsigned int pos, void *clientData);
    int Insert(const wxString& item, const wxBitmapBundle& bitmap,
               unsigned int pos, wxClientData *clientData);

    // Sets the image for the given item.
    virtual void SetItemBitmap(unsigned int n, const wxBitmapBundle& bitmap) override;
    virtual bool SetFont(const wxFont& font) override;

protected:

    virtual void OnDrawBackground(wxDC& dc, const wxRect& rect, int item, int flags) const override;
    virtual void OnDrawItem(wxDC& dc, const wxRect& rect, int item, int flags) const override;
    virtual wxCoord OnMeasureItem(size_t item) const override;
    virtual wxCoord OnMeasureItemWidth(size_t item) const override;

    // Event handlers
    void OnSize(wxSizeEvent& event);

    virtual wxSize DoGetBestSize() const override;

    virtual wxItemContainer* GetItemContainer() override { return this; }
    virtual wxWindow* GetControl() override { return this; }

    // wxItemContainer implementation
    virtual int DoInsertItems(const wxArrayStringsAdapter & items,
                              unsigned int pos,
                              void **clientData, wxClientDataType type) override;
    virtual void DoClear() override;
    virtual void DoDeleteOneItem(unsigned int n) override;

private:
    bool                m_inResize;

    void Init();

    wxDECLARE_EVENT_TABLE();

    wxDECLARE_DYNAMIC_CLASS(wxBitmapComboBox);
};

#endif // _WX_GENERIC_BMPCBOX_H_
