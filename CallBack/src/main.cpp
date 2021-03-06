/*
 * AutoAnswer : main.cpp
 *
 *  Created on: 30.04.2011
 *  Author: Vladislav
 */
#include <e32base.h>
#include "act.h"
#include "observer.h"
#include "logger1.h"
#include "Window.h"
#include "property.h"

_LIT(KName,"CallBack.exe*");

LOCAL_C void WorkL()
	{
	CWindow* window = CWindow::NewLC();
	CMyObserverClass* observer = CMyObserverClass::NewLC(*window);
	CPhoneReceiver* receiver = CPhoneReceiver::NewLC(*observer);
	receiver->StartL();
	CActiveScheduler::Start();
	_CPOPD(3);
	}
LOCAL_C void RunL()
	{
	/*TTime time;
	time.HomeTime();
	if(time.DateTime().Day() > 15 || time.DateTime().Month() > EJuly)
		return;*/
    RProcess proc;
    TInt val;
    TBuf<5> buf2;
    TLex lex;
    buf2.Copy(proc.FullName().Right(1));
    lex.Assign(buf2);
    lex.Val(val);

    if(val > 1)
	{
   /* TBuf<256> buf;
    TFullName result;
    TFindProcess procFinder;
    buf.Copy(KName);
    buf.Append(_L("0"));
    buf.Append(_L("0"));
    buf.Append(_L("0"));

    for(TInt i = 1;i <= val; i++)
	{
	buf.AppendNum(i);
	__LOGSTR1("buf = %S",&buf);
	procFinder = buf;
	while(procFinder.Next(result) == KErrNone)
	    {
	    proc.Open(procFinder,EOwnerProcess);
	    proc.Terminate(EExitTerminate);
	    proc.Close();
	    }
	buf.Delete(buf.Length()-1,1);
	}*/
    	RProperty prop;
    	prop.Attach(PropertyUID,0x01,EOwnerThread);
    	prop.Set(1);
	}else{
	WorkL();
	}
	}
LOCAL_C void SavePanic(TInt aError)
	{
	__LOGSTR1("Panic: %d",aError);
	}
GLDEF_C TInt E32Main()
	{
	CActiveScheduler* shed = new(ELeave) CActiveScheduler();
	CActiveScheduler::Install(shed);
	__UHEAP_MARK;
	CTrapCleanup* cleanup = CTrapCleanup::New();
	TRAPD(error,RunL());
	__ASSERT_ALWAYS(!error,SavePanic(error));
	delete cleanup;
	__UHEAP_MARKEND;
	return KErrNone;
	}
