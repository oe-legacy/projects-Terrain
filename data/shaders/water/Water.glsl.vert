const float tscale = 0.25;
const vec2 center = vec2(400.0, 400.0);

uniform vec3 viewpos;
uniform float time, time2;

varying vec2 waterFlow;
varying vec2 waterRipple;
varying vec4 projCoords;
varying vec3 eyeDir;

void main(void)
{
    // Eyedir should be transformed because the normalmaps upvector is
    // z, where ours is y.
    eyeDir = viewpos - gl_Vertex.xyz;

    // texcoords for making the water flow
    vec2 flowDir = center - vec2(gl_Vertex.x, gl_Vertex.z);
    waterFlow = gl_MultiTexCoord0.xy - time * flowDir;

    // texcoords for making the water ripple
    waterRipple = (gl_MultiTexCoord0.xy + vec2(0.0, time2)) * tscale;

    gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;
	gl_Position = projCoords = gl_ModelViewProjectionMatrix * gl_Vertex;
}
