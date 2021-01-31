#version 330 core
out vec4 FragColor;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
}; 

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct LightSpot {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoords;

  
uniform vec3 viewPos;
uniform Material material;
uniform Light light;
uniform LightSpot lightSpot;
void main()
{
    vec3 resultSpot = vec3(0.0);
    vec3 lightSpotDir = normalize(lightSpot.position - FragPos);
    float theta = dot(lightSpotDir, normalize(-lightSpot.direction));




    if(theta > lightSpot.outerCutOff){

        // ambient
                vec3 ambientSpot = lightSpot.ambient * vec3(texture(material.diffuse,TexCoords).rgb);

                // diffuse
                vec3 normSpot = normalize(Normal);
                float diffSpot = max(dot(normSpot, lightSpotDir), 0.0);
                vec3 diffuseSpot = lightSpot.diffuse * diffSpot * vec3(texture(material.diffuse,TexCoords).rgb);

                // specular
                vec3 viewSpotDir = normalize(viewPos - FragPos);
                vec3 reflectSpotDir = reflect(-lightSpotDir, normSpot);
                float specSpot = pow(max(dot(viewSpotDir, reflectSpotDir), 0.0), material.shininess);
                vec3 specularSpot = lightSpot.specular * specSpot * vec3(texture(material.specular,TexCoords).rgb);

                // attenuation
                float distanceSpot    = length(lightSpot.position - FragPos);
                float attenuationSpot = 1.0 / (lightSpot.constant + lightSpot.linear * distanceSpot + lightSpot.quadratic * (distanceSpot * distanceSpot));

                float epsilon = (lightSpot.cutOff - lightSpot.outerCutOff);
                float intensity = clamp((theta - lightSpot.outerCutOff) / epsilon, 0.0, 1.0);
                diffuseSpot *= intensity;
                specularSpot *= intensity;

                // ambient  *= attenuation; // remove attenuation from ambient, as otherwise at large distances the light would be darker inside than outside the spotlight due the ambient term in the else branche
                diffuseSpot   *= attenuationSpot;
                specularSpot *= attenuationSpot;

                resultSpot = ambientSpot + diffuseSpot + specularSpot;
                //resultSpot= vec3(1.0);

    }

    // ambient
        vec3 ambient = light.ambient * texture(material.diffuse, TexCoords).rgb;

        // diffuse
        vec3 lightDir = normalize(light.position - FragPos);
        vec3 norm = normalize(Normal);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = light.diffuse * diff * texture(material.diffuse, TexCoords).rgb;

        // specular
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        vec3 halfWayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfWayDir), 0.0), material.shininess*2);
        vec3 specular = light.specular * spec * vec3(texture(material.specular,TexCoords).rgb);

        // attenuation
        float distance    = length(light.position - FragPos);
        float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

        ambient  *= attenuation;
        diffuse   *= attenuation;
        specular *= attenuation;

        vec3 result = ambient + diffuse + specular + resultSpot;
        FragColor = vec4(result, 1.0);

}