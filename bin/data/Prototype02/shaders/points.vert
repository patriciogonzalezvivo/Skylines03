#version 120

uniform float nearClip;
uniform float farClip;

uniform float focusDistance;
uniform float focusAperture;

uniform float pointSize;
uniform float minPointSize;
uniform float maxPointSize;

float PI = 3.14159265359;
float HALF_PI = 1.57079632679;

void main(){
	float size = pointSize;
	float minSize = minPointSize*10.0;
	float maxSize = maxPointSize*10.0;

	float dist = focusDistance*100.0;
	float aper = focusAperture*0.01;

	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_FrontColor = gl_Color;
	
	float attenuation = ((farClip-nearClip)/2.)/distance(vec3(0),gl_Position.xyz);
	gl_PointSize = max( minSize, min( maxSize, size * attenuation ) );

	float radius = min( 1.0 + abs(gl_Position.z - dist) * aper, maxSize)*0.5;
	gl_FrontColor.a /= PI * radius * radius;
}
