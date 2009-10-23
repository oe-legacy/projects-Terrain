const vec3 tangent = vec3(1.0, 0.0, 0.0);
const vec3 norm = vec3(0.0, 1.0, 0.0);
const vec3 binormal = vec3(0.0, 0.0, 1.0);

const mat3 lightTransformation = mat3(tangent, binormal, norm);

uniform vec3 viewpos;
uniform float time, time2;

varying vec2 waterFlow;
varying vec2 waterRipple;
varying vec4 projCoords;
varying vec3 eyeDir;

void main(void)
{
    //vec4 temp = viewpos - gl_ModelViewMatrix * gl_Vertex;
    eyeDir = (viewpos - gl_Vertex.xyz) * lightTransformation;    

    //Is done in the renderingview
    //lightDir = (lightPos/* - gl_Vertex.xyz*/) * lightTransformation;

    // texcoords for making the water flow
    waterFlow = vec2(gl_MultiTexCoord0) + vec2(0.0, time);

    // texcoords for making the water ripple
    waterRipple = vec2(gl_MultiTexCoord0) + vec2(0.0, time2);

    gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;
	gl_Position = projCoords = gl_ModelViewProjectionMatrix * gl_Vertex;
}
