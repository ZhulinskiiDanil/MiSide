#include "MSOptionsData.hpp"

#include <Geode/modify/GJOptionsLayer.hpp>
#include <Geode/modify/KeybindingsLayer.hpp>
#include <Geode/modify/MoreOptionsLayer.hpp>
#include <Geode/modify/MoreVideoOptionsLayer.hpp>
#include <Geode/modify/ParentalOptionsLayer.hpp>
#include <Geode/modify/TextArea.hpp>

#include "MSAlerts.hpp"

#include <cstring>

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

// A text area keeps only its lines: the whole text it was given, kept
class $modify(MSTextAreaData, TextArea)
{
  struct Fields
  {
    std::string m_text;
  };

  void setString(gd::string text)
  {
    m_fields->m_text = std::string(text);
    TextArea::setString(text);
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

  bool runs(CCMenuItem* item, uintptr_t offset)
  {
    static_assert(sizeof(SEL_MenuHandler) == sizeof(uintptr_t), "GD's callbacks are plain function pointers");
    if (!item || !item->m_pfnSelector)
      return false;
    uintptr_t address = 0;
    std::memcpy(&address, &item->m_pfnSelector, sizeof(address));
    return address == geode::base::get() + offset;
  }

  namespace
  {
    void collectLabels(CCNode* node, std::vector<CCLabelBMFont*>& out)
    {
      if (!node || !node->isVisible())
        return;
      if (auto label = typeinfo_cast<CCLabelBMFont*>(node))
      {
        out.push_back(label);
        return;
      }
      for (auto child : CCArrayExt<CCNode*>(node->getChildren()))
        collectLabels(child, out);
    }
  } // namespace

  std::string textOf(CCNode* node)
  {
    if (!node)
      return "";
    if (auto label = typeinfo_cast<CCLabelBMFont*>(node))
      return plain(label->getString());
    if (auto area = typeinfo_cast<TextArea*>(node))
    {
      auto const& text = static_cast<MSTextAreaData*>(area)->m_fields->m_text;
      if (!text.empty())
        return plain(text);
    }
    std::vector<CCLabelBMFont*> labels;
    collectLabels(node, labels);
    std::stable_sort(labels.begin(), labels.end(), [](auto a, auto b) {
      return a->convertToWorldSpace(CCPointZero).y > b->convertToWorldSpace(CCPointZero).y;
    });
    std::string out;
    for (auto label : labels)
    {
      std::string line = label->getString();
      if (line.empty())
        continue;
      if (!out.empty())
        out += " ";
      out += line;
    }
    return plain(out);
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
