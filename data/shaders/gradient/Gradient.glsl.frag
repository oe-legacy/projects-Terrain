uniform sampler2D gradient;
uniform float timeOfDayRatio;
varying float sunAngle;

void main(void) {
    vec3 coords = gl_TexCoord[0].xyz;
    vec2 uv = vec2(timeOfDayRatio, abs(coords.y*2.0-1.0));
    gl_FragColor = texture2D(gradient, uv);
    //gl_FragColor = texture2D(gradient, uv + float2(offset, 0)) * col;
}
