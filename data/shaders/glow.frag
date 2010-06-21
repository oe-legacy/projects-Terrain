uniform sampler2D color0; // The horizontally blurred image
uniform sampler2D scene;
uniform sampler2DShadow depth;

uniform vec2 coefficients;
uniform float halfSamples;
uniform float samples;
uniform float offset;

varying vec2 texCoord;

void main () {
    vec4 blur = vec4(.0, .0, .0, .0);
    for (float y = -halfSamples; y <= halfSamples; ++y)
        blur += texture2D(color0, texCoord + vec2(0.0, y * offset));
    blur /= samples;

    vec4 orig = texture2D(scene, texCoord);

    gl_FragColor = coefficients.x * orig + coefficients.y * blur;
    gl_FragDepth = shadow2D(depth, vec3(texCoord, 0.0)).x;
}
