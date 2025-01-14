//  Copyright (c) 2016-2020, Marcin Drob

//  Kainote is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.

//  Kainote is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.

//  You should have received a copy of the GNU General Public License
//  along with Kainote.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include "wx/tglbtn.h"
#include "Hotkeys.h"

enum{
	MAKE_SQUARE_BUTTON = 0x200
};

class MappedButton :public wxWindow
{
public:
	MappedButton(wxWindow *parent, int id, const wxString& label, int window = -1,
		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0);
	MappedButton(wxWindow *parent, int id, const wxString& label, const wxString& tooltip,
		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, int window = EDITBOX_HOTKEY, long style = 0);
	MappedButton(wxWindow *parent, int id, const wxString& tooltip, const wxBitmap& bitmap, const wxPoint& pos,
		const wxSize& size = wxDefaultSize, int window = AUDIO_HOTKEY, long style = 0, const wxString &text = emptyString);
	void SetTwoHotkeys(){ twoHotkeys = true; }
	virtual ~MappedButton();
	void SetToolTip(const wxString &toolTip = emptyString);
	void SetBitmap(const wxBitmap & bitmap){ icon = bitmap; Refresh(false); };
	bool SetBackgroundColour(const wxColour color){
		isColorButton = true;
		buttonColor = color;
		Refresh(false);
		return true;
	}
	wxColour GetBackgroundColour(){ return buttonColor; }
	bool SetForegroundColour(const wxColour &fgcolor){
		changedForeground = true;
		wxWindow::SetForegroundColour(fgcolor);
		return true;
	}
	bool changedForeground;
	wxString GetLabelText() const { return name; }
	void SetLabelText(const wxString &label);
	bool Enable(bool enable = true);
	bool clicked;
	bool SetFont(const wxFont &font);
private:
	void OnSize(wxSizeEvent& evt);
	void OnPaint(wxPaintEvent& evt);
	void OnMouseEvent(wxMouseEvent &evt);
	void SendEvent();
	void CalculateSize(int *w, int *h);
	void PaintGDI(wxDC &dc, int w, int h);
	int Window;
	int textHeight = 0;
	bool twoHotkeys;
	bool enter;
	bool isColorButton;
	wxColour buttonColor;
	wxBitmap *bmp;
	wxBitmap icon;
	wxString name;
	wxDECLARE_ABSTRACT_CLASS(MappedButton);
};

class ToggleButton :public wxWindow
{
public:
	ToggleButton(wxWindow *parent, int id, const wxString& label, const wxString& tooltip = wxEmptyString,
		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0);
	~ToggleButton(){ if (bmp){ delete bmp; } }
	void SetBitmap(const wxBitmap &bmp){ icon = bmp; Refresh(false); }
	bool GetValue(){ return toggled; }
	void SetValue(bool toggle){ toggled = toggle; Refresh(false); }
	bool SetForegroundColour(const wxColour &fgcolor){
		changedForeground = true;
		wxWindow::SetForegroundColour(fgcolor); return true;
	}
	// this don't layout do it after changing font
	bool SetFont(const wxFont &font);
	bool changedForeground;
	bool clicked;
	bool Enable(bool enable = true);
private:
	void OnSize(wxSizeEvent& evt);
	void OnPaint(wxPaintEvent& evt);
	void OnMouseEvent(wxMouseEvent &evt);
	void SendEvent();
	void CalculateSize(int *w, int *h);
	void PaintGDI(wxDC &dc, int w, int h);
	wxBitmap *bmp;
	wxBitmap icon;
	wxString name;
	bool enter;
	bool toggled;
	int textHeight = 0;
};
