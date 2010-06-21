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

    // Square pyramidal numbers to quickly calculate the total weight
    float doubleSumOfError = (halfSamples * halfSamples + halfSamples) * (halfSamples + 0.5) / 1.5;
    float totWeight = (2.0 * halfSamples + 1.0) * error - doubleSumOfError;
    vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
    for (float y = -halfSamples; y < halfSamples; ++y){
        float weight = error - y * y;
        color += weight * texture2D(color0, texCoord + vec2(0.0,  y * blurOffset));
    }
    
    gl_FragColor = color / totWeight;
    gl_FragDepth = d;
}
