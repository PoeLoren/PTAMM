//#version 140
#extension GL_ARB_texture_rectangle : enable
uniform sampler2DRect txMediator;

float lookup(vec2 offSet)
{
	float t = texture2DRect(txMediator, gl_TexCoord[0].st + offSet).r;
	if(t==0.0)
		return 0.0;
	else
		return 1.0;
}

void main(void) 
{
	float shadow = 0.0;
	float x, y;
	for (y = -7.0 ; y <=7.0 ; y+=1.0)
	{
		for (x = -7.0 ; x <=7.0 ; x+=1.0)
		{
			shadow += lookup(vec2(x,y));
		}
	}
	shadow /= 225.0;

	gl_FragColor = vec4(shadow);

}