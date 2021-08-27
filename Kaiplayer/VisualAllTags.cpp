//  Copyright (c) 2021, Marcin Drob

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

#include "Visuals.h"
#include "TabPanel.h"
#include "VideoToolbar.h"
#include "Hotkeys.h"

AllTags::AllTags()
{
	replaceTagsInCursorPosition = false;
	for (size_t i = 0; i < 4; i++) {
		slider[i].SetAllTags(this);
	}
	SetupSlidersPosition();
	tags = VideoToolbar::GetTagsSettings();
	if (!tags->size()) {
		LoadSettings(tags);
	}
}

void AllTags::DrawVisual(int time)
{
	for (size_t i = 0; i < actualTag.numOfValues; i++) {
		slider[i].OnDraw();
	}
}

void AllTags::OnMouseEvent(wxMouseEvent& event)
{
	if (mode >= MULTIPLY)
		multiplyCounter = 0;

	float x = event.GetX();
	float y = event.GetY();
	//have to write shift working on two or more sliders
	bool shift = event.ShiftDown();

	// right holding
	// move sliders
	if (rholding) {
		SetupSlidersPosition(y + sliderPositionDiff);
		tab->Video->Render(false);
		for (size_t i = 0; i < 4; i++) {
			slider[i].ResetOnThumbAndSlider();
		}
		if (!event.RightUp()) {
			return;
		}
	}

	if (event.RightDown() || event.RightDClick()) {
		if (!tab->Video->HasCapture()) {
			tab->Video->CaptureMouse();
		}
		sliderPositionDiff = sliderPositionY - y;
		rholding = true;
	}

	if (event.RightUp()) {
		if (tab->Video->HasCapture()) {
			tab->Video->ReleaseMouse();
		}
		rholding = false;
	}

	
}

void AllTags::OnKeyPress(wxKeyEvent& evt)
{
	if (actualTag.tag != L"fad" || tagMode & IS_T_ANIMATION)
		return;

	int key = evt.GetKeyCode();
	auto accel = Hkeys.GetHKey(idAndType(EDITBOX_START_DIFFERENCE, EDITBOX_HOTKEY));
	auto accel1 = Hkeys.GetHKey(idAndType(EDITBOX_END_DIFFERENCE, EDITBOX_HOTKEY));
	bool hkeystart = evt.GetModifiers() == accel.GetFlags() && key == accel.GetKeyCode();
	bool hkeyend = evt.GetModifiers() == accel1.GetFlags() && key == accel1.GetKeyCode();
	if (hkeystart || hkeyend) {
		if (tab->Video->GetState() == None) { 
			wxBell(); return; 
		}
		int vidtime = tab->Video->Tell();
		if (vidtime < tab->Edit->line->Start.mstime ||
			vidtime > tab->Edit->line->End.mstime) {
			wxBell(); return;
		}
		int diff = (hkeystart || tagMode & IS_T_ANIMATION) ?
			vidtime - ZEROIT(tab->Edit->line->Start.mstime) :
			abs(vidtime - ZEROIT(tab->Edit->line->End.mstime));
		if (hkeystart) {
			slider[0].SetThumbValue(diff);
		}
		else {
			slider[1].SetThumbValue(diff);
		}
		slider[hkeystart? 0 : 1].SetHolding(true);
		if (tab->Edit->IsCursorOnStart()) {
			SetVisual(false);
		}
		else {
			SetVisual(true);
			SetVisual(false);
		}
		slider[hkeystart ? 0 : 1].SetHolding(false);
	}
}

void AllTags::CheckTag()
{
	if (actualTag.tag == L"1a" || actualTag.tag == L"2a" ||
		actualTag.tag == L"3a" || actualTag.tag == L"4a" ||
		actualTag.tag == L"alpha") 
	{
		tagMode = IS_HEX_ALPHA;
	}
	else if (actualTag.tag == L"1c" || actualTag.tag == L"2c" ||
		actualTag.tag == L"3c" || actualTag.tag == L"4c" ||
		actualTag.tag == L"c")
	{
		tagMode = IS_HEX_COLOR;
	}
	else if (actualTag.tag == L"p" || actualTag.tag == L"clip" ||
		actualTag.tag == L"iclip")
	{
		tagMode = IS_VECTOR;
	}
	else if (actualTag.tag == L"t") {
		tagMode = IS_T_ANIMATION;
	}
	else {
		tagMode = 0;
	}
}

void AllTags::SetupSlidersPosition(int _sliderPositionY)
{
	sliderPositionY = _sliderPositionY;
	float left = 20;
	float right = VideoSize.width - 40;
	float bottom = sliderPositionY;
	float top = sliderPositionY - 8;
	for (size_t i = 0; i < 4; i++) {
		slider[i].SetPosition(left, top, right, bottom);
		top += increase;
		bottom += increase;
	}
}

void AllTags::SetCurVisual()
{
	
	if (currentTag < 0 || currentTag >= tags->size())
		currentTag = 0;
	actualTag = (*tags)[currentTag];
	floatFormat = wxString::Format(L"5.%if", actualTag.digitsAfterDot);
	if (mode >= MULTIPLY) {
		for (size_t i = 0; i < actualTag.numOfValues; i++) {
			slider[0].SetFirstThumbValue(actualTag.values[i]);
		}
	}
	CheckTag();
	FindTagValues();
	if (mode < MULTIPLY) {
		for (size_t i = 0; i < actualTag.numOfValues; i++) {
			slider[0].SetThumbValue(actualTag.values[i]);
		}
	}
	tab->Video->Render(false);
}

void AllTags::FindTagValues()
{
	Styles* currentStyle = tab->Grid->GetStyle(0, tab->Edit->line->Style);
	wxString value;
	TagValueFromStyle(currentStyle, actualTag.tag, &value);
	double doubleValue = 0.;
	if (!value.ToDouble(&doubleValue))
		doubleValue = wxAtoi(value);
	
	if (FindTag(actualTag.tag + L"([-0-9.,\\(\\) ]+)", L"", actualTag.mode)) {
		const FindData& data = GetResult();
		if (data.finding.StartsWith(L"(")) {
			//remove brackets;
			wxStringTokenizer toknzr(data.finding.Mid(1, data.finding.length() - 2), L",", wxTOKEN_STRTOK);
			int i = 0;
			while (toknzr.HasMoreTokens())
			{
				wxString token = toknzr.GetNextToken().Trim(false).Trim();
				double val = 0;
				if (token.ToCDouble(&val)) {
					actualTag.values[i] = val;
					if (mode < MULTIPLY)
						CheckRange(val);
				}
				i++;
				if (i >= (actualTag.numOfValues))
					break;
			}

		}
		else {
			double val = 0;
			if (tagMode & IS_HEX_ALPHA) {
				AssColor col;
				col.SetAlphaString(data.finding);
				actualTag.values[0] = col.a;
			}
			else if (tagMode & IS_HEX_COLOR) {
				AssColor col(data.finding);
				actualTag.values[0] = col.r;
				actualTag.values[1] = col.g;
				actualTag.values[2] = col.b;
			}
			else if (data.finding.ToCDouble(&val)) {
				actualTag.values[0] = val;
			}
			else
				return;

			if (mode < MULTIPLY)
				CheckRange(val);
		}
	}
}

void AllTags::ChangeTool(int _tool, bool blockSetCurVisual)
{
	if (lastTool == _tool)
		return;

	mode = _tool >> 20;
	replaceTagsInCursorPosition = mode == INSERT;

	int curtag = _tool << 12;
	currentTag = curtag >> 12;
	if(!blockSetCurVisual)
		SetCurVisual();
}

void AllTags::GetVisualValue(wxString* visual, const wxString& curValue)
{
	float value = slider[0].GetThumbValue();
	float value2 = slider[1].GetThumbValue();
	float value3 = slider[2].GetThumbValue();
	float value4 = slider[3].GetThumbValue();
	float valuediff = slider[0].GetDiffValue();
	float valuediff2 = slider[1].GetDiffValue();
	float valuediff3 = slider[2].GetDiffValue();
	float valuediff4 = slider[3].GetDiffValue();
	wxString strval;
	if (curValue.empty() || mode == MULTIPLY) {
		//mode 2 for multiply
		//mode 1 for paste only one value
		float val1 = (mode >= MULTIPLY) ?
			actualTag.values[0] + (multiplyCounter * valuediff) : value;
		float val2 = (mode >= MULTIPLY) ?
			actualTag.values[1] + (multiplyCounter * valuediff2) : value2;
		float val3 = (mode >= MULTIPLY) ?
			actualTag.values[2] + (multiplyCounter * valuediff3) : value3;

		if (tagMode & IS_HEX_ALPHA) {
			strval = wxString::Format(L"&H%02X&", MID(0, val1, 255));
		}
		else if (tagMode & IS_HEX_COLOR) {
			strval = wxString::Format(L"&H%02X%02X%02X&",
				MID(0, val1, 255),
				MID(0, val2, 255),
				MID(0, val3, 255));
		}
		else if (actualTag.numOfValues > 1) {
			float val4 = (mode >= MULTIPLY) ?
				actualTag.values[3] + (multiplyCounter * valuediff4) : value4;
			strval = L"(" + getfloat(val1, floatFormat) + L"," +
				getfloat(val2, floatFormat);
			if (actualTag.numOfValues >= 3) {
				strval << L"," << getfloat(val3, floatFormat);
			}
			if (actualTag.numOfValues == 4) {
				strval << L"," << getfloat(val4, floatFormat);
			}

			strval << L")";
		}
		else
			strval = getfloat(val1, floatFormat);

		if (curValue.EndsWith(")")) {
			strval << ")";
		}
	}
	else if (curValue.StartsWith(L"(")) {
		bool hasLastBracket = curValue.EndsWith(L")");
		//remove brackets;
		wxStringTokenizer toknzr(curValue.Mid(1, hasLastBracket? curValue.length() - 2 : 
			curValue.length() - 2), L",", wxTOKEN_STRTOK);
		strval = L"(";
		int counter = 0;
		while (toknzr.HasMoreTokens())
		{
			wxString token = toknzr.GetNextToken().Trim(false).Trim();
			double val = 0;
			if (token.ToCDouble(&val)) {
				float valdiff = (counter % 2 == 0)? valuediff : 
					(counter % 2 == 1) ? valuediff2 :
					(counter % 2 == 2) ? valuediff3 : valuediff4;
				if (mode > MULTIPLY) {
					valdiff *= multiplyCounter;
				}
				strval << getfloat(val + valdiff, floatFormat) << L",";
			}
			counter++;
		}
		if (strval.EndsWith(L","))
			strval = strval.Mid(0, strval.length() - 1);
		if(hasLastBracket)
			strval << L")";
	}
	else {
		double val = 0;
		wxString trimed = curValue;
		trimed.Trim(false).Trim();
		//bug when tag is in \t
		bool hasEndBracked = false;
		if (trimed.EndsWith(")")) {
			trimed = trimed.Mid(0, trimed.length() - 1);
			hasEndBracked = true;
		}
		if (tagMode & IS_HEX_ALPHA) {
			AssColor col;
			col.SetAlphaString(trimed);
			float vala = (mode > MULTIPLY)? col.a + (valuediff * multiplyCounter) :
				col.a + valuediff;
			strval = wxString::Format(L"&H%02X&", MID(0, vala, 255));
		}
		else if (tagMode & IS_HEX_COLOR) {
			AssColor col(trimed);
			float valr = (mode > MULTIPLY) ? col.r + (valuediff * multiplyCounter) :
				col.r + valuediff;
			float valg = (mode > MULTIPLY) ? col.g + (valuediff * multiplyCounter) :
				col.g + valuediff2;
			float valb = (mode > MULTIPLY) ? col.b + (valuediff * multiplyCounter) :
				col.b + valuediff3;
			strval = wxString::Format(L"&H%02X%02X%02X&",
				MID(0, valb, 255),
				MID(0, valg, 255),
				MID(0, valr, 255));
		}
		else if (trimed.ToCDouble(&val)) {
			val += (mode > MULTIPLY) ? (valuediff * multiplyCounter) : valuediff;
			strval = getfloat(val, floatFormat);
		}
		else//dont add a bracket when value not set
			hasEndBracked = false;

		if (hasEndBracked)
			strval << L")";
	}
	
	*visual = strval;
}

wxPoint AllTags::ChangeVisual(wxString* txt)
{
	if (mode == 4) {
		auto replfunc = [=](const FindData& data, wxString* result, size_t numOfCharacters) {
			GetVisualValue(result, data.finding);
			multiplyCounter += (1.f / (numOfCharacters - 1));
		};
		ReplaceAllByChar(actualTag.tag + L"([-0-9.,\\(\\) ]+)", actualTag.tag, txt, replfunc);
	}
	else if (mode) {
		FindTag(actualTag.tag + L"([-0-9.,\\(\\) ]+)", *txt, actualTag.mode);
		wxString strValue, strFinding;
		GetTextResult(&strFinding);
		GetVisualValue(&strValue, strFinding);
		Replace(L"\\" + actualTag.tag + strValue, txt);
		//if there is one line there's no need to count it
	}
	else {
		auto replfunc = [=](const FindData& data, wxString* result) {
			GetVisualValue(result, data.finding);
		};
		ReplaceAll(actualTag.tag + L"([-0-9.,\\(\\) ]+)", actualTag.tag, txt, replfunc, true);
		FindTag(actualTag.tag + L"([-0-9.,\\(\\) ]+)", *txt);
	}
	return GetPositionInText();
}

void AllTags::ChangeVisual(wxString* txt, Dialogue *dial, size_t numOfSelections)
{
	if (mode == GRADIENT_TEXT) {
		auto replfunc = [=](const FindData& data, wxString* result, size_t numOfCharacters) {
			GetVisualValue(result, data.finding);
			multiplyCounter += (1.f / (numOfCharacters - 1));
		};
		ReplaceAllByChar(actualTag.tag + L"([-0-9.,\\(\\) ]+)", actualTag.tag, txt, replfunc);
	}
	else if (mode) {
		FindTag(actualTag.tag + L"([-0-9.,\\(\\) ]+)", *txt, 1);
		wxString strValue, strFinding;
		GetTextResult(&strFinding);
		GetVisualValue(&strValue, strFinding);
		Replace(L"\\" + actualTag.tag + strValue, txt);
		if (mode >= MULTIPLY)
			multiplyCounter++;
		else if (mode >= MULTIPLY_PLUS) {
			multiplyCounter += (1.f / (numOfSelections - 1));
		}
	}
	else {
		auto replfunc = [=](const FindData& data, wxString* result) {
			GetVisualValue(result, data.finding);
		};
		ReplaceAll(actualTag.tag + L"([-0-9.,\\(\\) ]+)", actualTag.tag, txt, replfunc, true);
	}
}

void AllTags::CheckRange(float val)
{
	if (val < actualTag.rangeMin)
		actualTag.rangeMin = val;
	if (val > actualTag.rangeMax)
		actualTag.rangeMax = val;
}

void AllTags::OnMouseCaptureLost(wxMouseCaptureLostEvent& evt)
{
	rholding = false;
	for (size_t i = 0; i < 4; i++) {
		slider[i].SetHolding(false);
	}
}

