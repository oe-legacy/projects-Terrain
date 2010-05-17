//uniform sampler2D clouds;
uniform sampler3D clouds;
uniform float interpolator;

void main(void) {
    // for 3d
    vec3 coords = vec3(gl_TexCoord[0].xy, interpolator);
    vec4 rgba = texture3D(clouds, coords);
    gl_FragColor = rgba;

    //vec3 rgba2 = vec3(gl_TexCoord[0].x,gl_TexCoord[0].y,interpolator);
    //vec4 color = vec4(rgba2.rgb, 1.0);
    //gl_FragColor = texture3D(clouds, rgba2);

    //vec4 rgba3 = texture3D(clouds, gl_TexCoord[0].xyz);
    //gl_FragColor = rgba3;

    // for 2d
    //vec4 rgba = texture2D(clouds, gl_TexCoord[0].xy);
    //vec4 color = vec4(rgba.rgb, interpolator*rgba.a);
    //gl_FragColor = color;

    // one coloe
    //gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
