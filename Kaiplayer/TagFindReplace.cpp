
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

#include "TagFindReplace.h"
#include "TabPanel.h"


bool TagFindReplace::FindTag(const wxString& pattern, const wxString& text, int mode, bool toEndOfSelection)
{
	result = FindData();
	wxString txt = text;
	lastPattern = pattern;
	lastMode = mode;
	regex.Compile(L"^" + pattern, wxRE_ADVANCED);

	if (currentTab) {
		TextEditor* editor = currentTab->Edit->GetEditor();
		if (txt.empty())
			txt = editor->GetValue();

		if (currentTab->Grid->file->SelectionsSize() < 2 && !from) {
			if (mode != 1) { editor->GetSelection(&from, &to); }
			if (mode == 2) {
				wxPoint brackets = FindBrackets(txt, from);
				if (brackets.x != 0) {
					from = to = 0;
				}
			}
			if (mode == 3) {
				from = to;
			}
		}
	}

	bool brkt = true;
	bool inbrkt = true;
	bool fromOriginal = false;

	if (txt.empty()) {
		result.positionInText.x = 0;
		result.positionInText.y = 0;
		result.inBracket = false;
		result.cursorPosition = 0;
		if (toEndOfSelection) { result.hasSelection = false; }
		return false;
	}

	if (toEndOfSelection && from == to) { result.hasSelection = false; }

	wxPoint brackets = FindBrackets(txt, from);
	int bracketStart = brackets.x;
	int bracketEnd = brackets.y;
	// not in {} brackets
	// if bracketStart == bracketEnd + 1 there is between brackets 
	// and it should be treated as result.inBrackets, new brackets needs
	// to be found
	if (bracketStart == -1 || (bracketStart > bracketEnd + 1)) {
		result.inBracket = false;
		inbrkt = false;
		bracketEnd = from;
		brkt = false;
	}//in brackets
	else {
		if (bracketStart == bracketEnd + 1) {
			wxPoint brackets = FindBrackets(txt, bracketStart + 1);
			bracketStart = brackets.x;
			bracketEnd = brackets.y;
		}
		result.inBracket = true;
	}

	result.positionInText.x = bracketEnd;
	result.positionInText.y = bracketEnd;
	if (toEndOfSelection && result.hasSelection) {
		result.cursorPosition = to;
		if (result.inBracket) { result.cursorPosition--; }
	}
	else {
		result.cursorPosition = bracketEnd;
	}
	bool isT = false;
	bool firstT = false;
	bool hasR = false;
	bool placedInT = false;
	// maybe this name is wrong, it's for end posiotion for \t without end bracket
	int endT;
	int lastT = endT = bracketEnd - 1;
	int lslash = bracketEnd + 1;
	int lastTag = -1;
	wxString found[2];
	wxPoint fpoints[2];
	if (bracketEnd == txt.length()) { bracketEnd--; }

	for (int i = bracketEnd; i >= 0; i--) {
		wxUniChar ch = txt[i];
		if (ch == L'\\' && brkt) {
			//tag is result.positionInText on begining of tags in bracket
			if (i >= bracketStart)
				lastTag = i;

			wxString ftag = txt.SubString(i + 1, lslash - 1);
			if (ftag == L"r") {
				hasR = true;
			}
			if (ftag.EndsWith(L")")) {
				//fixes \fn(name)
				if (/*ftag.Find(L'(') == -1 || ftag.Freq(L')') >= 2 && */ftag.Freq(L')') > ftag.Freq(L'(')
					|| ftag.StartsWith(L"t(")) {
					isT = true;
					endT = lslash - 1;
				}
			}
			if (ftag.StartsWith(L"t(")) {
				if (endT == -1)
					endT = lastT;

				if (i <= from && from <= endT) {

					if (!found[1].empty() && fpoints[1].y <= endT) {
						result.positionInText = fpoints[1];
						result.finding = found[1];
						return true;
					}
					else if (!found[0].empty()) {
						if (fpoints[0].y <= endT) { break; }
					}
					else {
						result.positionInText.x = endT;
						result.positionInText.y = result.positionInText.x;
						result.inBracket = true;
						placedInT = true;
						//return false;
					}

				}
				isT = false;
				lslash = i;
				endT = -1;
				// maybe this name is wrong, it's for end posiotion for \t without end bracket
				lastT = i;
				continue;
			}
			//fixes fontnames with (...) on end
			bool isFN = ftag.StartsWith(L"fn");
			int reps = regex.ReplaceAll(&ftag, L"\\1");
			if (reps > 0) {
				//maybe better for fix fn bug would be ftag.Freq(L')') > ftag.Freq(L'(') cause it also can prevent it for another tags
				if (ftag.EndsWith(L")") && !isFN && (!ftag.StartsWith(L"(") || ftag.Freq(L')') >= 2) || ftag.EndsWith(L"}")) {
					ftag.RemoveLast(1);
					lslash--;
				}

				if (found[0] == L"" && !isT) {
					found[0] = ftag;
					fpoints[0].x = (i < lastTag) ? lastTag : i;
					fpoints[0].y = (i < lastTag) ? lastTag : lslash - 1;
				}
				else {
					found[1] = ftag;
					fpoints[1].x = i;
					fpoints[1].y = (isT && txt[lslash - 1] == L')') ? lslash - 2 : lslash - 1;
				}
				//block break till i <= from cause of test if cursor is in \t tag
				//else it will fail if there is value without \t on the end
				if (!isT && found[0] != L"" && i <= from) {
					break;
				}
			}

			lslash = i;
		}
		else if (ch == L'{' && i > 0) {
			wxString textBeforeBracket = txt.Mid(0, i);
			int startBracket = textBeforeBracket.Find(L'{', true);
			int endBracket = textBeforeBracket.Find(L'}', true);
			if (endBracket >= startBracket) {
				brkt = false;
				if (txt[i - 1] != L'}') {
					inbrkt = false;
					if (hasR) { break; }
				}
			}
			else {
				lslash = i - 1;
			}
		}
		else if (ch == L'}' && i > 0) {
			wxString textBeforeBracket = txt.Mid(0, i);
			int startBracket = textBeforeBracket.Find(L'{', true);
			int endBracket = textBeforeBracket.Find(L'}', true);
			if (endBracket < startBracket) {
				lslash = i;
				brkt = true;
			}
		}

	}

	if (!isT && found[0] != L"") {
		//In bracket here blocks changing position of tag putting in plain text
		//inbrkt here changing value when plain text is on start, not use it here
		if (result.inBracket && !placedInT) {
			result.positionInText = fpoints[0];
		}
		result.finding = found[0];
		return true;
	}
	else if (lastTag >= 0 && result.inBracket && !placedInT) {
		result.positionInText.x = lastTag;
		result.positionInText.y = lastTag;
	}

	return false;
}

//void TagFindReplace::FindAllTags(const wxString& pattern, const wxString& text, std::function<void(const FindData&)> func, bool returnPosWhenNoTags)
//{
//	regex.Compile(L"^" + pattern, wxRE_ADVANCED);
//
//	int pos = result.positionInText.x + result.positionInText.y;
//	size_t startMatch = 0, lenMatch = 0;
//	size_t textPosition = 0;
//	wxString tmp;
//	bool findMatch = false;
//	while (regex.Matches(text.Mid(textPosition))) {
//		regex.GetMatch(&startMatch, &lenMatch, 1);
//		int position = textPosition + startMatch;
//		tmp = text.Mid(position, lenMatch);
//		FindData res(tmp, wxPoint(position, lenMatch), true, false);
//		func(res);
//		textPosition += startMatch + lenMatch;
//		findMatch = true;
//	}
//	if (returnPosWhenNoTags && !findMatch) {
//		int pos = (text.StartsWith("{")) ? 1 : 0;
//		FindData res(L"", wxPoint(pos, pos), false, false);
//		func(res);
//	}
//}


int TagFindReplace::ReplaceAll(const wxString& pattern, const wxString& tag, wxString * text, std::function< void(const FindData&, wxString*)> func, bool returnPosWhenNoTags)
{
	int replaces = 0;
	regex.Compile(L"\\\\" + pattern, wxRE_ADVANCED);

	int pos = result.positionInText.x + result.positionInText.y;
	size_t startMatch = 0, lenMatch = 0;
	size_t textPosition = 0;
	wxString tmp;
	bool needFirstReplace = true;
	wxPoint tpos = FindBrackets(*text, 0);

	while (regex.Matches(text->Mid(textPosition))) {
		wxString changedValue;
		regex.GetMatch(&startMatch, &lenMatch, 1);
		int position = textPosition + startMatch;
		tmp = text->Mid(position, lenMatch);
		FindData res(tmp, wxPoint(position, lenMatch), true, false);
		func(res, &changedValue);
		if (tpos.x <= position && position <= tpos.y) {
			needFirstReplace = false;
		}
		if (lenMatch) { text->erase(text->begin() + position, text->begin() + position + lenMatch); }
		if (!changedValue.empty()) {
			text->insert(position, changedValue);
			lenMatch = changedValue.length();
		}
		else
			lenMatch = 0;

		textPosition += startMatch + lenMatch;
		replaces++;
	}
	if (returnPosWhenNoTags && needFirstReplace) {
		int pos = (text->StartsWith("{")) ? 1 : 0;
		FindData res(L"", wxPoint(pos, pos), pos == 1, false);
		wxString changedValue;
		func(res, &changedValue);
		changedValue.Prepend(L"\\" + tag);
		if (!res.inBracket) {
			changedValue = L"{" + changedValue + L"}";
		}
		if (!changedValue.empty()) {
			text->insert(pos, changedValue);
		}
	}
	return replaces;
}

int TagFindReplace::Replace(const wxString& replaceTxt, wxString* text)
{
	const wxPoint& pos = result.positionInText;
	if (!result.inBracket) {
		text->insert(pos.x, L"{" + replaceTxt + L"}");
		return 1;
	}

	if (pos.x < pos.y) {
		if (pos.y + 1 >= text->length())
			text->erase(text->begin() + pos.x, text->end());
		else
			text->erase(text->begin() + pos.x, text->begin() + pos.y + 1);
	}
	text->insert(pos.x, replaceTxt);
	return 0;
}

//int TagFindReplace::ReplaceFromFindData(const wxString& replaceTxt, const FindData& data)
//{
//	
//	return 0;
//}

bool TagFindReplace::GetDouble(double* retval)
{
	if (result.finding.ToCDouble(retval)) {
		return true;
	}
	return false;
}

bool TagFindReplace::GetTwoValueInt(int* retval, int* retval2)
{
	wxString valtext = result.finding;
	bool bracketS = valtext.StartsWith(L"(");
	bool bracketE = valtext.EndsWith(L")");
	if (bracketS || bracketE) {
		valtext = valtext.Mid(bracketS ? 1 : 0, bracketE ? valtext.length() - 2 : -1);
	}
	double firstval = 0, secondval = 0;
	wxString sval;
	wxString fval = result.finding.BeforeFirst(L',', &sval);
	if (fval.ToCDouble(&firstval) && sval.ToCDouble(&secondval)) {
		*retval = (int)firstval;
		*retval2 = (int)secondval;
		return true;
	}
	return false;
}

bool TagFindReplace::GetTwoValueDouble(double* retval, double* retval2)
{
	wxString valtext = result.finding;
	bool bracketS = valtext.StartsWith(L"(");
	bool bracketE = valtext.EndsWith(L")");
	if (bracketS || bracketE) {
		valtext = valtext.Mid(bracketS ? 1 : 0, bracketE ? valtext.length() - 2 : -1);
	}
	wxString sval;
	wxString fval = result.finding.BeforeFirst(L',', &sval);
	if (fval.ToCDouble(retval) && sval.ToCDouble(retval2)) {
		return true;
	}
	return false;
}

bool TagFindReplace::GetInt(int* retval) 
{
	bool hasZero = result.finding.StartsWith(L"0");
	*retval = wxAtoi(result.finding);
	if (*retval == 0 && !hasZero)
		return false;

	return true;
}

//function return 1 when need to add bracket or 0
int TagFindReplace::ChangeText(wxString* txt, const wxString& what, bool inbracket, const wxPoint& pos)
{
	//use only if needed if pos.x == 0 and length == 0 than block 
	//puting tags for drawing
	/*if (pos.x > txt->length()) {
		return 0;
	}*/
	if (!inbracket) {
		txt->insert(pos.x, L"{" + what + L"}");
		return 1;
	}

	if (pos.x < pos.y) {
		if (pos.y + 1 >= txt->length())
			txt->erase(txt->begin() + pos.x, txt->end());
		else
			txt->erase(txt->begin() + pos.x, txt->begin() + pos.y + 1);
	}
	txt->insert(pos.x, what);
	return 0;
}

bool TagFindReplace::GetTextResult(wxString* rettext)
{
	if (result.finding.empty())
		return false;

	*rettext = result.finding;
	return true;
}

wxPoint TagFindReplace::GetPositionInText()
{
	return result.positionInText;
}

void TagFindReplace::SetPositionInText(const wxPoint& pos)
{
	result.positionInText = pos;
}

void TagFindReplace::PutTagInText(const wxString& tag, const wxString& resettag, bool focus)
{
	if (!currentTab)
		return;

	SubsGrid* grid = currentTab->Grid;
	EditBox* edit = currentTab->Edit;

	if (grid->file->SelectionsSize() < 2) {
		long whre;
		wxString txt = edit->TextEdit->GetValue();
		TextEditor* editor = edit->TextEdit;
		if (grid->hasTLMode && txt == L"") {
			txt = edit->TextEditOrig->GetValue();
			editor = edit->TextEditOrig;
		}
		if (!result.inBracket) {
			txt.insert(result.positionInText.x, L"{" + tag + L"}");
			whre = result.cursorPosition + tag.length() + 2;
		}
		else {
			if (result.positionInText.x < result.positionInText.y) {
				txt.erase(txt.begin() + result.positionInText.x, 
					txt.begin() + result.positionInText.y + 1);
				whre = (focus) ? result.cursorPosition + tag.length() -
					(result.positionInText.y - result.positionInText.x) : result.positionInText.x;
			}
			else { 
				whre = (focus) ? result.cursorPosition + 1 + tag.length() :
					result.positionInText.x; 
			}
			txt.insert(result.positionInText.x, tag);
		}
		if (tag == L"") { txt.Replace(L"{}", L""); }
		editor->SetTextS(txt, true);
		if (result.hasSelection && !resettag.empty() && lastMode == 0 && from != to) {
			FindTag(lastPattern, L"", 3);
			PutTagInText(resettag, L"", focus);
			return;
		}

		editor->SetSelection(whre, whre);
		if (focus) { editor->SetFocus(); }
	}
	else {
		wxArrayInt sels;
		grid->file->GetSelections(sels);
		for (size_t i = 0; i < sels.size(); i++) {
			Dialogue* dialc = grid->CopyDialogue(sels[i]);
			wxString txt = dialc->GetTextNoCopy();
			FindTag(lastPattern, txt);

			if (result.inBracket && txt != L"") {
				if (result.positionInText.x < result.positionInText.y) { 
					txt.erase(txt.begin() + result.positionInText.x, 
						txt.begin() + result.positionInText.y + 1);
				}
				txt.insert(result.positionInText.x, tag);
				dialc->SetText(txt);
			}
			else {
				if (grid->hasTLMode && dialc->TextTl != L"") {
					dialc->TextTl->Prepend(L"{" + tag + L"}");
				}
				else { dialc->Text->Prepend(L"{" + tag + L"}"); }
			}
		}
		grid->SetModified(EDITBOX_MULTILINE_EDITION);
		grid->Refresh(false);
	}

}

wxPoint FindBrackets(const wxString& text, long from)
{
	bool haveStartBracket = false;
	bool haveEndBracket = false;
	int endBrakcetPos = -1;
	int startBrakcetPos = -1;
	size_t len = text.length();
	size_t i = (from - 1 < 1) ? 1 : (from - 1);
	for (; i < len; i++) {
		const wxUniChar& ch = text[i];
		if (ch == L'}') {
			haveEndBracket = true;
			endBrakcetPos = i;
		}
		else if (ch == L'{' && i + 1 > from && i != from) {
			if (!haveEndBracket)
				break;
			else
				haveEndBracket = false;
		}
		else if (haveEndBracket) {
			break;
		}
	}
	size_t k = from < len ? from : len - 1;

	for (; k + 1 > 0; k--) {
		const wxUniChar& ch = text[k];
		if (ch == L'{') {
			haveStartBracket = true;
			startBrakcetPos = k;
		}
		else if (ch == L'}' && k + 1 < from) {
			if (!haveStartBracket)
				break;
			else
				haveStartBracket = false;
		}
		else if (haveStartBracket) {
			break;
		}

	}
	// no end bracket after block ...}{...cursor}
	// no need to correct it, end bracket without start is displayed as text

	// no first bracket after block {cursor...}{...
	if (haveEndBracket && i >= len && endBrakcetPos + 1 < len)
		endBrakcetPos = len - 1;
	// no end bracket
	if (startBrakcetPos != -1 && endBrakcetPos == -1)
		endBrakcetPos = len - 1;
	// no start bracket
	if (startBrakcetPos == -1 && endBrakcetPos != -1)
		endBrakcetPos = -1;

	return wxPoint(startBrakcetPos, endBrakcetPos);
}