//#version 140
#extension GL_ARB_texture_rectangle : enable
uniform sampler2D texture;

void main(void) {
	//add by 2012.2.26
	gl_FragColor = texture2D(texture,gl_TexCoord[0].st);
	//gl_FragColor = vec4(1.0, 0, 0, 0);
}