#include <Windows.h>
#include <stdio.h>
#include "resource.h"
#define WM_TIMER1 WM_USER+5 
#define WM_TIMER2 WM_USER+6
#define WM_TIMER3 WM_USER+7
bool IsDamage = false;//是否修改游戏的伤害计算代码
HWND hwndPVZ;	//游戏的窗口句柄
HWND hwndClient;	//当前程序的窗口句柄
bool IsGameRun;		//游戏是否运行
DWORD ProcId;		//游戏的进程ID
DWORD ThreadId;		//游戏窗口的线程ID
HANDLE hProc;		//游戏的进程句柄
HANDLE hThread;		//游戏窗口的线程句柄
DWORD cdAddr;//冷却时间的内存地址
const DWORD tmp = 5000;//冷却时间设为5000即无cd
const DWORD BaseAddr = 0x006a9ec0;	//基址
bool IsSetTimer = false;	//是否使程序窗口吸附于游戏
bool IsNoCD = false;	//是否开启无cd模式
HHOOK hHook;	//监视游戏窗口钩子的句柄
//RECT WndRect;	//窗口的位置大小
BOOL CALLBACK MainProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);//主窗口处理函数
DWORD ThreadProc(LPVOID lParam);//修改阳光内存函数
VOID CALLBACK TimerProc(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime);//查看阳光函数——用于定时器，每0.5秒函数体执行一次
VOID CALLBACK WndProc(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime);
DWORD ChangeMoney(LPVOID lParam);//修改金钱函数
void CalcCDAddr();//计算冷却时间的地址
VOID CALLBACK CDTimer(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime);//定时器，用于循环修改冷却时间
DWORD WINAPI KillAll(LPVOID lParam);	//创建线程执行杀死所有僵尸的函数
DWORD WINAPI SetDamage(LPVOID lParam);	//创建线程执行
/*	对僵尸造成伤害计算的汇编代码
	PVZ+131319	00531319	89 8D C8000000	mov [ebp+000000C8],edi	->	89 9D C8000000
	普通僵尸	修改地址:00531319+1=0053131a
	PVZ+13104D	0053104D	89 8D D0000000	mov [ebp+000000D0],ecx  ->	89 95 D0000000
	路障与铁桶	修改地址:0053104d+1=0053104e
	PVZ+130CA1	00530CA1	29 86 DC000000	sub [esi+000000DC],eax	->	29 BE DC000000
	铁门与梯子	修改地址:00530CA1+1=00530CA2
	所有修改处大小均为1字节
*/

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	HANDLE hToken;
	if(OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&hToken)){
		TOKEN_PRIVILEGES tp;
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if(LookupPrivilegeValue(NULL,SE_DEBUG_NAME,&tp.Privileges[0].Luid)){
			AdjustTokenPrivileges(hToken,FALSE,&tp,sizeof(tp),NULL,NULL);
		}
		CloseHandle(hToken);
	}
	IsGameRun = false;
	DialogBox(hInstance,MAKEINTRESOURCE(IDD_DIALOG1),NULL,MainProc);
	return 0;
}

BOOL CALLBACK MainProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			/////////////////////////////////////////////////////////
			hwndPVZ = FindWindow("MainWindow","植物大战僵尸中文版");
			if(hwndPVZ == NULL)
			{
				IsGameRun = false;
				break;
			}
			ShowWindow(GetDlgItem(hWnd,IDC_STATIC2),SW_SHOW);
			IsGameRun = true;
			ThreadId = GetWindowThreadProcessId(hwndPVZ,&ProcId);
			hProc = OpenProcess(PROCESS_ALL_ACCESS,false,ProcId);//获取游戏进程句柄
			if(hProc == NULL)
				return 0;
			SetTimer(hWnd,WM_TIMER1,500,TimerProc);
			hwndClient = hWnd;
			//获取窗口位置
			RECT WndRect;
			GetWindowRect(hwndPVZ,&WndRect);
			long temp = WndRect.left;
			if((temp|0x0000ffff) == 0xffff)
				SetWindowPos(hWnd,HWND_TOP,WndRect.left-125,WndRect.top,0,0,SWP_NOSIZE|SWP_NOSIZE|SWP_NOZORDER);
			//HINSTANCE hPVZinstance = (HINSTANCE)GetWindowLong(hwndPVZ,GWL_HINSTANCE);
		}
		break;
	case WM_COMMAND:
		{
			if(LOWORD(wParam) == IDC_BUTTON1 && IsGameRun == true)
			{
				DWORD nSunny = GetDlgItemInt(hWnd,IDC_EDIT1,NULL,true);
				//hThread = CreateRemoteThread(hProc,NULL,0,ThreadProc,&nSunny,0,NULL);
				//本来想创建远程线程到植物大战僵尸的主进程下的，显然多此一举了,这样反而会导致游戏崩溃...
				ThreadProc(&nSunny);
			}
			else if(LOWORD(wParam) == IDC_BUTTON2)
			{
				ShowWindow(hwndPVZ,SW_SHOWNORMAL);
				SetWindowPos(hwndPVZ,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE);
			}
			else if(LOWORD(wParam) == IDC_BUTTON3)
			{
				SendMessage(hWnd,WM_INITDIALOG,0,0);
			}
			else if(LOWORD(wParam) == IDC_BUTTON4)
			{
				DWORD nMoney = GetDlgItemInt(hWnd,IDC_EDIT1,NULL,true);
				ChangeMoney(&nMoney);
			}
			else if(LOWORD(wParam) == IDC_CHECK1)
			{
				LRESULT r = SendMessage((HWND)lParam,BM_GETCHECK,0,0);
				if(r == BST_CHECKED)
				{
					IsSetTimer = true;
					SetTimer(hWnd,WM_TIMER2,10,WndProc);
					ShowWindow(hWnd,SW_SHOWNORMAL);
				}
				else
				{
					IsSetTimer = false;
					KillTimer(hWnd,WM_TIMER2);
				}
			}
			else if (LOWORD(wParam) == IDC_BUTTON5)	//秒杀模式
			{
				CreateThread(NULL, 0, KillAll, NULL, 0, NULL);
			}
			else if(LOWORD(wParam) == IDC_CHECK2)	//无CD模式
			{
				LRESULT r = SendMessage((HWND)lParam,BM_GETCHECK,0,0);
				if(r == BST_CHECKED)
				{
					IsNoCD = true;
					CalcCDAddr();
					SetTimer(hWnd,WM_TIMER3,500,CDTimer);
				}
				else
				{
					KillTimer(hwndClient,WM_TIMER3);
					IsNoCD = false;
				}
			}
			break;
		}
	case WM_CLOSE:
		{
			EndDialog(hWnd,0);
			break;
		}
	default:
		break;
	}
	return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

DWORD ThreadProc(LPVOID lParam)
{
	DWORD ObjAdd = BaseAddr;
	DWORD nValue;
	ReadProcessMemory(hProc,(void*)ObjAdd,&nValue,4,NULL);
	ObjAdd = nValue + 0x768;
	ReadProcessMemory(hProc,(void*)ObjAdd,&nValue,4,NULL);
	ObjAdd = nValue + 0x5560;
	WriteProcessMemory(hProc,(void*)ObjAdd,lParam,4,NULL);
	//CloseHandle(hProc);
	return 0;
}

VOID CALLBACK TimerProc(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime)
{
	KillTimer(hwnd,WM_TIMER1);
	DWORD ObjAdd = BaseAddr;
	DWORD nValue;
	ReadProcessMemory(hProc,(void*)ObjAdd,&nValue,4,NULL);
	ObjAdd = nValue + 0x768;
	ReadProcessMemory(hProc,(void*)ObjAdd,&nValue,4,NULL);
	ObjAdd = nValue + 0x5560;
	ReadProcessMemory(hProc,(void*)ObjAdd,&nValue,4,NULL);
	char szSun[20];
	char szTemp[25] = "阳光:";
	itoa(nValue,szSun,10);
	strcat(szTemp,szSun);
	if(nValue > 999999)
	{
		strcpy(szTemp,"游戏停止运行");
		IsGameRun = false;
		SendMessage(hwndClient, WM_CLOSE, 0, 0);
	}
	SetDlgItemText(hwnd,IDC_EDIT2,szTemp);
	SetTimer(hwnd,WM_TIMER1,500,TimerProc);
}

VOID CALLBACK WndProc(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime)
{
	KillTimer(hwnd,WM_TIMER2);
	if(IsSetTimer == true && IsGameRun == true)
	{
		RECT rect;
		GetWindowRect(hwndPVZ,&rect);
		SetWindowPos(hwnd,HWND_TOP,rect.left-125,rect.top,0,0,SWP_NOSIZE|SWP_NOZORDER);
		SetTimer(hwnd,WM_TIMER2,10,WndProc);
	}
}

DWORD ChangeMoney(LPVOID lParam)
{
	DWORD ObjAdd = BaseAddr;
	DWORD nValue;
	ReadProcessMemory(hProc,(void*)ObjAdd,&nValue,4,NULL);
	ObjAdd = nValue+0x82c;
	ReadProcessMemory(hProc,(void*)ObjAdd,&nValue,4,NULL);
	ObjAdd = nValue+0x28;
	WriteProcessMemory(hProc,(void*)ObjAdd,lParam,4,NULL);
	return 0;
}

void CalcCDAddr()
{
	DWORD ObjAdd = BaseAddr;
	DWORD nValue;
	ReadProcessMemory(hProc,(void*)ObjAdd,&nValue,4,NULL);
	ObjAdd = nValue+0x768;
	ReadProcessMemory(hProc,(void*)ObjAdd,&nValue,4,NULL);
	ObjAdd = nValue+0x1C;
	ReadProcessMemory(hProc,(void*)ObjAdd,&nValue,4,NULL);
	ObjAdd = nValue+0x140;
	ReadProcessMemory(hProc,(void*)ObjAdd,&nValue,4,NULL);
	ObjAdd = nValue+0x4C;
	cdAddr = ObjAdd;
}

VOID CALLBACK CDTimer(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime)
{
	DWORD tmpAddr = cdAddr - 0x50;
	KillTimer(hwndClient,WM_TIMER3);
	if(IsGameRun == false)
	{
		return;
	}
	for(int i=0;i<10;i++)
	{
		tmpAddr = tmpAddr + 0x50;
		WriteProcessMemory(hProc,(void*)tmpAddr,&tmp,4,NULL);//遍历卡槽写入cd=5000
	}
	SetTimer(hwndClient,WM_TIMER3,500,CDTimer);
}


DWORD WINAPI SetDamage(LPVOID lParam)//可以修改成植物一击秒杀僵尸，但是需要修改游戏程序的代码
{
	if ((int)lParam == 1)
	{
		BYTE tmp = 0x9D;
		WriteProcessMemory(hProc, (void*)0x0053131a, &tmp, 1, NULL);
		tmp = 0x95;
		WriteProcessMemory(hProc, (void*)0x0053104e, &tmp, 1, NULL);
		tmp = 0xBE;
		WriteProcessMemory(hProc, (void*)0x00530CA2, &tmp, 1, NULL);
	}
	else
	{
		BYTE tmp = 0x8D;
		WriteProcessMemory(hProc, (void*)0x0053131a, &tmp, 1, NULL);
		tmp = 0x8D;
		WriteProcessMemory(hProc, (void*)0x0053104e, &tmp, 1, NULL);
		tmp = 0x86;
		WriteProcessMemory(hProc, (void*)0x00530CA2, &tmp, 1, NULL);
	}
	return 0;
}

DWORD WINAPI KillAll(LPVOID lParam)
{
	DWORD ObjAdd = BaseAddr;
	DWORD nValue;
	DWORD nNum;//怪物最大数量
	const DWORD Data = 3;//=3时为秒杀
	ReadProcessMemory(hProc, (void*)ObjAdd, &nValue, 4, NULL);
	ObjAdd = nValue + 0x768;
	ReadProcessMemory(hProc, (void*)ObjAdd, &nValue, 4, NULL);
	ObjAdd = nValue + 0x90;
	nNum = nValue + 0x94;
	ReadProcessMemory(hProc, (void*)nNum, &nNum, 4, NULL);
	ReadProcessMemory(hProc, (void*)ObjAdd, &nValue, 4, NULL);
	ObjAdd = nValue + 0x28;
	for (int i = 0; i < nNum; i++)
	{
		ObjAdd = ObjAdd + 0x15c;
		WriteProcessMemory(hProc, (void*)ObjAdd, &Data, 4, NULL);
	}
	SetFocus(hwndPVZ);
	SetWindowPos(hwndPVZ, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	SendMessage(hwndPVZ, WM_ACTIVATEAPP, 0, 0);
	return 0;
}