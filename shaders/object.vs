//#version 140
#extension GL_ARB_texture_rectangle : enable
varying vec3 normal;
varying vec3 lightDir;
varying vec3 eyeVec;
varying vec4 ShadowCoord;
varying vec4 diffuse,ambient;  
varying vec3 halfVector; 
varying vec4 c;
void main() {
	
    normal = normalize(gl_NormalMatrix * gl_Normal);  
 
    lightDir = normalize(vec3(gl_LightSource[0].position));  
	float NdotL = max(dot(normal,lightDir),0.0);
	
    halfVector = normalize(gl_LightSource[0].halfVector.xyz);  

    diffuse = gl_FrontMaterial.diffuse * gl_LightSource[0].diffuse;  
    ambient = gl_FrontMaterial.ambient * gl_LightSource[0].ambient;  
    ambient += gl_FrontMaterial.ambient * gl_LightModel.ambient; 
	float NdotHV = max(dot(normal,gl_LightSource[0].halfVector.xyz),0.0); ;
	
	//phong shader

	vec3 vVertex = vec3(gl_ModelViewMatrix * gl_Vertex);
	eyeVec   = -vVertex;
	
	ShadowCoord= gl_TextureMatrix[7] * gl_Vertex;
	
	gl_Position = ftransform();
	c = NdotL * diffuse + ambient + gl_FrontMaterial.specular * gl_LightSource[0].specular * pow(NdotHV, gl_FrontMaterial.shininess);  
	
	gl_TexCoord[0] = gl_MultiTexCoord0;
}
