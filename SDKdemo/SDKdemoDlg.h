
// SDKdemoDlg.h: 头文件
//

#pragma once
#include "IAgoraRtcEngine.h"
#include "CAGEngineEventHandler.h"

#include <map>

using namespace agora::rtc;

class CAgoraManager;
class SimpleWindow;

// CSDKdemoDlg 对话框
class CSDKdemoDlg : public CDialogEx
{
// 构造
public:
	CSDKdemoDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SDKDEMO_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedConfirm();
	afx_msg void OnBnClickedButtonQuit();
	afx_msg void OnBnClickedButtonPublishCamera();
	afx_msg void OnBnClickedButtonPreviewCamera();
	afx_msg void OnBnClickedButtonPublishScreen();
	afx_msg void OnCbnSelchangeComboUsers();
	afx_msg void OnBnClickedButtonUpdateUsers();
	afx_msg void OnEnChangeEditChannelName();
	afx_msg void OnBnClickedButtonPreviewScreen();
	afx_msg void OnBnClickedButtonMuteCamera();
	afx_msg void OnBnClickedButtonMuteScreen();
	afx_msg void OnCbnSelchangeComboCameraList();
	afx_msg void OnCbnSelchangeComboMicList();
	afx_msg void OnStnClickedStaticAudioDevice();
	afx_msg void OnStnClickedStaticVideoDevice();
	afx_msg void OnBnClickedCheckVideoObserver();
	afx_msg void OnBnClickedEnumDisplay();
	afx_msg void OnBnClickedEnumWin();
	afx_msg void OnBnClickedPublishCustom();

	void initCtrls();
	void initData();
	void joinChannel();
	void leaveChannel();
	void adjustVideoViews(bool local, bool remote);
	void publishCustomMedia();
	void unpublishCustomMedia();

private:

	CWnd m_localView;
	CWnd m_remoteView;
	CButton m_btnConfirm;
	CButton m_btnQuit;
	CEdit m_edtChannelName;

	CButton m_btnPublishCamera;
	CButton m_btnPublishScreen;
	CButton m_btnPreviewCamera;
	CButton m_btnPreviewScreen;
	CButton m_btnMuteCamera;
	CButton m_btnMuteScreen;
	CButton m_btnUpdateUsers;
	CButton m_btnEnableVideoObserver;
	CButton m_btnEnumWin;
	CButton m_btnEnumDisplay;
	CButton m_btnPublishCustom;

	CComboBox m_cmbUsers;
	CComboBox m_cmbCameraList;
	CComboBox m_cmbMicList;

	CStatic m_textVideoDeviceList;
	CStatic m_textAudioDeviceList;
	ULONG_PTR m_gdiplusToken = 0;

	IRtcEngine* m_lpRtcEngine;
	int m_width = 0;
	int m_height = 0;
	bool m_inChannel = false;
	bool m_set_view = false;

	CAgoraManager* m_agoraManager = nullptr;
	SimpleWindow* m_camera_win = nullptr;
	SimpleWindow* m_screen_win = nullptr;

	std::map<agora::rtc::uid_t, SimpleWindow*> m_users_win;
};
