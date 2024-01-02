
-------------------------------
#version 100
precision mediump float;
precision mediump int;
attribute highp vec4 _gl4es_Vertex;
attribute lowp vec4 _gl4es_Color;
attribute highp vec4 _gl4es_MultiTexCoord0;
uniform highp mat4 _gl4es_ModelViewProjectionMatrix;
// FPE_Shader generated
varying vec4 Color;
varying vec4 _gl4es_TexCoord_0;

void main() {
gl_Position = _gl4es_ModelViewProjectionMatrix * _gl4es_Vertex;
Color = _gl4es_Color;
_gl4es_TexCoord_0 = _gl4es_MultiTexCoord0.stpq;
}


----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
// FPE_Shader generated
varying vec4 Color;
varying vec4 _gl4es_TexCoord_0;
uniform sampler2D _gl4es_TexSampler_0;
void main() {
vec4 fColor = Color;
vec4 texColor0 = texture2DProj(_gl4es_TexSampler_0, _gl4es_TexCoord_0);
fColor.rgb *= texColor0.rgb;
gl_FragColor = fColor;
}

----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
attribute highp vec4 _gl4es_Vertex;
attribute lowp vec4 _gl4es_Color;
uniform highp mat4 _gl4es_ModelViewProjectionMatrix;
// FPE_Shader generated
varying vec4 Color;

void main() {
gl_Position = _gl4es_ModelViewProjectionMatrix * _gl4es_Vertex;
Color = _gl4es_Color;
}


----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
// FPE_Shader generated
varying vec4 Color;
void main() {
vec4 fColor = Color;
gl_FragColor = fColor;
}

----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
attribute highp vec4 _gl4es_Vertex;
attribute lowp vec4 _gl4es_Color;
attribute highp vec4 _gl4es_MultiTexCoord0;
uniform highp mat4 _gl4es_ModelViewProjectionMatrix;
// FPE_Shader generated
varying vec4 Color;
varying vec4 _gl4es_TexCoord_0;

void main() {
gl_Position = _gl4es_ModelViewProjectionMatrix * _gl4es_Vertex;
Color = _gl4es_Color;
_gl4es_TexCoord_0 = _gl4es_MultiTexCoord0.stpq;
}


----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
// FPE_Shader generated
varying vec4 Color;
varying vec4 _gl4es_TexCoord_0;
uniform sampler2D _gl4es_TexSampler_0;
uniform float _gl4es_AlphaRef;
void main() {
vec4 fColor = Color;
vec4 texColor0 = texture2DProj(_gl4es_TexSampler_0, _gl4es_TexCoord_0);
fColor *= texColor0;
if (floor(fColor.a*255.) <= _gl4es_AlphaRef) discard;
gl_FragColor = fColor;
}

----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
attribute highp vec4 _gl4es_Vertex;
attribute lowp vec4 _gl4es_Color;
attribute highp vec4 _gl4es_MultiTexCoord0;
attribute highp vec4 _gl4es_MultiTexCoord1;
attribute highp vec3 _gl4es_Normal;
uniform highp mat4 _gl4es_ModelViewProjectionMatrix;
uniform highp mat3 _gl4es_NormalMatrix;
struct _gl4es_LightModelParameters {
  vec4 ambient;
};
uniform _gl4es_LightModelParameters _gl4es_LightModel;
struct _gl4es_MaterialParameters
{
   vec4 emission;
   vec4 ambient;
   vec4 diffuse;
   vec4 specular;
   float shininess;
};
uniform _gl4es_MaterialParameters _gl4es_FrontMaterial;
uniform _gl4es_MaterialParameters _gl4es_BackMaterial;
// FPE_Shader generated
varying vec4 Color;
struct _gl4es_FPELightSourceParameters1
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
   highp float constantAttenuation;
   highp float linearAttenuation;
   highp float quadraticAttenuation;
};
struct _gl4es_FPELightSourceParameters0
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
};
struct _gl4es_LightProducts
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
};
uniform highp float _gl4es_FrontMaterial_shininess;
varying vec4 _gl4es_TexCoord_0;
varying vec4 _gl4es_TexCoord_1;

void main() {
vec3 normal = normalize(_gl4es_NormalMatrix * _gl4es_Normal);
gl_Position = _gl4es_ModelViewProjectionMatrix * _gl4es_Vertex;
Color = _gl4es_FrontMaterial.emission;
Color += _gl4es_Color*_gl4es_LightModel.ambient;
highp float att;
highp float spot;
highp vec3 VP;
highp float lVP;
highp float nVP;
highp vec3 aa,dd,ss;
highp vec3 hi;
Color.a = _gl4es_Color.a;
Color.rgb = clamp(Color.rgb, 0., 1.);
_gl4es_TexCoord_0 = _gl4es_MultiTexCoord0.stpq;
_gl4es_TexCoord_1 = _gl4es_MultiTexCoord1.stpq;
}


----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
// FPE_Shader generated
varying vec4 Color;
varying vec4 _gl4es_TexCoord_0;
uniform sampler2D _gl4es_TexSampler_0;
varying vec4 _gl4es_TexCoord_1;
uniform sampler2D _gl4es_TexSampler_1;
void main() {
vec4 fColor = Color;
vec4 texColor0 = texture2DProj(_gl4es_TexSampler_0, _gl4es_TexCoord_0);
vec4 texColor1 = texture2DProj(_gl4es_TexSampler_1, _gl4es_TexCoord_1);
fColor.rgb *= texColor0.rgb;
fColor *= texColor1;
gl_FragColor = fColor;
}

----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
attribute highp vec4 _gl4es_Vertex;
attribute lowp vec4 _gl4es_Color;
attribute highp vec4 _gl4es_MultiTexCoord0;
attribute highp vec4 _gl4es_MultiTexCoord1;
attribute highp vec3 _gl4es_Normal;
uniform highp mat4 _gl4es_ModelViewProjectionMatrix;
uniform highp mat3 _gl4es_NormalMatrix;
struct _gl4es_LightModelParameters {
  vec4 ambient;
};
uniform _gl4es_LightModelParameters _gl4es_LightModel;
struct _gl4es_MaterialParameters
{
   vec4 emission;
   vec4 ambient;
   vec4 diffuse;
   vec4 specular;
   float shininess;
};
uniform _gl4es_MaterialParameters _gl4es_FrontMaterial;
uniform _gl4es_MaterialParameters _gl4es_BackMaterial;
// FPE_Shader generated
varying vec4 Color;
struct _gl4es_FPELightSourceParameters1
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
   highp float constantAttenuation;
   highp float linearAttenuation;
   highp float quadraticAttenuation;
};
struct _gl4es_FPELightSourceParameters0
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
};
struct _gl4es_LightProducts
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
};
uniform highp float _gl4es_FrontMaterial_shininess;
varying vec4 _gl4es_TexCoord_0;
varying vec4 _gl4es_TexCoord_1;

void main() {
vec3 normal = normalize(_gl4es_NormalMatrix * _gl4es_Normal);
gl_Position = _gl4es_ModelViewProjectionMatrix * _gl4es_Vertex;
Color = _gl4es_FrontMaterial.emission;
Color += _gl4es_Color*_gl4es_LightModel.ambient;
highp float att;
highp float spot;
highp vec3 VP;
highp float lVP;
highp float nVP;
highp vec3 aa,dd,ss;
highp vec3 hi;
Color.a = _gl4es_Color.a;
Color.rgb = clamp(Color.rgb, 0., 1.);
_gl4es_TexCoord_0 = _gl4es_MultiTexCoord0.stpq;
_gl4es_TexCoord_1 = _gl4es_MultiTexCoord1.stpq;
}


----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
// FPE_Shader generated
varying vec4 Color;
varying vec4 _gl4es_TexCoord_0;
uniform sampler2D _gl4es_TexSampler_0;
varying vec4 _gl4es_TexCoord_1;
uniform sampler2D _gl4es_TexSampler_1;
void main() {
vec4 fColor = Color;
vec4 texColor0 = texture2DProj(_gl4es_TexSampler_0, _gl4es_TexCoord_0);
vec4 texColor1 = texture2DProj(_gl4es_TexSampler_1, _gl4es_TexCoord_1);
fColor.rgb *= texColor0.rgb;
fColor.rgb *= texColor1.rgb;
gl_FragColor = fColor;
}

----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
attribute highp vec4 _gl4es_Vertex;
attribute lowp vec4 _gl4es_Color;
attribute highp vec4 _gl4es_MultiTexCoord0;
attribute highp vec4 _gl4es_MultiTexCoord1;
attribute highp vec3 _gl4es_Normal;
uniform highp mat4 _gl4es_ModelViewMatrix;
uniform highp mat4 _gl4es_ModelViewProjectionMatrix;
uniform highp mat3 _gl4es_NormalMatrix;
struct _gl4es_LightModelParameters {
  vec4 ambient;
};
uniform _gl4es_LightModelParameters _gl4es_LightModel;
struct _gl4es_MaterialParameters
{
   vec4 emission;
   vec4 ambient;
   vec4 diffuse;
   vec4 specular;
   float shininess;
};
uniform _gl4es_MaterialParameters _gl4es_FrontMaterial;
uniform _gl4es_MaterialParameters _gl4es_BackMaterial;
// FPE_Shader generated
varying vec4 Color;
uniform highp vec4 _gl4es_ClipPlane_0;
varying mediump float clippedvertex_0;
uniform highp vec4 _gl4es_ClipPlane_1;
varying mediump float clippedvertex_1;
uniform highp vec4 _gl4es_ClipPlane_2;
varying mediump float clippedvertex_2;
struct _gl4es_FPELightSourceParameters1
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
   highp float constantAttenuation;
   highp float linearAttenuation;
   highp float quadraticAttenuation;
};
struct _gl4es_FPELightSourceParameters0
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
};
struct _gl4es_LightProducts
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
};
uniform highp float _gl4es_FrontMaterial_shininess;
varying vec4 _gl4es_TexCoord_0;
varying vec4 _gl4es_TexCoord_1;

void main() {
vec4 vertex = _gl4es_ModelViewMatrix * _gl4es_Vertex;
vec3 normal = normalize(_gl4es_NormalMatrix * _gl4es_Normal);
clippedvertex_0 = dot(vertex, _gl4es_ClipPlane_0);
clippedvertex_1 = dot(vertex, _gl4es_ClipPlane_1);
clippedvertex_2 = dot(vertex, _gl4es_ClipPlane_2);
gl_Position = _gl4es_ModelViewProjectionMatrix * _gl4es_Vertex;
Color = _gl4es_FrontMaterial.emission;
Color += _gl4es_Color*_gl4es_LightModel.ambient;
highp float att;
highp float spot;
highp vec3 VP;
highp float lVP;
highp float nVP;
highp vec3 aa,dd,ss;
highp vec3 hi;
Color.a = _gl4es_Color.a;
Color.rgb = clamp(Color.rgb, 0., 1.);
_gl4es_TexCoord_0 = _gl4es_MultiTexCoord0.stpq;
_gl4es_TexCoord_1 = _gl4es_MultiTexCoord1.stpq;
}


----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
// FPE_Shader generated
varying vec4 Color;
varying mediump float clippedvertex_0;
varying mediump float clippedvertex_1;
varying mediump float clippedvertex_2;
varying vec4 _gl4es_TexCoord_0;
uniform sampler2D _gl4es_TexSampler_0;
varying vec4 _gl4es_TexCoord_1;
uniform sampler2D _gl4es_TexSampler_1;
void main() {
if((min(0., clippedvertex_0)+min(0., clippedvertex_1)+min(0., clippedvertex_2))<0.) discard;
vec4 fColor = Color;
vec4 texColor0 = texture2DProj(_gl4es_TexSampler_0, _gl4es_TexCoord_0);
vec4 texColor1 = texture2DProj(_gl4es_TexSampler_1, _gl4es_TexCoord_1);
fColor.rgb *= texColor0.rgb;
fColor *= texColor1;
gl_FragColor = fColor;
}

----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
attribute highp vec4 _gl4es_Vertex;
attribute lowp vec4 _gl4es_Color;
attribute highp vec4 _gl4es_MultiTexCoord0;
attribute highp vec4 _gl4es_MultiTexCoord1;
attribute highp vec3 _gl4es_Normal;
uniform highp mat4 _gl4es_ModelViewMatrix;
uniform highp mat4 _gl4es_ModelViewProjectionMatrix;
uniform highp mat3 _gl4es_NormalMatrix;
struct _gl4es_LightModelParameters {
  vec4 ambient;
};
uniform _gl4es_LightModelParameters _gl4es_LightModel;
struct _gl4es_MaterialParameters
{
   vec4 emission;
   vec4 ambient;
   vec4 diffuse;
   vec4 specular;
   float shininess;
};
uniform _gl4es_MaterialParameters _gl4es_FrontMaterial;
uniform _gl4es_MaterialParameters _gl4es_BackMaterial;
// FPE_Shader generated
varying vec4 Color;
uniform highp vec4 _gl4es_ClipPlane_0;
varying mediump float clippedvertex_0;
uniform highp vec4 _gl4es_ClipPlane_1;
varying mediump float clippedvertex_1;
uniform highp vec4 _gl4es_ClipPlane_2;
varying mediump float clippedvertex_2;
struct _gl4es_FPELightSourceParameters1
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
   highp float constantAttenuation;
   highp float linearAttenuation;
   highp float quadraticAttenuation;
};
struct _gl4es_FPELightSourceParameters0
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
};
struct _gl4es_LightProducts
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
};
uniform highp float _gl4es_FrontMaterial_shininess;
varying vec4 _gl4es_TexCoord_0;
varying vec4 _gl4es_TexCoord_1;

void main() {
vec4 vertex = _gl4es_ModelViewMatrix * _gl4es_Vertex;
vec3 normal = normalize(_gl4es_NormalMatrix * _gl4es_Normal);
clippedvertex_0 = dot(vertex, _gl4es_ClipPlane_0);
clippedvertex_1 = dot(vertex, _gl4es_ClipPlane_1);
clippedvertex_2 = dot(vertex, _gl4es_ClipPlane_2);
gl_Position = _gl4es_ModelViewProjectionMatrix * _gl4es_Vertex;
Color = _gl4es_FrontMaterial.emission;
Color += _gl4es_Color*_gl4es_LightModel.ambient;
highp float att;
highp float spot;
highp vec3 VP;
highp float lVP;
highp float nVP;
highp vec3 aa,dd,ss;
highp vec3 hi;
Color.a = _gl4es_Color.a;
Color.rgb = clamp(Color.rgb, 0., 1.);
_gl4es_TexCoord_0 = _gl4es_MultiTexCoord0.stpq;
_gl4es_TexCoord_1 = _gl4es_MultiTexCoord1.stpq;
}


----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
// FPE_Shader generated
varying vec4 Color;
varying mediump float clippedvertex_0;
varying mediump float clippedvertex_1;
varying mediump float clippedvertex_2;
varying vec4 _gl4es_TexCoord_0;
uniform sampler2D _gl4es_TexSampler_0;
varying vec4 _gl4es_TexCoord_1;
uniform sampler2D _gl4es_TexSampler_1;
void main() {
if((min(0., clippedvertex_0)+min(0., clippedvertex_1)+min(0., clippedvertex_2))<0.) discard;
vec4 fColor = Color;
vec4 texColor0 = texture2DProj(_gl4es_TexSampler_0, _gl4es_TexCoord_0);
vec4 texColor1 = texture2DProj(_gl4es_TexSampler_1, _gl4es_TexCoord_1);
fColor.rgb *= texColor0.rgb;
fColor.rgb *= texColor1.rgb;
gl_FragColor = fColor;
}

----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
attribute highp vec4 _gl4es_Vertex;
attribute lowp vec4 _gl4es_Color;
attribute highp vec4 _gl4es_MultiTexCoord0;
attribute highp vec4 _gl4es_MultiTexCoord1;
attribute highp vec3 _gl4es_Normal;
uniform highp mat4 _gl4es_ModelViewMatrix;
uniform highp mat4 _gl4es_ModelViewProjectionMatrix;
uniform highp mat3 _gl4es_NormalMatrix;
struct _gl4es_LightModelParameters {
  vec4 ambient;
};
uniform _gl4es_LightModelParameters _gl4es_LightModel;
struct _gl4es_MaterialParameters
{
   vec4 emission;
   vec4 ambient;
   vec4 diffuse;
   vec4 specular;
   float shininess;
};
uniform _gl4es_MaterialParameters _gl4es_FrontMaterial;
uniform _gl4es_MaterialParameters _gl4es_BackMaterial;
// FPE_Shader generated
varying vec4 Color;
uniform highp vec4 _gl4es_ClipPlane_0;
varying mediump float clippedvertex_0;
uniform highp vec4 _gl4es_ClipPlane_1;
varying mediump float clippedvertex_1;
uniform highp vec4 _gl4es_ClipPlane_2;
varying mediump float clippedvertex_2;
struct _gl4es_FPELightSourceParameters1
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
   highp float constantAttenuation;
   highp float linearAttenuation;
   highp float quadraticAttenuation;
};
struct _gl4es_FPELightSourceParameters0
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
};
struct _gl4es_LightProducts
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
};
uniform highp float _gl4es_FrontMaterial_shininess;
varying vec4 _gl4es_TexCoord_0;
varying vec4 _gl4es_TexCoord_1;

void main() {
vec4 vertex = _gl4es_ModelViewMatrix * _gl4es_Vertex;
vec3 normal = normalize(_gl4es_NormalMatrix * _gl4es_Normal);
clippedvertex_0 = dot(vertex, _gl4es_ClipPlane_0);
clippedvertex_1 = dot(vertex, _gl4es_ClipPlane_1);
clippedvertex_2 = dot(vertex, _gl4es_ClipPlane_2);
gl_Position = _gl4es_ModelViewProjectionMatrix * _gl4es_Vertex;
Color = _gl4es_FrontMaterial.emission;
Color += _gl4es_Color*_gl4es_LightModel.ambient;
highp float att;
highp float spot;
highp vec3 VP;
highp float lVP;
highp float nVP;
highp vec3 aa,dd,ss;
highp vec3 hi;
Color.a = _gl4es_Color.a;
Color.rgb = clamp(Color.rgb, 0., 1.);
_gl4es_TexCoord_0 = _gl4es_MultiTexCoord0.stpq;
_gl4es_TexCoord_1 = _gl4es_MultiTexCoord1.stpq;
}


----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
// FPE_Shader generated
varying vec4 Color;
varying mediump float clippedvertex_0;
varying mediump float clippedvertex_1;
varying mediump float clippedvertex_2;
varying vec4 _gl4es_TexCoord_0;
uniform sampler2D _gl4es_TexSampler_0;
varying vec4 _gl4es_TexCoord_1;
uniform sampler2D _gl4es_TexSampler_1;
uniform float _gl4es_AlphaRef;
void main() {
if((min(0., clippedvertex_0)+min(0., clippedvertex_1)+min(0., clippedvertex_2))<0.) discard;
vec4 fColor = Color;
vec4 texColor0 = texture2DProj(_gl4es_TexSampler_0, _gl4es_TexCoord_0);
vec4 texColor1 = texture2DProj(_gl4es_TexSampler_1, _gl4es_TexCoord_1);
fColor.rgb *= texColor0.rgb;
fColor *= texColor1;
if (floor(fColor.a*255.) <= _gl4es_AlphaRef) discard;
gl_FragColor = fColor;
}

----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
attribute highp vec4 _gl4es_Vertex;
attribute lowp vec4 _gl4es_Color;
attribute highp vec4 _gl4es_MultiTexCoord0;
attribute highp vec4 _gl4es_MultiTexCoord1;
attribute highp vec3 _gl4es_Normal;
uniform highp mat4 _gl4es_ModelViewMatrix;
uniform highp mat4 _gl4es_ModelViewProjectionMatrix;
uniform highp mat3 _gl4es_NormalMatrix;
struct _gl4es_LightModelParameters {
  vec4 ambient;
};
uniform _gl4es_LightModelParameters _gl4es_LightModel;
struct _gl4es_MaterialParameters
{
   vec4 emission;
   vec4 ambient;
   vec4 diffuse;
   vec4 specular;
   float shininess;
};
uniform _gl4es_MaterialParameters _gl4es_FrontMaterial;
uniform _gl4es_MaterialParameters _gl4es_BackMaterial;
// FPE_Shader generated
varying vec4 Color;
uniform highp vec4 _gl4es_ClipPlane_0;
varying mediump float clippedvertex_0;
uniform highp vec4 _gl4es_ClipPlane_1;
varying mediump float clippedvertex_1;
uniform highp vec4 _gl4es_ClipPlane_2;
varying mediump float clippedvertex_2;
struct _gl4es_FPELightSourceParameters1
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
   highp float constantAttenuation;
   highp float linearAttenuation;
   highp float quadraticAttenuation;
};
struct _gl4es_FPELightSourceParameters0
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
};
struct _gl4es_LightProducts
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
};
uniform highp float _gl4es_FrontMaterial_shininess;
varying vec4 _gl4es_TexCoord_0;
varying vec4 _gl4es_TexCoord_1;

void main() {
vec4 vertex = _gl4es_ModelViewMatrix * _gl4es_Vertex;
vec3 normal = normalize(_gl4es_NormalMatrix * _gl4es_Normal);
clippedvertex_0 = dot(vertex, _gl4es_ClipPlane_0);
clippedvertex_1 = dot(vertex, _gl4es_ClipPlane_1);
clippedvertex_2 = dot(vertex, _gl4es_ClipPlane_2);
gl_Position = _gl4es_ModelViewProjectionMatrix * _gl4es_Vertex;
Color = _gl4es_FrontMaterial.emission;
Color += _gl4es_Color*_gl4es_LightModel.ambient;
highp float att;
highp float spot;
highp vec3 VP;
highp float lVP;
highp float nVP;
highp vec3 aa,dd,ss;
highp vec3 hi;
Color.a = _gl4es_Color.a;
Color.rgb = clamp(Color.rgb, 0., 1.);
_gl4es_TexCoord_0 = _gl4es_MultiTexCoord0.stpq;
_gl4es_TexCoord_1 = _gl4es_MultiTexCoord1.stpq;
}


----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
// FPE_Shader generated
varying vec4 Color;
varying mediump float clippedvertex_0;
varying mediump float clippedvertex_1;
varying mediump float clippedvertex_2;
varying vec4 _gl4es_TexCoord_0;
uniform sampler2D _gl4es_TexSampler_0;
varying vec4 _gl4es_TexCoord_1;
uniform sampler2D _gl4es_TexSampler_1;
uniform float _gl4es_AlphaRef;
void main() {
if((min(0., clippedvertex_0)+min(0., clippedvertex_1)+min(0., clippedvertex_2))<0.) discard;
vec4 fColor = Color;
vec4 texColor0 = texture2DProj(_gl4es_TexSampler_0, _gl4es_TexCoord_0);
vec4 texColor1 = texture2DProj(_gl4es_TexSampler_1, _gl4es_TexCoord_1);
fColor.rgb *= texColor0.rgb;
fColor.rgb *= texColor1.rgb;
if (floor(fColor.a*255.) <= _gl4es_AlphaRef) discard;
gl_FragColor = fColor;
}

----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
attribute highp vec4 _gl4es_Vertex;
attribute lowp vec4 _gl4es_Color;
attribute highp vec4 _gl4es_MultiTexCoord0;
attribute highp vec3 _gl4es_Normal;
uniform highp mat4 _gl4es_ModelViewMatrix;
uniform highp mat4 _gl4es_ModelViewProjectionMatrix;
uniform highp mat3 _gl4es_NormalMatrix;
struct _gl4es_LightModelParameters {
  vec4 ambient;
};
uniform _gl4es_LightModelParameters _gl4es_LightModel;
struct _gl4es_MaterialParameters
{
   vec4 emission;
   vec4 ambient;
   vec4 diffuse;
   vec4 specular;
   float shininess;
};
uniform _gl4es_MaterialParameters _gl4es_FrontMaterial;
uniform _gl4es_MaterialParameters _gl4es_BackMaterial;
// FPE_Shader generated
varying vec4 Color;
uniform highp vec4 _gl4es_ClipPlane_0;
varying mediump float clippedvertex_0;
uniform highp vec4 _gl4es_ClipPlane_1;
varying mediump float clippedvertex_1;
uniform highp vec4 _gl4es_ClipPlane_2;
varying mediump float clippedvertex_2;
struct _gl4es_FPELightSourceParameters1
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
   highp float constantAttenuation;
   highp float linearAttenuation;
   highp float quadraticAttenuation;
};
struct _gl4es_FPELightSourceParameters0
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
};
struct _gl4es_LightProducts
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
};
uniform highp float _gl4es_FrontMaterial_shininess;
varying vec4 _gl4es_TexCoord_0;

void main() {
vec4 vertex = _gl4es_ModelViewMatrix * _gl4es_Vertex;
vec3 normal = normalize(_gl4es_NormalMatrix * _gl4es_Normal);
clippedvertex_0 = dot(vertex, _gl4es_ClipPlane_0);
clippedvertex_1 = dot(vertex, _gl4es_ClipPlane_1);
clippedvertex_2 = dot(vertex, _gl4es_ClipPlane_2);
gl_Position = _gl4es_ModelViewProjectionMatrix * _gl4es_Vertex;
Color = _gl4es_FrontMaterial.emission;
Color += _gl4es_Color*_gl4es_LightModel.ambient;
highp float att;
highp float spot;
highp vec3 VP;
highp float lVP;
highp float nVP;
highp vec3 aa,dd,ss;
highp vec3 hi;
Color.a = _gl4es_Color.a;
Color.rgb = clamp(Color.rgb, 0., 1.);
_gl4es_TexCoord_0 = _gl4es_MultiTexCoord0.stpq;
}


----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
// FPE_Shader generated
varying vec4 Color;
varying mediump float clippedvertex_0;
varying mediump float clippedvertex_1;
varying mediump float clippedvertex_2;
varying vec4 _gl4es_TexCoord_0;
uniform sampler2D _gl4es_TexSampler_0;
uniform float _gl4es_AlphaRef;
void main() {
if((min(0., clippedvertex_0)+min(0., clippedvertex_1)+min(0., clippedvertex_2))<0.) discard;
vec4 fColor = Color;
vec4 texColor0 = texture2DProj(_gl4es_TexSampler_0, _gl4es_TexCoord_0);
fColor *= texColor0;
if (floor(fColor.a*255.) <= _gl4es_AlphaRef) discard;
gl_FragColor = fColor;
}

----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
attribute highp vec4 _gl4es_Vertex;
attribute lowp vec4 _gl4es_Color;
attribute highp vec3 _gl4es_Normal;
uniform highp mat4 _gl4es_ModelViewProjectionMatrix;
uniform highp mat3 _gl4es_NormalMatrix;
struct _gl4es_LightModelParameters {
  vec4 ambient;
};
uniform _gl4es_LightModelParameters _gl4es_LightModel;
struct _gl4es_MaterialParameters
{
   vec4 emission;
   vec4 ambient;
   vec4 diffuse;
   vec4 specular;
   float shininess;
};
uniform _gl4es_MaterialParameters _gl4es_FrontMaterial;
uniform _gl4es_MaterialParameters _gl4es_BackMaterial;
// FPE_Shader generated
varying vec4 Color;
struct _gl4es_FPELightSourceParameters1
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
   highp float constantAttenuation;
   highp float linearAttenuation;
   highp float quadraticAttenuation;
};
struct _gl4es_FPELightSourceParameters0
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
};
struct _gl4es_LightProducts
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
};
uniform highp float _gl4es_FrontMaterial_shininess;

void main() {
vec3 normal = normalize(_gl4es_NormalMatrix * _gl4es_Normal);
gl_Position = _gl4es_ModelViewProjectionMatrix * _gl4es_Vertex;
Color = _gl4es_FrontMaterial.emission;
Color += _gl4es_Color*_gl4es_LightModel.ambient;
highp float att;
highp float spot;
highp vec3 VP;
highp float lVP;
highp float nVP;
highp vec3 aa,dd,ss;
highp vec3 hi;
Color.a = _gl4es_Color.a;
Color.rgb = clamp(Color.rgb, 0., 1.);
}


----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
// FPE_Shader generated
varying vec4 Color;
void main() {
vec4 fColor = Color;
gl_FragColor = fColor;
}

----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
attribute highp vec4 _gl4es_Vertex;
attribute lowp vec4 _gl4es_Color;
attribute highp vec4 _gl4es_MultiTexCoord0;
attribute highp vec3 _gl4es_Normal;
uniform highp mat4 _gl4es_ModelViewProjectionMatrix;
uniform highp mat3 _gl4es_NormalMatrix;
struct _gl4es_LightModelParameters {
  vec4 ambient;
};
uniform _gl4es_LightModelParameters _gl4es_LightModel;
struct _gl4es_MaterialParameters
{
   vec4 emission;
   vec4 ambient;
   vec4 diffuse;
   vec4 specular;
   float shininess;
};
uniform _gl4es_MaterialParameters _gl4es_FrontMaterial;
uniform _gl4es_MaterialParameters _gl4es_BackMaterial;
// FPE_Shader generated
varying vec4 Color;
struct _gl4es_FPELightSourceParameters1
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
   highp float constantAttenuation;
   highp float linearAttenuation;
   highp float quadraticAttenuation;
};
struct _gl4es_FPELightSourceParameters0
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
};
struct _gl4es_LightProducts
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
};
uniform highp float _gl4es_FrontMaterial_shininess;
varying vec4 _gl4es_TexCoord_0;

void main() {
vec3 normal = normalize(_gl4es_NormalMatrix * _gl4es_Normal);
gl_Position = _gl4es_ModelViewProjectionMatrix * _gl4es_Vertex;
Color = _gl4es_FrontMaterial.emission;
Color += _gl4es_Color*_gl4es_LightModel.ambient;
highp float att;
highp float spot;
highp vec3 VP;
highp float lVP;
highp float nVP;
highp vec3 aa,dd,ss;
highp vec3 hi;
Color.a = _gl4es_Color.a;
Color.rgb = clamp(Color.rgb, 0., 1.);
_gl4es_TexCoord_0 = _gl4es_MultiTexCoord0.stpq;
}


----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
// FPE_Shader generated
varying vec4 Color;
varying vec4 _gl4es_TexCoord_0;
uniform sampler2D _gl4es_TexSampler_0;
uniform float _gl4es_AlphaRef;
void main() {
vec4 fColor = Color;
vec4 texColor0 = texture2DProj(_gl4es_TexSampler_0, _gl4es_TexCoord_0);
fColor *= texColor0;
if (floor(fColor.a*255.) <= _gl4es_AlphaRef) discard;
gl_FragColor = fColor;
}

----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
attribute highp vec4 _gl4es_Vertex;
attribute lowp vec4 _gl4es_Color;
attribute highp vec4 _gl4es_MultiTexCoord0;
attribute highp vec4 _gl4es_MultiTexCoord1;
attribute highp vec3 _gl4es_Normal;
uniform highp mat4 _gl4es_ModelViewProjectionMatrix;
uniform highp mat3 _gl4es_NormalMatrix;
struct _gl4es_LightModelParameters {
  vec4 ambient;
};
uniform _gl4es_LightModelParameters _gl4es_LightModel;
struct _gl4es_MaterialParameters
{
   vec4 emission;
   vec4 ambient;
   vec4 diffuse;
   vec4 specular;
   float shininess;
};
uniform _gl4es_MaterialParameters _gl4es_FrontMaterial;
uniform _gl4es_MaterialParameters _gl4es_BackMaterial;
// FPE_Shader generated
varying vec4 Color;
struct _gl4es_FPELightSourceParameters1
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
   highp float constantAttenuation;
   highp float linearAttenuation;
   highp float quadraticAttenuation;
};
struct _gl4es_FPELightSourceParameters0
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
};
struct _gl4es_LightProducts
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
};
uniform highp float _gl4es_FrontMaterial_shininess;
varying vec4 _gl4es_TexCoord_0;
varying vec4 _gl4es_TexCoord_1;

void main() {
vec3 normal = normalize(_gl4es_NormalMatrix * _gl4es_Normal);
gl_Position = _gl4es_ModelViewProjectionMatrix * _gl4es_Vertex;
Color = _gl4es_FrontMaterial.emission;
Color += _gl4es_Color*_gl4es_LightModel.ambient;
highp float att;
highp float spot;
highp vec3 VP;
highp float lVP;
highp float nVP;
highp vec3 aa,dd,ss;
highp vec3 hi;
Color.a = _gl4es_Color.a;
Color.rgb = clamp(Color.rgb, 0., 1.);
_gl4es_TexCoord_0 = _gl4es_MultiTexCoord0.stpq;
_gl4es_TexCoord_1 = _gl4es_MultiTexCoord1.stpq;
}


----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
// FPE_Shader generated
varying vec4 Color;
varying vec4 _gl4es_TexCoord_0;
uniform sampler2D _gl4es_TexSampler_0;
varying vec4 _gl4es_TexCoord_1;
uniform sampler2D _gl4es_TexSampler_1;
uniform float _gl4es_AlphaRef;
void main() {
vec4 fColor = Color;
vec4 texColor0 = texture2DProj(_gl4es_TexSampler_0, _gl4es_TexCoord_0);
vec4 texColor1 = texture2DProj(_gl4es_TexSampler_1, _gl4es_TexCoord_1);
fColor.rgb *= texColor0.rgb;
fColor *= texColor1;
if (floor(fColor.a*255.) <= _gl4es_AlphaRef) discard;
gl_FragColor = fColor;
}

----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
attribute highp vec4 _gl4es_Vertex;
attribute lowp vec4 _gl4es_Color;
attribute highp vec4 _gl4es_MultiTexCoord0;
attribute highp vec4 _gl4es_MultiTexCoord1;
attribute highp vec3 _gl4es_Normal;
uniform highp mat4 _gl4es_ModelViewProjectionMatrix;
uniform highp mat3 _gl4es_NormalMatrix;
struct _gl4es_LightModelParameters {
  vec4 ambient;
};
uniform _gl4es_LightModelParameters _gl4es_LightModel;
struct _gl4es_MaterialParameters
{
   vec4 emission;
   vec4 ambient;
   vec4 diffuse;
   vec4 specular;
   float shininess;
};
uniform _gl4es_MaterialParameters _gl4es_FrontMaterial;
uniform _gl4es_MaterialParameters _gl4es_BackMaterial;
// FPE_Shader generated
varying vec4 Color;
struct _gl4es_FPELightSourceParameters1
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
   highp float constantAttenuation;
   highp float linearAttenuation;
   highp float quadraticAttenuation;
};
struct _gl4es_FPELightSourceParameters0
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
   highp vec4 position;
   highp vec3 spotDirection;
   highp float spotExponent;
   highp float spotCosCutoff;
};
struct _gl4es_LightProducts
{
   highp vec4 ambient;
   highp vec4 diffuse;
   highp vec4 specular;
};
uniform highp float _gl4es_FrontMaterial_shininess;
varying vec4 _gl4es_TexCoord_0;
varying vec4 _gl4es_TexCoord_1;

void main() {
vec3 normal = normalize(_gl4es_NormalMatrix * _gl4es_Normal);
gl_Position = _gl4es_ModelViewProjectionMatrix * _gl4es_Vertex;
Color = _gl4es_FrontMaterial.emission;
Color += _gl4es_Color*_gl4es_LightModel.ambient;
highp float att;
highp float spot;
highp vec3 VP;
highp float lVP;
highp float nVP;
highp vec3 aa,dd,ss;
highp vec3 hi;
Color.a = _gl4es_Color.a;
Color.rgb = clamp(Color.rgb, 0., 1.);
_gl4es_TexCoord_0 = _gl4es_MultiTexCoord0.stpq;
_gl4es_TexCoord_1 = _gl4es_MultiTexCoord1.stpq;
}


----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
// FPE_Shader generated
varying vec4 Color;
varying vec4 _gl4es_TexCoord_0;
uniform sampler2D _gl4es_TexSampler_0;
varying vec4 _gl4es_TexCoord_1;
uniform sampler2D _gl4es_TexSampler_1;
uniform float _gl4es_AlphaRef;
void main() {
vec4 fColor = Color;
vec4 texColor0 = texture2DProj(_gl4es_TexSampler_0, _gl4es_TexCoord_0);
vec4 texColor1 = texture2DProj(_gl4es_TexSampler_1, _gl4es_TexCoord_1);
fColor.rgb *= texColor0.rgb;
fColor.rgb *= texColor1.rgb;
if (floor(fColor.a*255.) <= _gl4es_AlphaRef) discard;
gl_FragColor = fColor;
}

----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
attribute highp vec4 _gl4es_Vertex;
attribute lowp vec4 _gl4es_Color;
attribute highp vec4 _gl4es_MultiTexCoord0;
uniform highp mat4 _gl4es_ModelViewProjectionMatrix;
// FPE_Shader generated
varying vec4 Color;
varying vec4 _gl4es_TexCoord_0;

void main() {
gl_Position = _gl4es_ModelViewProjectionMatrix * _gl4es_Vertex;
Color = _gl4es_Color;
_gl4es_TexCoord_0 = _gl4es_MultiTexCoord0.stpq;
}


----------------------------

-------------------------------
#version 100
precision mediump float;
precision mediump int;
// FPE_Shader generated
varying vec4 Color;
varying vec4 _gl4es_TexCoord_0;
uniform sampler2D _gl4es_TexSampler_0;
void main() {
vec4 fColor = Color;
vec4 texColor0 = texture2DProj(_gl4es_TexSampler_0, _gl4es_TexCoord_0);
fColor.a *= texColor0.a;
gl_FragColor = fColor;
}

----------------------------
