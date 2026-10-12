#include "MSOptionsData.hpp"

#include <Geode/modify/GJOptionsLayer.hpp>
#include <Geode/modify/KeybindingsLayer.hpp>
#include <Geode/modify/MoreOptionsLayer.hpp>
#include <Geode/modify/MoreVideoOptionsLayer.hpp>
#include <Geode/modify/ParentalOptionsLayer.hpp>

#include <unordered_set>

using namespace geode::prelude;

namespace
{
  void collect(CCNode* node, std::vector<CCMenuItemToggler*>& out)
  {
    if (auto toggler = typeinfo_cast<CCMenuItemToggler*>(node))
    {
      out.push_back(toggler);
      // Its own on and off buttons are no toggles
      return;
    }
    for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
      collect(child, out);
  }

  // The toggle button addToggle just made: the one in the layer that no toggle has yet
  CCMenuItemToggler* newToggler(CCNode* layer, std::vector<ms::options::Toggle> const& known)
  {
    std::unordered_set<CCMenuItemToggler*> taken;
    for (auto const& toggle : known)
    {
      if (toggle.toggler)
        taken.insert(toggle.toggler);
    }
    std::vector<CCMenuItemToggler*> all;
    collect(layer, all);
    for (auto toggler : all)
    {
      if (!taken.contains(toggler))
        return toggler;
    }
    return nullptr;
  }

  // Every toggle from addToggle, with its button found right after it's made
  void record(CCNode* layer, std::vector<ms::options::Toggle>& toggles, char const* label, std::string key,
              char const* info, int page)
  {
    ms::options::Toggle toggle;
    toggle.label = label ? label : "";
    toggle.key = std::move(key);
    toggle.info = info ? info : "";
    toggle.page = page;
    toggle.toggler = newToggler(layer, toggles);
    if (!toggle.toggler)
      log::warn("No toggle button found for the option {} ({})", ms::options::oneLine(toggle.label), toggle.key);
    toggles.push_back(std::move(toggle));
  }

  std::vector<ms::options::Toggle> const& none()
  {
    static std::vector<ms::options::Toggle> const NONE;
    return NONE;
  }
} // namespace

class $modify(MSMoreOptionsData, MoreOptionsLayer)
{
  struct Fields
  {
    std::vector<ms::options::Toggle> m_toggles;
  };

  void addToggle(char const* label, char const* key, char const* description)
  {
    // GD's own page for it: ten a page, counted before it's added
    auto page = m_toggleCount / 10;
    MoreOptionsLayer::addToggle(label, key, description);
    record(this, m_fields->m_toggles, label, key ? key : "", description, page);
  }
};

class $modify(MSMoreVideoOptionsData, MoreVideoOptionsLayer)
{
  struct Fields
  {
    std::vector<ms::options::Toggle> m_toggles;
  };

  void addToggle(char const* label, char const* key, char const* description)
  {
    auto page = m_toggleCount / 10;
    MoreVideoOptionsLayer::addToggle(label, key, description);
    record(this, m_fields->m_toggles, label, key ? key : "", description, page);
  }
};

class $modify(MSParentalOptionsData, ParentalOptionsLayer)
{
  struct Fields
  {
    std::vector<ms::options::Toggle> m_toggles;
  };

  void addToggle(char const* label, char const* variable, char const* info)
  {
    auto page = m_toggleCount / 10;
    ParentalOptionsLayer::addToggle(label, variable, info);
    record(this, m_fields->m_toggles, label, variable ? variable : "", info, page);
  }
};

class $modify(MSGJOptionsData, GJOptionsLayer)
{
  struct Fields
  {
    std::vector<ms::options::Toggle> m_toggles;
  };

  void addToggleInternal(char const* title, int id, bool initial, char const* description)
  {
    auto page = m_togglesPerPage > 0 ? m_toggleCount / m_togglesPerPage : 0;
    GJOptionsLayer::addToggleInternal(title, id, initial, description);
    record(this, m_fields->m_toggles, title, std::to_string(id), description, page);
  }
};

class $modify(MSKeybindingsData, KeybindingsLayer)
{
  struct Fields
  {
    std::vector<ms::options::KeyPair> m_pairs;
  };

  CCLabelBMFont* addKeyPair(char const* action, char const* key)
  {
    m_fields->m_pairs.push_back({action ? action : "", key ? key : ""});
    return KeybindingsLayer::addKeyPair(action, key);
  }
};

namespace ms::options
{
  std::vector<Toggle> const& toggles(MoreOptionsLayer* layer)
  {
    return layer ? static_cast<MSMoreOptionsData*>(layer)->m_fields->m_toggles : none();
  }

  std::vector<Toggle> const& toggles(MoreVideoOptionsLayer* layer)
  {
    return layer ? static_cast<MSMoreVideoOptionsData*>(layer)->m_fields->m_toggles : none();
  }

  std::vector<Toggle> const& toggles(ParentalOptionsLayer* layer)
  {
    return layer ? static_cast<MSParentalOptionsData*>(layer)->m_fields->m_toggles : none();
  }

  std::vector<Toggle> const& toggles(GJOptionsLayer* layer)
  {
    return layer ? static_cast<MSGJOptionsData*>(layer)->m_fields->m_toggles : none();
  }

  std::vector<KeyPair> const& keyPairs(KeybindingsLayer* layer)
  {
    static std::vector<KeyPair> const NONE;
    return layer ? static_cast<MSKeybindingsData*>(layer)->m_fields->m_pairs : NONE;
  }

  std::vector<CCMenuItemToggler*> togglers(CCNode* layer)
  {
    std::vector<CCMenuItemToggler*> all;
    if (layer)
      collect(layer, all);
    return all;
  }

  void dispose(CCNode* layer)
  {
    // The paused actions a click's look left (the button's bounce) hold their nodes: stopped, so it all goes
    if (layer)
      layer->cleanup();
  }

  std::string oneLine(std::string const& text)
  {
    std::string out;
    out.reserve(text.size());
    for (auto c : text)
    {
      if (c == '\n')
      {
        if (!out.empty() && out.back() != ' ')
          out.push_back(' ');
      }
      else
        out.push_back(c);
    }
    return out;
  }
} // namespace ms::options
