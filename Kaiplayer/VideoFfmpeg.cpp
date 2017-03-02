﻿//  Copyright (c) 2016, Marcin Drob

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

#include <wx/dir.h>
#include "VideoFfmpeg.h"
#include "KainoteApp.h"
#include "Config.h"
#include "MKVWrap.h"
#include "KaiMessageBox.h"
#include <objbase.h>
#include <algorithm>
#include <process.h>
#include "include\ffmscompat.h"
#include "Stylelistbox.h"
#include <wx/file.h>



VideoFfmpeg::VideoFfmpeg(const wxString &filename, VideoRend *renderer, bool *_success)
	: rend(renderer)
	, eventStartPlayback (CreateEvent(0, FALSE, FALSE, 0))
	, eventRefresh (CreateEvent(0, FALSE, FALSE, 0))
	, eventKillSelf (CreateEvent(0, FALSE, FALSE, 0))
	, eventComplete (CreateEvent(0, FALSE, FALSE, 0))
	,blnum(0)
	,Cache(0)
	,Delay(0)
	,audiosource(0)
	,videosource(0)
	,progress(0)
	,thread(0)
	,lastframe(-1)
	,width(-1)
	,fp(NULL)
	,index(NULL)
	,isBusy(false)
{
	if(!Options.AudioOpts && !Options.LoadAudioOpts()){KaiMessageBox(_("Dupa blada, opcje się nie wczytały, na audio nie podziałasz"), _("Błędny błąd"));}
	disccache = !Options.GetBool(AudioRAMCache);

	success=false;
	fname = filename;
	kainoteApp *Kaia=(kainoteApp*)wxTheApp;
	progress = new ProgressSink(Kaia->Frame,_("Indeksowanie pliku wideo"));
	//listw tracks(Kaia->Frame);
	if(renderer){

		thread = (HANDLE)_beginthreadex(0, 0, FFMS2Proc, this, 0, 0);//CreateThread( NULL, 0,  (LPTHREAD_START_ROUTINE)FFMS2Proc, this, 0, 0);
		progress->ShowDialog();
		WaitForSingleObject(eventComplete, INFINITE);
		ResetEvent(eventComplete);
		*_success = success;
	}else{
		progress->SetAndRunTask([=](){return Init();});
		progress->ShowDialog();
		*_success=((int)progress->Wait() == 1 );
	}
	SAFE_DELETE(progress);
	if(index){FFMS_DestroyIndex(index);}
	if(audiosource){FFMS_DestroyAudioSource(audiosource);audiosource=0;}
}

unsigned int __stdcall VideoFfmpeg::FFMS2Proc(void* cls)
{
	((VideoFfmpeg*)cls)->Processing();
	return 0;
}

void VideoFfmpeg::Processing()
{
	HANDLE events_to_wait[] = {
		eventStartPlayback,
		eventRefresh,
		eventKillSelf
	};

	success=(Init()==1);
	
	progress->EndModal();

	int fplane=height * width * 4;
	int tdiff=0;

	SetEvent(eventComplete);
	if(width < 0){return;}

	while(1){
		DWORD wait_result = WaitForMultipleObjects(sizeof(events_to_wait)/sizeof(HANDLE), events_to_wait, FALSE, INFINITE);

		if(wait_result == WAIT_OBJECT_0+0)
		{
			byte *buff = (byte*)rend->datas;
			int acttime;
			while(1){

				//rend->lastframe = GetFramefromMS(timeGetTime() - rend->lasttime, rend->lastframe);
				if(rend->lastframe != lastframe){
					rend->time = Timecodes[rend->lastframe];
					lastframe = rend->lastframe;
				}
				
				fframe=FFMS_GetFrame(videosource, rend->lastframe, &errinfo);
				if(!fframe){continue;}
				memcpy(&buff[0],fframe->Data[0],fplane);

				rend->DrawTexture(buff);
				rend->Render(false);
				
				if(rend->time>=rend->playend || rend->lastframe >= NumFrames-1){
					wxCommandEvent *evt = new wxCommandEvent(wxEVT_COMMAND_BUTTON_CLICKED,23333);
					wxQueueEvent(rend, evt);
					break;
				}
				else if(rend->vstate!=Playing){
					break;
				}	
				acttime = timeGetTime() - rend->lasttime;

				rend->lastframe++;
				rend->time = Timecodes[rend->lastframe];

				tdiff = rend->time - acttime;
				
				if(tdiff>0){Sleep(tdiff);}
				else{
					while(1){
						if(Timecodes[rend->lastframe]>=acttime || rend->lastframe>=NumFrames){
							if(rend->lastframe>=NumFrames){rend->lastframe = NumFrames-1; rend->time = rend->playend;}
							break;
						}else{
							rend->lastframe++;
						}
					}
					
				}

			}
		}else if(wait_result == WAIT_OBJECT_0+1){
			byte *buff = (byte*)rend->datas;
			if(rend->lastframe != lastframe){
				fframe=FFMS_GetFrame(videosource, rend->lastframe, &errinfo);
				lastframe = rend->lastframe;
			}
			if(!fframe){SetEvent(eventComplete);continue;}
			memcpy(&buff[0],fframe->Data[0],fplane);
			rend->DrawTexture(buff);
			rend->Render(false);
			SetEvent(eventComplete);
			isBusy = false;
		}
		else{
			break;
		}

	}
}


int VideoFfmpeg::Init()
{

	FFMS_Init(0, 1);

	char errmsg[1024];
	errinfo.Buffer      = errmsg;
	errinfo.BufferSize  = sizeof(errmsg);
	errinfo.ErrorType   = FFMS_ERROR_SUCCESS;
	errinfo.SubType     = FFMS_ERROR_SUCCESS;

	FFMS_Indexer *Indexer = FFMS_CreateIndexer(fname.utf8_str(), &errinfo);
	if(!Indexer){
		wxLogMessage(_("Wystąpił błąd indeksowania: %s"), errinfo.Buffer); return 0;
	}

	int NumTracks = FFMS_GetNumTracksI(Indexer);
	int audiotrack=-1;
	wxArrayInt audiotable;
	int videotrack=-1;

	for (int i=0; i<NumTracks; i++) {
		if (FFMS_GetTrackTypeI(Indexer, i) == FFMS_TYPE_VIDEO && videotrack==-1) {
			videotrack=i;
		}
		else if (FFMS_GetTrackTypeI(Indexer, i) == FFMS_TYPE_AUDIO) {
			audiotable.Add(i);
		}
		//else if(audiotrack!=-1 && videotrack !=-1)
		//{break;}
	}
	bool ismkv=(fname.AfterLast('.').Lower()=="mkv");

	if(audiotable.size()>1 || ismkv){

		wxArrayString tracks;

		if(ismkv){
			MatroskaWrapper mw;
			if(mw.Open(fname,false)){
				if(audiotable.size()>1){
					for (size_t j=0;j<audiotable.size();j++){
						TrackInfo* ti=mkv_GetTrackInfo(mw.file,audiotable[j]);

						wxString all;
						char *opis = (ti->Name)? ti->Name : ti->Language;
						all<<audiotable[j]<<": "<<opis;
						tracks.Add(all);
					}
				}
				Chapter *chap=NULL;
				UINT nchap=0;
				mkv_GetChapters(mw.file,&chap,&nchap);
				//wxLogStatus("chap %i, %i", (int)chap, (int)nchap);
				if(chap && nchap && rend){
					for(int i=0; i<(int)chap->nChildren; i++){
						chapter ch;
						ch.name=wxString(chap->Children[i].Display->String);
						ch.time=(int)(chap->Children[i].Start/1000000.0);
						rend->chaps.push_back(ch);
					}
				}
				mw.Close();
			}
			if(audiotable.size()<2){audiotrack= (audiotable.size()>0)? audiotable[0] : -1; goto done;}
		}else{
			for (size_t j=0;j<audiotable.size();j++){
				wxString CodecName(FFMS_GetCodecNameI(Indexer, audiotable[j]), wxConvUTF8);
				wxString all;
				all<<audiotable[j]<<": "<<CodecName;
				tracks.Add(all);
			}
		}
		audiotrack = progress->ShowSecondaryDialog([=](){
			kainoteApp *Kaia=(kainoteApp*)wxTheApp;

			KaiListBox tracks(Kaia->Frame, tracks, _("Wybierz ścieżkę"),true);
			if(tracks.ShowModal()==wxID_OK){
				int result=wxAtoi(tracks.GetSelection().BeforeFirst(':'));
				return result;
			}
			return -1;
		});

		if(audiotrack == -1){
			FFMS_CancelIndexing(Indexer);
			return 0;
		}

	}else if(audiotable.size()>0){
		audiotrack=audiotable[0];
	}
done:

	wxString path=Options.pathfull + "\\Indices\\" + fname.AfterLast('\\').BeforeLast('.') + wxString::Format("_%i.ffindex",audiotrack);

	//FFMS_Index *index=NULL;


	//show dialog
	//progress->ShowDialog();

	if(wxFileExists(path)){
		index = FFMS_ReadIndex(path.utf8_str(), &errinfo);

		if(FFMS_IndexBelongsToFile(index, fname.utf8_str(), &errinfo))
		{
			FFMS_DestroyIndex(index);
			index=NULL;
		}else{
			FFMS_CancelIndexing(Indexer);
		}

	}
	bool newIndex=false;
	if(!index){
		FFMS_TrackIndexSettings(Indexer, audiotrack, 1, 0);
		FFMS_SetProgressCallback(Indexer, UpdateProgress, (void*)progress);
		index = FFMS_DoIndexing2(Indexer, FFMS_IEH_IGNORE, &errinfo);
		//in this moment indexer was released, there no need to release it
		if (index == NULL) {
			if(wxString(errinfo.Buffer).StartsWith("Cancelled")){
				wxLogStatus(_("Indeksowanie anulowane przez użytkownika"));
			}
			else{
				wxLogMessage(_("Wystąpił błąd indeksowania: %s"), errinfo.Buffer);
			}
			//FFMS_CancelIndexing(Indexer);
			return 0;
		}
		//wxLogStatus("write index "+path);
		if(!wxDir::Exists(path.BeforeLast('\\')))
		{
			wxDir::Make(path.BeforeLast('\\'));
		}
		//wxLogMessage("write index");
		if(FFMS_WriteIndex(path.utf8_str(), index, &errinfo))
		{
			wxLogMessage(_("Nie można zapisać indeksu, wystąpił błąd %s"), errinfo.Buffer);
			//FFMS_DestroyIndex(index);
			//FFMS_CancelIndexing(Indexer);
			//return 0;
		}
		newIndex=true;
	}
	

	//wxLogStatus("video");
	if(videotrack!=-1 && rend){	
		//wxLogStatus("num of cores %i", (int)std::thread::hardware_concurrency());
		SYSTEM_INFO sysinfo;
		GetSystemInfo( &sysinfo );
		videosource = FFMS_CreateVideoSource(
			fname.utf8_str(), 
			videotrack, 
			index, 
			sysinfo.dwNumberOfProcessors*2,
			Options.GetInt(FFMS2VideoSeeking),//FFMS_SEEK_NORMAL, // FFMS_SEEK_UNSAFE/*FFMS_SEEK_AGGRESSIVE*/
			&errinfo);
		//Since the index is copied into the video source object upon its creation,
		//we can and should now destroy the index object. 

		if (videosource == NULL) {
			wxLogMessage(_("Dupa bada, videosource nie utworzył się."));
			return 0;
		}

		const FFMS_VideoProperties *videoprops = FFMS_GetVideoProperties(videosource);

		NumFrames = videoprops->NumFrames;
		Duration=videoprops->LastTime;
		//Delay = videoprops->FirstTime + (Options.GetInt("Audio Delay")/1000);
		fps=(float)videoprops->FPSNumerator/(float)videoprops->FPSDenominator;

		const FFMS_Frame *propframe = FFMS_GetFrame(videosource, 0, &errinfo);

		width=propframe->EncodedWidth;
		height=propframe->EncodedHeight; 
		arwidth=(videoprops->SARNum==0)? width : (float)width*((float)videoprops->SARNum/(float)videoprops->SARDen);
		arheight= height;
		while(1){
			bool divided=false;
			for (int i = 10; i>1; i--){
				if((arwidth % i)==0 && (arheight % i)==0){
					arwidth/=i; arheight /=i;
					divided=true;
					break;
				}
			}
			if(!divided){break;}
		}


		int pixfmt[2];
		pixfmt[0] = FFMS_GetPixFmt("bgra");//PIX_FMT_YUYV422; //PIX_FMT_NV12 == 25  PIX_FMT_YUVJ420P;//PIX_FMT_YUV411P;//PIX_FMT_YUV420P; //PIX_FMT_YUYV422;//PIX_FMT_NV12;//FFMS_GetPixFmt("bgra");PIX_FMT_YUYV422;//
		pixfmt[1] = -1;

		if (FFMS_SetOutputFormatV2(videosource, pixfmt, width, height, FFMS_RESIZER_BILINEAR, &errinfo)) {
			wxLogMessage(_("Dupa bada, nie można przekonwertować wideo na RGBA"));
			return 0;
		}

		CS = propframe->ColorSpace;
		CR = propframe->ColorRange;

		if (CS == FFMS_CS_UNSPECIFIED)
			CS = width > 1024 || height >= 600 ? FFMS_CS_BT709 : FFMS_CS_BT470BG;
		ColorSpace = RealColorSpace = ColorCatrixDescription(CS, CR);
		Grid *grid = ((TabPanel*)rend->GetParent())->Grid1;
		wxString colormatrix = grid->GetSInfo("YCbCr Matrix");
		if(colormatrix.IsEmpty()){colormatrix=_("Brak");}
		if (CS != FFMS_CS_RGB && CS != FFMS_CS_BT470BG && ColorSpace != colormatrix && colormatrix == "TV.601") {
			if (FFMS_SetInputFormatV(videosource, FFMS_CS_BT470BG, CR, FFMS_GetPixFmt(""), &errinfo)){
				wxLogMessage(_("Dupa bada, macierz YCbCr się nie znieniła"));
			}
			ColorSpace = ColorCatrixDescription(FFMS_CS_BT470BG, CR);
		}

		FFMS_Track *FrameData = FFMS_GetTrackFromVideo(videosource);
		if (FrameData == NULL){
			wxLogMessage(_("Dupa bada, nie można pobrać ścieżki wideo"));
			return 0;}
		const FFMS_TrackTimeBase *TimeBase = FFMS_GetTimeBase(FrameData);
		if (TimeBase == NULL){
			wxLogMessage(_("Dupa bada, nie można pobrać informacji o wideo"));
			return 0;}

		const FFMS_FrameInfo *CurFrameData;


		// build list of keyframes and timecodes
		for (int CurFrameNum = 0; CurFrameNum < videoprops->NumFrames; CurFrameNum++) {
			CurFrameData = FFMS_GetFrameInfo(FrameData, CurFrameNum);
			if (CurFrameData == NULL) {
				continue;
			}

			// keyframe?

			int Timestamp = ((CurFrameData->PTS * TimeBase->Num) / TimeBase->Den);
			if (CurFrameData->KeyFrame){KeyFrames.Add(Timestamp);}
			Timecodes.push_back(Timestamp);


		}

	}
	/* Retrieve the track number of the first audio track */
	//int trackn = FFMS_GetFirstTrackOfType(index, FFMS_TYPE_AUDIO, &errinfo);

	if(audiotrack==-1){ SampleRate=-1; return 1;}
	/* We now have enough information to create the audio source object */
	audiosource = FFMS_CreateAudioSource(fname.utf8_str(), audiotrack, index, FFMS_DELAY_FIRST_VIDEO_TRACK, &errinfo);//FFMS_DELAY_FIRST_VIDEO_TRACK
	if (audiosource == NULL) {
		/* handle error (you should know what to do by now) */
		wxLogMessage(_("Wystąpił błąd tworzenia źródła audio: %s"),errinfo.Buffer);
		return 0;
	}

	FFMS_ResampleOptions *resopts=FFMS_CreateResampleOptions(audiosource);
	resopts->ChannelLayout=FFMS_CH_FRONT_CENTER;
	resopts->SampleFormat=FFMS_FMT_S16;

	if (FFMS_SetOutputFormatA(audiosource, resopts, &errinfo)){
		wxLogMessage(_("Wystąpił błąd konwertowania audio: %s"),errinfo.Buffer);
	}
	else{
		BytesPerSample=2;
		Channels=1;
	}
	FFMS_DestroyResampleOptions(resopts);
	const FFMS_AudioProperties *audioprops = FFMS_GetAudioProperties(audiosource);

	SampleRate=audioprops->SampleRate;
	//BytesPerSample=audioprops->BitsPerSample/8;
	//Channels=audioprops->Channels;
	Delay=(Options.GetInt(AudioDelay)/1000);
	NumSamples=audioprops->NumSamples;
	//audioprops = FFMS_GetAudioProperties(audiosource);
	if(Delay >= (SampleRate*NumSamples*BytesPerSample)){
		wxLogMessage(_("Z opóźnienia nici, przekracza czas trwania audio"));
		Delay=0;
	}

	if(disccache){
		diskCacheFilename="";
		diskCacheFilename << Options.pathfull << "\\AudioCache\\" << fname.AfterLast('\\').BeforeLast('.')<< "_track" << audiotrack << ".w64";
		if(newIndex && wxFileExists(diskCacheFilename)){wxRemoveFile(diskCacheFilename);}
		if(!DiskCache()){return 0;}
	}else{
		if(!CacheIt()){return 0;}
	}
	return 1;
}



VideoFfmpeg::~VideoFfmpeg()
{
	if(thread){ 
		SetEvent(eventKillSelf);
		WaitForSingleObject(thread,2000);
		CloseHandle(thread);
		CloseHandle(eventStartPlayback);
		CloseHandle(eventRefresh);
		CloseHandle(eventKillSelf);
	}
	KeyFrames.Clear();
	Timecodes.clear();

	if(videosource){
		FFMS_DestroyVideoSource(videosource);videosource=NULL;
	}

	if(disccache){Cleardiskc();}else{Clearcache();}
}



int FFMS_CC VideoFfmpeg::UpdateProgress(int64_t Current, int64_t Total, void *ICPrivate)
{
	ProgressSink *progress= (ProgressSink*)ICPrivate;
	progress->Progress(((double)Current/(double)Total)*100);
	return progress->WasCancelled();
}

void VideoFfmpeg::GetFrame(int ttime, byte *buff)
{
	//if(lastframe!=ttime){fframe=FFMS_GetFrame(videosource, ttime, &errinfo);}//fframe=FFMS_GetFrameByTime(videosource, (double)ttime/1000.0, &errinfo);}
	//lastframe=ttime;
	byte* cpy= (byte *)fframe->Data[0];
	memcpy(&buff[0],cpy,height*width*4);
	
}

void VideoFfmpeg::GetAudio(void *buf, int64_t start, int64_t count)
{
	//wxMutexLocker lock(blockaudio);
	//wxLogStatus(_("weszło"));

	if (count == 0 || !audiosource) return;
	if (start+count > NumSamples) {
		int64_t oldcount = count;
		count = NumSamples-start;
		if (count < 0) count = 0;

		// Fill beyond with zero

		short *temp = (short *) buf;
		for (int64_t i=count;i<oldcount;i++) {
			temp[i] = 0;
		}

	}

	FFMS_GetAudio(audiosource, buf, start, count, &errinfo);

}

void VideoFfmpeg::GetBuffer(void *buf, int64_t start, int64_t count, double volume)
{
	wxMutexLocker lock(blockaudio);

	if (start+count > NumSamples) {
		int64_t oldcount = count;
		count = NumSamples-start;
		if (count < 0) count = 0;


		short *temp = (short *) buf;
		for (int i=count;i<oldcount;i++) {
			temp[i] = 0;
		}
	}

	if (count) {
		if(disccache){
			/*if(file_cache.IsOpened()){
				file_cache.Seek(start* BytesPerSample);
				file_cache.Read((char*)buf,count* BytesPerSample);}*/
			if(fp){
				_int64 pos = start* BytesPerSample;
				_fseeki64(fp, pos, SEEK_SET);
				fread(buf, 1, count* BytesPerSample, fp);
			}
		}
		else{
			if(!Cache){return;}
			char *tmpbuf = (char *)buf;
			int i = (start* BytesPerSample) >> 22;
			int blsize=(1<<22);
			int offset = (start* BytesPerSample) & (blsize-1);
			int64_t remaining = count* BytesPerSample;
			int readsize=remaining;

			while(remaining){
				readsize = MIN(remaining,blsize - offset);

				memcpy(tmpbuf,(char *)(Cache[i++]+offset),readsize);
				//wxLogStatus(_("i %i, readsize %i, end %i"), i, readsize, end);
				tmpbuf+=readsize;
				offset=0;
				remaining-=readsize;

			}
		}
		if (volume == 1.0) return;


		// Read raw samples
		short *buffer = (short*) buf;
		int value;

		// Modify
		for (int64_t i=0;i<count;i++) {
			value = (int)(buffer[i]*volume+0.5);
			if (value < -0x8000) value = -0x8000;
			if (value > 0x7FFF) value = 0x7FFF;
			buffer[i] = value;
		}

	}
}


void VideoFfmpeg::GetWaveForm(int *min,int *peak,int64_t start,int w,int h,int samples,float scale)
{
	wxMutexLocker lock(blockaudio);
	int n = w * samples;
	for (int i=0;i<w;i++) {
		peak[i] = 0;
		min[i] = h;
	}

	// Prepare waveform
	int cur;
	int curvalue;

	// Prepare buffers
	int needLen = n * BytesPerSample;

	void *raw;
	raw = new char[needLen];


	short *raw_short = (short*) raw;
	GetBuffer(raw,start,n);
	int half_h = h/2;
	int half_amplitude = int(half_h * scale);
	//wxLogStatus("before for");
	// Calculate waveform
	for (int i=0;i<n;i++) {
		cur = i/samples;
		curvalue = half_h - (int(raw_short[i])*half_amplitude)/0x8000;
		if (curvalue > h) curvalue = h;
		if (curvalue < 0) curvalue = 0;
		if (curvalue < min[cur]) min[cur] = curvalue;
		if (curvalue > peak[cur]) peak[cur] = curvalue;
	}
	//wxLogStatus("after for");

	delete[] raw;
	//wxLogStatus("del");

}

int VideoFfmpeg::GetSampleRate()
{
	return SampleRate;
}

int VideoFfmpeg::GetBytesPerSample()
{
	return BytesPerSample;
}

int VideoFfmpeg::GetChannels()
{
	return 1;
}

int64_t VideoFfmpeg::GetNumSamples()
{
	return NumSamples;
}

bool VideoFfmpeg::CacheIt()
{
	progress->Title(_("Zapisywanie do pamięci RAM"));
	//progress->cancel->Enable(false);
	int64_t end=NumSamples*BytesPerSample;

	int blsize=(1<<22);
	blnum=((float)end/(float)blsize)+1;
	Cache=NULL;
	Cache=new char*[blnum];
	if(Cache==NULL){KaiMessageBox(_("Za mało pamięci RAM"));return false;}

	//int64_t pos=0;
	int64_t pos= (Delay<0)? -(SampleRate * Delay * BytesPerSample) : 0;
	int halfsize=(blsize/BytesPerSample);


	for(int i = 0; i< blnum; i++)
	{
		if(i >= blnum-1){blsize=end-pos; halfsize=(blsize/BytesPerSample);}
		Cache[i]= new char[blsize];
		//wxLogStatus("pos %i, size %i, end %i, pos+size %i",(int)pos, blsize, (int)end, ((int)pos+blsize));
		if(Delay>0 && i == 0){
			int delaysize=SampleRate*Delay*BytesPerSample;
			if(delaysize%2==1){delaysize++;}
			int halfdiff= halfsize - (delaysize/BytesPerSample);
			memset(Cache[i],0,delaysize);
			GetAudio(&Cache[i][delaysize], 0, halfdiff);
			pos+=halfdiff;
		}else{
			GetAudio(Cache[i], pos, halfsize);
			pos+=halfsize;
		}

		progress->Progress(((float)i/(float)(blnum-1))*100);
		if(progress->WasCancelled()){blnum=i+1;Clearcache();return false;}
	}
	if(Delay<0){NumSamples += (SampleRate * Delay * BytesPerSample);}
	return true;
}



void VideoFfmpeg::Clearcache()
{
	if(!Cache){return;}
	for(int i=0; i<blnum; i++)
	{
		//wxLogStatus("i %i",i);
		delete[ ] Cache[i];
	}
	//wxLogStatus("del cache");
	delete[ ] Cache;
	//wxLogStatus("deleted");
	Cache=0;
	blnum=0;
}

int VideoFfmpeg::TimefromFrame(int nframe)
{
	if(nframe<0){nframe=0;}
	if(nframe>=NumFrames){nframe=NumFrames-1;}
	return Timecodes[nframe];
}

int VideoFfmpeg::FramefromTime(int time)
{
	if(time<=0){return 0;}
	int start=lastframe;
	if(lasttime>time)
	{
		start=0;
	}
	int wframe=NumFrames-1;	
	for(int i=start;i<NumFrames-1;i++)
	{
		if(Timecodes[i]>=time && time<Timecodes[i+1])
		{
			wframe= i;
			break;
		}
	}
	//if(lastframe==wframe){return-1;}
	lastframe=wframe;
	lasttime=time;	
	return lastframe;
}

bool VideoFfmpeg::DiskCache()
{
	progress->Title(_("Zapisywanie na dysk twardy"));

	progress->Progress(0);

	bool good=true;
	wxFileName fname;
	fname.Assign(diskCacheFilename);
	if(!fname.DirExists()){wxMkdir(diskCacheFilename.BeforeLast('\\'));}
	if(wxFileExists(diskCacheFilename)){
		//file_cache.Open(diskCacheFilename,wxFile::read);
		fp = _wfopen(diskCacheFilename.wc_str(), L"rb");
		if(fp)return true;
		else return false;
	}else{
		//file_cache.Create(diskCacheFilename,true,wxS_DEFAULT);
		//file_cache.Open(diskCacheFilename,wxFile::read_write);
		fp = _wfopen(diskCacheFilename.wc_str(), L"w+b");
		if(!fp)return false;
	}
	int block = 332768;
	//int block2=block*2
	if(Delay>0){

		int size=(SampleRate*Delay*BytesPerSample);
		if(size%2==1){size++;}
		char *silence=new char[size];
		memset(silence,0,size);
		//int wr= file_cache.Write(silence,size); 
		fwrite(silence, 1 ,size, fp);
		delete[] silence;
	}
	try {
		char *data= new char[block*BytesPerSample];
		int all=(NumSamples/block)+1;
		//int64_t pos=0;
		int64_t pos= (Delay<0)? -(SampleRate * Delay * BytesPerSample) : 0;
		for (int i=0;i<all; i++) {
			if (block+pos > NumSamples) block = NumSamples - pos;
			//wxLogStatus("i %i block %i nums %i", (int)pos, block, (int)NumSamples);
			GetAudio(data,pos,block);
			//wxLogStatus("write");
			//file_cache.Write(data,block*BytesPerSample);
			fwrite(data, 1 ,block*BytesPerSample, fp);
			//wxLogStatus("Progress");
			pos+=block;
			progress->Progress(((float)pos/(float)(NumSamples))*100);
			if(progress->WasCancelled()){
				//file_cache.Close();
				fclose(fp);
				wxRemoveFile(diskCacheFilename);
				good=false;
				delete[] data;
				return false;
			}
		}
		delete[] data;
		//file_cache.Seek(0);
		rewind(fp);
		if(Delay<0){NumSamples += (SampleRate * Delay * BytesPerSample);}
	}
	catch (...) {
		good=false;
	}

	if(!good){Cleardiskc();}

	return good;
}

void VideoFfmpeg::Cleardiskc()
{
	//file_cache.Close();
	if(fp){fclose(fp);fp=NULL;}
	//wxRemoveFile(diskCacheFilename);
}

int VideoFfmpeg::GetMSfromFrame(int frame)
{
	return Timecodes[frame];
}

int VideoFfmpeg::GetFramefromMS(int MS, int seekfrom)
{
	if (MS<=0) return 0;
	//else if(MS>=Duration) return NumFrames-1;
	int result=NumFrames-1;
	for(int i=seekfrom; i<NumFrames; i++)
	{
		if(Timecodes[i]>=MS)
		{
			result = i;
			break;
		}
	}
	return result;
}

void VideoFfmpeg::DeleteOldAudioCache()
{
	wxString path = Options.pathfull + "\\AudioCache";
	size_t tabsSize = Notebook::GetTabs()->Size();
	size_t maxAudio = (tabsSize < 10)? 10 : tabsSize;
	wxDir kat(path);
	wxArrayString audioCaches;
	if(kat.IsOpened()){
		kat.GetAllFiles(path, &audioCaches, "*.w64", wxDIR_FILES);
	}
	if(audioCaches.size()<=maxAudio){return;}
	FILETIME ft;
	SYSTEMTIME st;
	std::map<unsigned __int64, int> dates;
	unsigned __int64 datetime;
	for(size_t i = 0; i< audioCaches.size(); i++){
		HANDLE ffile = CreateFile(audioCaches[i].wc_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

		GetFileTime(ffile,0,&ft,0);
		CloseHandle(ffile);
		FileTimeToSystemTime(&ft, &st);
		if(st.wYear>3000){st.wYear=3000;}
		datetime= (st.wYear *360000000000000) + (st.wMonth *36000000000) + (st.wDay *360000000) + (st.wHour*3600000)+(st.wMinute*60000)+(st.wSecond*1000)+st.wMilliseconds;
		//wxLogStatus("date %llu %i, %i, %i, %i, %i, %i, %i, %s", datetime, (int)st.wYear, (int)st.wMonth, (int)st.wDay, (int)st.wHour, (int)st.wMinute, (int)st.wSecond, (int)st.wMilliseconds, audioCaches[i]);
		dates[datetime]=i;

	}
	int count = 0;
	int diff = audioCaches.size() - maxAudio;
	for(auto cur = dates.begin(); cur != dates.end(); cur++){
		if(count >= diff){break;}
		int isgood = _wremove(audioCaches[cur->second].wchar_str());
		//wxLogStatus("usuwa plik %i "+audioCaches[cur->second], isgood);
		count++;
	}

}

void VideoFfmpeg::Refresh(bool wait){
	//wxMutexLocker lock(blockvideo);
	isBusy = true;
	ResetEvent(eventComplete);
	SetEvent(eventRefresh);
	//wxLogStatus("set event");
	if(rend->vstate==Paused && wait){
		WaitForSingleObject(eventComplete, 4000);
	}
};

wxString VideoFfmpeg::ColorCatrixDescription(int cs, int cr) {
	// Assuming TV for unspecified
	std::string str = cr == FFMS_CR_JPEG ? "PC" : "TV";

	switch (cs) {
		case FFMS_CS_RGB:
			return "None";
		case FFMS_CS_BT709:
			return str + ".709";
		case FFMS_CS_FCC:
			return str + ".FCC";
		case FFMS_CS_BT470BG:
		case FFMS_CS_SMPTE170M:
			return str + ".601";
		case FFMS_CS_SMPTE240M:
			return str + ".240M";
		default:
			return _("Brak");
	}
}

void VideoFfmpeg::SetColorSpace(const wxString& matrix){

		if (matrix == ColorSpace) return;
		if (matrix == RealColorSpace || matrix == _("Brak"))
			FFMS_SetInputFormatV(videosource, CS, CR, FFMS_GetPixFmt(""), nullptr);
		else if (matrix == "TV.601")
			FFMS_SetInputFormatV(videosource, FFMS_CS_BT470BG, CR, FFMS_GetPixFmt(""), nullptr);
		else
			return;
		ColorSpace = matrix;

}