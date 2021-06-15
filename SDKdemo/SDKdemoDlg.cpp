
// SDKdemoDlg.cpp: 实现文件
//

#include "stdafx.h"
#include "SDKdemo.h"
#include "SDKdemoDlg.h"
#include "afxdialogex.h"
#include "AGEventDef.h"

#include "AgoraBase.h"

#include "CAgoraManager.h"
#include "SimpleWindow.h"
#include "WinEnumerImpl.h"
#include "process.h"

#include <fstream>
#include <time.h>
#include <string>
#include <sstream>
#include <gdiplus.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

std::wstring utf82wide(const std::string& utf8) {
  if (utf8.empty()) return std::wstring();
  int len = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), utf8.size(), nullptr, 0);
  wchar_t* buf = new wchar_t[len + 1];
  if (!buf) return std::wstring();
  ZeroMemory(buf, sizeof(wchar_t) * (len + 1));
  MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), utf8.size(), buf, sizeof(wchar_t) * (len + 1));
  std::wstring result(buf);
  delete[] buf;
  return result;
}

std::string wide2ansi(const std::wstring& wide) {
  if (wide.empty()) return std::string();
  int len = WideCharToMultiByte(CP_ACP, 0, wide.c_str(), wide.size(), nullptr, 0, nullptr, nullptr);
  char* buf = new char[len + 1];
  if (!buf) return std::string();
  ZeroMemory(buf, len + 1);
  WideCharToMultiByte(CP_ACP, 0, wide.c_str(), wide.size(), buf, len + 1, nullptr, nullptr);
  std::string result(buf);
  delete[] buf;
  return result;
}

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
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

// CSDKdemoDlg 对话框



CSDKdemoDlg::CSDKdemoDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SDKDEMO_DIALOG, pParent)
{
	srand((unsigned)time(NULL));

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSDKdemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BUTTON_CONFIRM, m_btnConfirm);
	DDX_Control(pDX, IDC_BUTTON_QUIT, m_btnQuit);
	DDX_Control(pDX, IDC_BUTTON_PUBLISH_CAMERA, m_btnPublishCamera);
	DDX_Control(pDX, IDC_BUTTON_PUBLISH_SCREEN, m_btnPublishScreen);
	DDX_Control(pDX, IDC_BUTTON_PREVIEW_CAMERA, m_btnPreviewCamera);
	DDX_Control(pDX, IDC_BUTTON_PREVIEW_SCREEN, m_btnPreviewScreen);
	DDX_Control(pDX, IDC_BUTTON_MUTE_CAMERA, m_btnMuteCamera);
	DDX_Control(pDX, IDC_BUTTON_MUTE_SCREEN, m_btnMuteScreen);
	DDX_Control(pDX, IDC_BUTTON_UPDATE_USERS, m_btnUpdateUsers);
	DDX_Control(pDX, IDC_EDIT_CHANNEL_NAME, m_edtChannelName);
	DDX_Control(pDX, IDC_COMBO_USERS, m_cmbUsers);
	DDX_Control(pDX, IDC_COMBO_CAMERA_LIST, m_cmbCameraList);
	DDX_Control(pDX, IDC_COMBO_MIC_LIST, m_cmbMicList);
	DDX_Control(pDX, IDC_STATIC_AUDIO_DEVICE, m_textAudioDeviceList);
	DDX_Control(pDX, IDC_STATIC_VIDEO_DEVICE, m_textVideoDeviceList);
	DDX_Control(pDX, IDC_CHECK_VIDEO_OBSERVER, m_btnEnableVideoObserver);
	DDX_Control(pDX, IDC_ENUM_WIN, m_btnEnumWin);
	DDX_Control(pDX, IDC_ENUM_DISPLAY, m_btnEnumDisplay);
	DDX_Control(pDX, IDC_PUBLISH_CUSTOM, m_btnPublishCustom);
}

BEGIN_MESSAGE_MAP(CSDKdemoDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_CONFIRM, &CSDKdemoDlg::OnBnClickedConfirm)
	ON_BN_CLICKED(IDC_BUTTON_QUIT, &CSDKdemoDlg::OnBnClickedButtonQuit)
	ON_EN_CHANGE(IDC_EDIT_CHANNEL_NAME, &CSDKdemoDlg::OnEnChangeEditChannelName)
	ON_BN_CLICKED(IDC_BUTTON_PUBLISH_CAMERA, &CSDKdemoDlg::OnBnClickedButtonPublishCamera)
	ON_BN_CLICKED(IDC_BUTTON_PREVIEW_CAMERA, &CSDKdemoDlg::OnBnClickedButtonPreviewCamera)
	ON_CBN_SELCHANGE(IDC_COMBO_USERS, &CSDKdemoDlg::OnCbnSelchangeComboUsers)
	ON_BN_CLICKED(IDC_BUTTON_UPDATE_USERS, &CSDKdemoDlg::OnBnClickedButtonUpdateUsers)
	ON_BN_CLICKED(IDC_BUTTON_PUBLISH_SCREEN, &CSDKdemoDlg::OnBnClickedButtonPublishScreen)
	ON_BN_CLICKED(IDC_BUTTON_PREVIEW_SCREEN, &CSDKdemoDlg::OnBnClickedButtonPreviewScreen)
	ON_BN_CLICKED(IDC_BUTTON_MUTE_CAMERA, &CSDKdemoDlg::OnBnClickedButtonMuteCamera)
	ON_BN_CLICKED(IDC_BUTTON_MUTE_SCREEN, &CSDKdemoDlg::OnBnClickedButtonMuteScreen)
	ON_CBN_SELCHANGE(IDC_COMBO_CAMERA_LIST, &CSDKdemoDlg::OnCbnSelchangeComboCameraList)
	ON_CBN_SELCHANGE(IDC_COMBO_MIC_LIST, &CSDKdemoDlg::OnCbnSelchangeComboMicList)
	ON_STN_CLICKED(IDC_STATIC_AUDIO_DEVICE, &CSDKdemoDlg::OnStnClickedStaticAudioDevice)
	ON_STN_CLICKED(IDC_STATIC_VIDEO_DEVICE, &CSDKdemoDlg::OnStnClickedStaticVideoDevice)
	ON_BN_CLICKED(IDC_CHECK_VIDEO_OBSERVER, &CSDKdemoDlg::OnBnClickedCheckVideoObserver)
	ON_BN_CLICKED(IDC_ENUM_DISPLAY, &CSDKdemoDlg::OnBnClickedEnumDisplay)
	ON_BN_CLICKED(IDC_ENUM_WIN, &CSDKdemoDlg::OnBnClickedEnumWin)
	ON_BN_CLICKED(IDC_PUBLISH_CUSTOM, &CSDKdemoDlg::OnBnClickedPublishCustom)
END_MESSAGE_MAP()


// CSDKdemoDlg 消息处理程序

BOOL CSDKdemoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	//ModifyStyle(0, WS_SYSMENU | WS_MINIMIZEBOX);
	ModifyStyle(0, WS_SYSMENU);
	ModifyStyle(0, WS_CAPTION);

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

	Gdiplus::GdiplusStartupInput startupInput;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &startupInput, nullptr);

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

	m_width = rcDesktop.Width() * 0.3;
	m_height = m_width * 1.7;

	MoveWindow(0, 0, m_width, m_height, TRUE);
	CenterWindow();
	GetWindowRect(rc);

	int row_space = 50;
	int col_space = 200;
	int base_left = 30;
	int base_top = 30;
	int base_width = 150;
	int base_height = 40;

	m_edtChannelName.SetWindowText(_T("ChannelName"));
	m_edtChannelName.MoveWindow(base_left, base_top, base_width, base_height, TRUE);

	m_btnConfirm.SetWindowText(_T("Join"));
	m_btnConfirm.MoveWindow(base_left, base_top + row_space, base_width, base_height, TRUE);

	m_btnPublishCamera.SetWindowText(_T("Publish Camera"));
	m_btnPublishCamera.MoveWindow(base_left, base_top + row_space * 2, base_width, base_height, TRUE);

	m_btnPublishScreen.SetWindowText(_T("Publish Screen"));
	m_btnPublishScreen.MoveWindow(base_left, base_top + row_space * 3, base_width, base_height, TRUE);

	m_btnPreviewScreen.SetWindowText(_T("Allocate Local View"));
	m_btnPreviewScreen.MoveWindow(base_left, base_top + row_space * 4, base_width, base_height, TRUE);

	m_btnMuteCamera.SetWindowText(_T("Pause Camera"));
	m_btnMuteCamera.MoveWindow(base_left, base_top + row_space * 5, base_width, base_height, TRUE);

	m_btnMuteScreen.SetWindowText(_T("Pause Screen"));
	m_btnMuteScreen.MoveWindow(base_left, base_top + row_space * 6, base_width, base_height, TRUE);

	m_btnQuit.SetWindowText(_T("Quit"));
	m_btnQuit.MoveWindow(base_left, base_top + row_space * 7, base_width, base_height, TRUE);

	m_btnEnableVideoObserver.SetWindowText(_T("Enable Video Observer"));
	m_btnEnableVideoObserver.MoveWindow(base_left, base_top + row_space * 8, base_width + 100, base_height, TRUE);
	m_btnEnableVideoObserver.ShowWindow(SW_HIDE);

	m_btnUpdateUsers.SetWindowText(_T("Update Users"));
	m_btnUpdateUsers.MoveWindow(base_left + col_space, base_top, base_width, base_height, TRUE);

	m_cmbUsers.MoveWindow(base_left + col_space, base_top + row_space, base_width, base_height, TRUE);


	m_btnEnumWin.SetWindowText(_T("Enum Win"));
	m_btnEnumWin.MoveWindow(base_left + col_space, base_top + row_space * 2, base_width, base_height, TRUE);

	m_btnEnumDisplay.SetWindowText(_T("Enum Display"));
	m_btnEnumDisplay.MoveWindow(base_left + col_space, base_top + row_space * 3, base_width, base_height, TRUE);

	m_btnPublishCustom.SetWindowText(_T("Publish Custom"));
	m_btnPublishCustom.MoveWindow(base_left + col_space, base_top + row_space * 4, base_width, base_height, TRUE);

	m_textVideoDeviceList.SetWindowText(_T("Video Device:"));
	m_textVideoDeviceList.MoveWindow(base_left, base_top + row_space * 13, 120, base_height, TRUE);
	m_cmbCameraList.MoveWindow(base_left + 120, base_top + row_space * 13, base_width * 2, base_height, TRUE);

	m_textAudioDeviceList.SetWindowText(_T("Audio Device:"));
	m_textAudioDeviceList.MoveWindow(base_left, base_top + row_space * 14, 120, base_height, TRUE);
	m_cmbMicList.MoveWindow(base_left + 120, base_top + row_space * 14, base_width * 2, base_height, TRUE);

	

	//m_btnPreviewCamera.SetWindowText(_T("Preview Camera"));
	//m_btnPreviewCamera.MoveWindow(rc.Width() / 2 + 80, rc.Height() - 200, 120, 40, TRUE);
	m_btnPreviewCamera.ShowWindow(SW_HIDE);

	m_localView.Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPCHILDREN, CRect(0, 0, 1, 1), this, 1234);
	m_remoteView.Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPCHILDREN, CRect(0, 0, 1, 1), this, 1235);
}

static std::string GetAppId() {
    std::ifstream in("d:/appid.txt");
    std::string appid;
    in >> appid;
    in.close();
    return appid;
}

void CSDKdemoDlg::initData()
{
	static bool done = false;
	if(!done)
	{
		m_agoraManager = CAgoraManager::GetInstace();
		m_agoraManager->EnableVideoFrameObserver(true);
		m_agoraManager->Init(GetAppId().c_str());

		std::vector<CAgoraManager::CameraInfo> camera_list;
		m_agoraManager->GetCameraList(camera_list);
		for (int i = 0; i < camera_list.size(); i++) {
			CString device_name_ansi(wide2ansi(utf82wide(camera_list[i].device_name_utf8)).c_str());
			m_cmbCameraList.InsertString(i, (LPCTSTR)(device_name_ansi));
		}

		if(m_cmbCameraList.GetCount() != 0)
			m_cmbCameraList.SetCurSel(0);

		std::vector<CAgoraManager::MicInfo> mic_list;
		m_agoraManager->GetMicList(mic_list);
		for (int i = 0; i < mic_list.size(); i++) {
			//CString device_name(mic_list[i].device_name.c_str());
			CString device_name_ansi(wide2ansi(utf82wide(mic_list[i].device_name_utf8)).c_str());
			m_cmbMicList.InsertString(i, (LPCTSTR)(device_name_ansi));
		}

		if (m_cmbMicList.GetCount() != 0)
			m_cmbMicList.SetCurSel(0);

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
	CString channelName;
	m_edtChannelName.GetWindowText(channelName);

	uid_t screen_uid = rand() % 99999 + 1;

	m_agoraManager->JoinChannel((const char*)CStringA(channelName), nullptr, 0, nullptr, screen_uid, nullptr, screen_uid + 1);
}

void CSDKdemoDlg::leaveChannel()
{
	if (m_set_view) {
		if (m_camera_win) {
			delete m_camera_win;
			m_camera_win = nullptr;
		}
		if (m_screen_win) {
			delete m_screen_win;
			m_screen_win = nullptr;
		}
		m_set_view = false;
	}

	m_agoraManager->LeaveChannel();

	for (auto it = m_users_win.begin(); it != m_users_win.end(); it++) {
		if (it->second) {
			delete it->second;
		}
	}
	m_users_win.clear();
	m_cmbUsers.ResetContent();
}

void CSDKdemoDlg::OnBnClickedConfirm()
{
	// TODO: 在此添加控件通知处理程序代码
	if(m_agoraManager->IsJoinChannel())
	{
		leaveChannel();
		m_btnConfirm.SetWindowText(_T("Join"));
	}
	else
	{
		joinChannel();
		m_btnConfirm.SetWindowText(_T("Leave"));
	}
}


void CSDKdemoDlg::OnBnClickedButtonQuit()
{
	// TODO: 在此添加控件通知处理程序代码
	m_localView.DestroyWindow();
	m_remoteView.DestroyWindow();

	Gdiplus::GdiplusShutdown(m_gdiplusToken);

	CAgoraManager::Destroy();
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


void CSDKdemoDlg::OnBnClickedButtonPublishCamera()
{
	if(m_agoraManager->IsPushCamera())
	{
		m_agoraManager->StopPushCamera();
		m_btnPublishCamera.SetWindowText(_T("Publish Camera"));
	}
	else
	{
		m_agoraManager->StartPushCamera(true);
		m_btnPublishCamera.SetWindowText(_T("Unpublish Camera"));
	}
}


void CSDKdemoDlg::OnBnClickedButtonPreviewCamera()
{
	//if (m_agoraManager->IsPreview()) {
	//	m_agoraManager->StopPreview();
	//	if (m_camera_win) {
	//		delete m_camera_win;
	//		m_camera_win = nullptr;
	//	}
	//	m_btnPreviewCamera.SetWindowText(_T("Preview Camera"));
	//} else {
	//	if (m_camera_win) {
	//		delete m_camera_win;
	//		m_camera_win = nullptr;
	//	}
	//	m_camera_win = new SimpleWindow("Camera");
	//	m_agoraManager->SetCameraShowHwnd(m_camera_win->GetView());
	//	m_agoraManager->StartPreview();
	//	m_btnPreviewCamera.SetWindowText(_T("Hide Preview"));
	//}
}


void CSDKdemoDlg::OnCbnSelchangeComboUsers()
{
	int pos = m_cmbUsers.GetCurSel();
	CString str_uid;
	m_cmbUsers.GetLBText(pos, str_uid);
	agora::rtc::uid_t uid = std::stoul((const char *)CStringA(str_uid), nullptr, 0);
	if (m_users_win.find(uid) == m_users_win.end()) {
		SimpleWindow* win = new SimpleWindow((const char *)CStringA(str_uid));
		m_agoraManager->SetPlayerShowHwnd(uid, win->GetView());
		m_users_win[uid] = win;
		m_agoraManager->StartPlayer(uid);
	}
}


void CSDKdemoDlg::OnBnClickedButtonUpdateUsers()
{
	std::vector<agora::rtc::uid_t> user_list;
	m_agoraManager->GetPlayerUID(user_list);

	m_cmbUsers.ResetContent();

	for (auto user : user_list) {
		std::string uid = std::to_string(user);
		CString str_uid(uid.c_str());
		m_cmbUsers.AddString((LPCTSTR)(str_uid));
	}
}


void CSDKdemoDlg::OnBnClickedButtonPublishScreen()
{
	if(m_agoraManager->IsPushScreen())
	{
		m_agoraManager->StopPushScreen();
		m_btnPublishScreen.SetWindowText(_T("Publish Screen"));
	}
	else
	{
		m_agoraManager->StartPushScreen(true);
		m_btnPublishScreen.SetWindowText(_T("Unpublish Screen"));
	}
}


void CSDKdemoDlg::OnBnClickedButtonPreviewScreen()
{
	if (m_set_view) {
		m_agoraManager->SetCameraShowHwnd(0);
		m_agoraManager->SetWindowDesktopShowHwnd(0);

		if (m_camera_win) {
			delete m_camera_win;
			m_camera_win = nullptr;
		}
		if (m_screen_win) {
			delete m_screen_win;
			m_screen_win = nullptr;
		}
		m_btnPreviewScreen.SetWindowText(_T("Allocate Local View"));
		m_set_view = false;
	} else {
		if (m_camera_win) {
			delete m_camera_win;
			m_camera_win = nullptr;
		}
		if (m_screen_win) {
			delete m_screen_win;
			m_screen_win = nullptr;
		}

		m_camera_win = new SimpleWindow("Camera");
		m_agoraManager->SetCameraShowHwnd(m_camera_win->GetView());

		m_screen_win = new SimpleWindow("Screen");
		m_agoraManager->SetWindowDesktopShowHwnd(m_screen_win->GetView());

		m_btnPreviewScreen.SetWindowText(_T("Clear Local View"));
		m_set_view = true;
	}
}


void CSDKdemoDlg::OnBnClickedButtonMuteCamera()
{
	if(m_agoraManager->IsPushCameraPause())
	{
		m_agoraManager->SetPushCameraPause(false);
		m_btnMuteCamera.SetWindowText(_T("Pause Camera"));
	}
	else
	{
		m_agoraManager->SetPushCameraPause(true);
		m_btnMuteCamera.SetWindowText(_T("Resume Camera"));
	}
}


void CSDKdemoDlg::OnBnClickedButtonMuteScreen()
{
	if(m_agoraManager->IsPushScreenPause())
	{
		m_agoraManager->SetPushScreenPause(false);
		m_btnMuteScreen.SetWindowText(_T("Pause Screen"));
	}
	else
	{
		m_agoraManager->SetPushScreenPause(true);
		m_btnMuteScreen.SetWindowText(_T("Resume Screen"));
	}
}


void CSDKdemoDlg::OnCbnSelchangeComboCameraList()
{
	int id = m_cmbCameraList.GetCurSel();
	std::vector<CAgoraManager::CameraInfo> camera_list;
	m_agoraManager->GetCameraList(camera_list);

	if (id < camera_list.size()) {
		m_agoraManager->SetPushCamera(id);
	}
}


void CSDKdemoDlg::OnCbnSelchangeComboMicList()
{
	int id = m_cmbMicList.GetCurSel();
	std::vector<CAgoraManager::MicInfo> mic_list;
	m_agoraManager->GetMicList(mic_list);

	if (id < mic_list.size()) {
		m_agoraManager->SetMic(id);
	}
}


void CSDKdemoDlg::OnStnClickedStaticAudioDevice()
{
	// TODO: Add your control notification handler code here
}


void CSDKdemoDlg::OnStnClickedStaticVideoDevice()
{
	// TODO: Add your control notification handler code here
}


void CSDKdemoDlg::OnBnClickedCheckVideoObserver()
{
	bool enable = m_btnEnableVideoObserver.GetCheck();
	m_agoraManager->EnableVideoFrameObserver(enable);
}

void CSDKdemoDlg::OnBnClickedEnumDisplay()
{
	std::vector<CAgoraManager::DesktopInfo> vDesktop;
	m_agoraManager->GetDesktopList(vDesktop);
}


void CSDKdemoDlg::OnBnClickedEnumWin()
{
	using namespace app::utils;
	using ListWindowsInfo = std::list<WindowEnumer::WINDOW_INFO>;
	using MapWindowInfo = std::map<std::string, std::list<WindowEnumer::WINDOW_INFO>>;

	std::list<std::string> vecFilters;
	MapWindowInfo mapWindows = app::utils::WindowEnumer::EnumAllWindows(vecFilters);

	int winNum = mapWindows.size();
}


void CSDKdemoDlg::OnBnClickedPublishCustom()
{
	if (!m_agoraManager->IsPushCustom()) {
		publishCustomMedia();
		m_btnPublishCustom.SetWindowText(_T("Unpublish Custom"));
	}
	else {
		unpublishCustomMedia();
		m_btnPublishCustom.SetWindowText(_T("Publish Custom"));
	}
	
}

void CSDKdemoDlg::publishCustomMedia()
{
	if (!m_agoraManager->IsJoinChannel() || m_agoraManager->IsPushCustom())
		return;

	m_agoraManager->StartPushCustom();

	std::thread th([this] {
		const int VIDEO_WIDTH = 640;
		const int VIDEO_HEIGHT = 360;

		const char* test_filepath = "d:\\broadcaster_uid_0_640_360_rgba.yuv";
		FILE* file = fopen(test_filepath, "rb");
		if (!file)
			return;

		uint64_t ms = 0;
		int fps = 30;
		int diff = 1000 / fps;
		int FrameSize = VIDEO_WIDTH * VIDEO_HEIGHT * 4;
		uint8_t* frameBuff = new uint8_t[FrameSize];

		while (m_agoraManager->IsPushCustom()) {
			size_t readBytes = fread(frameBuff, FrameSize, 1, file);
			if (readBytes == 0) {
				fseek(file, 0, SEEK_SET);
				readBytes = fread(frameBuff, FrameSize, 1, file);
				if (readBytes == 0)
					break;
			}

			bool ret = m_agoraManager->PushVideoFrame(frameBuff, VIDEO_WIDTH, VIDEO_HEIGHT, ms);
			if (!ret) {
				printf("[W]: PushVideoFrame ret: %d\n", ret);
			}
			//ms += diff;
			Sleep(diff);
		}

		if (m_agoraManager->IsPushCustom())
			m_agoraManager->StopPushCustom();

		delete[] frameBuff;
		fclose(file);
	});

	th.detach();

	std::thread th2([this] {
		const int VIDEO_WIDTH = 640;
		const int VIDEO_HEIGHT = 360;

		const char* test_filepath = "d:\\far_in.pcm";
		FILE* file = fopen(test_filepath, "rb");
		if (!file)
			return;

		uint64_t ms = 0;
		int sampleRate = 48000;
		int channelNum = 2;
		int frameDuration = 10;
		int FrameSize = sampleRate * frameDuration / 1000 * channelNum * 2;
		uint8_t* frameBuff = new uint8_t[FrameSize];

		while (m_agoraManager->IsPushCustom() && fread(frameBuff, FrameSize, 1, file)) {
			bool ret = m_agoraManager->PushAudioFrame(frameBuff, FrameSize, sampleRate, channelNum, 0);
			if (!ret) {
				printf("[W]: PushVideoFrame ret: %d\n", ret);
			}
			Sleep(frameDuration - 1);
		}

		if (m_agoraManager->IsPushCustom())
			m_agoraManager->StopPushCustom();

		delete[] frameBuff;
		fclose(file);
	});

	th2.detach();
}

void CSDKdemoDlg::unpublishCustomMedia()
{
	if(m_agoraManager->IsPushCustom())
		m_agoraManager->StopPushCustom();
}