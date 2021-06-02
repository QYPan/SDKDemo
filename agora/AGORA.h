#pragma once

struct TRTCImageBuffer
{
    const char * buffer;     ///< 图内容
    uint32_t length;         ///< 图缓存大小
    uint32_t width;          ///< 图宽
    uint32_t height;         ///< 图高
    TRTCImageBuffer()
        : buffer(nullptr)
        , length(0)
        , width(0)
        , height(0)
    {};
};

class WinProg
{
public:
 HWND sourceId;
 const char *    sourceName;///< 窗口标题名称，UTF8 编码
 bool            isMinimizeWindow;///< 是否为最小化窗口
 int    x; //窗口位置x
 int    y; //窗口位置y
 int    width;//窗口宽度
 int    height;//窗口高度
 TRTCImageBuffer iconBGRA;           ///< 图标内容
 TRTCImageBuffer thumbBGRA;          ///< 缩略图内容
};

class DesktopProg
{
 HWND sourceId;
 const char *    sourceName;///< 采集源名称，UTF8 编码
 int    x; //全局虚拟桌面位置x
 int    y; //全局虚拟桌面位置y
 int    width;//选定桌面宽度
 int    height;//选定桌面窗口高度
 TRTCImageBuffer thumbBGRA;       ///< 缩略图内容
 bool            isMainScreen;    ///< 是否为主屏
};

//void GetWindowList(std::vector<WindowProg>& vWindows, int nThumbSizeW = 0, int nThumbSizeH = 0, int nIconSizeW = 0, int nIconSizeH = 0);
//void GetDesktopList(std::vector<DesktopProg>& vDesktops, int nThumbSizeW = 0, int nThumbSizeH = 0);