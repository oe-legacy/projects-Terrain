uniform sampler2D clouds;
//uniform sampler3D clouds;
uniform float interpolator;

void main(void) {
    // for 3d
    //vec3 coords = vec3(gl_TexCoord[0].xy, interpolator);
    //vec4 rgba = texture3D(clouds, coords);
    //gl_FragColor = rgba;

    // for 2d
    vec4 rgba = texture2D(clouds, gl_TexCoord[0].xy);
    vec4 color = vec4(rgba.rgb, interpolator*rgba.a);
    gl_FragColor = color;

    // one coloe
    //gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
