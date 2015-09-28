//#version 140
#extension GL_ARB_texture_rectangle : enable
varying vec4 ShadowCoord;

void main() {
  ShadowCoord= gl_TextureMatrix[7] * gl_Vertex;

  gl_Position = ftransform();	
  gl_FrontColor = gl_Color;
}
