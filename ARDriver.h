// -*- c++ -*-
// Copyright 2009 Isis Innovation Limited
// ARDriver.h
// This file declares the ARDriver class
//
// ARDriver provides basic graphics services for drawing augmented
// graphics. It manages the OpenGL setup and the camera's radial
// distortion so that real and distorted virtual graphics can be
// properly blended.
//
#ifndef __AR_Driver_H
#define __AR_Driver_H
#include <TooN/se3.h>
#include "ATANCamera.h"
#include "GLWindow2.h"
#include "OpenGL.h"
#include <cvd/image.h>
#include <cvd/rgb.h>
#include <cvd/byte.h>
#include "GLShader.h"

#ifndef NAN
#include <limits>
#endif 

namespace PTAMM {

using namespace std;
using namespace CVD;

class Map;

/***
FrameBuffer Object所绑定的纹理对应表

FBO_SHADOW	GL_DEPTH_ATTACHMENT		---->	mtexs[TEX_DEPTH]
FBO_OBJECT	GL_COLOR_ATTACHMENT0	---->	mtex[TEX_OBJECT]
			GL_DEPTH_ATTACHMENT		---->	mtex[TEX_DEPTH1]
FBO_GHOST	GL_COLOR_ATTACHMENT0	---->	mtex[TEX_GHOST]
			GL_DEPTH_ATTACHMENT		---->	mtex[TEX_DEPTH2]
FBO_MED		GL_COLOR_ATTACHMENT0	---->	mtex[TEX_MEDIATOR]
			GL_DEPTH_ATTACHMENT		---->	mtex[TEX_DEPTH3]
FBO_MIRROR	GL_COLOR_ATTACHMENT0	---->	mtex[TEX_MIRROR]
			GL_DEPTH_ATTACHMENT		---->	mtex[TEX_DEPTH4]
FBO_EDGE	GL_COLOR_ATTACHMENT0	---->	mtex[TEX_BACKEDGE]
			GL_COLOR_ATTACHMENT1	---->	mtex[TEX_BOUNDARY]
			GL_COLOR_ATTACHMENT2	---->	mtex[TEX_EDGEMASK]
			GL_DEPTH_ATTACHMENT		---->	mtex[TEX_DEPTH5]
FBO_BLD		GL_COLOR_ATTACHMENT0	---->	mnFrameBufferTex
			GL_DEPTH_ATTACHMENT		---->	mtex[TEX_DEPTH6]
***/

class ARDriver
{
  public:
    ARDriver(const ATANCamera &cam, ImageRef irFrameSize, GLWindow2 &glw, Map &map);
	void Render(Image<Rgb<CVD::byte> > &imFrame, SE3<> se3CamFromWorld, bool bLost, int statusFlag);
    void Reset();
    void Init();

    void HandleClick(int nButton, ImageRef irWin );
    void HandleKeyPress( std::string sKey );
    void AdvanceLogic();
    void LoadGame(std::string sName);
  
    void SetCurrentMap(Map &map) { mpMap = &map; mnCounter = 0; }

  protected:
    void MakeFrameBuffer();
    void DrawDistortedFB();
    void SetFrustum();
    
    bool PosAndDirnInPlane(Vector<2> v2VidCoords, Vector<2> &v2Pos, Vector<2> &v2Dirn);

    
  protected:
    ATANCamera mCamera;
    GLWindow2 &mGLWindow;
    Map *mpMap;
  
    // Texture stuff:
    GLuint mnFrameBufferTex;  //最后显示的图片
    
    int mnCounter;
    ImageRef mirFBSize;
    ImageRef mirFrameSize;
    SE3<> mse3CfromW;
    bool mbInitialised;

	Image<Rgba<CVD::byte> > mLostOverlay;

private:
	void SetTexture(GLuint texId);
	void DrawQuad();
	//void InitGaussTexture(GLuint texId);

	// 帧缓存：FBO_OBJECT-虚拟物体，FBO_GHOST-ghost物体，FBO_MED-中介面物体，FBO_MIRROR-镜面物体，FBO_EDGE-描边物体（已放弃使用），FBO_BLD-整合所有前几个物体
	enum {FBO_OBJECT, FBO_GHOST, FBO_MED, FBO_MIRROR, FBO_EDGE, FBO_BLD};  //FBO_OBJECT保存导入的model和阴影，FBO_BLD保存FBO_OBJECT与视频图像的融合
	// 纹理：TEX_BOUNDARY-E:\lab\万老师讨论组\ptamm-md\PTAMM中的boundary.bmp
	enum {TEX_OBJECT, TEX_GHOST, TEX_IMAGE, TEX_MEDIATOR, TEX_MIRROR, TEX_BACKEDGE, TEX_BOUNDARY, TEX_EDGEMASK, TEX_DEPTH1, TEX_DEPTH2, TEX_DEPTH3, TEX_DEPTH4, TEX_DEPTH5, TEX_DEPTH6};

	enum {PROG_EDGE, PROG_BOUNDARY, PROG_BLEND};

	GLuint mfbo[6];
	GLuint mtex[14];
	Shader mshaders[3];
};

}

#endif
