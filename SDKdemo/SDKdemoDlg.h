
// SDKdemoDlg.h: 头文件
//

#pragma once
#include "IAgoraRtcEngine.h"
#include "CAGEngineEventHandler.h"

using namespace agora::rtc;

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
	afx_msg void OnEnChangeEditChannelName();

	afx_msg LRESULT OnJoinChannelSuccess(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnLeaveChannel(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT onUserJoined(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT onUserOffline(WPARAM wParam, LPARAM lParam);

	void initCtrls();
	void initData();
	void joinChannel();
	void leaveChannel();
	void adjustVideoViews(bool local, bool remote);

private:
	CAGEngineEventHandler m_engineEventHandler;

	CWnd m_localView;
	CWnd m_remoteView;
	CButton m_btnConfirm;
	CButton m_btnQuit;
	CEdit m_edtChannelName;
	IRtcEngine* m_lpRtcEngine;
	int m_width;
	int m_height;
	bool m_inChannel;
	
};
