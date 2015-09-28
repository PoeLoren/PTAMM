#extension GL_ARB_texture_rectangle : enable
varying vec3 normal;
varying vec3 lightDir;
varying vec3 eyeVec;

varying vec4 ShadowCoord;

void main() {	
  normal = gl_NormalMatrix * gl_Normal;
  vec3 vVertex = vec3(gl_ModelViewMatrix * gl_Vertex);
  lightDir = vec3(gl_LightSource[0].position.xyz - vVertex);
  eyeVec   = -vVertex;
  
  ShadowCoord= gl_TextureMatrix[7] * gl_Vertex;
  gl_Position = ftransform();	
  gl_FrontColor = gl_Color;
}
