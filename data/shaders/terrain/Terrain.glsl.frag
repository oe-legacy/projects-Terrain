// material pecular, the alpha channel is the shininess
const vec4 SNOW_SPECULAR = vec4(0.7, 0.7, 1.0, 32.0);
const vec4 GRASS_SPECULAR = vec4(0.0, 0.0, 0.0, 1.0);
const vec4 SAND_SPECULAR = vec4(0.8, 0.8, 0.5, 128.0);

const vec4 WATER_COLOR = vec4(0.0, 0.5, 0.0, 1.0);

uniform sampler2D sandTex;
uniform sampler2D grassTex;
uniform sampler2D snowTex;
uniform sampler2D normalMap;

uniform vec3 lightDir; // Should be pre-normalized. Or else the world will BURN IN RIGHTEOUS FIRE!!

varying float snowFactor;
varying float grassFactor;
varying float sandFactor;
varying vec3 eyeDir;

void main()
{
    // Calculating the texture blend factor
    float snowFactor = clamp(snowFactor, 0.0, 1.0);
    float grassFactor = clamp(grassFactor, 0.0, 1.0);
    float sandFactor = clamp(sandFactor, 0.0, 1.0);

    // Extract normal
    vec3 normal = texture2D(normalMap, gl_TexCoord[1].xy).xyz;
    normal = normal * 2.0 - 1.0;  // normal already in normalized form after these calculations

    // Calculate diffuse
    float ndotl = dot(normal, lightDir);
    float diffuse = clamp(ndotl, 0.0, 1.0);
    
    // mix shininess and specular
    vec4 matSpecular = mix(GRASS_SPECULAR, SNOW_SPECULAR, snowFactor);
    matSpecular = mix(SAND_SPECULAR, matSpecular, grassFactor);

    // Calculate specular
    //vec3 vRef = normalize(reflect(-lightDir, normal));
    vec3 vRef = reflect(-lightDir, normal);
    float stemp = clamp(dot(normalize(eyeDir), vRef), 0.0, 1.0);
    vec4 specular = matSpecular * pow(stemp, matSpecular.w);
    
    // Looking up the texture values
    vec2 srcUV = gl_TexCoord[0].xy;
    vec4 grass = texture2D(grassTex, srcUV);
    vec4 snow = texture2D(snowTex, srcUV);
    vec4 sand = texture2D(sandTex, srcUV);

    // Mix textures and add the diffuse
    vec4 text = mix(grass, snow, snowFactor);
    text = mix(sand, text, grassFactor);

    vec4 color = text * gl_LightSource[0].ambient;
    color += text * gl_LightSource[0].diffuse * diffuse + gl_LightSource[0].specular * specular;

    gl_FragColor = mix(WATER_COLOR, color, sandFactor);
    //gl_FragColor.a = 1.0 - sandFactor;
}
