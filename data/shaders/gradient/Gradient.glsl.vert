uniform vec3 viewPos;

varying vec3 normal;
varying vec3 eyeDir;

void main(void) {
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    gl_TexCoord[0].xyz = gl_MultiTexCoord0.xyz;
    
    normal = gl_Normal;

    eyeDir = gl_Vertex.xyz - viewPos;
}
