uniform sampler2D gradient;
uniform sampler2D stars;
uniform float timeOfDayRatio;
uniform vec3 lightDir;

varying vec3 normal;
varying vec3 eyeDir;

void main(void) {
    vec3 coords = gl_TexCoord[0].xyz;
    vec2 uv = vec2(timeOfDayRatio, clamp(coords.y*2.0-1.0, 0.0, 1.0));
    vec4 color = texture2D(gradient, uv);
    vec4 star = texture2D(stars, coords.xz);
    
    //float intensity = clamp(dot(lightDir, normalize(normal)), 0.0, 1.0);
    float intensity = clamp(dot(lightDir, normalize(eyeDir)), 0.0, 1.0);
    intensity = pow(intensity, 512.0);

    gl_FragColor = mix(mix(star,color,color.a), gl_LightSource[0].diffuse, intensity);
}
