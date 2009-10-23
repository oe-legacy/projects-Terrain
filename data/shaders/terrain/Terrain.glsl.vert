uniform sampler2D normalMap;

uniform float snowStartHeight;
uniform float snowBlend;
uniform float grassStartHeight;
uniform float grassBlend;
uniform float sandStartHeight;
uniform float sandBlend;

varying float snowFactor;
varying float grassFactor;
varying float sandFactor;

void main()
{
    gl_TexCoord[0] = gl_MultiTexCoord0;
    gl_TexCoord[1] = gl_MultiTexCoord1;

	// Transforming The Vertex
    gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

	// Transforming The Normal To ModelView-Space
    /*
    vec4 texNormal = texture2D(normalMap, gl_TexCoord[1].xy);
    vec3 vNorm = texNormal.xyz * 2.0 - 1.0;
    normal = gl_NormalMatrix * vNorm;
    */

    // Dorectional light, so the position is the direction
    //lightDir = normalize(vec3(gl_LightSource[0].position));

    float height = gl_Vertex.y;
    snowFactor = (height - snowStartHeight) / snowBlend;
    grassFactor = (height - grassStartHeight) / grassBlend;
    sandFactor = (height - sandStartHeight) / sandBlend;
}
