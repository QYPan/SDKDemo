#pragma once

#include <functional>
#include <vector>
#include <winapifamily.h>

#pragma region Desktop Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

#ifndef __wincodec_h__
#include <wincodec.h>
#endif

#define WC_MAGNIFIERA             "Magnifier"
#define WC_MAGNIFIERW             L"Magnifier"

#ifdef UNICODE
#define WC_MAGNIFIER              WC_MAGNIFIERW
#else
#define WC_MAGNIFIER              WC_MAGNIFIERA
#endif

#else
#define WC_MAGNIFIER              "Magnifier"
#endif

/* Magnifier Window Styles */
#define MS_SHOWMAGNIFIEDCURSOR      0x0001L
#define MS_CLIPAROUNDCURSOR         0x0002L
#define MS_INVERTCOLORS             0x0004L


/* Filter Modes */
#define MW_FILTERMODE_EXCLUDE   0
#define MW_FILTERMODE_INCLUDE   1


/* Structures */
typedef struct tagMAGTRANSFORM
{
	float v[3][3];
} MAGTRANSFORM, *PMAGTRANSFORM;

typedef struct tagMAGIMAGEHEADER
{
	UINT width;
	UINT height;
	WICPixelFormatGUID format;
	UINT stride;
	UINT offset;
	SIZE_T cbSize;
} MAGIMAGEHEADER, *PMAGIMAGEHEADER;

typedef struct tagMAGCOLOREFFECT
{
	float transform[5][5];
} MAGCOLOREFFECT, *PMAGCOLOREFFECT;

typedef BOOL(WINAPI* MAGINITIALIZE)();
typedef BOOL(WINAPI* MAGUNINITIALIZE)();
typedef BOOL(WINAPI* MAGSETWINDOWSOURCE)(HWND, RECT);
typedef BOOL(WINAPI* MAGSETWINDOWFILTERLIST)(HWND, DWORD, int, HWND*);
typedef BOOL(CALLBACK* MagImageScalingCallback)(HWND hwnd, void * srcdata, MAGIMAGEHEADER srcheader, void * destdata, MAGIMAGEHEADER destheader, RECT unclipped, RECT clipped, HRGN dirty);
typedef BOOL(WINAPI* MAGSETIMAGESCALINGCALLBACK)(HWND, MagImageScalingCallback);

class MagnificationCapture
{
public:
	MagnificationCapture();
	~MagnificationCapture();

	void UnLoadMagnificationLibrary();

	BOOL LoadMagnificationLibrary();

	BOOL CaptureFrame(HWND selectedHwnd, RECT rect);

	void OnCaptured(void* data, const MAGIMAGEHEADER& header);

	BOOL GetFrameInfo(int& imgWidth, int& imgHeight, std::vector<BYTE>& imgData);
private:
	BOOL isCaptureSucceed;
	std::vector<BYTE> cache_img;
	MAGIMAGEHEADER img_header;
	HINSTANCE magnificationLibrary;

	MAGINITIALIZE MagInitialize;
	MAGUNINITIALIZE MagUninitialize;
	MAGSETWINDOWSOURCE MagSetWindowSource;
	MAGSETWINDOWFILTERLIST MagSetWindowFilterList;
	MAGSETIMAGESCALINGCALLBACK MagSetImageScalingCallback;
};

