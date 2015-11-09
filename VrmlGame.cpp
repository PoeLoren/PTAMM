#include "VrmlGame.h"
#include "Map.h"
#include "KeyFrame.h"
#include "GLShader.h"
#include <cvd/gl_helpers.h>
#include <cvd/image_io.h>
#include <gvars3/instances.h>
#include <GL/GLAux.h>
#include <GL/glut.h>
#include "../opencv/include/opencv/cv.h"
#include "../opencv/include/opencv/cxcore.h"
#include "../opencv/include/opencv/highgui.h"
#include <fstream>
#include "readrgb.h"
#include "imdebuggl.h"
//#include <stdio.h>
//#include <stdlib.h>

namespace PTAMM{

#define printOpenGLError() printOglError(__FILE__, __LINE__)

#define SHADOW_MAP_RATIO 2
#define RENDER_WIDTH 640.0
#define RENDER_HEIGHT 480.0

static bool CheckFramebufferStatus();

	using namespace CVD;
	using namespace TooN;
	using namespace GVars3;
	extern "C" unsigned *read_texture(const char *name,int *width,int *height,int *components);

	VrmlGame::VrmlGame():Game("Vrml"),b(std::cout,std::cerr),ghostb(std::cout,std::cerr)
	{	
		mbInitialised = false;
		isInitMap = false;
		isBindTexture = false;

		std::vector<std::string> parameter;
		std::vector<std::string> uri;
		std::vector<std::string> ghosturi;

		uri.push_back("../vrmlmodels/BMW_2002__2_.wrl");	// 文件路径
		ghosturi.push_back("../vrmlmodels/desk.wrl");
		
		b.load_url(uri, parameter);	// 加载文件
		ghostb.load_url(ghosturi, parameter);
		//glGenTextures(1, texture);
		
		ObjectShadow.SetShaderFile("shaders/object.vs","shaders/object.fs");
		ObjectShadow.UseShader(false);
		GhostShadow.SetShaderFile("shaders/ghost.vs", "shaders/ghost.fs");
		GhostShadow.UseShader(false);
		MediatorShadow.SetShaderFile("shaders/mediator.vs", "shaders/mediator.fs");
		MediatorShadow.UseShader(false);
		BTShadow.SetShaderFile("shaders/bt.vs", "shaders/bt.fs");
		BTShadow.UseShader(false);
		glGenTextures(4, mtexs);
		std::cout<< mtexs[TEX_DEPTH] << "," << mtexs[TEX_MED] << "," << mtexs[TEX_EDGEMASK] << std::endl;
		GeneratrShadowFBO();  //产生
		InitMedMaskTexture(); //产生中介面的mask
		std::cout<<"VrmlGame generate method......"<<std::endl;

		LoadMatrixFile();
		bInit = false;
		position = -1.8;
	}

	int VrmlGame::printOglError(char *file, int line) const
	{
		GLenum glErr;
		int retCode = 0;
		glErr = glGetError();///获取错误
		while (glErr != GL_NO_ERROR)
		{
			printf("glError in file %s @ line %d: %s\n", file, line, gluErrorString(glErr));
			retCode = 1;
			glErr = glGetError();///获取下一个错误
		}
		return retCode;
	}

	//add by shihongzhi 2012.12.12
	void VrmlGame::LoadMatrixFile(){
		//读取matrix文件
		std::string matrixfile = "map000000/matrix.txt";
		std::ifstream ifsmatrix(matrixfile.c_str());
		if (!ifsmatrix)
		{
			std::cout << "can't open the matrix.txt file" <<std::endl;
		}
		//ifsmatrix>>scale;
		ifsmatrix>>PInverse(0, 0);
		ifsmatrix>>PInverse(0, 1);
		ifsmatrix>>PInverse(0, 2);
		ifsmatrix>>PInverse(0, 3);
		ifsmatrix>>PInverse(1, 0);
		ifsmatrix>>PInverse(1, 1);
		ifsmatrix>>PInverse(1, 2);
		ifsmatrix>>PInverse(1, 3);
		ifsmatrix>>PInverse(2, 0);
		ifsmatrix>>PInverse(2, 1);
		ifsmatrix>>PInverse(2, 2);
		ifsmatrix>>PInverse(2, 3);
		ifsmatrix>>PInverse(3, 0);
		ifsmatrix>>PInverse(3, 1);
		ifsmatrix>>PInverse(3, 2);
		ifsmatrix>>PInverse(3, 3);
		ifsmatrix>>scaleInverse;
		std::cout<<PInverse(0, 0)<<"\t"<<PInverse(0, 1)<<"\t"<<PInverse(0, 2)<<"\t"<<PInverse(0, 3)<<std::endl;
		std::cout<<PInverse(1, 0)<<"\t"<<PInverse(1, 1)<<"\t"<<PInverse(1, 2)<<"\t"<<PInverse(1, 3)<<std::endl;
		std::cout<<PInverse(2, 0)<<"\t"<<PInverse(2, 1)<<"\t"<<PInverse(2, 2)<<"\t"<<PInverse(2, 3)<<std::endl;
		std::cout<<PInverse(3, 0)<<"\t"<<PInverse(3, 1)<<"\t"<<PInverse(3, 2)<<"\t"<<PInverse(3, 3)<<std::endl;
		std::cout<<scaleInverse<<std::endl;
		ifsmatrix.close();
		std::cout<<"Load matrix.txt file date successed......"<<std::endl;
	}

	//读取boundary.bmp 和medmask.bmp
	void VrmlGame::InitMedMaskTexture(){
		AUX_RGBImageRec *textureImage[4] = {NULL, NULL, NULL}; 
		GLuint texids[4] = {mtexs[TEX_MED], mtexs[TEX_EDGEMASK], mtexs[TEX_BOX],mtexs[TEX_DESK]};
		char* filenames[4] = {"boundary.bmp", "boundary.bmp", "40.bmp", "desk_tex.bmp"};

		for(int i=0; i<4; i++)
		{
			textureImage[i] = auxDIBImageLoad(filenames[i]);
			if (textureImage[i] == NULL)
			{
				std::cout << "no texture file,please convert rgb file to bmp file"<< std::endl;
				return;
			}
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texids[i]);
			glTexImage2D(GL_TEXTURE_2D, 0,
				3, textureImage[i]->sizeX, textureImage[i]->sizeY, 0,
				GL_RGB, GL_UNSIGNED_BYTE, textureImage[i]->data);
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);//指定过滤模式  
			glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
			if(textureImage[i])
			{
				if(textureImage[i]->data)
				{
					free(textureImage[i]->data);
					std::cout<< "free textureImage"<<i<< std::endl;
				}
				free(textureImage[i]);
			}
		}
	}


	//更新位置
	void VrmlGame::update()
	{
		float light_mvnt = 0.5f;
		animationTrans[0] = light_mvnt * cos(glutGet(GLUT_ELAPSED_TIME)/1000.0);
		animationTrans[1] = 0.0f;
		animationTrans[2] = light_mvnt * sin(glutGet(GLUT_ELAPSED_TIME)/1000.0);
	}

	// 通过VRML文件中的三维物体绘制场景中的虚拟物体
	void VrmlGame::DrawVirtualObject()
	{
		//glPushMatrix();
		//glTranslatef(animationTrans[0],animationTrans[1],animationTrans[2]);

		//glMatrixMode(GL_TEXTURE);
		//glActiveTexture(GL_TEXTURE7);
		//glPushMatrix();
		//glTranslatef(animationTrans[0],animationTrans[1],animationTrans[2]);
		//glMatrixMode(GL_MODELVIEW);
		//Virtual object


		//glMatrixMode(GL_TEXTURE);
		//glActiveTexture(GL_TEXTURE7);
		//glPopMatrix();
		//glMatrixMode(GL_MODELVIEW);
		//glPopMatrix();


		//坐标系统（x，z，y），x正向朝右，y正向朝下，z正向朝外
		// 设置滚动小球

		//平移
		/*glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glTranslatef(position,0.2,0);
		glMatrixMode(GL_TEXTURE);
		glActiveTexture(GL_TEXTURE7);
		glPushMatrix();
		glTranslatef(position,0.2,0);
		glMatrixMode(GL_MODELVIEW);*/

		//绘制bunny
		//glPushMatrix();
		//glTranslatef(0,0.6,0.0);
		glScaled(0.8,0.8,0.8);
		//glTranslatef(-0.8f, 0.0f, -1.0f);
		std::vector<openvrml::node_ptr> mfn;
		mfn = b.root_nodes();
		if (mfn.size() == 0)
		{
			return;
		}
		else
		{
			for (unsigned int i=0; i<mfn.size(); i++)
			{
				openvrml::node* vrml_node = mfn[i].get();
				Draw3DFromVRML(vrml_node);
			}
		}
		//glPopMatrix();
		
		//glutSolidSphere(0.2, 20, 20);

		glMatrixMode(GL_TEXTURE);
		glActiveTexture(GL_TEXTURE7);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();

		position += 5.01;
		
		/*if (position>=-0.4)
		{
			position = -1.8;
		}*/
		
	}
	BOOL SaveDepthFromOpenGl( string lpFileName )  
	{  
		glEnable(GL_DEPTH_TEST); 
		int width = RENDER_WIDTH*SHADOW_MAP_RATIO;
		int height = RENDER_HEIGHT*SHADOW_MAP_RATIO;   

		int nAlignWidth = width + width%4;  
		float* pdata = new float[nAlignWidth * height];  
		memset( pdata, 0, nAlignWidth * height*sizeof(float) );  
		glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, pdata);  

		cv::Mat depth = cv::Mat::zeros( height, width, CV_32F );

		for ( int i=0; i<height; i++ )
		{
			for ( int j=0; j<width; j++ )
			{
				//depth.at<float>(i,j) = pdata[(height-1-i)*width+j];
				float val = pdata[(height-1-i)*width+j];
				if (val==1)
				{
					depth.at<float>(i,j) = 0;
				}
				else
				{
					depth.at<float>(i,j) = val;
				}
			}
		}


		cv::normalize( depth, depth, 0, 255, 32 );
		cv::imwrite( lpFileName, depth );

		delete[] pdata;  
		return TRUE;  
	}  
	// 用于测试程序，未用到
	void VrmlGame::TestDrawObject()
	{
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mtexs[TEX_EDGEMASK]);
		glBegin(GL_POLYGON);
		glNormal3f( 0.0f, 1.0f, 0.0f);
		glTexCoord2f(1.0, 1.0);	
		glVertex3f(0.2f, 0.0f, 0.2f);
		glTexCoord2f(1.0, 0.0); 
		glVertex3f(0.2f, 0.0f, -0.2f);
		glTexCoord2f(0.0, 0.0);	
		glVertex3f(-0.2f, 0.0f, -0.2f);
		glTexCoord2f(0.0, 1.0);	
		glVertex3f(-0.2f, 0.0f, 0.2f);
		glEnd();
		glDisable(GL_TEXTURE_2D);
	}

	// 画shadow mapping用于绘制阴影的深度图中的物体
	void VrmlGame::DrawDepthObject()
	{
		////mediator
		//glColor4f(0.8f, 0.8f, 0.8f, 1.0f);   //这里的alpha值在之后test.fs中用于判断是否为中介面  ---2.23
		//glBegin(GL_POLYGON);
		//glVertex3f(1.2f, 0.0f, 1.2f);
		//glVertex3f(1.2f, 0.0f, -1.2f);
		//glVertex3f(-1.2f, 0.0f, -1.2f);
		//glVertex3f(-1.2f, 0.0f, 1.2f);
		//glEnd();
        //glPushMatrix();
		glScaled(0.8,0.8,0.8);
		std::vector<openvrml::node_ptr> mfn;
		mfn = b.root_nodes();
		if (mfn.size() == 0)
		{
			return;
		}
		else
		{

            for (unsigned int i=0; i<mfn.size(); i++)
			{
				openvrml::node* vrml_node = mfn[i].get();
				Draw3DFromVRML(vrml_node);	// 从VRML中绘制出来
			}
		}
       // glPopMatrix();
		SaveDepthFromOpenGl("depth.jpg");
		DrawGhostObject();
		
		
		// 绘制滚动小球准备shadow map
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glTranslatef(position,0.2,0);
		glMatrixMode(GL_TEXTURE);
		glActiveTexture(GL_TEXTURE7);
		glPushMatrix();
		glTranslatef(position,0.2,0);
		glMatrixMode(GL_MODELVIEW);

		//glutSolidSphere(0.2, 20, 20);	// 绘制滚动小球
		//glutSolidOctahedron();

		glMatrixMode(GL_TEXTURE);
		glActiveTexture(GL_TEXTURE7);
		glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
	}

	void VrmlGame::DrawGhostObject()
	{
		std::vector<openvrml::node_ptr> mfn;
		mfn = ghostb.root_nodes();
		if (mfn.size() == 0)
		{
			return;
		}
		else
		{
			for (unsigned int i=0; i<mfn.size(); i++)
			{
				openvrml::node* vrml_node = mfn[i].get();
				Draw3DFromVRML(vrml_node);
			}
		}
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

		cout << "glCheckFrameBufferStatus returned an error."<< n<< endl;
		return false;
	}
	
	void VrmlGame::GeneratrShadowFBO()
	{
		int shadowMapWidth = RENDER_WIDTH * SHADOW_MAP_RATIO;
		int shadowMapHeight = RENDER_HEIGHT * SHADOW_MAP_RATIO;

		//GLfloat borderColor[4] = {0,0,0,0};

		GLenum FBOstatus;

		// Try to use a texture depth component
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mtexs[TEX_DEPTH]);

		// GL_LINEAR does not make sense for depth texture. However, next tutorial shows usage of GL_LINEAR and PCF
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		// Remove artefact on the edges of the shadowmap
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

		// No need to force GL_DEPTH_COMPONENT24, drivers usually give you the max precision if available 
		glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowMapWidth, shadowMapHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);

		// create a framebuffer object
		glGenFramebuffersEXT(1, &mfbos[FBO_SHADOW]);
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mfbos[FBO_SHADOW]);

		// Instruct openGL that we won't bind a color texture with the currently binded FBO
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);

		// attach the texture to FBO depth attachment point
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D, mtexs[TEX_DEPTH], 0);

		// check FBO status
		FBOstatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if(FBOstatus != GL_FRAMEBUFFER_COMPLETE)
			printf("GL_FRAMEBUFFER_COMPLETE_EXT failed, CANNOT use FBO\n");

		// switch back to window-system-provided framebuffer
		glBindFramebufferEXT(GL_FRAMEBUFFER, 0);
		std::cout<< "Generate Shadow FBO" << std::endl;
	}

	// 设置GL_TEXTURE7矩阵
	void VrmlGame::SetTextureMatrix()
	{
		double modelView[16];
		double projection[16];

		// This is matrix transform every coordinate x,y,z
		// x = x* 0.5 + 0.5 
		// y = y* 0.5 + 0.5 
		// z = z* 0.5 + 0.5 
		// Moving from unit cube [-1,1] to [0,1]  
		const GLdouble bias[16] = {	
			0.5, 0.0, 0.0, 0.0, 
			0.0, 0.5, 0.0, 0.0,
			0.0, 0.0, 0.5, 0.0,
			0.5, 0.5, 0.5, 1.0};

		// Grab modelview and transformation matrices
		glGetDoublev(GL_MODELVIEW_MATRIX, modelView);
		glGetDoublev(GL_PROJECTION_MATRIX, projection);


		glMatrixMode(GL_TEXTURE);
		glActiveTexture(GL_TEXTURE7);

		glLoadIdentity();	
		glLoadMatrixd(bias);

		// concatating all matrice into one.
		glMultMatrixd (projection);
		glMultMatrixd (modelView);

		// Go back to normal matrix mode
		glMatrixMode(GL_MODELVIEW);
	}

	// 绘制所有虚拟物体
	void VrmlGame::Draw3D( const GLWindow2 &glWindow, Map &map, SE3<> se3CfromW, ATANCamera &mCamera)
	{
		if(!bInit) {
			Init();
			std::cout<<"Init();"<<std::endl;

			bInit = true;
		}
		DrawDepthImage();		

		//add by shihongzhi at 2012.12.16
		//OpenGL的坐标转换是左乘的。
		//因为V绝对 = PV相对
		//所以V相对 = PinverseV绝对
		//因此需要MultMatrix Pinverse矩阵。并且注意MultMatrix操作的参数矩阵是列优先的。并且最后需要做scale操作
		for(int i=0; i<4; i++)
		{
			for(int j=0; j<4; j++)
			{
				PInversetrans[i*4 + j] = PInverse(j, i);
			}
		}

		//update();
		
		
 		Draw3DObject(se3CfromW, mCamera);
		Draw3DGhost(se3CfromW, mCamera);
		//Draw3DMirror(se3CfromW, mCamera);
        //test(se3CfromW, mCamera);
		Draw3DMediator(se3CfromW, mCamera);
		Draw3DBoundary(se3CfromW, mCamera);
        

		//TestDrawObject();
		////test shadow mapping
		////-----2012.12.19
		//glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//glUseProgram(0);
		//glMatrixMode(GL_PROJECTION);
		//glLoadIdentity();
		//glOrtho(-RENDER_WIDTH/2,RENDER_WIDTH/2,-RENDER_HEIGHT/2,RENDER_HEIGHT/2,1,20);
		//glMatrixMode(GL_MODELVIEW);
		//glLoadIdentity();
		//glColor4f(1,1,1,1);
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D,mtexs[TEX_EDGEMASK]);
		//glEnable(GL_TEXTURE_2D);
		//glTranslated(0,0,-10);
		//glBegin(GL_QUADS);
		//glTexCoord2d(0,0);glVertex3f(0,0,0);
		//glTexCoord2d(1,0);glVertex3f(RENDER_WIDTH/2,0,0);
		//glTexCoord2d(1,1);glVertex3f(RENDER_WIDTH/2,RENDER_HEIGHT/2,0);
		//glTexCoord2d(0,1);glVertex3f(0,RENDER_HEIGHT/2,0);
		//glEnd();
		//glDisable(GL_TEXTURE_2D);

		//glDisable(GL_LIGHTING);
		//glDisable(GL_DEPTH_TEST);
	}

	/***
	FBO_SHADOW	GL_DEPTH_ATTACHMENT		---->	mtexs[TEX_DEPTH]
	***/
	void VrmlGame::DrawDepthImage()
	{
		//画shadow 用shadow mapping需要帧缓存
		//http://fabiensanglard.net/shadowmapping/index.php
		glEnable(GL_DEPTH_TEST);

		glBindFramebuffer(GL_FRAMEBUFFER, mfbos[FBO_SHADOW]);
		CheckFramebufferStatus();	// 检查帧缓存状态是否出错
		glUseProgram(0);	
		glViewport(0,0,RENDER_WIDTH*SHADOW_MAP_RATIO,RENDER_HEIGHT*SHADOW_MAP_RATIO);	// 设置视图大小
		// Clear previous frame values
		glClear( GL_DEPTH_BUFFER_BIT);
		//Disable color rendering, we only want to write to the Z-Buffer
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);	// 只需深度，不用color
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(90,RENDER_WIDTH/RENDER_HEIGHT,0.1,40000);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(-0.2, 3, 2, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);   //设置产生阴影的灯光

		glEnable(GL_CULL_FACE);  //不要忘记剔除前景
		glCullFace(GL_FRONT);
		DrawDepthObject();

		SetTextureMatrix(); // 设置GL_TEXTURE7矩阵
		// 恢复状态
		glDisable(GL_CULL_FACE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDisable(GL_DEPTH_TEST);
	}

	/***
	FBO_OBJECT	GL_COLOR_ATTACHMENT0	---->	mtex[TEX_OBJECT]
	***/
	void VrmlGame::Draw3DObject(SE3<> se3CfromW, ATANCamera &mCamera)
	{
		//added by shihongzhi 2011.12.8  
		//Draw virtual object
		glBindFramebuffer(GL_FRAMEBUFFER,mfbos[FBO_OBJECT]);
		//ATTACHMENT0---TEX_OBJECT
		CheckFramebufferStatus();
		glViewport(0,0,RENDER_WIDTH,RENDER_HEIGHT);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST);
		// Set up 3D projection
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMultMatrix(mCamera.MakeUFBLinearFrustumMatrix(0.005, 100));
		//opengl staff
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_NORMALIZE);
		//glEnable(GL_COLOR_MATERIAL);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(0,0,0,0,0,-100,0,1,0);  //默认是这个的
		glMultMatrix(se3CfromW);
		glMultMatrixf(PInversetrans);
		glScaled(scaleInverse, scaleInverse, scaleInverse);


		//设置phong渲染时需要的光源
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		GLfloat af[4]; 
		af[0]=1.0; af[1]=1.0; af[2]=1.0; af[3]=1.0;
		glLightfv(GL_LIGHT0, GL_AMBIENT, af);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, af);
		af[0]=0.0; af[1]=30.0; af[2]=0.0; af[3]=1.0;
		glLightfv(GL_LIGHT0, GL_POSITION, af);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		ObjectShadow.UseShader(true);
		ObjectShadow.SetUniVar("xPixelOffset", (float)(1.0/(RENDER_WIDTH*SHADOW_MAP_RATIO)));
		ObjectShadow.SetUniVar("yPixelOffset", (float)(1.0/(RENDER_HEIGHT*SHADOW_MAP_RATIO)));
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE7);	// GL_TEXTURE7-shadow map的深度图纹理
		glBindTexture(GL_TEXTURE_2D, mtexs[TEX_DEPTH]);
		ObjectShadow.SetSampler("ShadowMap",7);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, mtexs[TEX_BOX]);
		ObjectShadow.SetSampler("texture", 6);
		//DrawVirtualObject();	// 通过VRML文件中的三维物体绘制场景中的虚拟物体
		//TestDrawObject();
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		ObjectShadow.UseShader(false);
	}

	/***
	FBO_GHOST	GL_COLOR_ATTACHMENT0	---->	mtex[TEX_GHOST]
	***/
	void VrmlGame::Draw3DGhost(SE3<> se3CfromW, ATANCamera &mCamera)
	{
		//added by shihongzhi 2012.12.28  
		//Draw ghost object
		glBindFramebuffer(GL_FRAMEBUFFER,mfbos[FBO_GHOST]);
		CheckFramebufferStatus();
		glViewport(0,0,RENDER_WIDTH,RENDER_HEIGHT);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST);
		// Set up 3D projection
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMultMatrix(mCamera.MakeUFBLinearFrustumMatrix(0.005, 100));
		
		//opengl staff
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_NORMALIZE);
		//glEnable(GL_COLOR_MATERIAL);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrix(se3CfromW);
		glMultMatrixf(PInversetrans);
		glScaled(scaleInverse, scaleInverse, scaleInverse);
		gluLookAt(0,0,0,0,0,-100,0,1,0);  //默认是这个的

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		GhostShadow.UseShader(true);
		glEnable(GL_TEXTURE_2D);
		GhostShadow.SetSampler("ShadowMap",7);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, mtexs[TEX_DEPTH]);
		DrawGhostObject();	// 通过VRML文件中的三维物体绘制场景中的ghost object
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		GhostShadow.UseShader(false);
	}

	void VrmlGame::Draw3DMirror(SE3<> se3CfromW, ATANCamera &mCamera)
	{
		glBindFramebuffer(GL_FRAMEBUFFER,mfbos[FBO_MIRROR]);
		//ATTACHMENT0---TEX_OBJECT
		CheckFramebufferStatus();
		glViewport(0,0,RENDER_WIDTH,RENDER_HEIGHT);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST);
		// Set up 3D projection
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMultMatrix(mCamera.MakeUFBLinearFrustumMatrix(0.005, 100));
		//opengl staff
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		glEnable(GL_NORMALIZE);
		//glEnable(GL_COLOR_MATERIAL);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(0,0,0,0,0,-100,0,1,0);  //默认是这个的
		glMultMatrix(se3CfromW);
		glMultMatrixf(PInversetrans);
		glScaled(scaleInverse, scaleInverse, scaleInverse);
		// 做了物体关于平面的颠倒
		GLfloat mirrorTrans[16] = 
        {
            1.0, 0.0,  0.0, 0.0,
            0.0, -1.0, 0.0, 0.0,
            0.0, 0.0,  1.0, 0.0,
            0.0, 0.0,  0.0, 1.0};	
		glMultMatrixf(mirrorTrans);
		//glMultMatrixf(mirrorTrans);

		/*
		//设置phong渲染时需要的光源
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		GLfloat af[4]; 
		af[0]=0.5; af[1]=0.5; af[2]=0.5; af[3]=1.0;
		glLightfv(GL_LIGHT0, GL_AMBIENT, af);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, af);
		af[0]=0.0; af[1]=10.0; af[2]=0.0; af[3]=1.0;
		glLightfv(GL_LIGHT0, GL_POSITION, af);
		*/

		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		ObjectShadow.UseShader(true);
		ObjectShadow.SetUniVar("xPixelOffset", (float)(1.0/(RENDER_WIDTH*SHADOW_MAP_RATIO)));
		ObjectShadow.SetUniVar("yPixelOffset", (float)(1.0/(RENDER_HEIGHT*SHADOW_MAP_RATIO)));
		glEnable(GL_TEXTURE_2D);
		ObjectShadow.SetSampler("ShadowMap",7);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, mtexs[TEX_DEPTH]);
		ObjectShadow.SetSampler("texture", 6);
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, mtexs[TEX_BOX]);
		DrawVirtualObject();	// 通过VRML文件中的三维物体绘制场景中的虚拟物体
		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		ObjectShadow.UseShader(false);
	}

	/***
	FBO_MED	GL_COLOR_ATTACHMENT0	---->	mtex[TEX_MEDIATOR]
	***/
	void VrmlGame::Draw3DMediator(SE3<> se3CfromW, ATANCamera &mCamera)
	{
		//added by shihongzhi 2012.12.28
		//Draw mediator object
		glBindFramebuffer(GL_FRAMEBUFFER,mfbos[FBO_MED]);
		//ATTACHMENT0---TEX_OBJECT
		CheckFramebufferStatus();
		glViewport(0,0,RENDER_WIDTH,RENDER_HEIGHT);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST);
		// Set up 3D projection
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMultMatrix(mCamera.MakeUFBLinearFrustumMatrix(0.005, 100));
		glMultMatrix(se3CfromW);
		glMultMatrixf(PInversetrans);
		glScaled(scaleInverse, scaleInverse, scaleInverse);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glDisable(GL_LIGHTING);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		MediatorShadow.UseShader(true);
		MediatorShadow.SetUniVar("xPixelOffset", (float)(1.0/(RENDER_WIDTH*SHADOW_MAP_RATIO)));
		MediatorShadow.SetUniVar("yPixelOffset", (float)(1.0/(RENDER_HEIGHT*SHADOW_MAP_RATIO)));
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE7);
		MediatorShadow.SetSampler("ShadowMap",7);
		glBindTexture(GL_TEXTURE_2D,mtexs[TEX_DEPTH]);
		

		glActiveTexture(GL_TEXTURE0);
		MediatorShadow.SetSampler("MediatorTexture",0);
		glBindTexture(GL_TEXTURE_2D, mtexs[TEX_DESK]);
		
			//中间的正方形 不产生EDGE
			//glColor4f(0.53725f, 0.5608f, 0.553f, 1.0f);
			//glActiveTexture(GL_TEXTURE0);
			//glBindTexture(GL_TEXTURE_2D, 0);
			//DrawMediator();	// 绘制中介面
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_CULL_FACE);
		MediatorShadow.UseShader(false);
	}

	//added by shihongzhi 2013.1.11
	//绘制扣边mask和过度透明mask
	void VrmlGame::Draw3DBoundary(SE3<> se3CfromW, ATANCamera &mCamera)
	{
		glDisable(GL_LIGHTING);
		
		//FBO_EDGE	GL_COLOR_ATTACHMENT1	---->	mtex[TEX_BOUNDARY]
		glBindFramebuffer(GL_FRAMEBUFFER,mfbos[FBO_EDGE]);
		CheckFramebufferStatus();
		glViewport(0,0,RENDER_WIDTH,RENDER_HEIGHT);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDrawBuffer(GL_COLOR_ATTACHMENT1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_BACK);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMultMatrix(mCamera.MakeUFBLinearFrustumMatrix(0.005, 100));
		glMultMatrix(se3CfromW);
		glMultMatrixf(PInversetrans);
		glScaled(scaleInverse, scaleInverse, scaleInverse);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glDisable(GL_LIGHTING);
		glColor4f(0.725f, 0.78f, 0.78f, 1.0f);
		glEnable(GL_TEXTURE_2D);
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, mtexs[TEX_MED]);
		BTShadow.UseShader(true);
		ObjectShadow.SetSampler("texture", 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mtexs[TEX_MED]);
		DrawMediator();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		BTShadow.UseShader(false);
//imdebugTexImage(GL_TEXTURE_2D, mtexs[TEX_MED], GL_RGBA);

		//FBO_EDGE	GL_COLOR_ATTACHMENT2	---->	mtex[TEX_EDGEMASK]
		glBindFramebuffer(GL_FRAMEBUFFER,mfbos[FBO_EDGE]);
		CheckFramebufferStatus();
		glViewport(0,0,RENDER_WIDTH,RENDER_HEIGHT);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glDrawBuffer(GL_COLOR_ATTACHMENT2);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glCullFace(GL_BACK);
		glEnable(GL_DEPTH_TEST);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMultMatrix(mCamera.MakeUFBLinearFrustumMatrix(0.005, 100));
		glMultMatrix(se3CfromW);
		glMultMatrixf(PInversetrans);
		glScaled(scaleInverse, scaleInverse, scaleInverse);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mtexs[TEX_EDGEMASK]);
		//std::cout<< mtexs[TEX_EDGEMASK] << std::endl;
		DrawMediator();
		glDisable(GL_TEXTURE_2D);
	}

	//add by shihongzhi 2013.1.11
	//用于绘制中介面
	/*	
	void VrmlGame::DrawMediator()
	{
		glBegin(GL_POLYGON);
		glNormal3f( 0.0f, 1.0f, 0.0f);
		glTexCoord2f(1.0, 1.0);	
		glVertex3f(1.7f, 0.0f, 1.3f);
		glTexCoord2f(1.0, 0.0); 
		glVertex3f(1.7f, 0.0f, -1.3f);
		glTexCoord2f(0.0, 0.0);	
		glVertex3f(-1.7f, 0.0f, -1.3f);
		glTexCoord2f(0.0, 1.0);	
		glVertex3f(-1.7f, 0.0f, 1.3f);
		glEnd();
	}
	*/
	//用于绘制中介面
	void VrmlGame::DrawMediator()
	{
		glBegin(GL_POLYGON);
		glNormal3f( 0.0f, 1.0f, 0.0f);
		glTexCoord2f(1.0, 1.0);	
		glVertex3f(1.0f, 0.0f, 1.3f);
		glTexCoord2f(1.0, 0.0); 
		glVertex3f(1.0f, 0.0f, -1.3f);
		glTexCoord2f(0.0, 0.0);	
		glVertex3f(-1.0f, 0.0f, -1.3f);
		glTexCoord2f(0.0, 1.0);	
		glVertex3f(-1.0f, 0.0f, 1.3f);
		glEnd();
	}
	void VrmlGame::Draw3DHand(SE3<> se3CfromW, ATANCamera &mCamera)
	{
		
	}

    void VrmlGame::test(SE3<> se3CfromW, ATANCamera &mCamera)
    {
        glBindFramebuffer(GL_FRAMEBUFFER,mfbos[FBO_MED]);
        //ATTACHMENT0---TEX_OBJECT
        CheckFramebufferStatus();
        glViewport(0,0,RENDER_WIDTH,RENDER_HEIGHT);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glCullFace(GL_BACK);
        glEnable(GL_DEPTH_TEST);
        // Set up 3D projection
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glMultMatrix(mCamera.MakeUFBLinearFrustumMatrix(0.005, 100));
        glMultMatrix(se3CfromW);
        glMultMatrixf(PInversetrans);
        glScaled(scaleInverse, scaleInverse, scaleInverse);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glDisable(GL_LIGHTING);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        MediatorShadow.UseShader(true);
        MediatorShadow.SetUniVar("xPixelOffset", (float)(1.0/(RENDER_WIDTH*SHADOW_MAP_RATIO)));
        MediatorShadow.SetUniVar("yPixelOffset", (float)(1.0/(RENDER_HEIGHT*SHADOW_MAP_RATIO)));
        glEnable(GL_TEXTURE_2D);
        MediatorShadow.SetSampler("ShadowMap",7);
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D,mtexs[TEX_MED]);
        MediatorShadow.SetSampler("MediatorTexture",0);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, mtexs[TEX_DEPTH]);
        //中间的正方形 不产生EDGE
        glColor4f(0.53725f, 0.5608f, 0.553f, 1.0f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mtexs[TEX_DESK]);
        glBegin(GL_POLYGON);
            glNormal3f( 0.0f, 1.0f, 0.0f);
            glTexCoord2f(1.0, 1.0);	
            glVertex3f(1.0f, 0.0f, 1.3f);
            glTexCoord2f(1.0, 0.0); 
            glVertex3f(1.0f, 0.0f, -1.3f);
            glTexCoord2f(0.0, 0.0);	
            glVertex3f(-1.0f, 0.0f, -1.3f);
            glTexCoord2f(0.0, 1.0);	
            glVertex3f(-1.0f, 0.0f, 1.3f);
        glEnd();
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_CULL_FACE);
        MediatorShadow.UseShader(false);
    }
	// 从VRML中绘制三维物体
	void VrmlGame::Draw3DFromVRML(openvrml::node *obj)
	{
		try
		{
			//可能抛出openvrml::unsupported_interface异常
			const openvrml::field_value &fv_appearance = obj->field("appearance");
			if (fv_appearance.type() == openvrml::field_value::sfnode_id)
			{
				//std::cout<<"appearance"<<std::endl;
				const openvrml::sfnode &sfn = dynamic_cast<const openvrml::sfnode&>(fv_appearance);
				openvrml::vrml97_node::appearance_node *vrml_app =
					static_cast<openvrml::vrml97_node::appearance_node*>(sfn.value.get()->to_appearance());
				const openvrml::node_ptr &vrml_material_node = vrml_app->material();
				const openvrml::node_ptr &vrml_texture_node = vrml_app->texture();

				const openvrml::vrml97_node::material_node *vrml_material =
					dynamic_cast<const openvrml::vrml97_node::material_node *>(vrml_material_node.get());
				if (vrml_material != NULL)
				{
					// 读取VRML文件中物体材质
					GLfloat temp_material_mat[4];
					temp_material_mat[0] = vrml_material->ambient_intensity();
					temp_material_mat[1] = vrml_material->ambient_intensity();
					temp_material_mat[2] = vrml_material->ambient_intensity();
					temp_material_mat[3] = 1.0;
                    /*std::cout << "GL_AMBIENT:" << endl;
                    std::cout << temp_material_mat[0] << " " << temp_material_mat[1] << " " << temp_material_mat[2] << " " << temp_material_mat[3] << endl;*/
					glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, temp_material_mat);
					temp_material_mat[0] = vrml_material->diffuse_color().r();
					temp_material_mat[1] = vrml_material->diffuse_color().g();
					temp_material_mat[2] = vrml_material->diffuse_color().b();
					temp_material_mat[3] = 1.0;
                    /*std::cout << "GL_DIFFUSE:" << endl;
                    std::cout << temp_material_mat[0] << " " << temp_material_mat[1] << " " << temp_material_mat[2] << " " << temp_material_mat[3] << endl;*/
					glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, temp_material_mat);
					temp_material_mat[0] = vrml_material->emissive_color().r();
					temp_material_mat[1] = vrml_material->emissive_color().g();
					temp_material_mat[2] = vrml_material->emissive_color().b();
					temp_material_mat[3] = 1.0;
                    /*std::cout << "GL_EMISSION:" << endl;
                    std::cout << temp_material_mat[0] << " " << temp_material_mat[1] << " " << temp_material_mat[2] << " " << temp_material_mat[3] << endl;*/
					glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, temp_material_mat);
					temp_material_mat[0] =vrml_material->specular_color().r();
					temp_material_mat[1] =vrml_material->specular_color().g();
					temp_material_mat[2] =vrml_material->specular_color().b();
					temp_material_mat[3] = 1.0;
                    /*std::cout << "GL_SPECULAR:" << endl;
                    std::cout << temp_material_mat[0] << " " << temp_material_mat[1] << " " << temp_material_mat[2] << " " << temp_material_mat[3] << endl;*/
					glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, temp_material_mat);
					glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, vrml_material->shininess());

					if (vrml_material->transparency() > 0.0f)
					{
						//设置透明性
					}
					else
					{

					}
					//std::cout<<"material"<<std::endl;
				}
                #pragma region bind texture
				//bind texture
				const openvrml::vrml97_node::image_texture_node *vrml_texture = 
					dynamic_cast<const openvrml::vrml97_node::image_texture_node *>(vrml_texture_node.get());
				if (vrml_texture != 0)
				{
					const openvrml::field_value &texture_url_fv = vrml_texture->field("url");
					const openvrml::mfstring &mfs = dynamic_cast<const openvrml::mfstring &>(texture_url_fv);

					std::pair<set<std::string>::iterator, bool> ret;

					//修改后缀为bmp
					std::string url = mfs.value[0];
					std::string dot(".");
					std::size_t found;
					found = url.find(dot);
					if (found != std::string::npos)
					{
						std::string bmp("bmp");
						url.replace(found+1,3,bmp);
						ret = textureUrls.insert(url);
						//std::cout<< url.c_str() << std::endl;
					}
					std::string common(":");
					std::size_t common_pos;
					common_pos = url.find(common);
					if(common_pos != std::string::npos)
					{
						url = url.substr(common_pos+1,url.size()-common_pos);
					}
					if (ret.second == true)  //如果url插入成功了，即来了一个新的url，则设置isBindTexture，使得之后生成新的纹理
					{
						isBindTexture = false;
					}

					//即查找现在的url为第一个纹理
					int count = 0;
					for (std::set<std::string>::iterator it=textureUrls.begin(); it!=textureUrls.end(); it++,count++)
					{
						if (*it == *ret.first)
						{
							textureNum = count;
							//std::cout<< count << std::endl;
							break;
						}
					}
					//绑定纹理
					if (!isBindTexture)
					{
						glEnable(GL_TEXTURE_2D);
						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, texture[textureNum]);
						AUX_RGBImageRec *texImage;
						texImage = auxDIBImageLoad(url.c_str());
						
						if (texImage == NULL)
						{
							std::cout << "no texture file,please convert rgb file to bmp file"<< std::cout;
							return;
						}
						glTexImage2D(GL_TEXTURE_2D, 0, 3,texImage->sizeX, texImage->sizeY,
							0, GL_RGB, GL_UNSIGNED_BYTE, texImage->data);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

						if(texImage)
						{
							if(texImage->data)
							{
								free(texImage->data);
							}
							free(texImage);
						}
						//std::cout<< texture << std::endl;
						std::cout<<"bind texture ok"<<std::endl;
						isBindTexture = true;
						glDisable(GL_TEXTURE_2D);
					}
				}
                #pragma endregion
			}
		}
		catch(openvrml::unsupported_interface&)
		{

		}
		

		std::string name = obj->id();
        /*static int i = 0;
        std::cout << i++ << name << std::endl;*/
		if (obj->type.id == "Group")
		{
			//std::cout<<"Group"<<std::endl;
			openvrml::vrml97_node::group_node *vrml_group;
			vrml_group = dynamic_cast<openvrml::vrml97_node::group_node *>(obj);

			try
			{
				const openvrml::field_value &fv = obj->field("children");

				if (fv.type() == openvrml::field_value::mfnode_id)
				{
					const openvrml::mfnode &mfn = dynamic_cast<const openvrml::mfnode &>(fv);
                    /*bool flag = TRUE;
                    if(flag){
                        std::cout << "group:children():mfn.value.size()= " << mfn.value.size() << std::endl;
                        flag = FALSE;
                    }*/
					for (unsigned int i=0; i<mfn.value.size(); i++)
					{
						openvrml::node *node = mfn.value[i].get();
						Draw3DFromVRML(node);
					}
				}
			}
			catch (openvrml::unsupported_interface&)
			{
				//no children
			}
		}
		else if (obj->type.id == "Transform")	// 位置平移、旋转等
		{
			//std::cout<<"Transform"<<std::endl;
			openvrml::vrml97_node::transform_node* vrml_transform;
			vrml_transform = dynamic_cast<openvrml::vrml97_node::transform_node*>(obj);

			openvrml::mat4f vrml_m = vrml_transform->transform();

			//opengl 添加转换
			glPushMatrix();
			GLfloat temp_trans[] = {vrml_m[0][0], vrml_m[0][1], vrml_m[0][2], vrml_m[0][3], vrml_m[1][0], vrml_m[1][1], vrml_m[1][2], vrml_m[1][3], vrml_m[2][0], vrml_m[2][1], vrml_m[2][2], vrml_m[2][3], vrml_m[3][0], vrml_m[3][1], vrml_m[3][2], vrml_m[3][3]};
			//glMultTransposeMatrixf(temp_trans);  
			glMultMatrixf(temp_trans);	//不需要转制，列优先
			//std::cout<< "glPushMatrix()" << std::endl;

			//add 12.9
			glMatrixMode(GL_TEXTURE);
			glActiveTexture(GL_TEXTURE7);
			glPushMatrix();
			glMultMatrixf(temp_trans);
			glMatrixMode(GL_MODELVIEW);  //add at 2013.2.25
			try
			{
				const openvrml::field_value &fv = obj->field("children");

				if (fv.type() == openvrml::field_value::mfnode_id)
				{
					const openvrml::mfnode &mfn = dynamic_cast<const openvrml::mfnode &>(fv);
                    /*bool flag = TRUE;
                    if(flag){
                        std::cout << "transform:children():mfn.value.size()= " << mfn.value.size() << std::endl;
                        flag = FALSE;
                    }*/
					for (unsigned int i=0; i<mfn.value.size(); i++)
					{
						openvrml::node *node = mfn.value[i].get();
						Draw3DFromVRML(node);
					}
				}
			}
			catch(openvrml::unsupported_interface&)
			{
				//no children
			}
			glMatrixMode(GL_TEXTURE);   //add at 2013.2.25
			glPopMatrix();
			glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		}
		else if(obj->type.id == "Shape")	// 形状绘制
		{
			const openvrml::field_value &fv = obj->field("geometry");
			if (fv.type() == openvrml::field_value::sfnode_id)
			{
				const openvrml::sfnode &sfn = dynamic_cast<const openvrml::sfnode&>(fv);

				openvrml::vrml97_node::abstract_geometry_node* vrml_geom =
					static_cast<openvrml::vrml97_node::abstract_geometry_node*>(sfn.value.get()->to_geometry());

				if (openvrml::vrml97_node::indexed_face_set_node* vrml_ifs = dynamic_cast<openvrml::vrml97_node::indexed_face_set_node *>(vrml_geom))
				{
					// 绘制面片 基本都用这个
					DrawIndexedFaceSet(vrml_ifs);
				}
				else if (openvrml::vrml97_node::box_node* vrml_box = dynamic_cast<openvrml::vrml97_node::box_node*>(vrml_geom))
				{
					//opengl box
					//const openvrml::vec3f& size = static_cast<const openvrml::sfvec3f&>(vrml_box->field("size")).value;
					std::cout<<"box"<<std::endl;
					DrawBox(vrml_box);
				}
				else if (openvrml::vrml97_node::sphere_node* vrml_sphere = dynamic_cast<openvrml::vrml97_node::sphere_node*>(vrml_geom))
				{
					//sphere
					std::cout<<"sphere"<<std::endl;
					DrawSphere(vrml_sphere);
				}
				else if (openvrml::vrml97_node::cone_node* vrml_cone = dynamic_cast<openvrml::vrml97_node::cone_node*>(vrml_geom))
				{
					//cone
				}
				else if (openvrml::vrml97_node::cylinder_node* vrml_cylinder = dynamic_cast<openvrml::vrml97_node::cylinder_node*>(vrml_geom))
				{
					//cylinder
				}
				else
				{

				}
			}
		}
	}

	void VrmlGame::DrawBox(openvrml::vrml97_node::box_node* vrml_box) const
	{
		const openvrml::vec3f& size = static_cast<const openvrml::sfvec3f&>(vrml_box->field("size")).value;
		std::cout << size[0]<< ","<< size[1]<<","<< size[2]<<std::endl;
		GLfloat a,b,c;
		a = size[0]*0.5;
		b = size[1]*0.5;
		c = size[2]*0.5;
		static GLfloat vertex_list[][3] = {
			-a,-b,-c,
			 a,-b,-c,
		    -a, b,-c,
			 a, b,-c,
		    -a,-b, c,
			 a,-b, c,
		    -a, b, c,
			 a, b, c,
		};
		static GLint index_list[][4] = {
			0,2,3,1,
			0,4,6,2,
			0,1,5,4,
			4,5,7,6,
			1,3,7,5,
			2,6,7,3,
		};
		int i,j;
		glColor3f(0.0,0.0,0.0);
		glBegin(GL_QUADS);
			for(i=0; i<6; ++i)
				for(j=0; j<6; ++j)
					glVertex3fv(vertex_list[index_list[i][j]]);
		glEnd();
	}

	void VrmlGame::DrawSphere(openvrml::vrml97_node::sphere_node* vrml_sphere) const
	{
		float radius = static_cast<const openvrml::sffloat&>(vrml_sphere->field("radius")).value;
		GLUquadricObj* quadric = NULL;
		quadric = gluNewQuadric();
		gluSphere(quadric, radius, 36, 36);
		gluDeleteQuadric(quadric);
	}

	bool VrmlGame::isEqual(float m, float n) const
	{
		if(abs(m-n) < 0.00001)
			return true;
		else
			return false;
	}

	// 绘制面片 一般情况下都不会用到color和normal参数
	void VrmlGame::DrawIndexedFaceSet(openvrml::vrml97_node::indexed_face_set_node* vrml_ifs) const
	{
		//get array of coordinate_nodes
		{
			//coord value
			const openvrml::field_value& fv_coord = vrml_ifs->field("coord");
			const openvrml::sfnode& sfn_coord = dynamic_cast<const openvrml::sfnode&>(fv_coord);
			openvrml::vrml97_node::coordinate_node* vrml_coord_node = 
				dynamic_cast<openvrml::vrml97_node::coordinate_node*>(sfn_coord.value.get());
			//texture value
			const openvrml::field_value &fv_texCoord = vrml_ifs->field("texCoord");
			const openvrml::sfnode &sfn = dynamic_cast<const openvrml::sfnode &>(fv_texCoord);
			openvrml::vrml97_node::texture_coordinate_node *vrml_tex_coord_node =
				dynamic_cast<openvrml::vrml97_node::texture_coordinate_node *>(sfn.value.get());
			//color value
			const openvrml::field_value &fv_color = vrml_ifs->field("color");
			const openvrml::sfnode &sfn_color = dynamic_cast<const openvrml::sfnode &>(fv_color);
			openvrml::vrml97_node::color_node *vrml_color_node =
				dynamic_cast<openvrml::vrml97_node::color_node *>(sfn_color.value.get());
			//normal value
			const openvrml::field_value &fv_normal = vrml_ifs->field("normal");
			const openvrml::sfnode &sfn_normal = dynamic_cast<const openvrml::sfnode&>(fv_normal);
			openvrml::vrml97_node::normal_node* vrml_normal_node =
				dynamic_cast<openvrml::vrml97_node::normal_node*>(sfn_normal.value.get());

			//coord index
			const std::vector<openvrml::vec3f>& vrml_coord = vrml_coord_node->point();
			const openvrml::field_value &fv2_coord = vrml_ifs->field("coordIndex");
			const openvrml::mfint32 &vrml_coord_index = dynamic_cast<const openvrml::mfint32&>(fv2_coord);

			//texture index
			std::vector<openvrml::vec2f> vrml_tex_coord;
			std::vector<int> tex_coord_index;
			if (vrml_tex_coord_node != 0)
			{
				vrml_tex_coord = vrml_tex_coord_node->point();
				const openvrml::field_value &fv2_texCoord = vrml_ifs->field("texCoordIndex");
				const openvrml::mfint32 &vrml_tex_coord_index = dynamic_cast<const openvrml::mfint32 &>(fv2_texCoord);
				if (vrml_tex_coord_index.value.size() > 0)  //一般是这种情况
				{
					for (unsigned int i=0; i<vrml_tex_coord_index.value.size(); i++)
					{
						int index = vrml_tex_coord_index.value[i];
						if (index != -1)
						{
							tex_coord_index.push_back(index);
						}
					}
				}
				else
				{
					for (unsigned int i=0; i<vrml_tex_coord_index.value.size(); i++)
					{
						tex_coord_index.push_back(i);
					}
				}
			}
 
			//color index
			//这里有错误
			//引用初始化问题不知道如何解决
			//const std::vector<openvrml::color>& vrml_colors = vrml_color_node->color();
			std::vector<openvrml::color> vrml_colors;
			std::vector<int> color_index;
			bool is_vrml_color_per_vertex;
			if (vrml_color_node != 0)
			{
				vrml_colors = vrml_color_node->color();
				const openvrml::field_value &fv2_color = vrml_ifs->field("colorIndex");
				const openvrml::mfint32 &vrml_color_index = dynamic_cast<const openvrml::mfint32 &>(fv2_color);
				if (vrml_color_index.value.size() > 0)
				{
					for (unsigned int i=0; i<vrml_color_index.value.size(); i++)
					{
						int index = vrml_color_index.value[i];
						if (index != -1)
						{
							color_index.push_back(index);
						}
					}
				} 
				else
				{
					for (unsigned int i=0; i<vrml_coord_index.value.size(); i++)
					{
						color_index.push_back(i);
					}
				}
				const openvrml::field_value &fv3_color = vrml_ifs->field("colorPerVertex");
				const openvrml::sfbool &vrml_color_per_vertex = dynamic_cast<const openvrml::sfbool &>(fv3_color);
				is_vrml_color_per_vertex = vrml_color_per_vertex.value;
			}

			//normal index
			std::vector<openvrml::vec3f> vrml_normal_coord;
			std::vector<int> normal_index;
			bool is_vrml_normal_per_vertex;
			if (vrml_normal_node != 0)
			{
				vrml_normal_coord = vrml_normal_node->vector();
				const openvrml::field_value &fv2_normal = vrml_ifs->field("normalIndex");
				const openvrml::mfint32 &vrml_normal_index = dynamic_cast<const openvrml::mfint32&>(fv2_normal);
				if (vrml_normal_index.value.size() > 0)
				{
					for (unsigned int i=0; i<vrml_normal_index.value.size(); i++)
					{
						int index = vrml_normal_index.value[i];
						if (index != -1)
						{
							normal_index.push_back(index);
						}
					}
				}
				else
				{
					for (unsigned int i=0; i<vrml_coord_index.value.size(); i++)
					{
						normal_index.push_back(i);
					}
				}
				const openvrml::field_value &fv3_normal = vrml_ifs->field("normalPerVertex");
				const openvrml::sfbool &vrml_norm_per_vertex = dynamic_cast<const openvrml::sfbool &>(fv3_normal);
				is_vrml_normal_per_vertex = vrml_norm_per_vertex.value;
			}


			std::vector<int> vert_index;
			int num_vert = 0;  //标记总共画了一个vertex
			int num_polygon = 0;
            /*GLfloat mat[4][4];
            glGetMaterialfv(GL_FRONT,GL_DIFFUSE,mat[0]);
            glGetMaterialfv(GL_FRONT,GL_SPECULAR,mat[1]);
            glGetMaterialfv(GL_FRONT,GL_EMISSION,mat[2]);
            glGetMaterialfv(GL_FRONT,GL_AMBIENT,mat[3]);
            for (int i = 0; i< 4;i ++)
            {
                for (int j = 0;j < 4;j++)
                {
                    std::cout << mat[i][j] << " ";
                }
                std::cout << std::endl;
            }*/
            /*std::cout << "vrml_coord_index.value.size() = " <<vrml_coord_index.value.size() << std::endl;*/
			for (unsigned int i=0; i<vrml_coord_index.value.size(); i++)
			{
				int index = vrml_coord_index.value[i];
				if (index == -1)
				{
					GLfloat normal[3];
					GLfloat vc1[3],vc2[3];
					GLfloat a,b,c;
					GLdouble r;
					vc1[0] = vrml_coord[vert_index[1]][0] - vrml_coord[vert_index[0]][0];
					vc1[1] = vrml_coord[vert_index[1]][1] - vrml_coord[vert_index[0]][1];
					vc1[2] = vrml_coord[vert_index[1]][2] - vrml_coord[vert_index[0]][2];
					
					vc2[0] = vrml_coord[vert_index[2]][0] - vrml_coord[vert_index[0]][0];
					vc2[1] = vrml_coord[vert_index[2]][1] - vrml_coord[vert_index[0]][1];
					vc2[2] = vrml_coord[vert_index[2]][2] - vrml_coord[vert_index[0]][2];
					a = vc1[1] * vc2[2] - vc2[1] * vc1[2];
					b = vc2[0] * vc1[2] - vc1[0] * vc2[2];
					c = vc1[0] * vc2[1] - vc2[0] * vc1[1];
					r = sqrt( a * a + b* b + c * c);
					normal[0] = (isEqual(a, 0.0) ? 0.0 : a/r);
					normal[1] = (isEqual(b, 0.0) ? 0.0 : b/r);
					normal[2] = (isEqual(c, 0.0) ? 0.0 : c/r);
					
					//set color if have color
					if (vrml_color_node != 0 && !is_vrml_color_per_vertex)
					{
						//glColor4f(vrml_colors[color_index[num_polygon]][0],vrml_colors[color_index[num_polygon]][1],vrml_colors[color_index[num_polygon]][2], 1.0f);
					}
					else if(vrml_color_node == 0) //如果没有颜色属性，而且没有问题，则默认为白色
					{
						glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
					}
					//set normal if polygon
					if (vrml_normal_node !=0 && !is_vrml_normal_per_vertex)
					{
						glNormal3f(vrml_normal_coord[normal_index[num_polygon]][0],vrml_normal_coord[normal_index[num_polygon]][1],vrml_normal_coord[normal_index[num_polygon]][2]);
					}

					//COMMNET AT 2013.3.7
					//printOpenGLError();
					if (vrml_tex_coord_node != 0)
					{
						//glEnable(GL_TEXTURE_2D);
						
						//glActiveTexture(GL_TEXTURE0);
						//glBindTexture(GL_TEXTURE_2D, mtexs[TEX_EDGEMASK]);
						//std::cout<<"textureNum:"<<textureNum<<std::endl;
					}
                    
					glBegin(GL_POLYGON);
					for (std::vector<int>::iterator it = vert_index.begin(); it!=vert_index.end(); ++it)
					{
						//set color if per vertex
						if (vrml_color_node != 0 && is_vrml_color_per_vertex)
						{
							//glColor4f(vrml_colors[color_index[num_vert]][0],vrml_colors[color_index[num_vert]][1],vrml_colors[color_index[num_vert]][2], 1.0f);
							//glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
						}
						
						//set normal if per vertex
						if (vrml_normal_node !=0 && is_vrml_normal_per_vertex)
						{
							glNormal3f(vrml_normal_coord[normal_index[num_vert]][0],vrml_normal_coord[normal_index[num_vert]][1],vrml_normal_coord[normal_index[num_vert]][2]);
						}
						else if (vrml_normal_node == 0) //default
						{
							//glNormal3f(vrml_coord[*it][0],vrml_coord[*it][1],vrml_coord[*it][2]);
							glNormal3f(normal[0], normal[1], normal[2]);
						}
						
						//set texture coord
						if (vrml_tex_coord_node != 0)
						{
							//std::cout<<"vrml_tex_coord_node != 0"<<std::endl;
							//glTexCoord2f(vrml_tex_coord[tex_coord_index[num_vert]][0],vrml_tex_coord[tex_coord_index[num_vert]][1]);
							//std::cout<<vrml_tex_coord[tex_coord_index[num_vert]][0]<<","<<vrml_tex_coord[tex_coord_index[num_vert]][1]<<std::endl;
						}
						glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
						glVertex3f(vrml_coord[*it][0],vrml_coord[*it][1],vrml_coord[*it][2]);
						num_vert++;
					}
					glEnd();
					if (vrml_tex_coord_node != 0)
					{
						//glDisable(GL_TEXTURE_2D);
					}
					num_polygon++;
					vert_index.clear();
				} 
				else
				{
					vert_index.push_back(index);
				} 
			}
            glFlush();
		}
	}

	void VrmlGame::Init()
	{
		if(mbInitialised) return;
		mbInitialised = true;

		Reset();
		std::cout<<"VrmlGame Init"<<std::endl;
	}

	void VrmlGame::Reset()
	{

	}
}