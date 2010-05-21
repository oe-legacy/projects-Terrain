uniform vec3 center;
uniform float multiplier;

void main(void) {
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    gl_TexCoord[0] = vec4(normalize(gl_Vertex.xyz + center)*multiplier,0.0);
    //gl_TexCoord[0] = gl_MultiTexCoord0;
}
