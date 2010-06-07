uniform vec3 center;
uniform float multiplier;
varying vec3 pos;

void main(void) {
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    pos = normalize(gl_Vertex.xyz + center);

    // unit sphere: -1 <-> 1, centered in 0,0
    vec3 coords = normalize(gl_Vertex.xyz + center);

    // unite sphere: 0 <-> 2, centered in 1,1
    coords += 1.0;

    // to tex coords 0 <-> 1
    vec3 texCoords = coords * 0.5;

    gl_TexCoord[0] = vec4(texCoords * multiplier, 0.0);
    //gl_TexCoord[0] = gl_MultiTexCoord0;
}
