#pragma once

#include "../ui/MSMenuItem.hpp"

#include <Geode/Geode.hpp>

#include <functional>

// The chosen level as MiSide's save card: a white-pink floppy disk with the level's picture in its pink frame (a
// real frame of the level, its proportions kept), and on its label the name, difficulty, stars, coins and the normal
// and practice progress, with PLAY. A new level's picture fades in over the last one
class MSLevelCard : public cocos2d::CCNode
{
public:
  static MSLevelCard* create(std::function<void()> play);

  void show(GJGameLevel* level);
  // Fades out (taking nothing) or in
  void setShown(bool shown);
  bool onKey(cocos2d::enumKeyCodes key);

protected:
  bool init(std::function<void()> play);
  void showPicture(GJGameLevel* level);
  void showInfo(GJGameLevel* level);
  // A label on the card: the title font, no shadow, anchored at its left middle with its capitals' top at capTop
  cocos2d::CCNode* text(std::string const& value, float scale, cocos2d::ccColor3B color, float x, float capTop);

  std::function<void()> m_play;
  cocos2d::CCRect m_window;
  cocos2d::CCNode* m_pictures = nullptr;
  cocos2d::CCNode* m_picture = nullptr;
  cocos2d::CCNode* m_info = nullptr;
  cocos2d::CCMenu* m_menu = nullptr;
  MSMenuItem* m_playButton = nullptr;
  int m_levelID = 0;
};
