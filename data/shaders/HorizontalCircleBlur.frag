uniform sampler2D color0;
uniform sampler2DShadow depth;

uniform float halfSamples;
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
    vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
    for (float x = -halfSamples; x < halfSamples; ++x){
        float weight = error - x * x;
        totWeight += weight;
        color += weight * texture2D(color0, texCoord + vec2(x * offset, 0.0));
    }
    
    gl_FragColor = color / totWeight;
    gl_FragDepth = shadow2D(depth, vec3(texCoord, 0.0)).x;
}
