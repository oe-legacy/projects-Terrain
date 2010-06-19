uniform sampler2D gradient;
uniform sampler2D stars;
uniform float timeOfDayRatio;
varying float sunAngle;

void main(void) {
    vec3 coords = gl_TexCoord[0].xyz;
    vec2 uv = vec2(timeOfDayRatio, clamp(coords.y*2.0-1.0, 0.0, 1.0));
    vec4 color = texture2D(gradient, uv);
    vec4 star = texture2D(stars, coords.xz);
    gl_FragColor = mix(star,color,color.a);
}
