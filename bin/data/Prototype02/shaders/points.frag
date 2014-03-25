#version 120

uniform float nearClip;
uniform float farClip;

uniform float focusDistance;
uniform float focusAperture;

uniform float pointSize;
uniform float minPointSize;
uniform float maxPointSize;

float linearizeDepth( in float d ) {
    return (2.0 * nearClip) / (farClip + nearClip - d * (farClip - nearClip));
}

void main(){
	float depthVal = 1. - pow( linearizeDepth( gl_FragCoord.z ), 2.);
	gl_FragColor = gl_Color*depthVal;
}