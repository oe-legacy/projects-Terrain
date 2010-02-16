const vec4 WATER_COLOR = vec4(0.0, 0.0, 0.5, 1.0);
const float sca = 0.02;
const float sca2 = 0.02;

const float exponent = 128.0;

uniform sampler2D normalmap;
uniform sampler2D reflection;
uniform sampler2D dudvmap;

uniform vec3 lightDir;

varying vec2 waterFlow; //moving texcoords
varying vec2 waterRipple; //moving texcoords
varying vec4 projCoords; //for projection
varying vec3 eyeDir; //viewts

void main(void)
{
    vec3 viewt = normalize(eyeDir);
    
    vec2 rippleEffect = sca2 * texture2D(dudvmap, waterRipple).xy;
    //rippleEffect = rippleEffect * 2.0 + vec2(-1.0);
    vec3 normal = texture2D(normalmap, waterFlow + rippleEffect).xzy;
    normal = normalize(normal - 0.5);

    // Reflection distortion
    vec2 fdist = texture2D(dudvmap, waterFlow + rippleEffect).xy;
    fdist = fdist * 2.0 + vec2(-1.0);
    fdist *= sca;
                 
    //calculate specular highlight
    vec3 vRef = normalize(reflect(-lightDir, normal));
    float stemp = clamp(dot(viewt, vRef), 0.0, 1.0);
    // aproximated specular light
    //vec3 halfVec = normalize(viewt + lightDir);
    //float stemp = clamp(dot(halfVec, normal), 0.0, 1.0);
    vec4 specular = gl_LightSource[0].specular * pow(stemp, exponent);

    //calculate fresnel and inverted fresnel
    float invfres = dot(normal, viewt);
    float fres = 1.0 - invfres;

    //get projective texcoords
    vec2 projCoord = projCoords.xy / projCoords.w;
    projCoord = projCoord * 0.5 + 0.5;
    fdist.y = -abs(fdist.y);
    projCoord.x += fdist.x;

    //load and calculate reflection
    vec4 refl = texture2D(reflection, projCoord);
    refl *= fres; // creates waves

    // Set the water color
    vec4 waterColor = WATER_COLOR * invfres;
    
    // Add specular to the water
    vec4 color = refl + waterColor;
    color *= (gl_LightSource[0].ambient + gl_LightSource[0].diffuse);    

    //add it all up for the effect
    gl_FragColor = color + specular;
    gl_FragColor.a = -0.35 * gl_LightSource[0].diffuse.a + 0.95;
}
