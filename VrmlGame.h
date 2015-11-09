#ifndef __VRMLGAME_H
#define __VRMLGAME_H

#include <TooN/TooN.h>
#include "OpenGL.h"
#include "Game.h"
#include <openvrml/vrml97node.h>
#include <openvrml/common.h>
#include <openvrml/browser.h>
#include <openvrml/node.h>
#include <openvrml/node_ptr.h>
#include <openvrml/field.h>
#include "Math/Basic/WmlMath/include/WmlQuaternion.h"
// library
#include "Math/Basic/WmlMath/lib/wmlmathlink.h"

//////////////////////////////////////////////////////////////////////////
// header
#include "motionsolver/Robust3DRecovery.h"

#include "vision/image/simpleimage/include/simpleimage.h"
#include "vision/image/simpleimage/include/simpleimagecolor.h"
#include "vision/image/simpleimage/include/simpleimageoperation.h"

#include "simplemarkerdetector.h"
#include "artificialfeaturematch.h"

#include "utility/floattype.h"
#include "GLShader.h"
#include <string.h>
#include <iostream>
#include <fstream>

#ifdef _DEBUG
#pragma comment( lib, "marker100-win32-vc8-mtd-d.lib" )
#else if
#pragma comment( lib, "marker100-win32-vc8-mtd-r.lib" )
#endif

namespace PTAMM {

	using namespace TooN;

	class Map;
	class KeyFrame;


	class VrmlGame : public Game
	{
	public:
   		VrmlGame( );
		void Draw3D( const GLWindow2 &glWindow, Map &map, SE3<> se3CfromW, ATANCamera &mCamera);

		void GeneratrShadowFBO();
		void SetTextureMatrix();
		void Reset();
		void Init();
		void Draw3DFromVRML(openvrml::node *obj);	// 从VRML中绘制三维物体
		void DrawBox(openvrml::vrml97_node::box_node* vrml_box) const;
		void DrawSphere(openvrml::vrml97_node::sphere_node* vrml_sphere) const;
		void DrawIndexedFaceSet(openvrml::vrml97_node::indexed_face_set_node* vrml_ifs) const;
		void InitMedMaskTexture();
		bool isEqual(float m, float n) const;
		void setStatusFlag(int id){ statusFlag = id; }
		void setFBOOBJECT(GLuint id){
			mfbos[FBO_OBJECT] = id;
		}
		void setFBOGHOST(GLuint id){
			mfbos[FBO_GHOST] = id;
		}
		void setFBOMED(GLuint id){
			mfbos[FBO_MED] = id;
		}
		void setFBOEDGE(GLuint id){
			mfbos[FBO_EDGE] = id;
		}
		void setFBOMIRROR(GLuint id){
			mfbos[FBO_MIRROR] = id;
		}
		void DrawDepthImage();
		void Draw3DObject(SE3<> se3CfromW, ATANCamera &mCamera);	// 绘制场景中的虚拟物体
		void Draw3DGhost(SE3<> se3CfromW, ATANCamera &mCamera);		// 绘制场景中的ghost object
		void Draw3DMediator(SE3<> se3CfromW, ATANCamera &mCamera);  // 绘制中介面
		void Draw3DBoundary(SE3<> se3CfromW, ATANCamera &mCamera);  // 绘制抠边
		void Draw3DMirror(SE3<> se3CfromW, ATANCamera &mCamera);    // 绘制镜像
		void Draw3DHand(SE3<> se3CfromW, ATANCamera &mCamera);		// 绘制虚拟手
		void DrawVirtualObject();	// 通过VRML文件中的三维物体绘制场景中的虚拟物体
		void TestDrawObject();
		void DrawDepthObject();	// 画shadow mapping用于绘制阴影的深度图中的物体
		void DrawGhostObject();	// 通过VRML文件中的三维物体绘制场景中的ghost object
		void DrawMediator();
		void update();
		int printOglError(char *file, int line) const;
        void test(SE3<> se3CfromW, ATANCamera &mCamera);
	private:
		void LoadMatrixFile();		//读取matrix.txt文件中的Pinverse矩阵
		openvrml::browser b;	// 绑定虚拟物体的VRML文件
		openvrml::browser ghostb;
		Map * mpMap;                    // The associated map
		bool isInitMap;
		bool isBindTexture;
		bool bInit;
		Wml::Matrix4d PInverse;    //相机矩阵
		double scaleInverse;
		//GLuint texture[1];  //最多支持20个纹理
		int textureNum;   //表示现在为第几个纹理
		std::set<std::string> textureUrls;
		int statusFlag;  //绘制状态，决定中介面是否绘制
		GLfloat PInversetrans[16];  //用于相对坐标系向绝对坐标系的转换
		float position;

		Shader ObjectShadow;
		Shader GhostShadow;
		Shader MediatorShadow;
		Shader BTShadow;   //用于绑定boundary.bmp纹理的Shader
		float animationTrans[3];

		enum {FBO_SHADOW, FBO_OBJECT, FBO_GHOST, FBO_MED, FBO_MIRROR, FBO_EDGE};
		GLuint mfbos[6];
		enum {TEX_DEPTH, TEX_MED, TEX_EDGEMASK, TEX_BOX, TEX_DESK};
		GLuint mtexs[4];
		GLuint texture[4];
	};

}
#endif