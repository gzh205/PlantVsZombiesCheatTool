#include <Windows.h>
#include <stdio.h>
#include "resource.h"
#define WM_TIMER1 WM_USER+5 
#define WM_TIMER2 WM_USER+6
#define WM_TIMER3 WM_USER+7
bool IsDamage = false;//�Ƿ��޸���Ϸ���˺��������
HWND hwndPVZ;	//��Ϸ�Ĵ��ھ��
HWND hwndClient;	//��ǰ����Ĵ��ھ��
bool IsGameRun;		//��Ϸ�Ƿ�����
DWORD ProcId;		//��Ϸ�Ľ���ID
DWORD ThreadId;		//��Ϸ���ڵ��߳�ID
HANDLE hProc;		//��Ϸ�Ľ��̾��
HANDLE hThread;		//��Ϸ���ڵ��߳̾��
DWORD cdAddr;//��ȴʱ����ڴ��ַ
const DWORD tmp = 5000;//��ȴʱ����Ϊ5000����cd
const DWORD BaseAddr = 0x006a9ec0;	//��ַ
bool IsSetTimer = false;	//�Ƿ�ʹ���򴰿���������Ϸ
bool IsNoCD = false;	//�Ƿ�����cdģʽ
HHOOK hHook;	//������Ϸ���ڹ��ӵľ��
//RECT WndRect;	//���ڵ�λ�ô�С
BOOL CALLBACK MainProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam);//�����ڴ�����
DWORD ThreadProc(LPVOID lParam);//�޸������ڴ溯��
VOID CALLBACK TimerProc(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime);//�鿴���⺯���������ڶ�ʱ����ÿ0.5�뺯����ִ��һ��
VOID CALLBACK WndProc(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime);
DWORD ChangeMoney(LPVOID lParam);//�޸Ľ�Ǯ����
void CalcCDAddr();//������ȴʱ��ĵ�ַ
VOID CALLBACK CDTimer(HWND hwnd,UINT uMsg,UINT_PTR idEvent,DWORD dwTime);//��ʱ��������ѭ���޸���ȴʱ��
DWORD WINAPI KillAll(LPVOID lParam);	//�����߳�ִ��ɱ�����н�ʬ�ĺ���
DWORD WINAPI SetDamage(LPVOID lParam);	//�����߳�ִ��
/*	�Խ�ʬ����˺�����Ļ�����
	PVZ+131319	00531319	89 8D C8000000	mov [ebp+000000C8],edi	->	89 9D C8000000
	��ͨ��ʬ	�޸ĵ�ַ:00531319+1=0053131a
	PVZ+13104D	0053104D	89 8D D0000000	mov [ebp+000000D0],ecx  ->	89 95 D0000000
	·������Ͱ	�޸ĵ�ַ:0053104d+1=0053104e
	PVZ+130CA1	00530CA1	29 86 DC000000	sub [esi+000000DC],eax	->	29 BE DC000000
	����������	�޸ĵ�ַ:00530CA1+1=00530CA2
	�����޸Ĵ���С��Ϊ1�ֽ�
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
			hwndPVZ = FindWindow("MainWindow","ֲ���ս��ʬ���İ�");
			if(hwndPVZ == NULL)
			{
				IsGameRun = false;
				break;
			}
			ShowWindow(GetDlgItem(hWnd,IDC_STATIC2),SW_SHOW);
			IsGameRun = true;
			ThreadId = GetWindowThreadProcessId(hwndPVZ,&ProcId);
			hProc = OpenProcess(PROCESS_ALL_ACCESS,false,ProcId);//��ȡ��Ϸ���̾��
			if(hProc == NULL)
				return 0;
			SetTimer(hWnd,WM_TIMER1,500,TimerProc);
			hwndClient = hWnd;
			//��ȡ����λ��
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
				//�����봴��Զ���̵߳�ֲ���ս��ʬ���������µģ���Ȼ���һ����,���������ᵼ����Ϸ����...
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
			else if (LOWORD(wParam) == IDC_BUTTON5)	//��ɱģʽ
			{
				CreateThread(NULL, 0, KillAll, NULL, 0, NULL);
			}
			else if(LOWORD(wParam) == IDC_CHECK2)	//��CDģʽ
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
	char szTemp[25] = "����:";
	itoa(nValue,szSun,10);
	strcat(szTemp,szSun);
	if(nValue > 999999)
	{
		strcpy(szTemp,"��Ϸֹͣ����");
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
		WriteProcessMemory(hProc,(void*)tmpAddr,&tmp,4,NULL);//��������д��cd=5000
	}
	SetTimer(hwndClient,WM_TIMER3,500,CDTimer);
}


DWORD WINAPI SetDamage(LPVOID lParam)//�����޸ĳ�ֲ��һ����ɱ��ʬ��������Ҫ�޸���Ϸ����Ĵ���
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
	DWORD nNum;//�����������
	const DWORD Data = 3;//=3ʱΪ��ɱ
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