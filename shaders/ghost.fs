//#version 140
#extension GL_ARB_texture_rectangle : enable
uniform sampler2D ShadowMap;
varying vec4 ShadowCoord;


void main(void) {

	vec4 shadowCoordinateWdivide = ShadowCoord / ShadowCoord.w ;
		
	// Used to lower moiré pattern and self-shadowing  这个阈值变小之后可以把书本旁边的齿轮变小，但是没法消掉，不知道怎么消
	shadowCoordinateWdivide.z += 0.0005;
	
	float distanceFromLight = texture2D(ShadowMap,shadowCoordinateWdivide.st).z;
	
 	bool shadow = false;
 	if (ShadowCoord.w > 0.0)
	{
		if(distanceFromLight < shadowCoordinateWdivide.z)
			shadow = true;
	}
	if(shadow)
	{
		gl_FragColor = gl_Color * 0.5;
	}
	else
	{
		gl_FragColor = vec4(0.0);
	}
	//gl_FragColor = gl_Color;
}
