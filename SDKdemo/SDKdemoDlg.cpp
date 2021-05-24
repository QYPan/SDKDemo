
// SDKdemoDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "SDKdemo.h"
#include "SDKdemoDlg.h"
#include "afxdialogex.h"
#include "AGEventDef.h"

#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CSDKdemoDlg 对话框



CSDKdemoDlg::CSDKdemoDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SDKDEMO_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSDKdemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_CONFIRM, m_btnConfirm);
	DDX_Control(pDX, IDC_BUTTON_QUIT, m_btnQuit);
	DDX_Control(pDX, IDC_EDIT_CHANNEL_NAME, m_edtChannelName);
}

BEGIN_MESSAGE_MAP(CSDKdemoDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_CONFIRM, &CSDKdemoDlg::OnBnClickedConfirm)
	ON_BN_CLICKED(IDC_BUTTON_QUIT, &CSDKdemoDlg::OnBnClickedButtonQuit)
	ON_EN_CHANGE(IDC_EDIT_CHANNEL_NAME, &CSDKdemoDlg::OnEnChangeEditChannelName)

	ON_MESSAGE(MSGID_JOIN_CHANNEL_SUCCESS, &CSDKdemoDlg::OnJoinChannelSuccess)
	ON_MESSAGE(MSGID_LEAVE_CHANNEL, &CSDKdemoDlg::OnLeaveChannel)
	ON_MESSAGE(MSGID_USER_JOINED, &CSDKdemoDlg::onUserJoined)
	ON_MESSAGE(MSGID_USER_OFFLINE, &CSDKdemoDlg::onUserOffline)
END_MESSAGE_MAP()


// CSDKdemoDlg 消息处理程序

BOOL CSDKdemoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	initCtrls();
	initData();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CSDKdemoDlg::initCtrls()
{
	CRect		rcDesktop;
	CRect		rc;

	CWnd	*lpWndDesktop = GetDesktopWindow();
	lpWndDesktop->GetWindowRect(&rcDesktop);

	m_width = rcDesktop.Width() / 4;
	m_height = m_width * 1.2;

	MoveWindow(0, 0, m_width, m_height, TRUE);
	CenterWindow();
	GetWindowRect(rc);

	m_btnConfirm.SetWindowText(_T("Join"));
	m_btnConfirm.MoveWindow(rc.Width() / 2 - 60, rc.Height() - 150, 120, 40, TRUE);

	m_btnQuit.SetWindowText(_T("Quit"));
	m_btnQuit.MoveWindow(rc.Width() / 2 - 60, rc.Height() - 100, 120, 40, TRUE);

	m_edtChannelName.SetWindowText(_T("ChannelName"));
	m_edtChannelName.MoveWindow(rc.Width() / 2 - 90, rc.Height() / 2.5, 180, 45, TRUE);

	m_localView.Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPCHILDREN, CRect(0, 0, 1, 1), this, 1234);
	m_remoteView.Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPCHILDREN, CRect(0, 0, 1, 1), this, 1235);

	m_engineEventHandler.setMainWnd(GetSafeHwnd());
}

void CSDKdemoDlg::initData()
{
	static bool done = false;
	if(!done)
	{
		m_inChannel = false;
		m_lpRtcEngine = createAgoraRtcEngine();

		RtcEngineContext ctx;

		ctx.eventHandler = &m_engineEventHandler;
		ctx.appId = "aab8b8f5a8cd4469a63042fcfafe7063";

		m_lpRtcEngine->initialize(ctx);

		done = true;
	}
}

void CSDKdemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CSDKdemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CSDKdemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CSDKdemoDlg::adjustVideoViews(bool local, bool remote)
{
	CRect rect;
	CRect rc;
	GetWindowRect(&rect);
	rc.CopyRect(&rect);

	rc.top = 25;
	rc.left = 0;
	rc.right = rect.Width();
	rc.bottom = rect.Height() - 150;

	if(local && !remote)
	{
		m_localView.MoveWindow(&rc, TRUE);
		m_localView.ShowWindow(SW_SHOW);
		m_localView.SetParent(this);
	}
	else if(!local && remote)
	{
		m_remoteView.MoveWindow(&rc, TRUE);
		m_remoteView.ShowWindow(SW_SHOW);
		m_remoteView.SetParent(this);
	}
	else if(local && remote)
	{
		CRect rcLocal;
		CRect rcRemote;
		rcLocal.CopyRect(&rc);
		rcLocal.right = rect.Width() / 2 - 10;

		m_localView.MoveWindow(&rcLocal, TRUE);
		m_localView.ShowWindow(SW_SHOW);
		m_localView.SetParent(this);

		rcRemote.CopyRect(&rc);
		rcRemote.left = rect.Width() / 2 + 10;

		m_remoteView.MoveWindow(&rcRemote, TRUE);
		m_remoteView.ShowWindow(SW_SHOW);
		m_remoteView.SetParent(this);
	}
	else
	{
		m_localView.ShowWindow(SW_HIDE);
		m_remoteView.ShowWindow(SW_SHOW);
	}
}

void CSDKdemoDlg::joinChannel()
{
	CRect rcDesktop;
	CRect rc;
	CWnd *lpWndDesktop = GetDesktopWindow();
	lpWndDesktop->GetWindowRect(&rcDesktop);

	int width = rcDesktop.Width() / 3;
	int height = width;

	MoveWindow(0, 0, width, height, TRUE);
	CenterWindow();
	GetWindowRect(rc);
	m_btnConfirm.MoveWindow(rc.Width() / 2 - 60, rc.Height() - 100, 120, 40, TRUE);
	m_btnQuit.ShowWindow(SW_HIDE);
	m_edtChannelName.ShowWindow(SW_HIDE);

	if(m_lpRtcEngine)
	{
		VideoCanvas vc;
		vc.view = m_localView.GetSafeHwnd();
		vc.renderMode = RENDER_MODE_TYPE::RENDER_MODE_FIT;
		m_lpRtcEngine->setupLocalVideo(vc);
		m_lpRtcEngine->enableVideo();
		m_lpRtcEngine->startPreview();

		adjustVideoViews(true, false);

		LPCSTR lpStreamInfo = "{\"owner\":true,\"width\":640,\"height\":480,\"bitrate\":500}";

		CString channelName;
		m_edtChannelName.GetWindowText(channelName);
		m_lpRtcEngine->joinChannel(nullptr, (const char*)CStringA(channelName), lpStreamInfo, 0);
	}

}

void CSDKdemoDlg::leaveChannel()
{
	CRect rc;
	MoveWindow(0, 0, m_width, m_height, TRUE);
	CenterWindow();
	GetWindowRect(rc);
	m_btnConfirm.MoveWindow(rc.Width() / 2 - 60, rc.Height() - 150, 120, 40, TRUE);
	m_btnQuit.MoveWindow(rc.Width() / 2 - 60, rc.Height() - 100, 120, 40, TRUE);
	m_edtChannelName.MoveWindow(rc.Width() / 2 - 90, rc.Height() / 2.5, 180, 45, TRUE);
	m_btnQuit.ShowWindow(SW_SHOW);
	m_edtChannelName.ShowWindow(SW_SHOW);
	m_localView.ShowWindow(SW_HIDE);
	m_remoteView.ShowWindow(SW_HIDE);

	if(m_lpRtcEngine)
	{
		m_lpRtcEngine->leaveChannel();
	}
}

LRESULT CSDKdemoDlg::OnJoinChannelSuccess(WPARAM wParam, LPARAM lParam)
{
	JoinChannelSuccessData *lpData = reinterpret_cast<JoinChannelSuccessData *>(wParam);

	std::string channelName = lpData->channelName;
	unsigned int uid = lpData->uid;
	int elapsed = lpData->elapsed;

	return 0;
}

LRESULT CSDKdemoDlg::OnLeaveChannel(WPARAM wParam, LPARAM lParam)
{
	LeaveChannelData *lpData = reinterpret_cast<LeaveChannelData *>(wParam);

	unsigned int txBytes = lpData->txBytes;
	unsigned int rxBytes = lpData->rxBytes;
	unsigned short txKBitRate = lpData->txKBitRate;
	unsigned short rxKBitRate = lpData->rxKBitRate;

	return 0;
}

LRESULT CSDKdemoDlg::onUserJoined(WPARAM wParam, LPARAM lParam)
{
	UserJoinedData *lpData = reinterpret_cast<UserJoinedData *>(wParam);

	unsigned int uid = lpData->uid;
	int elapsed = lpData->elapsed;

	if(m_lpRtcEngine)
	{
		VideoCanvas	vc;

		vc.uid = uid;
		vc.renderMode = RENDER_MODE_TYPE::RENDER_MODE_FIT;
		vc.view = m_remoteView.GetSafeHwnd();
		vc.priv = NULL;
		m_lpRtcEngine->setupRemoteVideo(vc);
	}

	adjustVideoViews(true, true);

	return 0;
}

LRESULT CSDKdemoDlg::onUserOffline(WPARAM wParam, LPARAM lParam)
{
	UserOfflineData *lpData = reinterpret_cast<UserOfflineData *>(wParam);

	unsigned int uid = lpData->uid;
	int reason = lpData->reason;

	adjustVideoViews(true, false);

	return 0;
}

void CSDKdemoDlg::OnBnClickedConfirm()
{
	// TODO: 在此添加控件通知处理程序代码
	if(m_inChannel)
	{
		leaveChannel();
		m_btnConfirm.SetWindowText(_T("Join"));
		m_inChannel = false;
	}
	else
	{
		joinChannel();
		m_btnConfirm.SetWindowText(_T("Leave"));
		m_inChannel = true;
	}
}


void CSDKdemoDlg::OnBnClickedButtonQuit()
{
	// TODO: 在此添加控件通知处理程序代码
	m_localView.DestroyWindow();
	m_remoteView.DestroyWindow();

	m_lpRtcEngine->release(true);
	CDialogEx::OnCancel();
}


void CSDKdemoDlg::OnEnChangeEditChannelName()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}
