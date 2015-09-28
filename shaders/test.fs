//#version 140    //写上之后场景图就显示不了了
#extension GL_ARB_texture_rectangle : enable
uniform sampler2DRect txObject;   //虚拟物体
uniform sampler2DRect txGhost;	  //实物虚化
uniform sampler2DRect txImage;    //背景图片
uniform sampler2DRect txDepth1;   //虚物的深度
uniform sampler2DRect txDepth2;   //实物虚化的深度
uniform sampler2DRect txDepth3;   //中介面的深度
uniform sampler2DRect txBackEdge; //边线
uniform sampler2DRect txBoundary; //透明度
uniform sampler2DRect txMediator;
uniform sampler2DRect txEdgemask;
uniform sampler2DRect txMirror;

void main(void)
{
	vec2 curr = gl_TexCoord[0].st;
	vec4 img = texture2DRect(txImage, curr);
	vec4 object = texture2DRect(txObject, curr);
	vec4 backEdge = texture2DRect(txBackEdge, curr);
	vec4 ghost = texture2DRect(txGhost, curr);
	vec4 depth1 = texture2DRect(txDepth1, curr);
	vec4 depth2 = texture2DRect(txDepth2, curr);
	vec4 depth3 = texture2DRect(txDepth3, curr);
	vec4 boundary = texture2DRect(txBoundary, curr);
	vec4 mediator = texture2DRect(txMediator, curr);
	vec4 edgemash = texture2DRect(txEdgemask, curr);
	vec4 mirror = texture2DRect(txMirror, curr);

	if(depth1.z >= depth2.z && depth3.z >= depth2.z)   //ghost物体在前面，画背景
	{
		gl_FragColor = img;
	}
	else                       //
	{
		//if(backEdge.r == 0.0 && depth1.z >= depth3.z)
		//{
		//	gl_FragColor = backEdge;
		//}
		//else   //
		//{
			if(depth1.z < depth3.z)
			{
				gl_FragColor = object;
			}
			else if(mediator.a > 0.5)  //这个参数需要微调 中介面中非影子
			{
				if(mirror.r == 0.0)
				{
					gl_FragColor = mediator * boundary.r + img * (1.0 - boundary.r);
				}
				else		//有镜子物体
				{
					float t = boundary.r;
					gl_FragColor = mediator * t/2 + mirror*(t-t/2) + img * (1.0 - t);
				}
			}
			else   //中介面上的影子
			{
				if(mirror.r == 0.0)
				{
					gl_FragColor = mediator;
				}
				else
				{
					gl_FragColor = mediator*0.5 + mirror*0.5;
				}
			}
		//}
	}
	//gl_FragColor = boundary;

	//comment by 2012.2.25
	//gl_FragColor = mediator;

	//if(depth1.z >= depth2.z && depth3.z >= depth2.z)   //ghost物体在前面，画背景
	//{
	//	gl_FragColor = img;
	//}
	//else                       //
	//{
	//		gl_FragColor = mediator * boundary.r + img * (1.0 - boundary.r);
	//}

	//if(mediator.rgb != vec3(0.0))
	//{
	//	gl_FragColor = mediator * boundary.r + img * (1.0 - boundary.r);
	//}
	//else
	//{
	//	gl_FragColor = img;
	//}
}
