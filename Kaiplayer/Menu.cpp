
#include "Menu.h"
#include "Config.h"
#include "Hotkeys.h"

static wxFont font = wxFont(10,wxSWISS,wxFONTSTYLE_NORMAL,wxNORMAL,false,"Tahoma");
const static int height = 22;
const static int maxVisible = 30;

wxDEFINE_EVENT(EVT_MENU_OPENED, MenuEvent);

MenuItem::MenuItem(int _id, const wxString& _label, const wxString& _help, bool _enable, wxBitmap *_icon, Menu *Submenu, byte _type)
{
	icon=_icon; 
	label=_label; 
	id=_id; 
	enabled=_enable;
	type=_type;
	submenu=Submenu; 
	check=false;
	help = _help;
	accel=NULL;
}
MenuItem::~MenuItem()
{
	if(submenu){delete submenu;}
	if(icon){delete icon;}
}
bool MenuItem::Enable(bool enable)
{
	if(enabled!=enable){enabled=enable;return true;}
	return false;
}
wxBitmap MenuItem::GetBitmap()
{
	if(!icon){return wxBitmap();}
	if(!enabled){
		return wxBitmap(icon->ConvertToImage().ConvertToGreyscale());
	}
	return *icon;
}

void MenuItem::SetAccel(wxAcceleratorEntry *entry)
{
	accel = entry;
	if(label.find("\t")!=-1){label = label.BeforeFirst('\t');}
	label += "\t" + entry->ToString();
	//if(MenuBar::Menubar){MenuBar::Menubar->SetAccelerators();}
}

Menu::Menu()
	:dialog(NULL)
	,parentMenu(NULL)
{
}


int Menu::GetPopupMenuSelection(const wxPoint &pos, wxWindow *parent, int *accels, bool clientPos)
{
	wxPoint npos= pos;
	wxSize size;
	CalcPosAndSize(parent, &npos, &size, clientPos);
	//wxLogStatus("Pos & size %i, %i %i %i %i %i", npos.x, npos.y, size.x, size.y, pos.x, pos.y);
	//if(dialog){dialog->Destroy();}
	dialog = new MenuDialog(this, parent, npos, size, false);
	int ret = dialog->ShowPartialModal();
	if(accels){*accels = dialog->accel;}
	dialog->Destroy();
	dialog = NULL;
	return ret;
}

void Menu::PopupMenu(const wxPoint &pos, wxWindow *parent, bool clientPos)
{
	wxPoint npos= pos;
	wxSize size;
	CalcPosAndSize(parent, &npos, &size, clientPos);
	//wxLogStatus("Pos & size %i, %i %i %i %i %i", npos.x, npos.y, size.x, size.y, pos.x, pos.y);
	
	dialog = new MenuDialog(this, parent, npos, size);

	//dialog->Show();
	
	dialog->ShowWithEffect(wxSHOW_EFFECT_BLEND ,1);//wxSHOW_EFFECT_BLEND
	//if((size_t)maxVisible < items.size()){dialog->SetFocus();}
	dialog->Refresh(false);
	//MenuDialog::blockHideDialog=false;
}

void Menu::CalcPosAndSize(wxWindow *parent, wxPoint *pos, wxSize *size, bool clientPos)
{
	int tx=0, ty=0;
	if(clientPos){*pos = parent->ClientToScreen(*pos);}
	size_t isize = items.size();
	for(size_t i = 0; i < isize; i++){
		parent->GetTextExtent(items[i]->label, &tx, &ty, 0, 0, &font);
		if(tx > size->x){size->x = tx;}
	}
	
	size->x += 54;
	if(isize > maxVisible) {size->x += 20; isize=maxVisible;}
	size->y = height * isize + 4;
	int w,h;
	wxRect workArea = wxGetClientDisplayRect();
	w = workArea.width - workArea.x;
	h = workArea.height - workArea.y;
	if(size->y > h){ size->y = h; }
	if((pos->x + size->x) > w){
		pos->x -= size->x;
		//wxLogStatus("dialog %i %i", (int)parentMenu, (parentMenu)? (int)parentMenu->dialog : 0);
		if(parentMenu && parentMenu->dialog){
			wxSize size = parentMenu->dialog->GetClientSize();
			//wxLogStatus("posx %i", pos->x);
			pos->x -= size.x;
			//wxLogStatus("posx1 %i", pos->x);
		}
	}
	if((pos->y + size->y) > h){
		if(size->y > h/2){pos->y -= size->y;if(pos->y<0){pos->y=0;}}
		else{pos->y = h - size->y;}
	}
	
}

MenuItem *Menu::Append(int _id, const wxString& _label, const wxString& _help, bool _enable, wxBitmap *_icon, Menu* Submenu, byte _type)
{
	if(Submenu){Submenu->parentMenu=this;}
	MenuItem *item = new MenuItem(_id, _label, _help, _enable, _icon, Submenu, _type);
	items.push_back(item);
	return item;
}
MenuItem *Menu::Append(int _id,const wxString& _label, Menu* Submenu, const wxString& _help, byte _type, bool _enable, wxBitmap *_icon)
{
	if(Submenu){Submenu->parentMenu=this;}
	MenuItem *item = new MenuItem(_id, _label, _help, _enable, _icon, Submenu, _type);
	items.push_back(item);
	return item;
}
	
MenuItem *Menu::Append(MenuItem *item)
{
	if(item->submenu){item->submenu->parentMenu=this;}
	items.push_back(item);
	return item;
}
	
MenuItem *Menu::Prepend(int _id, const wxString& _label, const wxString& _help, bool _enable, wxBitmap *_icon, Menu* Submenu, byte _type)
{
	if(Submenu){Submenu->parentMenu=this;}
	MenuItem *item = new MenuItem(_id, _label, _help, _enable, _icon, Submenu, _type);
	items.insert(items.begin(),item);
	return item;
}
	
MenuItem *Menu::Prepend(MenuItem *item)
{
	if(item->submenu){item->submenu->parentMenu=this;}
	items.insert(items.begin(),item);
	return item;
}
	
MenuItem *Menu::Insert(int position, int _id, const wxString& _label, const wxString& _help, bool _enable, wxBitmap *_icon, Menu* Submenu, byte _type)
{
	if(Submenu){Submenu->parentMenu=this;}
	MenuItem *item = new MenuItem(_id, _label, _help, _enable, _icon, Submenu, _type);
	items.insert(items.begin() + position,item);
	return item;
}
	
MenuItem *Menu::Insert(int position, MenuItem *item)
{
	if(item->submenu){item->submenu->parentMenu=this;}
	items.insert(items.begin() + position,item);
	return item;
}
	
bool Menu::Destroy(MenuItem *item)
{
	for(size_t i = 0; i < items.size(); i++ ){
		if(items[i] == item){
			delete item;
			items.erase(items.begin()+i);
			return true;
		}else if(items[i]->submenu){
			if(items[i]->submenu->Destroy(item)){return true;}
		}

	}
	delete item;
	return false;
}

void Menu::Delete(int position)
{
	delete items[position];
	items.erase(items.begin()+position);
}
	
int Menu::GetMenuItemCount()
{
	return items.size();
}
	
MenuItem *Menu::FindItem(int id)
{
	for(size_t i = 0; i < items.size(); i++ ){
		if(items[i]->id==id){
			return items[i];
		}else if(items[i]->submenu){
			MenuItem * item = items[i]->submenu->FindItem(id);
			if(item){return item;}
		}

	}
	return NULL;
}
	
MenuItem *Menu::FindItem(const wxString& label)
{
	for(size_t i = 0; i < items.size(); i++ ){
		if(items[i]->label==label){
			return items[i];
		}else if(items[i]->submenu){
			MenuItem * item = items[i]->submenu->FindItem(label);
			if(item){return item;}
		}

	}
	return NULL;
}

	
MenuItem *Menu::FindItemByPosition(int pos)
{
	if(pos>(int)items.size() || pos<0){return NULL;} 
	return items[pos];
}

void Menu::Check(int id, bool check)
{
	MenuItem * item = FindItem(id);
	if(item){
		item->check = check;
	}
}
	
void Menu::AppendSeparator()
{
	MenuItem *item = new MenuItem(-2, "", "", false, 0, 0, ITEM_SEPARATOR);
	items.push_back(item);
}

void Menu::DestroyDialog()
{
	if(dialog){
		dialog->Destroy(); dialog=NULL;
	}
}

MenuItem *Menu::SetAccMenu(int id, const wxString &txt, const wxString &help, bool enable, int kind)
{
	wxString hkey=Hkeys.GetMenuH(id);
	wxString mtext=(hkey!="")? txt.BeforeFirst('\t')+"\t"+hkey : txt;
	if(hkey!="" && Hkeys.hkeys[id].Name==""){Hkeys.hkeys[id].Name=txt.BeforeFirst('\t');}
	return Append(id,mtext,help,true,0,0,kind);
}

MenuItem *Menu::SetAccMenu(MenuItem *menuitem, const wxString &name)
{
	int id=0;
	for(auto cur=Hkeys.hkeys.rbegin(); cur!=Hkeys.hkeys.rend(); cur++)
	{
		if(cur->first<30100){break;}
		if(cur->second.Name == name ){
			//hkey = cur->second.Accel;
			id = cur->first;	
		}
	}

	if(id){menuitem->SetAccel(&Hkeys.GetHKey(id));}
	return Append(menuitem);
}

void Menu::GetAccelerators(std::vector <wxAcceleratorEntry> *entries)
{
	for(auto item : items){
		if(item->submenu){
			item->submenu->GetAccelerators(entries);
		}else if(item->accel){
			entries->push_back(*item->accel);
		}

	}
}

MenuDialog* MenuDialog::ParentMenu=NULL;
HHOOK MenuDialog::Hook=NULL;
volatile int MenuDialog::id=-3;

MenuDialog::MenuDialog(Menu *_parent, wxWindow *DialogParent, const wxPoint &pos, const wxSize &size, bool sendEvent)
	:wxFrame(DialogParent,-1,"",pos, size, wxFRAME_NO_TASKBAR)
	,wxGUIEventLoop()
	,parent(_parent)
	,sel(-1)
	,scPos(0)
	,subMenuIsShown(false)
	,isPartialModal(false)
	,showMnemonics(false)
	,submenuShown(-1)
	,submenuToHide(-1)
	,bmp(NULL)
	,accel(0)
{
	if(!ParentMenu){
		ParentMenu=this;
		Hook = NULL;
		Hook = SetWindowsHookEx(WH_GETMESSAGE, &OnMouseClick, NULL,GetCurrentThreadId());
		//wxLogStatus("hasHook %i",(int)Hook);
	}
	//show
	showSubmenuTimer.SetOwner(this, 13475);
	Bind(wxEVT_TIMER,[=](wxTimerEvent &evt){
		//wxLogStatus("show %i %i",submenuShown,sel);
		if(submenuShown == -1 || sel == -1){return;}//|| sel != submenuShown
		//if(submenuToHide != -1){hideSubmenuTimer.Start(200,true);}
		int scrollPos = submenuShown-scPos;
		wxSize size = GetSize();
		wxPoint pos = GetPosition();
		pos.x += size.x;
		pos.y += scrollPos * height;
		parent->items[submenuShown]->submenu->PopupMenu(pos, this, false);
		submenuToHide=submenuShown;
		subMenuIsShown=true;
	},13475);
	//hide
	hideSubmenuTimer.SetOwner(this, 13476);
	Bind(wxEVT_TIMER,[=](wxTimerEvent &evt){
		if(submenuToHide == -1){return;}
		//if(sel == submenuToHide){subMenuIsShown=true; return;}
		//wxLogStatus("Hidesubmenu timer %i",submenuToHide);
		MenuItem *olditem=parent->items[submenuToHide];
		if(olditem->submenu->dialog){
			wxRect rc = olditem->submenu->dialog->GetScreenRect();
			wxPoint pos = wxGetMousePosition();
			if(rc.Contains(pos)){return;}
			wxRect rc1 = GetScreenRect();
			if(!rc1.Contains(pos)){sel=-1; Refresh(false);}
			else if (sel == submenuToHide){subMenuIsShown=true; return;}
			olditem->submenu->dialog->HideWithEffect(wxSHOW_EFFECT_BLEND,1);
			olditem->submenu->DestroyDialog();
		}
		if(submenuShown != submenuToHide){showSubmenuTimer.Start(1,true);}
		submenuToHide=-1;
	},13476);
	//if(!ParentMenu->HasCapture()){CaptureMouse();}
	this->AcceptFocus(false);
}

MenuDialog::~MenuDialog()
{
	//if(HasCapture()){ReleaseMouse();}
	if(IsRunning()){
		Exit(0);
	}
	wxDELETE(bmp);
}

void MenuDialog::OnMouseEvent(wxMouseEvent &evt)
{
	bool leftdown=evt.LeftDown();
	int x=evt.GetX();
	int y=evt.GetY();
	//int w=0;
	//int h=0;
	//wxRect rc = GetClientRect();
	//bool contains = rc.Contains(evt.GetPosition());
	//if(!contains && evt.ButtonDown()){
	//	//if(HasCapture()){ReleaseMouse();}
	//	if(ParentMenu){HideMenus();}
	//	//if(!leftdown){evt.Skip();}
	//	wxLogStatus("mamy event myszowy");
	//	return;
	//}
	int elem = y/height;

	if(evt.Leaving()){
		if(submenuToHide != -1 ){
			hideSubmenuTimer.Start(400,true);
			subMenuIsShown=false;
		}
		if(sel!=submenuToHide){sel=-1; Refresh(false);}
		//if(!ParentMenu){return;}
		//int subMenu=ParentMenu->submenuShown;
		////wxLogStatus("submenu");
		//Menu *menu= ParentMenu->parent;
		////wxLogStatus("submenu1");
		//wxPoint posOnScreen = wxGetMousePosition();
		//while(menu){
		//	//wxLogStatus("menu %i, %i", subMenu, (int)menu);
		//	if(menu->dialog){
		//		if(menu->dialog != this){
		//			wxRect rc = menu->dialog->GetRect();
		//			//menu->dialog->GetPosition(&rc.x, &rc.y);
		//			//wxLogStatus("pos and rect %i %i %i %i %i %i", posOnScreen.x,posOnScreen.y,rc.x,rc.y, rc.GetRight(), rc.GetBottom());
		//			if(rc.Contains(posOnScreen)){
		//				wxLogStatus("contains");
		//				//if(HasCapture()){ReleaseMouse();}
		//				if(!menu->dialog->HasCapture()){menu->dialog->CaptureMouse();}
		//			}
		//		}
		//		subMenu = menu->dialog->submenuShown;
		//		menu = (subMenu != -1)? menu->items[subMenu]->submenu : NULL;
		//	}
		//	else{menu=NULL;subMenu = -1;}
		//	
		//}
		return;
	}//else{
	else if(evt.Entering()){
		if(submenuShown != -1 ){
			//if(submenuShown != elem){submenuToHide=submenuShown; hideSubmenuTimer.Start((parent->items[elem]->submenu)? 10 : 400,true);subMenuIsShown=false;}
			subMenuIsShown=true;
		}
		/*if( !((elem < (int)parent->items.size() && elem > 0 ) && 
			parent->items[elem]->submenu && submenuShown==-1)){
				sel=elem; Refresh(false);return;
		}*/
	}
	//}
	if (evt.GetWheelRotation() != 0) {
		int step = 3 * evt.GetWheelRotation() / evt.GetWheelDelta();
		scPos -=step;
		if(scPos<0){scPos=0;}
		else if(scPos > (int)parent->items.size()-maxVisible){scPos = parent->items.size()-maxVisible;}
	}
	
	elem+=scPos;
	if(elem>=(int)parent->items.size() || elem < 0){return;}
	if(elem!=sel){
		MenuItem *item=parent->items[elem];
		sel=elem;
		if(subMenuIsShown){
			hideSubmenuTimer.Start(400,true);
			subMenuIsShown=false;
			//wxLogStatus("Hidesubmenu %i",submenuToHide);
		}
		//wxLogStatus("dialog %i", (item->submenu)? (int)item->submenu->dialog : 0);	
		if(item->submenu && item->submenu->dialog==NULL){// 
			submenuShown=elem;
			if(submenuToHide == -1){showSubmenuTimer.Start((leftdown)? 1 : 200,true);}// : (submenuToHide != -1)? 500
		}
		
		Refresh(false);
		if(item->help != ""){
			//wxLogStatus(item->help);
		}
	}
	
	if(leftdown && parent->items[elem]->submenu == NULL){
		MenuItem *item=parent->items[elem];
		if(!item->enabled){return;}
		if(item->type == ITEM_CHECK){
			item->check = !item->check;
			Refresh(false);
			int evtid = (ParentMenu->isPartialModal)? ID_CHECK_EVENT : item->id;
			wxCommandEvent *evt= new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, evtid);
			evt->SetClientData(item);
			evt->SetInt(accel);
			wxQueueEvent(ParentMenu->GetParent(),evt);
			return;
		}
		accel = evt.GetModifiers();
		id = item->id;
		if(!isPartialModal){
			wxCommandEvent *evt= new wxCommandEvent(wxEVT_COMMAND_MENU_SELECTED, item->id);
			evt->SetClientData(item);
			evt->SetInt(accel);
			wxQueueEvent(ParentMenu->GetParent(),evt);
		}
		
		//if(HasCapture()){ReleaseMouse();}
		HideMenus();
	}else if(leftdown && submenuShown != elem){
		submenuShown=elem;
		showSubmenuTimer.Start(1,true);
	}
}
	
void MenuDialog::OnPaint(wxPaintEvent &event)
{
	int itemsize = parent->items.size();
	if(scPos>=itemsize-maxVisible){scPos=itemsize-maxVisible;}
	if(scPos<0){scPos=0;}
	int maxsize=itemsize;
	if(itemsize>maxVisible){
		maxsize=maxVisible;
		SetScrollbar(wxVERTICAL, scPos, maxVisible, itemsize);
	}
	int w=0;
	int h=0;
	GetClientSize (&w, &h);
	if(w==0||h==0){return;}
	wxMemoryDC tdc;
	if (bmp && (bmp->GetWidth() < w || bmp->GetHeight() < h)) {
		delete bmp;
		bmp = NULL;
	}
	if(!bmp){bmp=new wxBitmap(w,h);}
	tdc.SelectObject(*bmp);
	wxBitmap checkbmp = wxBITMAP_PNG("check");
	wxBitmap dot = wxBITMAP_PNG("dot");
	wxColour highlight = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
	wxColour text = wxSystemSettings::GetColour(wxSYS_COLOUR_MENUTEXT);
	wxColour graytext = wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
	tdc.SetFont(font);
	tdc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_MENU)));
	tdc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_MENU)));
	tdc.DrawRectangle(0,0,w,h);
	tdc.SetTextForeground(text);
	wxSize mnbefsize;
	wxSize linesize;
	bool hasMnemonics=false;

	bool noRadio=true;
	for(int i=0;i<maxsize; i++)
	{
		int scrollPos=i+scPos;
		MenuItem *item=parent->items[scrollPos];//+scPos
		if(item->type==ITEM_SEPARATOR){
			tdc.SetPen(wxPen("#FFFFFF"));
			tdc.SetBrush(wxBrush("#000000"));
			tdc.DrawRectangle(30,height*i+10,w-36,1);
			noRadio=true;
			continue;
		}
		if(scrollPos==sel){
			tdc.SetPen(wxPen("#000000"));
			tdc.SetBrush(wxBrush(highlight));
			tdc.DrawRectangle(0, height*i,w,height);
		}
		//tdc.SetPen(wxPen("#497CB0",2));
		//tdc.SetBrush(*wxTRANSPARENT_BRUSH);
		
		
		if(item->type==ITEM_CHECK && item->check){
			tdc.DrawBitmap(checkbmp,4,(height*i)+2);
		}else if(item->type==ITEM_RADIO && noRadio){
			tdc.DrawBitmap(dot,4,(height*i)+2);
			noRadio=false;
		}else if(item->icon){
			tdc.DrawBitmap(item->GetBitmap(),4,(height*i)+2);
		}
		wxString desc=item->GetLabel();
		tdc.SetTextForeground((item->enabled)? text : graytext);
		if(showMnemonics && desc.Find('&')!=-1){
			wxString rest;
			wxString beforemn = desc.BeforeFirst('&', &rest);
			mnbefsize = tdc.GetTextExtent(beforemn);
		
			linesize = tdc.GetTextExtent(rest[0]);
			hasMnemonics=true;
		}else{hasMnemonics=false;}
		
		desc.Replace("&","");
		int find=desc.find("\t");
		//tdc.SetPen(wxPen("#497CB0"));
		if (find!= -1 ){
			int fw, fhh;
			wxString accel=desc.AfterLast('\t');
			tdc.GetTextExtent(accel, &fw, &fhh);
			tdc.DrawText(accel,w-fw-20,(height*i)+2);
			desc=desc.BeforeLast('\t');
		}
		tdc.DrawText(desc,28,(height*i)+2);
		if(hasMnemonics){
			tdc.SetPen(wxPen(text));
			tdc.DrawLine(28+mnbefsize.x, (height*(i+1))-4, 28+mnbefsize.x+linesize.x, (height*(i+1))-4);
		}
		if(item->submenu){
			wxPoint points[3];
			int pos = w-10;
			int pos1= (height*i)+12;
			points[0]=wxPoint(pos,pos1-6);
			points[1]=wxPoint(pos,pos1);
			points[2]=wxPoint(pos+4,pos1-3);
			tdc.SetBrush(wxBrush(text));
			tdc.SetPen(wxPen(graytext));
			tdc.DrawPolygon(3,points);
		}

	}

	/*tdc.SetPen(wxPen("#FFFFFF"));
	tdc.SetBrush(wxBrush(graytext));
	tdc.DrawRectangle(22,6,1,h-12);*/
	wxPaintDC dc(this);
	dc.Blit(0,0,w,h,&tdc,0,0);
}
	
void MenuDialog::OnScroll(wxScrollWinEvent& event)
{
	int newPos=0;
	int tsize=parent->items.size();
	if(event.GetEventType()==wxEVT_SCROLLWIN_LINEUP)
	{
		newPos=scPos-1;
		if(newPos<0){newPos=0;return;}
	}
	else if(event.GetEventType()==wxEVT_SCROLLWIN_LINEDOWN)
	{
		newPos=scPos+1;
	}
	else if(event.GetEventType()==wxEVT_SCROLLWIN_PAGEUP)
	{
		wxSize size=GetClientSize();
		newPos=scPos;
		newPos-=(size.y/height - 1);
		newPos=MAX(0,newPos);
	}
	else if(event.GetEventType()==wxEVT_SCROLLWIN_PAGEDOWN)
	{
		wxSize size=GetClientSize();
		newPos=scPos;
		newPos+=(size.y/height - 1);
		newPos=MIN(newPos,tsize-1);
	}
	else{newPos = event.GetPosition();}
	if (scPos != newPos) {
		scPos = newPos;
		Refresh(false);
	}
}

void MenuDialog::HideMenus()
{
	if(!ParentMenu){return;}
	UnhookWindowsHookEx( ParentMenu->Hook );
	//ParentMenu->DestroyChildren();
	
	//wxWindow *nsibling = ParentMenu->GetNextSibling();
	//wxLogStatus("sibling1 %i", (int)nsibling);
	//Menu *menu = parent;
	////int count = 0;
	//while (menu){
	//	wxLogStatus("sibling %i", (int)menu);
	//	//MenuDialog *Dialog = (MenuDialog *)nsibling;
	//	parent->dialog=NULL;
	//	if(menu != ParentMenu->parent){menu = parent->parentMenu;}else{ menu=0; }
	//	//count++;
	//}
	
	int subMenu=ParentMenu->submenuToHide;
	//wxLogStatus("submenu");
	Menu *menu=ParentMenu->parent;
	while(subMenu!= -1 && menu->items[subMenu]->submenu){
		menu = menu->items[subMenu]->submenu;
		//wxLogStatus("menu %i, %i", subMenu, (int)menu);
		if(menu->dialog){subMenu = menu->dialog->submenuToHide;}
		else{subMenu = -1;break;}
		menu->dialog->HideWithEffect(wxSHOW_EFFECT_BLEND,1);
		menu->DestroyDialog();
	}
	if(ParentMenu->isPartialModal){ParentMenu->EndPartialModal(0);}
	else{ParentMenu->HideWithEffect(wxSHOW_EFFECT_BLEND,1); ParentMenu->parent->DestroyDialog();}
	ParentMenu=NULL;
}

void MenuDialog::OnLostCapture(wxMouseCaptureLostEvent &evt){
	//if(HasCapture()){ReleaseMouse();}
	//wxLogStatus("lostcapture");
	if(ParentMenu){HideMenus();}
}
//Pokazuje okno menu dodaj�c p�tl� czekaj�c� do odwo�ania
int MenuDialog::ShowPartialModal()
{
	isPartialModal=true;
	ShowWithEffect(wxSHOW_EFFECT_BLEND ,1);//wxSHOW_EFFECT_NONE
	Refresh(false);
	//if((size_t)maxVisible < parent->items.size()){SetFocus();}
	if(IsShown()){
		Run();
	}
	return id;
}
//Odwo�uje p�tl� czekaj�c�
void MenuDialog::EndPartialModal(int ReturnId)
{
	Exit(ReturnId);
	HideWithEffect(wxSHOW_EFFECT_BLEND ,1);
}

LRESULT CALLBACK MenuDialog::OnMouseClick( int code, WPARAM wParam, LPARAM lParam )
{
	if(code < 0)
	{ 
		CallNextHookEx(ParentMenu->Hook, code, wParam, lParam);
		return 0;
	}
	LPMSG msg = (LPMSG)lParam;
	if( msg->message == WM_MOUSEWHEEL ){
		POINT mouse;
		GetCursorPos (&mouse);
		HWND hWndPointed = WindowFromPoint(mouse);
		if (hWndPointed != NULL && msg->hwnd != hWndPointed){
			//wxLogStatus("rolka message");
			PostMessage(hWndPointed, WM_MOUSEWHEEL, msg->wParam, msg->lParam);
			return 1;
		}
	}
	if( msg->message == WM_LBUTTONDOWN || msg->message == WM_NCLBUTTONDOWN || 
		msg->message == WM_RBUTTONDOWN || msg->message == WM_NCRBUTTONDOWN){// || msg->message == WM_MBUTTONDOWN || msg->message == WM_NCMBUTTONDOWN

		MenuDialog::id=-3;
		if(!ParentMenu){return 0;}
		int subMenu=ParentMenu->submenuShown;
		Menu *menu= ParentMenu->parent;
		wxPoint posOnScreen = wxGetMousePosition();
		bool contains=false;
		while(menu){
			if(menu->dialog){
				wxRect rc = menu->dialog->GetRect();
				if(rc.Contains(posOnScreen)){
					contains=true;
				}
				
				subMenu = menu->dialog->submenuShown;
				menu = (subMenu != -1)? menu->items[subMenu]->submenu : NULL;
			}
			else{menu=NULL;subMenu = -1;}
		}
		//wxLogStatus("hide contains %i", (int)contains);
		if(contains) {return 0;}
		ParentMenu->HideMenus();
		return 1;
	}
	return CallNextHookEx( 0, code, wParam, lParam );
}

WXLRESULT MenuDialog::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam) {
	
    if (message == 28 ) {
		ParentMenu->HideMenus();
    }
    return wxFrame::MSWWindowProc(message, wParam, lParam);
}


BEGIN_EVENT_TABLE(MenuDialog, wxFrame)
	EVT_MOUSE_EVENTS(MenuDialog::OnMouseEvent)
	EVT_PAINT(MenuDialog::OnPaint)
	//EVT_MOUSE_CAPTURE_LOST(MenuDialog::OnLostCapture)
	//EVT_KILL_FOCUS(MenuDialog::OnKillFocus)
	EVT_SCROLLWIN(MenuDialog::OnScroll)
END_EVENT_TABLE()

static int menuIndent = 20;
static int halfIndent = 10;
MenuBar *MenuBar::Menubar = NULL;
HHOOK MenuBar::Hook = NULL;

MenuBar::MenuBar(wxWindow *_parent)
	:wxWindow(_parent, -1, wxDefaultPosition, wxSize(-1,height))
	,parent(_parent)
	,bmp(NULL)
	,sel(-1)
	,clicked(false)
	,showMnemonics(false)
	,oldelem(-1)
	,shownMenu(-1)
{
	SetFont(font);
	Refresh(false);
	Bind(wxEVT_TIMER,[=](wxTimerEvent &event){
		if(shownMenu == -1){return;}
		MenuEvent evt(EVT_MENU_OPENED, GetId(), Menus[shownMenu]);
		ProcessEvent(evt);
		wxSize rc= GetClientSize();
		int posX=halfIndent;
		for(int i = 0; i < shownMenu; i++){
			Menu *menu = Menus[i];
			wxString desc=menu->GetTitle();
			desc.Replace("&","");
			wxSize te = GetTextExtent(desc);
			posX += te.x + menuIndent;
		}
		wxPoint pos1(posX, rc.y); 
		Menus[shownMenu]->PopupMenu(pos1, this);
	},56432);
	showMenuTimer.SetOwner(this, 56432);
	Menubar=this;
	Hook = NULL;
	Hook = SetWindowsHookEx(WH_KEYBOARD, &OnKey, NULL,GetCurrentThreadId());
}

char findMnemonics(const wxString &label){
	int result = label.Find('&');
	if(result != -1){
		wxString rawmnemonics = label.Mid(result + 1, 1);
		return rawmnemonics.Upper()[0];
	}
	return 0;
}

void MenuBar::Append(Menu *menu, const wxString &title)
{
	menu->SetTitle(title);
	Menus.push_back(menu);
	char mn = findMnemonics(title);
	if(mn){mnemonics[mn] = Menus.size()-1;}
}
	
void MenuBar::Prepend(Menu *menu, const wxString &title)
{
	menu->SetTitle(title);
	Menus.insert(Menus.begin(), menu);
	char mn = findMnemonics(title);
	if(mn){mnemonics[mn] = 0;}
}
	
void MenuBar::Insert(int position, Menu *menu, const wxString &title)
{
	menu->SetTitle(title);
	Menus.insert(Menus.begin()+position, menu);
	char mn = findMnemonics(title);
	if(mn){mnemonics[mn] = position;}
}

void MenuBar::OnMouseEvent(wxMouseEvent &evt)
{
	if(evt.LeftUp()){clicked = false; Refresh(false);}
	
	wxPoint pos = evt.GetPosition();
	int elem = CalcMousePos(&pos);
	if(evt.Leaving()){
		sel=-1; Refresh(false); 
		return;
	}
	if(evt.Entering()){
		if(shownMenu!=-1 && Menus[shownMenu]->dialog==NULL){shownMenu=-1;}
		oldelem=-1;
		if(shownMenu == elem){oldelem=sel=elem;Refresh(false);return;}
	}
	if(elem != oldelem && elem != -1){
		oldelem=elem;
		sel=elem;
		Refresh(false);
		if(shownMenu!=-1){
			
			if(Menus[shownMenu]->dialog){Menus[shownMenu]->dialog->HideMenus();}
			shownMenu=elem;
			showMenuTimer.Start(200,true);
			
		}
		
	}
	
	if(evt.LeftDown() && elem>=0){
		clicked = true;
		Refresh(false);
		if(shownMenu==elem ){
			shownMenu = -1;
			return;
		}
		shownMenu=elem;
		showMenuTimer.Start(10,true);
	}
	
}
	
void MenuBar::OnPaint(wxPaintEvent &event)
{
	int w=0;
	int h=0;
	GetClientSize (&w, &h);
	if(w==0||h==0){return;}

	wxMemoryDC tdc;
	if (bmp && (bmp->GetWidth() < w || bmp->GetHeight() < h)) {
		delete bmp;
		bmp = NULL;
	}
	if(!bmp){bmp=new wxBitmap(w,h);}
	tdc.SelectObject(*bmp);
	tdc.SetFont(font);
	tdc.SetBrush(wxBrush(wxSystemSettings::GetColour(wxSYS_COLOUR_MENUBAR)));
	tdc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_MENUBAR)));
	tdc.DrawRectangle(0,0,w,h);
	tdc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_MENUTEXT));
	tdc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_MENUTEXT)));
	int posX=halfIndent;
	wxSize mnbefsize;
	wxSize linesize;
	bool hasMnemonics=false;
	for(size_t i = 0; i < Menus.size(); i++){
		Menu *menu = Menus[i];
		wxString desc=menu->GetTitle();
		
		
		
		if(showMnemonics && desc.Find('&')!=-1){
			wxString rest;
			wxString beforemn = desc.BeforeFirst('&', &rest);
			mnbefsize = tdc.GetTextExtent(beforemn);
		
			linesize = tdc.GetTextExtent(rest[0]);
			hasMnemonics=true;
		}else{hasMnemonics=false;}

		desc.Replace("&","");
		wxSize te = tdc.GetTextExtent(desc);
		//wxLogStatus("desc %i"+desc, posX);
		if(i == sel){
			tdc.SetBrush(wxBrush(wxSystemSettings::GetColour(clicked? wxSYS_COLOUR_BTNFACE : wxSYS_COLOUR_HIGHLIGHT)));
			tdc.SetPen(wxPen(wxSystemSettings::GetColour(clicked? wxSYS_COLOUR_BTNSHADOW : wxSYS_COLOUR_HIGHLIGHT)));
			tdc.DrawRoundedRectangle(posX-2, 2, te.x+4, h-4, 2.0);
			tdc.SetPen(wxPen(wxSystemSettings::GetColour(wxSYS_COLOUR_MENUTEXT)));
		}
		tdc.DrawText(desc,posX,2);//(h-te.y)/2.0
		if(hasMnemonics){tdc.DrawLine(posX+mnbefsize.x, h-4, posX+mnbefsize.x+linesize.x, h-4);}
		posX += te.x + menuIndent;
	}

	wxPaintDC dc(this);
	dc.Blit(0,0,w,h,&tdc,0,0);
}

int MenuBar::CalcMousePos(wxPoint *pos)
{
	int posX=0;
	for(size_t i = 0; i < Menus.size(); i++){
		Menu *menu = Menus[i];
		wxString desc=menu->GetTitle();
		desc.Replace("&","");
		wxSize te = GetTextExtent(desc);
		if(pos->x > posX && (posX + te.x + menuIndent) > pos->x){
			pos->x = posX, pos->y = posX + te.x + halfIndent;
			return i;
		}
		posX += te.x + menuIndent;
	}
	return -1;
}

MenuItem *MenuBar::FindItem(int id)
{
	MenuItem *item=NULL;
	for(auto menu : Menus){
		item = menu->FindItem(id);
		if (item) {return item;}
	}
	return NULL;
}
	
void MenuBar::Enable(int id, bool enable)
{
	MenuItem * item = FindItem(id);
	if(item){item->Enable(enable);}
	else{wxLogStatus("Cannot enable item with id %i", id);}
}

void MenuBar::AppendAccelerators(std::vector <wxAcceleratorEntry> *entries)
{
	for(size_t i = 0; i<Menus.size(); i++){
		Menus[i]->GetAccelerators(entries);
	}
}

LRESULT CALLBACK MenuBar::OnKey( int code, WPARAM wParam, LPARAM lParam ){

	if(code < 0)
	{ 
		CallNextHookEx(Hook, code, wParam, lParam);
		return 0;
	}
	if(wParam == VK_MENU && (!Menubar->showMnemonics || !(lParam & 536870912))){
		//wxLogStatus("alt down %i", (int)wParam);
		Menubar->showMnemonics = !Menubar->showMnemonics;
		Menubar->Refresh(false);
	}else if(Menubar->showMnemonics && (wParam >= 0x41 && wParam <= 0x5A) && !(lParam & 2147483648)){
		auto foundmnemonics = Menubar->mnemonics.find(wParam);
		if(foundmnemonics != Menubar->mnemonics.end()){
			//wxLogStatus("letter up %i", (int)wParam);
			if(Menubar->shownMenu != -1 && Menubar->Menus[Menubar->shownMenu]->dialog){
				Menubar->Menus[Menubar->shownMenu]->dialog->HideMenus();
			}
			Menubar->shownMenu = foundmnemonics->second;
			Menubar->showMenuTimer.Start(10,true);
			//tymczasowo p�ki nie mamy obs�ugi mnemonics�w w samych menu
			Menubar->showMnemonics=false;
		}
	}

	return CallNextHookEx(Hook, code, wParam, lParam);
}

BEGIN_EVENT_TABLE(MenuBar, wxWindow)
	EVT_MOUSE_EVENTS(MenuBar::OnMouseEvent)
	EVT_PAINT(MenuBar::OnPaint)
END_EVENT_TABLE()