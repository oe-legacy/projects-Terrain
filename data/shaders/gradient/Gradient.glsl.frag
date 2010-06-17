uniform sampler2D gradient;
uniform float interpolator;

void main(void) {
    vec3 coords = gl_TexCoord[0].xyz;
    float t = interpolator;
    gl_FragColor = texture2D(gradient, vec2(t,coords.y*2.0-1.0));
}
