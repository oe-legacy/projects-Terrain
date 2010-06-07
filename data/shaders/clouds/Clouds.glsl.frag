//uniform sampler2D clouds;
uniform sampler3D clouds;
uniform float interpolator;
uniform bool showTexCoords;
varying vec3 pos;
uniform vec3 wind;

void main(void) {
    // for 3d
    //vec3 coords = vec3(gl_TexCoord[0].xy, interpolator);
    vec3 coords = gl_TexCoord[0].xyz;
    coords += wind;
    //coords -= floor(coords);

    vec4 rgba = texture3D(clouds, coords);

    gl_FragColor = rgba;
    float hlim = 0.55;
    float llim = 0.50;
    //float hlim = 0.75;
    //float llim = 0.55;
    if(gl_TexCoord[0].y >= hlim) {
        gl_FragColor.a *= 1.0;
    } else if(gl_TexCoord[0].y < hlim && gl_TexCoord[0].y > llim) {
        gl_FragColor.a *= abs(gl_TexCoord[0].y - llim) / (hlim-llim);
    } else 
        gl_FragColor.a *= 0.0;

    if (showTexCoords) {
        gl_FragColor.rgb = coords.xyz;
        gl_FragColor.a = 1.0;
    }
    //gl_FragColor = vec4(normalize(pos),0.0);

    //vec3 n = noise3(coords.xyz);
    //gl_FragColor = vec4(n,0.0);
}
