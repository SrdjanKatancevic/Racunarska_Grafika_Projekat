#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;
in int flag;

// texture sampler
uniform sampler2D texture1;
uniform sampler2D texture2;

void main()
{
    if(flag==0){
	    FragColor = texture(texture2, TexCoord);
    }else if(flag==2){
        FragColor = texture(texture1, TexCoord);
    }else{
        FragColor = vec4(1.0);
    }
}