// Copyright 2010 Isis Innovation Limited
// This VideoSource for Win32 uses the videoInput library 0.1995 by Theodore Watson
// available at 
// http://muonics.net/school/spring05/videoInput/


#include "VideoSource.h"
#include <videoInput.h> // External lib
#include <gvars3/instances.h>
#include <cvd/utility.h>
#define WIN32_MEAN_AND_LEAN
#include <windows.h>
#include <iostream>
#include <iomanip>

using namespace std;
using namespace CVD;
using namespace GVars3;

//#define INPUT_FROM_WEBCAMERA

namespace PTAMM {

struct VideoInputInfo
{
	videoInput *pVideoInput;
	int nDevice;
};

VideoSource::VideoSource()
{
#ifdef INPUT_FROM_WEBCAMERA
	VideoInputInfo *pInfo = new VideoInputInfo;
	mptr = (void*) pInfo;

	pInfo->pVideoInput =  new videoInput;
	pInfo->nDevice = GV3::get<int>("VideoInput.DeviceNumber", 0, HIDDEN);;
	int nIdealFrameRate = GV3::get<int>("VideoInput.IdealFrameRate", 30, HIDDEN);
	ImageRef irIdealSize = GV3::get<ImageRef>("VideoInput.IdealSize", ImageRef(640,480), HIDDEN);

	pInfo->pVideoInput->setIdealFramerate(pInfo->nDevice, nIdealFrameRate);
	pInfo->pVideoInput->setupDevice(pInfo->nDevice, irIdealSize.x, irIdealSize.y);

	mirSize.x = pInfo->pVideoInput->getWidth(pInfo->nDevice);
	mirSize.y = pInfo->pVideoInput->getHeight(pInfo->nDevice);
#endif


#ifndef INPUT_FROM_WEBCAMERA
	img = cv::imread("../8/0000.jpg");
	mirSize.x = img.cols;
	mirSize.y = img.rows;
	filepath = "../8/";
	uid = 0;
#endif
}

void VideoSource::GetAndFillFrameBWandRGB(Image<CVD::byte> &imBW, Image<CVD::Rgb<CVD::byte> > &imRGB)
{
	imRGB.resize(mirSize);
	imBW.resize(mirSize);
#ifdef INPUT_FROM_WEBCAMERA
	VideoInputInfo *pInfo = (VideoInputInfo*) mptr;
	while(!pInfo->pVideoInput->isFrameNew(pInfo->nDevice))
		Sleep(1);

	pInfo->pVideoInput->getPixels(pInfo->nDevice, (CVD::byte*) imRGB.data(), true, true);
	copy(imRGB, imBW);
#endif


#ifndef INPUT_FROM_WEBCAMERA
	//图片序列
	ostringstream os;
	os << setfill('0') << setw(4) << uid << ".jpg";
	uid++;
	string sFileBaseName = os.str();
	//std::cout << sFileBaseName <<": "<<uid<<std::endl;
	img = cv::imread(filepath + sFileBaseName);
	if(!img.data)
	{
		uid = 0;
		img = cv::imread("../sequence/0000.jpg");
	}
	Sleep(10);

	//imRGB和img的RGB的顺序是倒着的
	for (int i=0; i<mirSize.x; i++)
	{
		for (int j=0; j<mirSize.y; j++)
		{
			//std::cout<<i*mirSize.y*3+j*3<<std::endl;
			memcpy((unsigned char*)imRGB.data()+i*mirSize.y*3+j*3, (unsigned char*)img.data+i*mirSize.y*3+j*3+2, sizeof(unsigned char));
			memcpy((unsigned char*)imRGB.data()+i*mirSize.y*3+j*3+1, (unsigned char*)img.data+i*mirSize.y*3+j*3+1, sizeof(unsigned char));
			memcpy((unsigned char*)imRGB.data()+i*mirSize.y*3+j*3+2, (unsigned char*)img.data+i*mirSize.y*3+j*3, sizeof(unsigned char));
		}
	}
	copy(imRGB, imBW);
#endif
}

ImageRef VideoSource::Size()
{
	return mirSize;
}

}
