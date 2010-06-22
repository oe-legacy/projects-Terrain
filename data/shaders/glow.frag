uniform sampler2D color0; // The horizontally blurred image
uniform sampler2D scene;
uniform sampler2DShadow depth;

uniform vec2 coefficients;
uniform float halfSamples;
//uniform float samples;
uniform float offset;

varying vec2 texCoord;

void main () {
    // The error term
    float error = halfSamples + 1.0;
    error *= error;

    // Square pyramidal numbers
    //float doubleSumOfError = (halfSamples * halfSamples + halfSamples) * (halfSamples + 0.5) / 1.5;
    //float totWeight = (2.0 * halfSamples + 1.0) * error - doubleSumOfError;
    float totWeight = 0.0;
    vec4 blur = vec4(0.0, 0.0, 0.0, 0.0);
    for (float x = -halfSamples; x < halfSamples; ++x){
        float weight = error - x * x;
        totWeight += weight;
        blur += weight * texture2D(color0, texCoord + vec2(x * offset, 0.0));
    }
    blur /= totWeight;

    /*
    vec4 blur = vec4(.0, .0, .0, .0);
    for (float y = -halfSamples; y <= halfSamples; ++y)
        blur += texture2D(color0, texCoord + vec2(0.0, y * offset));
    blur /= samples;
    */

    vec4 orig = texture2D(scene, texCoord);

    gl_FragColor = coefficients.x * orig + coefficients.y * blur;
    gl_FragDepth = shadow2D(depth, vec3(texCoord, 0.0)).x;
}
