#ifndef __GLSHADER_H_
#define __GLSHADER_H_

#include <windows.h>
#include <stdio.h>
#include <GL/glew.h>

#pragma warning(disable:4267)
#pragma warning(disable:4996)

namespace PTAMM {

class Shader
{
public:
	Shader();
	~Shader();	

protected:
	void CleanUp();
	bool ReadVertextShader(char *_fn);
	bool ReadFragmentShader(char *_fn);
	GLint getUniLoc(GLuint program, const GLchar *name);
	int printOglError(char *file, int line);
	void printShaderInfoLog(GLuint obj);
	void printProgramInfoLog(GLuint obj);
	bool TextFileRead(char *_fn, GLchar *&_shader);
	GLchar *m_VertexShader;
	GLchar *m_FragmentShader;

	GLuint m_Program;
	GLuint m_Vert,m_Frag;

public:
	bool SetShaderFile(char* sVSFileName, char* vFSFileName);
	void UseShader(bool bOn);
	//设置初始一致变量值
	void SetUniVar(char* sVarName, float fValue0, float fValue1, float fValue2);
	void SetUniVar(char* sVarName, float fValue0, float fValue1);
	void SetUniVar(char* sVarName, float fValue0);
	void SetUniVar(char* sVarName, int nValue0);
	void SetSampler(char* sVarName, int tex);
	GLuint GetProgram(){return m_Program;};
};

}
#endif