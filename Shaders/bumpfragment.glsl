# version 330 core
uniform sampler2D diffuseTex;
uniform sampler2D bumpTex;

uniform vec3 cameraPos;
uniform vec4 lightColour[4];
uniform vec3 lightPos[4];
uniform float lightRadius[4];
uniform vec4 u_fogColor;
uniform float u_fogAmount;

in Vertex {
	vec3 colour;
	vec2 texCoord;
	vec3 normal;
	vec3 tangent; 
	vec3 binormal;
	vec3 worldPos;
} IN;

out vec4 fragColour ;

void main ( void ) {
	vec3 viewDir = normalize ( cameraPos - IN . worldPos );
	vec4 diffuse = texture ( diffuseTex , IN . texCoord );
	vec3 surface;

	for (int i = 0; i < 4; i++) {
		vec3 incident = normalize ( lightPos[i] - IN . worldPos );
		vec3 halfDir = normalize ( incident + viewDir );

		 mat3 TBN = mat3 ( normalize ( IN.tangent ), normalize ( IN.binormal ), normalize ( IN.normal ));
		
		vec3 bumpNormal = texture ( bumpTex , IN . texCoord ). rgb ;
		bumpNormal = normalize ( TBN * normalize ( bumpNormal * 2.0 - 1.0));

		float lambert = max ( dot ( incident , bumpNormal ), 0.0f );
		float distance = length ( lightPos[i] - IN.worldPos );
		float attenuation = 1.0f - clamp ( distance / lightRadius[i], 0.0 ,1.0);

		float specFactor = clamp ( dot ( halfDir , bumpNormal ), 0.0 ,1.0);
		specFactor = pow ( specFactor , 60.0);

		surface += ( diffuse.rgb * lightColour[i].rgb );
		fragColour.rgb += surface * lambert * attenuation ;
		fragColour.rgb += ( lightColour[i].rgb * specFactor )* attenuation *0.33;
		fragColour.a = diffuse.a ;
	}

	fragColour.rgb += surface * 0.1f ;
}

