uniform float snowStartHeight;
uniform float snowBlend;
uniform float grassStartHeight;
uniform float grassBlend;
uniform float sandStartHeight;
uniform float sandBlend;

uniform vec3 viewPos;
uniform float baseDistance;
uniform float invIncDistance;

varying float snowFactor;
varying float grassFactor;
varying float sandFactor;


void main()
{
    gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_TexCoord[1] = gl_MultiTexCoord1;

	// Transforming The Vertex
    vec4 vertex = vec4(gl_Vertex.xyz, 1.0);
    float morphValue = gl_Vertex.w;
    
    vec3 patchCenter = vec3(gl_Normal.x, 0, gl_Normal.y);
    float lod = gl_Normal.z;
    float distance = length(viewPos - patchCenter) - baseDistance;

    float morphScale = clamp(distance * invIncDistance - lod, 0.0, 1.0);
    vertex.y += morphScale * morphValue;

    // Calculating the texture factors
    snowFactor = (vertex.y - snowStartHeight) / snowBlend;
    grassFactor = (vertex.y - grassStartHeight) / grassBlend;
    sandFactor = (vertex.y - sandStartHeight) / sandBlend;

    // Doing the stuff
    gl_ClipVertex = gl_ModelViewMatrix * vertex;
	gl_Position = gl_ModelViewProjectionMatrix * vertex;
}
