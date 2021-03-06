/*
 * Window.cpp
 *
 *  Created on: 29.06.2011
 *      Author: vl
 */

#include "Window.h"

CWindow::CWindow() : CActive(EPriorityHigh),iText(_L("Перезвонить")),iImageIsLoaded(EFalse),
    iWorkMode(EModeNormal),iRectColor(KRgbGreen),iIsVisible(EFalse)
    {

    }
CWindow::~CWindow()
    {
	Cancel();
	iWs.Close();
    iFs.Close();
    /*if(iSender)
    	{
    	_CPOPD(iSender);
    	}
    if(iReader)
    	{
    	_CPOPD(iReader);
        }*/
    CleanupStack::PopAndDestroy(4);
    if(iTimer != NULL)
        {
            delete iTimer;
            iTimer = NULL;
        }
    }
CWindow* CWindow::NewL()
    {
	CWindow* self = CWindow::NewLC();
	_CPOP(self);
    return self;
    }
CWindow* CWindow::NewLC()
	{
	CWindow* self = new(ELeave) CWindow();
	_CPUSH(self);
	self->ConstructL();
	return self;
	}
void CWindow::ConstructL()
    {
    __LOGSTR("Construct");
    CActiveScheduler::Add(this);
    iWs = RWsSession();
    iFs.Connect();
    iWs.Connect(iFs);
    ReadConf();

    SetNeedToRecall(EFalse);
    iSchedulerWait = new(ELeave) CActiveSchedulerWait();
    _CPUSH(iSchedulerWait);//+1 в стек

    iBackgroundColor = KRgbBlack;
    iTextColor = KRgbWhite;
    iWg = RWindowGroup(iWs);
    iWg.Construct((TUint32)&iWg,EFalse);
    iWg.SetOrdinalPosition(0,ECoeWinPriorityAlwaysAtFront + 100);
#ifdef __3RD__
    iWg.EnableReceiptOfFocus(ETrue);//На 3rd нужен фокус
#else
    iWg.EnableReceiptOfFocus(EFalse);//На 5th нет
#endif
    //iWg.EnableReceiptOfFocus(EFalse);

    iWindow = RWindow(iWs);
    iWindow.Construct(iWg,(TUint32)&iWg+1);
    iWindow.PointerFilter(EPointerFilterDrag, 0);
    iWindow.Activate();
    iWindow.SetNonFading(ETrue);
    //
    iWindow.SetVisible(EFalse);
    iWindow.SetBackgroundColor(KRgbBlack);

    iScreen=new (ELeave) CWsScreenDevice(iWs);
    User::LeaveIfError(iScreen->Construct());
    User::LeaveIfError(iScreen->CreateContext(iGc));
    GetFontSize(iText);
    iWindow.SetExtent(TPoint(iPosition.iX,iPosition.iY),TSize(iSize.iWidth,iSize.iHeight));
    iWs.Flush();
   // iCaller = CCaller::NewLC();
   // Draw();
    LoadImage();
    iWs.Flush();
    iPropImpl = CPropImpl::NewL();
    iPropImpl->SetObserver(this);
    iPropImpl->Subscribe();
    iReader = CATReader::NewLC();//+3 в стек
    iSender = CATSender::NewLC(*iReader);//+4 в стек
    iTimer = CMyTimer::NewL();//Засунуть в стек!
    iTimer->SetObserver(NULL);//this
    }
void CWindow::ReadConf()
	{
    __LOGSTR("ReadConf");
    if(iFile.Open(iFs,KFilePos,EFileRead) != KErrNone)//file not exist
	{
    __LOGSTR("File not exist");
	CreateConfigFile();
	ReadConf();
	}else{
	__LOGSTR("File exist");
	//file exist
	TFileText text;
	TLex lex;
	TBuf<32> buf;
	text.Set(iFile);
	text.Read(buf);
	lex.Assign(buf);
	lex.Val(iPosition.iX);

	text.Read(buf);
	lex.Assign(buf);
	lex.Val(iPosition.iY);

	text.Read(buf);
	lex.Assign(buf);
	lex.Val(iPosition.iWidth);

	text.Read(buf);
	lex.Assign(buf);
	lex.Val(iPosition.iHeight);

	text.Read(buf);
	lex.Assign(buf);
	lex.Val(iFontSize);

	iFile.Close();
	}
	}
void CWindow::SaveConfig()
    {
    if(iFile.Create(iFs,KFilePos,EFileWrite) == KErrNone)
    {

    }
    }

/*void CWindow::SetText(const TInt& aNumText)
	{
	iText.Delete(0,iText.Length());
	iText.AppendNum(aNumText);
	}*/
void CWindow::GetFontSize(const TDesC& aText)
	{
	//__LOGSTR1("Get font size of text: %S",&aText);
	iFontName.FillZ(KMaxTypefaceNameLength);
	iScreen->TypefaceSupport(iTypefaceSupport, 0);
	iFontName = iTypefaceSupport.iTypeface.iName;
	// get font
	iFontSpec = TFontSpec(iFontName, 10*iFontSize);
	iFontSpec.iTypeface.SetIsProportional(ETrue);
	iScreen->GetNearestFontInTwips(iFont,iFontSpec);

	iSize=TSize(iFont->TextWidthInPixels(aText),iFont->HeightInPixels());//+1);
	//__LOGSTR2("iSize.width: %d, iSize.height: %d",iSize.iWidth,iSize.iHeight);
	}
void CWindow::LoadImage()
	{
    __LOGSTR("Load image");
    if(BaflUtils::FileExists(iFs,KFileImagePath) == EFalse)
    	{
    	__LOGSTR("File not exist");
    	return ;
    	}
    iBitmap = new(ELeave) CFbsBitmap();
    //iBitmapMask = new (ELeave) CFbsBitmap();
    _CPUSH(iBitmap);//+2 в стек, если картинка есть
    CImageDecoder* decoder = CImageDecoder::FileNewL(iFs,KFileImagePath);
    TFrameInfo frameInfo = decoder->FrameInfo(0);
    iImageRect = frameInfo.iFrameCoordsInPixels;
    iBitmap->Create(iImageRect.Size(),EColor16M);
    //iBitmapMask->Create(iImageRect.Size(),EColor16M);
    decoder->Convert(&iStatus,*iBitmap);

    SetActive();
    iSchedulerWait->Start();
    _CPOP(iBitmap);
    __LOGSTR("Image converted");
	}
void CWindow::RunL()
    {
    __LOGSTR("RunL");
    if(iSchedulerWait->IsStarted())
	{
            __LOGSTR1("ImageConvert status: %d",iStatus.Int());
    if(iStatus.Int() == KErrNone)
	    {
		__LOGSTR2("bitmapHeight: %d, bitmapWidth: %d",iBitmap->SizeInPixels().iHeight,iBitmap->SizeInPixels().iWidth);
	    iWindow.SetExtent(TPoint(iPosition.iX,iPosition.iY),iBitmap->SizeInPixels());
		iPosition.iWidth = iBitmap->SizeInPixels().iWidth;
		iPosition.iHeight = iBitmap->SizeInPixels().iHeight;
	    iImageIsLoaded = ETrue;
	    }
	iSchedulerWait->AsyncStop();
	//iWindow.SetVisible(EFalse);
	}else{
	GetWgEvent();
	}
    }
TInt CWindow::RunError(TInt aError)
    {
    __LOGSTR1("RunError: %d",aError);
    return KErrNone;
    }
void CWindow::DoCancel()
    {
        iWs.EventReadyCancel();
    }
void CWindow::SetVisible(TBool aIsVisible)
	{
        iWindow.SetVisible(aIsVisible);
        //iWg.EnableReceiptOfFocus(aIsVisible);
        iIsVisible = aIsVisible;
        iWs.Flush();
	}
TBool CWindow::IsVisible() const
    {
        return iIsVisible;
    }
void CWindow::SetReceiptOfFocus(TBool aIsReceiptOfFocus)
    {
        iWg.EnableReceiptOfFocus(aIsReceiptOfFocus);
        iWs.Flush();
    }

void CWindow::SetNeedToRecall(TBool aIsNeed)
	{
        iIsNeedToReCall = aIsNeed;
	}
TBool CWindow::GetNeedToRecall()
	{
        return iIsNeedToReCall;
	}
void CWindow::GetWgEvent()
    {
    __LOGSTR1("GetWgEvent,iStatus: %d",iStatus.Int());
    if(iStatus == KErrNone)
    	{
    	TWsEvent e;
    	iWs.GetEvent(e);
    	__LOGSTR1("event: %d",e.Type());
    	if(iWorkMode == EModeNormal)
    		{
    	switch(e.Type())
    		{
#ifndef __3RD__
    		case EEventPointer:
    		case EEventPointerEnter:
    			{
    			__LOGSTR("Tapped");
    			//e.Pointer()->iPosition
    			//Тапнули
                SetVisible(EFalse);
                SetReceiptOfFocus(EFalse);
                //iWs.Flush();
    			//HangUpAT();
    			HangUpEtel();
    			SetNeedToRecall(ETrue);
    			break;
    			}
#endif
#ifdef __3RD__
        	case EEventFocusGained:
        		__LOGSTR("Focus gained");
        		iIsFocus = ETrue;
                //iColor = KRgbGreen;
        		break;
        	case EEventFocusLost:
        		__LOGSTR("Focus lost");
        		iIsFocus = EFalse;
                //iColor = KRgbRed;
        		break;
        	case EEventKeyDown:
        		__LOGSTR2("EEventKeyDown,iIsFocus: %d,iScanCode: %d",iIsFocus,e.Key()->iScanCode);
        		if(iIsFocus && (e.Key()->iScanCode == 167))
        			{
        			iWindow.SetVisible(EFalse);
        			iWs.Flush();
        			//HangUpEtel();
        			HangUpAT();
        			SetNeedToRecall(ETrue);
        			}
        		break;
#endif
    		default:
    	    	Draw();
    			break;
    		}
    		}else{
    			switch(e.Type())
                    {
                /*case EEventPointerEnter:
                {
                    __LOGSTR("Start timer");
                    iTimer->StartTimer();
                    break;
                }
                case EEventPointerExit:
                {
                    __LOGSTR("Stop Timer");
                    iTimer->StopTimer();
                    break;
                }*/
                 case EEventPointer:
                 {
                     __LOGSTR("Event pointer");
                    //iWorkMode = EModeNormal;
                     if(Abs(e.Pointer()->iPosition.iX - iOldpos.iX) > 5 ||
                        Abs(e.Pointer()->iPosition.iY - iOldpos.iY) > 5)
                     {
                        //Если тапнули дальше, чем в 5 пикселах от старой позиции, обновить картинку
                         iOldpos = e.Pointer()->iPosition;
                         Draw_pos();
                    }else{
                        //Иначе остановить
                        if(!iTimer->IsActive())
                        {
                            iTimer->StartTimer();
                        }else{
                            iTimer->StopTimer();
                            iWorkMode = EModeNormal;
                            SaveConfig();
                        }
                     }
                     break;
                 }
    				case EEventKey:
    					{
    					__LOGSTR1("Key code: %d",e.Key()->iScanCode);
    					if(e.Key()->iScanCode == EStdKeyYes)
    						{
    							iWorkMode = EModeNormal;
    							SetVisible(EFalse);
                                SetReceiptOfFocus(EFalse);
    							Cancel();
                                CreateConfigFile(iCurrentPos.iX,iCurrentPos.iY,iSize.iWidth,iSize.iHeight);
                                iPosition.iX = iCurrentPos.iX;
                                iPosition.iY = iCurrentPos.iY;
    							return ;
    							break;
    						}
    					switch(e.Key()->iScanCode)
    						{
    						case EStdKeyUpArrow:
    							{
    							//движение вверх
    							iPosition.iY -= 30;
    							break;
    							}
    						case EStdKeyDownArrow:
    							{
    							//движение вниз
    							iPosition.iY += 30;
    							break;
    							}
    						case EStdKeyLeftArrow:
    							{
    							//Движение влево
    							iPosition.iX -= 30;
    							break;
    							}
    						case EStdKeyRightArrow:
    							{
    							//Движение вправо
    							iPosition.iX += 30;
    							break;
    							}
    						}

    					break;
    					}
    				default:
    					{
    					__LOGSTR1("Even type: %d",e.Type());
    					break;
    					}
    				}
    		}
    	WaitWgEvent();
    	iWs.Flush();
    	}
    }
void CWindow::HangUp()
	{
	__LOGSTR("HangUpKey");
	TInt aScan = EStdKeyNo;
	//RWsSession ws;
	//ws.Connect();
	TRawEvent RawEvent;
	RawEvent.Set(TRawEvent::EKeyDown,aScan);
	iWs.SimulateRawEvent(RawEvent);
	User::After(100000);

	__LOGSTR("simulate 100000");

	RawEvent.Set(TRawEvent::EKeyUp,aScan);
	iWs.SimulateRawEvent(RawEvent);

	__LOGSTR1("SimulateKeyEvent: %d",aScan);

	//ws.Close();
	}
void CWindow::HangUpEtel()
	{
	__LOGSTR("HangupEtel");
	RTelServer iTelServer;
	RPhone iPhone;
	RLine iLine;
	RCall iCall;
	TName callName;
	RTelServer::TPhoneInfo phoneInfo;
	RLine::TLineInfo lineInfo;
	RPhone::TLineInfo plineInfo;
	RLine::TCallInfo callInfo;
	__LOGSTR("Going hangup");
	User::LeaveIfError(iTelServer.Connect());
	User::LeaveIfError(iTelServer.GetPhoneInfo(0, phoneInfo));
	User::LeaveIfError(iPhone.Open(iTelServer, phoneInfo.iName));
	//TInt numberLines = 0;
	//User::LeaveIfError(iPhone.EnumerateLines(numberLines));
	//if(numberLines == 0)
		//{
		User::LeaveIfError(iPhone.GetLineInfo(0, plineInfo));
		User::LeaveIfError(iLine.Open(iPhone, plineInfo.iName));
		User::LeaveIfError(iLine.GetCallInfo(0,callInfo));
		User::LeaveIfError(iLine.GetInfo(lineInfo));
		callName.Copy(lineInfo.iNameOfCallForAnswering);
		User::LeaveIfError(iCall.OpenExistingCall(iLine, callName));
		User::LeaveIfError( iCall.HangUp() );
		iCall.Close();
		iLine.Close();
		//}
	iPhone.Close();
	iTelServer.Close();
	}
void CWindow::HangUpAT()
	{
	__LOGSTR("HangUpAT");
	iSender->SendAT();
	}
/*void CWindow::RunCall()
	{
	RProcess proc;
	TInt err = proc.Create(_L("FastCallBackDial.exe"),KNullDesC);
	__LOGSTR1("Run process FastCallBackDial.exe: %d",err);
	proc.Resume();
	}*/
void CWindow::WaitWgEvent()
    {
    iWg.EnableFocusChangeEvents();
    //iWg.EnableGroupListChangeEvents();
    __LOGSTR("WaitWgEvent");
    iWs.EventReady(&iStatus);
    SetActive();
    }
void CWindow::Draw()
	{
	__LOGSTR("CWindow::Draw");
	iBackgroundColor = KRgbBlack;

    SetVisible(ETrue);
    SetReceiptOfFocus(ETrue);//Для обработки нажатий клавиш
    //iWs.Flush();
	//GetFontSize(iText);
	if(iImageIsLoaded == EFalse)
		{
		iWindow.SetPosition(TPoint(iPosition.iX,iPosition.iY));
		iWindow.SetSize(TSize(iSize.iWidth,iPosition.iHeight));
		//iWindow.SetVisible(ETrue);
		iGc->Activate(iWindow);
		iRect = TRect(TPoint(0,0),TSize(iSize.iWidth,iPosition.iHeight));
		iWindow.Invalidate();
		iWindow.BeginRedraw();
		iGc->Clear();
		iGc->SetPenColor(iTextColor);
		iGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
		iGc->UseFont(iFont);
		iGc->SetBrushColor(iBackgroundColor);
		iGc->Clear(iRect);
		iGc->DrawText(iText,iRect/*TRect(0,0, iRect.Width(),iRect.Height())*/,iFont->FontMaxAscent(), CGraphicsContext::ECenter, 0);
		iWindow.EndRedraw();
		iGc->Deactivate();
		iWs.Flush();
		}else{
			iWindow.SetPosition(TPoint(iPosition.iX,iPosition.iY));
			iWindow.SetSize(TSize(iSize.iWidth,iPosition.iHeight));
		    iGc->Activate(iWindow);
		    iWindow.BeginRedraw(iImageRect);
		    iGc->Clear();
            //iGc->DrawBitmap(iImageRect,iBitmap);
            //iGc->DrawBitmapMasked(iImageRect,iBitmap,iImageRect,iBitmapMask,EFalse);
		    iWindow.EndRedraw();
		    iGc->Deactivate();
		    iWs.Flush();
		}
	}
CWindow::EWorkMode CWindow::GetWorkMode()
	{
	return iWorkMode;
	}
void CWindow::NeedSetPos()
	{
	__LOGSTR("CWindow::NeedSetPos");
	iWorkMode = EModeSetpos;
	Draw_pos();
	WaitWgEvent();
	}
void CWindow::Draw_pos()
	{
	__LOGSTR("CWindow::Draw");
		iBackgroundColor = KRgbBlack;
		TRgb backgroundColour;
	    if(KErrNone == iWindow.SetTransparencyAlphaChannel())
	    	{
                backgroundColour.SetAlpha(0);
	    	}
	    iWindow.SetBackgroundColor(backgroundColour);
        SetVisible(ETrue);
        SetReceiptOfFocus(ETrue);//Для обработки нажатий клавиш
        iWindow.SetPosition(TPoint(0,0));//В левый верхний угол
        TPixelsAndRotation pixelsAndRotation;//Получение разрешения дисплея
        iScreen->GetDefaultScreenSizeAndRotation(pixelsAndRotation);
        iWindow.SetSize(pixelsAndRotation.iPixelSize);//Установка размеров окна
        iWs.Flush();//Сброс буфера на всякий
        iGc->Activate(iWindow);//Активация контекста графики в окне
		iWindow.Invalidate();
        iWindow.BeginRedraw();//Начало перерисовки
		iGc->Clear();
        iGc->SetPenColor(iRectColor);
		iGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
		iGc->SetBrushColor(backgroundColour);
        TRect rect(TPoint(iOldpos.iX - 50,iOldpos.iY - 50),TSize(iPosition.iWidth,iPosition.iHeight));
        //iGc->DrawBitmapMasked();
		iGc->DrawRect(rect);
		iWindow.EndRedraw();
		iGc->Deactivate();
		iWs.Flush();
        iCurrentPos = iOldpos;
		//GetFontSize(iText);
		/*	iWindow.SetVisible(ETrue);
			iGc->Activate(iWindow);
			iRect = TRect(TPoint(0,0),TSize(iSize.iWidth,iPosition.iHeight));
			iWindow.Invalidate();
			iWindow.BeginRedraw();
			iGc->Clear();
			iGc->SetPenColor(iTextColor);
			iGc->SetBrushStyle(CGraphicsContext::ESolidBrush);
			iGc->UseFont(iFont);
			iGc->SetBrushColor(iBackgroundColor);
            iGc->Clear(iRect);
            //iGc->DrawText(iText,iRect,iFont->FontMaxAscent(), CGraphicsContext::ECenter, 0);
            //iWindow.EndRedraw();
            //iGc->Deactivate();
            //iWs.Flush();
            }else{
				TRect rect;
			    iGc->Activate(iWindow);
			    //iGc->Clear();
			    //iGc->Clear(rect);
			    iWindow.Invalidate(iImageRect);
			    iWindow.BeginRedraw(iImageRect);
			    iGc->Clear(iImageRect);
			    TSize size = iImageRect.Size();
			    rect = TRect(iOldpos,TSize(108,108));
			    iGc->DrawBitmap(rect,iBitmap);
			    iWindow.EndRedraw();
			    iGc->Deactivate();
			    iImageRect = rect;
			    iWs.Flush();
			}*/
	}
void CWindow::TimerExpired()
    {
        __LOGSTR("Timer expired!");
        iRectColor = KRgbMagenta;
        SetVisible(EFalse);
        SetReceiptOfFocus(EFalse);
    }
