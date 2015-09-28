// Copyright 2009 Isis Innovation Limited
#define GL_GLEXT_PROTOTYPES 1
#include "ARDriver.h"
#include "Map.h"
#include "Games.h"
#include "imdebuggl.h"

#include <GL/GLAux.h>  //add by myself
#include <cvd/image_io.h>

namespace PTAMM {

using namespace GVars3;
using namespace CVD;
using namespace std;

static bool CheckFramebufferStatus();

/**
 * Constructor
 * @param cam Reference to the camera
 * @param irFrameSize the size of the frame
 * @param glw the glwindow
 * @param map the current map
 */
ARDriver::ARDriver(const ATANCamera &cam, ImageRef irFrameSize, GLWindow2 &glw, Map &map)
  :mCamera(cam), mGLWindow(glw), mpMap( &map )
{
  mirFrameSize = irFrameSize;
  mCamera.SetImageSize(mirFrameSize);
  mbInitialised = false;
  
  //设置各个shader所采用的文件
  mshaders[PROG_EDGE].SetShaderFile("shaders/default.vs", "shaders/edge.fs");
  mshaders[PROG_EDGE].UseShader(false);
  mshaders[PROG_BOUNDARY].SetShaderFile("shaders/default.vs", "shaders/boundary.fs");
  mshaders[PROG_BOUNDARY].UseShader(false);
  mshaders[PROG_BLEND].SetShaderFile("shaders/default.vs","shaders/test.fs");
  mshaders[PROG_BLEND].UseShader(false);
  glGenTextures(14, mtex);  //如果是16，会导致最终显示只有虚拟物体，可能是产生的texture uint标识与mnFrameBufferTex标识冲突了
  glGenFramebuffers(6, mfbo);
}

/**
 * Initialize the AR driver
 */
void ARDriver::Init()
{
  mbInitialised = true;
  mirFBSize = GV3::get<ImageRef>("ARDriver.FrameBufferSize", ImageRef(640,480), SILENT);
  glBindTexture(GL_TEXTURE_RECTANGLE,mtex[TEX_IMAGE]);
  glTexImage2D(GL_TEXTURE_RECTANGLE, 0,
	       GL_RGBA, mirFrameSize.x, mirFrameSize.y, 0,
	       GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  MakeFrameBuffer();

  try {
    CVD::img_load(mLostOverlay, "ARData/Overlays/searching.png");
  }
  catch(CVD::Exceptions::All err) {
    cerr << "Failed to load searching image " << "\"ARData/Overlays/searching.png\"" << ": " << err.what << endl;
  }   
}


/**
 * Reset the game and the frame counter
 */
void ARDriver::Reset()
{
  if(mpMap->pGame) {
    mpMap->pGame->Reset();
  }

  mnCounter = 0;
}


/**
 * Render the AR composite image
 * @param imFrame The camera frame
 * @param se3CfromW The camera position
 * @param bLost Is the camera lost
 */
void ARDriver::Render(Image<Rgb<CVD::byte> > &imFrame, SE3<> se3CfromW, bool bLost, int statusFlag)
{
  if(!mbInitialised)
  {
    Init();
    Reset();
  };

  mse3CfromW = se3CfromW;
  mnCounter++;

  // Upload the image to our frame texture
  int tempheight,tempwidth;
  tempheight = imFrame.size().y;
  tempwidth = imFrame.size().x;
  //convert coordinate 图片坐标系颠倒-左上到左下
  unsigned char* tempdata = (unsigned char*)malloc(sizeof(CVD::Rgb<CVD::byte>)*tempwidth*tempheight);
  for (int j=0; j<tempheight; j++)
  {
	  memcpy(tempdata+tempwidth*sizeof(CVD::Rgb<CVD::byte>)*(tempheight-1-j),(CVD::byte*)imFrame.data()+tempwidth*sizeof(CVD::Rgb<CVD::byte>)*j,tempwidth*sizeof(CVD::Rgb<CVD::byte>));
  }
  // 将背景图片*tempdata绑定到TEX_IMAGE
  glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_IMAGE]);	
  glTexSubImage2D(GL_TEXTURE_RECTANGLE,
		  0, 0, 0,
		  mirFrameSize.x, mirFrameSize.y,
		  GL_RGB,
		  GL_UNSIGNED_BYTE,
		  tempdata);
  free(tempdata);

  //FBO_OBJECT shader
  if(!bLost)	// 地图没丢到，设置pGame帧缓存参数
  {
    if(mpMap->pGame) {
	  mpMap->pGame->setStatusFlag(statusFlag);
	  mpMap->pGame->setFBOOBJECT(mfbo[FBO_OBJECT]);
	  mpMap->pGame->setFBOGHOST(mfbo[FBO_GHOST]);
	  mpMap->pGame->setFBOMED(mfbo[FBO_MED]);
	  mpMap->pGame->setFBOEDGE(mfbo[FBO_EDGE]);
	  mpMap->pGame->setFBOMIRROR(mfbo[FBO_MIRROR]);
	  mpMap->pGame->Draw3D( mGLWindow, *mpMap, se3CfromW, mCamera);	// 绘制场景
    }
  }
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);


//imdebugTexImage(GL_TEXTURE_RECTANGLE, mtex[TEX_BOUNDARY], GL_RGBA);


  //add by 2012.12.29
  //FBO_MED	GL_COLOR_ATTACHMENT1	---->	mtex[TEX_BACKEDGE]
  /*
  // 用于扣边 add by wwj 2013.03.15
  glBindFramebuffer(GL_FRAMEBUFFER,mfbo[FBO_EDGE]);
  CheckFramebufferStatus();
  glViewport(0,0,mirFBSize.x,mirFBSize.y);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0,1,1,0,0,1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  mshaders[PROG_EDGE].UseShader(true);
  glEnable(GL_TEXTURE_RECTANGLE);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_IMAGE]);
  mshaders[PROG_EDGE].SetSampler("txImage",0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_EDGEMASK]);
  mshaders[PROG_EDGE].SetSampler("txEdgemask", 1);
  DrawQuad();
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_RECTANGLE, 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_RECTANGLE, 0);
  glDisable(GL_TEXTURE_RECTANGLE);
  mshaders[PROG_EDGE].UseShader(false);
  */

  ////add by 2012.17
  ////FBO_MED	GL_COLOR_ATTACHMENT2	---->	mtex[TEX_BOUNDARY]
  //glBindFramebuffer(GL_FRAMEBUFFER,mfbo[FBO_MED]);
  //CheckFramebufferStatus();
  //glViewport(0,0,mirFBSize.x,mirFBSize.y);
  //glDrawBuffer(GL_COLOR_ATTACHMENT2);
  //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  //glMatrixMode(GL_PROJECTION);
  //glLoadIdentity();
  //glOrtho(0,1,1,0,0,1);
  //glMatrixMode(GL_MODELVIEW);
  //glLoadIdentity();
  //mshaders[PROG_BOUNDARY].UseShader(true);
  //glEnable(GL_TEXTURE_RECTANGLE);
  //glActiveTexture(GL_TEXTURE0);
  //glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_OBJECT]);
  //mshaders[PROG_BOUNDARY].SetSampler("txMediator",0);
  //DrawQuad();
  //glActiveTexture(GL_TEXTURE0);
  //glBindTexture(GL_TEXTURE_RECTANGLE, 0);
  //glDisable(GL_TEXTURE_RECTANGLE);
  //mshaders[PROG_BOUNDARY].UseShader(false);

  
  //FBO_BLD		GL_COLOR_ATTACHMENT0	---->	mnFrameBufferTex
  glBindFramebuffer(GL_FRAMEBUFFER,mfbo[FBO_BLD]);
  CheckFramebufferStatus();
  glViewport(0,0,mirFBSize.x,mirFBSize.y);
  glDrawBuffer(GL_COLOR_ATTACHMENT0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0,1,1,0,0,1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  
  mshaders[PROG_BLEND].UseShader(true);
  glEnable(GL_TEXTURE_RECTANGLE);
	// 以下三句用于设置mshaders的PROG_BLEND的txObject参数
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_OBJECT]);
  mshaders[PROG_BLEND].SetSampler("txObject",0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_GHOST]);
  mshaders[PROG_BLEND].SetSampler("txGhost", 1);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_DEPTH1]);
  mshaders[PROG_BLEND].SetSampler("txDepth1", 2);
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_DEPTH2]);
  mshaders[PROG_BLEND].SetSampler("txDepth2", 3);
  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_IMAGE]);
  mshaders[PROG_BLEND].SetSampler("txImage",4);
  glActiveTexture(GL_TEXTURE5);
  glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_BACKEDGE]);
  mshaders[PROG_BLEND].SetSampler("txBackEdge",5);
  glActiveTexture(GL_TEXTURE12);
  glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_BOUNDARY]);
  mshaders[PROG_BLEND].SetSampler("txBoundary", 12);
  glActiveTexture(GL_TEXTURE8);
  glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_MEDIATOR]);
  mshaders[PROG_BLEND].SetSampler("txMediator", 8);
  glActiveTexture(GL_TEXTURE9);
  glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_DEPTH3]);
  mshaders[PROG_BLEND].SetSampler("txDepth3", 9);
  glActiveTexture(GL_TEXTURE10);
  glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_EDGEMASK]);
  mshaders[PROG_BLEND].SetSampler("txEdgemask", 10);
  glActiveTexture(GL_TEXTURE11);
  glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_MIRROR]);
  mshaders[PROG_BLEND].SetSampler("txMirror", 11);
  DrawQuad();	// 绘制窗口平面
  // 以下为取消纹理绑定
  glActiveTexture(GL_TEXTURE11);
  glBindTexture(GL_TEXTURE_RECTANGLE, 0);
  glActiveTexture(GL_TEXTURE10);
  glBindTexture(GL_TEXTURE_RECTANGLE, 0);
  glActiveTexture(GL_TEXTURE9);
  glBindTexture(GL_TEXTURE_RECTANGLE, 0);
  glActiveTexture(GL_TEXTURE8);
  glBindTexture(GL_TEXTURE_RECTANGLE, 0);
  glActiveTexture(GL_TEXTURE12);
  glBindTexture(GL_TEXTURE_RECTANGLE, 0);
  glActiveTexture(GL_TEXTURE5);
  glBindTexture(GL_TEXTURE_RECTANGLE, 0);
  glActiveTexture(GL_TEXTURE4);
  glBindTexture(GL_TEXTURE_RECTANGLE, 0);
  glActiveTexture(GL_TEXTURE3);
  glBindTexture(GL_TEXTURE_RECTANGLE, 0);
  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_RECTANGLE, 0);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_RECTANGLE, 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_RECTANGLE, 0);
  glDisable(GL_TEXTURE_RECTANGLE);
  mshaders[PROG_BLEND].UseShader(false);
  


  //Draw mnFrameBufferTex texture to Windows
  glDisable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_BLEND);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Set up for drawing 2D stuff:
  glBindFramebuffer(GL_FRAMEBUFFER,0);

  DrawDistortedFB();
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  mGLWindow.SetupViewport();
  mGLWindow.SetupVideoOrtho();
  mGLWindow.SetupVideoRasterPosAndZoom();

  
  //2d drawing， 如“search”文字等
  if(!bLost)
  {   
    if(mpMap->pGame) {
      mpMap->pGame->Draw2D(mGLWindow, *mpMap);
    }
  }
  else
  {
    //draw the lost ar overlays
    glEnable(GL_BLEND);
    glRasterPos2i( ( mGLWindow.size().x - mLostOverlay.size().x )/2,
                   ( mGLWindow.size().y - mLostOverlay.size().y )/2 );
    glDrawPixels(mLostOverlay);
    glDisable(GL_BLEND);
  }

  ////用于测试
  //glUseProgram(0);
  //glMatrixMode(GL_PROJECTION);
  //glLoadIdentity();
  //glOrtho(-mirFrameSize.x/2,mirFrameSize.x/2,-mirFrameSize.y/2,mirFrameSize.y/2,1,20);
  //glMatrixMode(GL_MODELVIEW);
  //glLoadIdentity();
  //glColor4f(1,1,1,1);
  //glActiveTexture(GL_TEXTURE0);
  //glBindTexture(GL_TEXTURE_2D,15);  //15是过度面 16是 edge mask
  //glEnable(GL_TEXTURE_2D);
  //glTranslated(0,0,-10);
  //glBegin(GL_QUADS);
  //glTexCoord2d(0,0);glVertex3f(0,0,0);
  //glTexCoord2d(1,0);glVertex3f(mirFrameSize.x/2,0,0);
  //glTexCoord2d(1,1);glVertex3f(mirFrameSize.x/2,mirFrameSize.y/2,0);
  //glTexCoord2d(0,1);glVertex3f(0,mirFrameSize.y/2,0);
  //glEnd();
  //glDisable(GL_TEXTURE_2D);
}

// 绘制窗口大小的平面
void ARDriver::DrawQuad()
{
	static bool bFirstRun = true;
	static GLuint nList;
	glDisable(GL_POLYGON_SMOOTH);
	glDisable(GL_BLEND);
	if(bFirstRun)
	{
		bFirstRun = false;
		nList = glGenLists(1);
		glNewList(nList, GL_COMPILE_AND_EXECUTE);
		// How many grid divisions in the x and y directions to use?
		int nStepsX = 24; // Pretty arbitrary..
		int nStepsY = (int) (nStepsX * ((double) mirFrameSize.x / mirFrameSize.y)); // Scaled by aspect ratio
		if(nStepsY < 2)
			nStepsY = 2;
		glColor3f(1,1,1);
		for(int ystep = 0; ystep<nStepsY; ystep++)
		{
			glBegin(GL_QUAD_STRIP);
			for(int xstep = 0; xstep<=nStepsX; xstep++)
				for(int yystep = ystep; yystep<=ystep + 1; yystep++) // Two y-coords in one go - magic.
				{
					Vector<2> v2Iter;
					v2Iter[0] = (double) xstep / nStepsX;
					v2Iter[1] = (double) yystep / nStepsY;
					Vector<2> v2UFBDistorted = v2Iter;
					Vector<2> v2UFBUnDistorted = mCamera.UFBLinearProject(mCamera.UFBUnProject(v2UFBDistorted));
					glTexCoord2d(v2UFBUnDistorted[0] * mirFBSize.x, (1.0-v2UFBUnDistorted[1]) * mirFBSize.y);
					glVertex(v2UFBDistorted);
				}
				glEnd();
		}
		glEndList();
	}
	else
		glCallList(nList);
}

void ARDriver::SetTexture(GLuint texId)
{
	glBindTexture(GL_TEXTURE_RECTANGLE,texId);
	glTexImage2D(GL_TEXTURE_RECTANGLE, 0,
		GL_RGBA, mirFBSize.x, mirFBSize.y, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

//void ARDriver::InitGaussTexture(GLuint texId)
//{
//	glBindTexture(GL_TEXTURE_2D,texId);
//	float fGaussKernel[]={0.01, 0.05, 0.09, 0.12, 0.15, 0.16, 0.15, 0.12, 0.09, 0.05, 0.01};
//	float *fGaussTexData = new float[11*4];
//	for (int i=0; i<11; ++i)
//	{
//		for (int j=0; j<4; ++j)
//		{
//			fGaussTexData[4*i+j] = fGaussKernel[i];
//		}
//	}
//	glTexImage2D(GL_TEXTURE_2D, 0,
//		GL_RGBA, 11, 1, 0,
//		GL_RGBA, GL_FLOAT, fGaussTexData);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
//	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//	delete [] fGaussTexData;
//}

/**
 * Make the frame buffer
 */
void ARDriver::MakeFrameBuffer()
{
  // Needs nvidia drivers >= 97.46
  cout << "  ARDriver: Creating FBO... ";
  std::cout<< "GL_VERSION: "<<glGetString(GL_VERSION) << std::endl;
  if (GLEW_EXT_framebuffer_object) 
  { 
	  std::cout<< "Driver support FBO\n" << std::endl;
  }
  //texture stuff
  glGenTextures(1, &mnFrameBufferTex);
  glBindTexture(GL_TEXTURE_RECTANGLE,mnFrameBufferTex);
  glTexImage2D(GL_TEXTURE_RECTANGLE, 0,
	       GL_RGBA, mirFBSize.x, mirFBSize.y, 0,
	       GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  SetTexture(mtex[TEX_OBJECT]);
  SetTexture(mtex[TEX_GHOST]);
  SetTexture(mtex[TEX_MEDIATOR]);
  SetTexture(mtex[TEX_BACKEDGE]);
  SetTexture(mtex[TEX_BOUNDARY]);
  SetTexture(mtex[TEX_EDGEMASK]);
  SetTexture(mtex[TEX_MIRROR]);

  //TEX_XTOON 读取XTOON.bmp图片作为纹理
  //glBindTexture(GL_TEXTURE_2D, mtex[TEX_TEST]);
  //AUX_RGBImageRec *texXtoonImage;
  //texXtoonImage = auxDIBImageLoad("MEDMASK.bmp");
  //if (texXtoonImage == NULL)
  //{
	 // std::cout << "no texture file,please convert rgb file to bmp file"<< std::cout;
	 // return;
  //}
  //glTexImage2D(GL_TEXTURE_2D, 0,
	 // 3, texXtoonImage->sizeX, texXtoonImage->sizeY, 0,
	 // GL_RGB, GL_UNSIGNED_BYTE, texXtoonImage->data);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  //if(texXtoonImage)
  //{
	 // if(texXtoonImage->data)
	 // {
		//  free(texXtoonImage->data);
	 // }
	 // free(texXtoonImage);
  //}
  //std::cout<< "Load TEX_TEST texture" << std::endl;
  //glBindTexture(GL_TEXTURE_2D, 0);

  //bind texture with FBO_OBJECT
  glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_DEPTH1]);
  glTexImage2D( GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT, mirFBSize.x, mirFBSize.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP );
  glTexParameterf( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP );
  glBindTexture(GL_TEXTURE_RECTANGLE, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, mfbo[FBO_OBJECT]);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_RECTANGLE, mtex[TEX_OBJECT], 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  //一定要加上depth，要不然会出现黑色的小三角，不知道为什么
				GL_TEXTURE_RECTANGLE, mtex[TEX_DEPTH1], 0);

  //bind texture with FBO_GHOST
  glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_DEPTH2]);
  glTexImage2D( GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT, mirFBSize.x, mirFBSize.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP );
  glTexParameterf( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP );
  glBindTexture(GL_TEXTURE_RECTANGLE, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, mfbo[FBO_GHOST]);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_RECTANGLE, mtex[TEX_GHOST], 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
				GL_TEXTURE_RECTANGLE, mtex[TEX_DEPTH2], 0);

  //bind texture with FBO_MED
  glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_DEPTH3]);
  glTexImage2D( GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT, mirFBSize.x, mirFBSize.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP );
  glTexParameterf( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP );
  glBindTexture(GL_TEXTURE_RECTANGLE, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, mfbo[FBO_MED]);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
				GL_TEXTURE_RECTANGLE, mtex[TEX_MEDIATOR], 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
				GL_TEXTURE_RECTANGLE, mtex[TEX_DEPTH3], 0);

  glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_DEPTH4]);
  glTexImage2D( GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT, mirFBSize.x, mirFBSize.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP );
  glTexParameterf( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP );
  glBindTexture(GL_TEXTURE_RECTANGLE, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, mfbo[FBO_MIRROR]);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
					GL_TEXTURE_RECTANGLE, mtex[TEX_MIRROR], 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
					GL_TEXTURE_RECTANGLE, mtex[TEX_DEPTH4], 0);

  //bind texture with FBO_EDGE
  glBindTexture(GL_TEXTURE_RECTANGLE, mtex[TEX_DEPTH5]);	// 绑定深度纹理
  // 设置纹理属性
  glTexImage2D( GL_TEXTURE_RECTANGLE, 0, GL_DEPTH_COMPONENT, mirFBSize.x, mirFBSize.y, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameterf( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP );
  glTexParameterf( GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP );
  glBindTexture(GL_TEXTURE_RECTANGLE, 0);	// 释放绑定
  // 绑定FBO_EDGE帧缓存
  glBindFramebuffer(GL_FRAMEBUFFER, mfbo[FBO_EDGE]);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_RECTANGLE, mtex[TEX_BACKEDGE], 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
				GL_TEXTURE_RECTANGLE, mtex[TEX_BOUNDARY], 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2,
				GL_TEXTURE_RECTANGLE, mtex[TEX_EDGEMASK], 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
				GL_TEXTURE_RECTANGLE, mtex[TEX_DEPTH5], 0);

  //bind texture with FBO_BLD
  glBindRenderbuffer(GL_RENDERBUFFER, mtex[TEX_DEPTH6]);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mirFBSize.x, mirFBSize.y);

  glBindFramebuffer(GL_FRAMEBUFFER, mfbo[FBO_BLD]);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			    GL_TEXTURE_RECTANGLE, mnFrameBufferTex, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
  			       GL_RENDERBUFFER, mtex[TEX_DEPTH6]);

  CheckFramebufferStatus();
  cout << " .. created FBO." << endl;
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}


/**
 * check the status of the frame buffer
 */
static bool CheckFramebufferStatus()
{
  GLenum n;
  n = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if(n == GL_FRAMEBUFFER_COMPLETE)
    return true; // All good
  
  cout << "glCheckFrameBufferStatus returned an error.dfa" << n << endl;
  return false;
}



/**
 * Draw the distorted frame buffer
 */
void ARDriver::DrawDistortedFB()
{
  static bool bFirstRun = true;
  static GLuint nList;
  mGLWindow.SetupViewport();
  mGLWindow.SetupUnitOrtho();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glEnable(GL_TEXTURE_RECTANGLE);
  //glBindTexture(GL_TEXTURE_RECTANGLE_ARB, mtex[TEX_OBJECT]);  //用于测试中间结果
  glBindTexture(GL_TEXTURE_RECTANGLE, mnFrameBufferTex);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glDisable(GL_POLYGON_SMOOTH);
  glDisable(GL_BLEND);
  if(bFirstRun)
  {
      bFirstRun = false;
      nList = glGenLists(1);
      glNewList(nList, GL_COMPILE_AND_EXECUTE);
      // How many grid divisions in the x and y directions to use?
      int nStepsX = 24; // Pretty arbitrary..
      int nStepsY = (int) (nStepsX * ((double) mirFrameSize.x / mirFrameSize.y)); // Scaled by aspect ratio
      if(nStepsY < 2)
		nStepsY = 2;
      glColor3f(1,1,1);
      for(int ystep = 0; ystep<nStepsY; ystep++)
	  {
		glBegin(GL_QUAD_STRIP);
		for(int xstep = 0; xstep<=nStepsX; xstep++)
			for(int yystep = ystep; yystep<=ystep + 1; yystep++) // Two y-coords in one go - magic.
			{
				Vector<2> v2Iter;
				v2Iter[0] = (double) xstep / nStepsX;
				v2Iter[1] = (double) yystep / nStepsY;
				Vector<2> v2UFBDistorted = v2Iter;
				Vector<2> v2UFBUnDistorted = mCamera.UFBLinearProject(mCamera.UFBUnProject(v2UFBDistorted));
				glTexCoord2d(v2UFBUnDistorted[0] * mirFBSize.x, (1.0 - v2UFBUnDistorted[1]) * mirFBSize.y);
				glVertex(v2UFBDistorted);
			}
		glEnd();
	  }
      glEndList();
  }
  else
    glCallList(nList);
  glDisable(GL_TEXTURE_RECTANGLE);
}


/**
 * What to do when the user clicks on the screen.
 * Calculates the 3d postion of the click on the plane
 * and passes info to a game, if there is one.
 * @param nButton the button pressed
 * @param irWin the window x, y location 
 */
void ARDriver::HandleClick(int nButton, ImageRef irWin )
{
  //The window may have been resized, so want to work out the coords based on the orignal image size
  Vector<2> v2VidCoords = mGLWindow.VidFromWinCoords( irWin );
  
  
  Vector<2> v2UFBCoords;
#ifdef WIN32
  Vector<2> v2PlaneCoords;   v2PlaneCoords[0] = numeric_limits<double>::quiet_NaN();   v2PlaneCoords[1] = numeric_limits<double>::quiet_NaN();
#else
  Vector<2> v2PlaneCoords;   v2PlaneCoords[0] = NAN;   v2PlaneCoords[1] = NAN;
#endif
  Vector<3> v3RayDirn_W;

  // Work out image coords 0..1:
  v2UFBCoords[0] = (v2VidCoords[0] + 0.5) / mCamera.GetImageSize()[0];
  v2UFBCoords[1] = (v2VidCoords[1] + 0.5) / mCamera.GetImageSize()[1];

  // Work out plane coords:
  Vector<2> v2ImPlane = mCamera.UnProject(v2VidCoords);
  Vector<3> v3C = unproject(v2ImPlane);
  Vector<4> v4C = unproject(v3C);
  SE3<> se3CamInv = mse3CfromW.inverse();
  Vector<4> v4W = se3CamInv * v4C;
  double t = se3CamInv.get_translation()[2];
  double dDistToPlane = -t / (v4W[2] - t);

  if(v4W[2] -t <= 0) // Clicked the wrong side of the horizon?
  {
    v4C.slice<0,3>() *= dDistToPlane;
    Vector<4> v4Result = se3CamInv * v4C;
    v2PlaneCoords = v4Result.slice<0,2>(); // <--- result
  }

  // Ray dirn:
  v3RayDirn_W = v4W.slice<0,3>() - se3CamInv.get_translation();
  normalize(v3RayDirn_W);

  if(mpMap->pGame) {
    mpMap->pGame->HandleClick(v2VidCoords, v2UFBCoords, v3RayDirn_W, v2PlaneCoords, nButton);
  }
}



/**
 * Handle the user pressing a key
 * @param sKey the key the user pressed.
 */
void ARDriver::HandleKeyPress( std::string sKey )
{
  if(mpMap && mpMap->pGame ) {
    mpMap->pGame->HandleKeyPress( sKey );
  }

}


/**printf("%s\n", );
 * Load a game by name.
 * @param sName Name of the game
 */
void ARDriver::LoadGame(std::string sName)
{
  if(mpMap->pGame)
  {
    delete mpMap->pGame;
    mpMap->pGame = NULL;
  }

  mpMap->pGame = LoadAGame( sName, "");
  if( mpMap->pGame ) {
    mpMap->pGame->Init();
  }
 
}



/**
 * Advance the game logic
 */
void ARDriver::AdvanceLogic()
{
  if(mpMap->pGame) {
    mpMap->pGame->Advance();
  }
}


}
