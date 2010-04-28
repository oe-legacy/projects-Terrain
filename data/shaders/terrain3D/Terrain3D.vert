const vec3 startHeight = vec3(-10.76, 5.0, 50.0); // {grass, grass, snow}
const vec3 blend = vec3(10.0, 5.0, 20.0); // {grass, grass, snow}

uniform vec3 viewPos;
uniform float baseDistance;
uniform float invIncDistance;

varying vec3 factors; // {sandFactor, grassFactor, snowFactor}

varying vec3 eyeDir;

varying vec2 texCoord;

void main()
{
    texCoord = gl_MultiTexCoord0.xy;

    vec4 vertex = vec4(gl_Vertex.xyz, 1.0);
    float morphValue = gl_Vertex.w;

    vec3 patchCenter = vec3(gl_Normal.x, 0, gl_Normal.y);
    float lod = gl_Normal.z;
    float dist = distance(viewPos, patchCenter) - baseDistance;

    float morphScale = clamp(dist * invIncDistance - lod, 0.0, 1.0);
    vertex.y += morphScale * morphValue;
    
    // Calculate the eyeDir relative to the vertex.
    eyeDir = viewPos - vertex.xyz;

    // Calculating the texture factors
    factors = (vertex.yyy - startHeight) / blend;

    // Doing the stuff
    gl_ClipVertex = gl_ModelViewMatrix * vertex;
	gl_Position = gl_ModelViewProjectionMatrix * vertex;
}
