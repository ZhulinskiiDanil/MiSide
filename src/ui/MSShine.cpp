#include "MSShine.hpp"

#include "../Theme.hpp"
#include "MSStyle.hpp"

#include <cmath>

using namespace geode::prelude;

namespace
{
  // The usual sprite vertex shader
  constexpr char const* SHINE_VERTEX = R"(
attribute vec4 a_position;
attribute vec2 a_texCoord;
attribute vec4 a_color;
#ifdef GL_ES
varying lowp vec4 v_fragmentColor;
varying mediump vec2 v_texCoord;
#else
varying vec4 v_fragmentColor;
varying vec2 v_texCoord;
#endif
void main()
{
  gl_Position = CC_MVPMatrix * a_position;
  v_fragmentColor = a_color;
  v_texCoord = a_texCoord;
}
)";

  // The texture is the label rendered white, premultiplied. The band is measured in the label's width: the
  // texture coordinate divided by where the label ends in the texture. Out: premultiplied, only where the
  // letters are
  constexpr char const* SHINE_FRAGMENT = R"(
#ifdef GL_ES
precision mediump float;
#endif
varying vec4 v_fragmentColor;
varying vec2 v_texCoord;
uniform vec2 u_extent;
uniform float u_center;
uniform float u_width;
uniform float u_strength;
uniform float u_slant;
uniform float u_core;
uniform vec3 u_tint;
uniform vec3 u_coreColor;
void main()
{
  vec4 letters = texture2D(CC_Texture0, v_texCoord);
  vec2 along = v_texCoord / u_extent;
  float x = along.x + (along.y - 0.5) * u_slant;
  float halfWidth = max(u_width * 0.5, 0.0001);
  float distance = abs(x - u_center);
  float band = 1.0 - smoothstep(0.0, halfWidth, distance);
  float core = 1.0 - smoothstep(0.0, max(halfWidth * u_core, 0.0001), distance);
  float alpha = clamp(letters.a * band * u_strength, 0.0, 1.0);
  vec3 color = mix(u_tint, u_coreColor, core);
  gl_FragColor = vec4(color * alpha, alpha) * v_fragmentColor;
}
)";

  struct Shader
  {
    Ref<CCGLProgram> program;
    GLint extent = -1;
    GLint center = -1;
    GLint width = -1;
    GLint strength = -1;
    GLint slant = -1;
    GLint tint = -1;
    GLint core = -1;
    GLint coreColor = -1;
  };

  // Made once for the session, on the main thread; null when it doesn't compile
  Shader const* shader()
  {
    static Shader s_shader;
    static bool s_tried = false;
    if (s_tried)
      return s_shader.program ? &s_shader : nullptr;
    s_tried = true;
    auto program = new CCGLProgram();
    auto ok = program->initWithVertexShaderByteArray(SHINE_VERTEX, SHINE_FRAGMENT);
    if (ok)
    {
      program->addAttribute(kCCAttributeNamePosition, kCCVertexAttrib_Position);
      program->addAttribute(kCCAttributeNameColor, kCCVertexAttrib_Color);
      program->addAttribute(kCCAttributeNameTexCoord, kCCVertexAttrib_TexCoords);
      ok = program->link();
    }
    if (!ok)
    {
      program->release();
      log::warn("The shine shader doesn't compile here: the items have no shine");
      return nullptr;
    }
    program->updateUniforms();
    s_shader.program = program;
    program->release();
    s_shader.extent = program->getUniformLocationForName("u_extent");
    s_shader.center = program->getUniformLocationForName("u_center");
    s_shader.width = program->getUniformLocationForName("u_width");
    s_shader.strength = program->getUniformLocationForName("u_strength");
    s_shader.slant = program->getUniformLocationForName("u_slant");
    s_shader.tint = program->getUniformLocationForName("u_tint");
    s_shader.core = program->getUniformLocationForName("u_core");
    s_shader.coreColor = program->getUniformLocationForName("u_coreColor");
    return &s_shader;
  }
} // namespace

MSShine* MSShine::create(std::string const& text, float scale, std::string const& font)
{
  auto shine = shader();
  if (!shine)
    return nullptr;
  auto label = ms::createLabel(text, font);
  if (!label)
    return nullptr;
  label->setScale(scale);
  label->setAnchorPoint({0, 0});
  label->setPosition({0, 0});
  ms::setColor(label, ccWHITE);
  auto size = label->getScaledContentSize();
  // Once, at the label's own size in points (the texture gets the content scale factor's pixels: as sharp as the
  // label itself)
  auto texture = CCRenderTexture::create(static_cast<int>(std::ceil(size.width)), static_cast<int>(std::ceil(size.height)));
  if (!texture)
    return nullptr;
  texture->beginWithClear(0, 0, 0, 0);
  label->visit();
  texture->end();

  auto sprite = new MSShine();
  if (!sprite->initWithTexture(texture->getSprite()->getTexture()))
  {
    delete sprite;
    return nullptr;
  }
  sprite->autorelease();
  // A render texture is upside down; its pixels are premultiplied by the label's drawing
  sprite->setFlipY(true);
  sprite->setBlendFunc({GL_ONE, GL_ONE_MINUS_SRC_ALPHA});
  sprite->setOpacityModifyRGB(true);
  sprite->getTexture()->setAntiAliasTexParameters();
  sprite->setShaderProgram(shine->program);
  sprite->setAnchorPoint({0, 0});
  sprite->setVisible(false);
  return sprite;
}

void MSShine::setPass(float center, float strength)
{
  m_center = center;
  m_strength = strength;
  this->setVisible(strength > 0);
}

void MSShine::draw()
{
  auto shine = shader();
  if (!shine || m_strength <= 0)
    return;
  auto const& theme = Theme::get();
  auto program = shine->program.data();
  // This shine's pass, right before it draws: every item has its own
  program->use();
  auto texture = this->getTexture();
  program->setUniformLocationWith2f(shine->extent, std::max(texture->getMaxS(), 0.0001f),
                                    std::max(texture->getMaxT(), 0.0001f));
  program->setUniformLocationWith1f(shine->center, m_center);
  program->setUniformLocationWith1f(shine->width, theme.shineBandWidth);
  program->setUniformLocationWith1f(shine->strength, m_strength);
  program->setUniformLocationWith1f(shine->slant, theme.shineSlant);
  program->setUniformLocationWith3f(shine->tint, theme.shineTint.r / 255.f, theme.shineTint.g / 255.f,
                                    theme.shineTint.b / 255.f);
  program->setUniformLocationWith1f(shine->core, std::clamp(theme.shineCore, 0.f, 1.f));
  program->setUniformLocationWith3f(shine->coreColor, theme.shineCoreColor.r / 255.f,
                                    theme.shineCoreColor.g / 255.f, theme.shineCoreColor.b / 255.f);
  CCSprite::draw();
}
