#version 330
layout(lines) in;
layout(triangle_strip, max_vertices=4) out;

uniform mat4 ProjectionTransform;
uniform float LineWidth;

void main() {
    vec2 P = gl_in[0].gl_Position.xy;
    vec2 Q = gl_in[1].gl_Position.xy;
    vec2 LineVec = Q - P;
    vec2 HalfOrtho = 0.5f * LineWidth * normalize(vec2(-LineVec.y, LineVec.x));
	
    gl_Position = ProjectionTransform * (vec4(P + HalfOrtho, 0, 1));
    EmitVertex();

    gl_Position = ProjectionTransform * (vec4(P - HalfOrtho, 0, 1));
    EmitVertex();

    gl_Position = ProjectionTransform * (vec4(Q + HalfOrtho, 0, 1));
    EmitVertex();

    gl_Position = ProjectionTransform * (vec4(Q - HalfOrtho, 0, 1));
    EmitVertex();
}