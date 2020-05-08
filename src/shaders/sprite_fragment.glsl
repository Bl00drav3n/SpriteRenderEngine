#version 330

uniform sampler2DArray Spritemaps;

in vec3 TexCoord;
in vec4 Tint;

out vec4 FinalColor;
void main()
{
	vec4 Color = Tint * texture(Spritemaps, TexCoord);
	FinalColor = vec4(Color.rgb * Color.a, Color.a);
    //FinalColor = texture(Spritemaps, vec3(0.40625, 0.0, 0.0));
    
    //FinalColor = vec4(1.0, 0.0, 1.0, 1.0);
	//FinalColor = vec4(TexCoord, 1.0);
}