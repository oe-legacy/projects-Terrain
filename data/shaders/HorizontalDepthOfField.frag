uniform sampler2D color0;
uniform sampler2DShadow depth;

uniform float halfSamples;
uniform float offsetScale;

varying vec2 texCoord;

void main () {

    float focus = shadow2D(depth, vec3(0.5, 0.5, 0.0)).x;
    float d = shadow2D(depth, vec3(texCoord, 0.0)).x;

    // Blur offset computed from the deviation from the focus depth.
    float blurOffset = (d - focus) * offsetScale;
    blurOffset = clamp(blurOffset, -0.0005, 0.0009);

    // The error term
    float error = halfSamples + 1.0;
    error *= error;

    vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
    for (float x = -halfSamples; x < halfSamples; ++x){
        float weight = error - x * x;
        color += weight * texture2D(color0, texCoord + vec2(x * blurOffset, 0.0));
    }
    
    // Square pyramidal numbers used for calculating the total weight
    float halfSamples = halfSamples + 1.0;
    float doubleSumOfError = (halfSamples * halfSamples + halfSamples)
        * (halfSamples + 0.5) / 1.5;
    float totWeight = (2.0 * halfSamples + 1.0) * error - doubleSumOfError;

    gl_FragColor = color / totWeight;
    gl_FragDepth = d;
}
