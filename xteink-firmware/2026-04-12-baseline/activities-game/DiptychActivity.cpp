#include "DiptychActivity.h"

#include <Arduino.h>
#include <HalDisplay.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "components/UITheme.h"
#include "fontIds.h"

using Button = MappedInputManager::Button;

namespace {
constexpr int GRID = 15;
constexpr int TILE = 24;
constexpr int TOTAL_SHARDS = 5;
constexpr int SPLIT_STEPS = 5;
constexpr int VIEW = GRID * TILE;
constexpr int VIEW_X = (480 - VIEW) / 2;
constexpr int LIGHT_Y = 24;
constexpr int SHADOW_Y = 408;
constexpr int STATUS_Y = 772;
constexpr int DOOR_LO = 5;
constexpr int DOOR_HI = 9;
constexpr unsigned long EXIT_HOLD_MS = 1300;

struct ShardDef {
  int8_t rx;
  int8_t ry;
  int8_t lx;
  int8_t ly;
  int8_t sx;
  int8_t sy;
};

constexpr ShardDef SHARDS[] = {
    {0, -1, 10, 7, 11, 7},
    {-1, 0, 8, 7, 10, 7},
    {1, 1, 9, 5, 9, 3},
    {2, -1, 9, 7, 12, 7},
    {0, 2, 10, 7, 13, 7},
};

constexpr const char* PICKUP_LINES[] = {
    "1/5 - A quiet star wakes.",
    "2/5 - The dark learns its name.",
    "3/5 - Two roads remember one sky.",
    "4/5 - The rift begins to sing.",
    "5/5 - Return to the Watcher.",
};
}  // namespace

void DiptychActivity::onEnter() {
  Activity::onEnter();
  resetGame();
  requestUpdate();
}

bool DiptychActivity::preventAutoSleep() { return screen_ == Screen::Play; }

void DiptychActivity::resetGame() {
  screen_ = Screen::Intro;
  roomX_ = 0;
  roomY_ = 0;
  lightX_ = shadowX_ = 7;
  lightY_ = shadowY_ = 7;
  splitMode_ = SplitMode::None;
  splitSteps_ = 0;
  splitAnchorX_ = splitAnchorY_ = 7;
  shardsMask_ = 0;
  steps_ = 0;
  pendingVictory_ = false;
  backConsumed_ = false;
  needsFullRefresh_ = true;
  message_ = "Talk to the Watcher.";
  pickupMessage_.clear();
  dialogue_.clear();
  dialogueLine_ = 0;
  buildRoom(0, 0, light_, shadow_);
}

void DiptychActivity::clearWorld(World& world) const {
  for (int y = 0; y < GRID; ++y) {
    for (int x = 0; x < GRID; ++x) {
      world.tiles[y][x] = Tile::Empty;
    }
  }
  world.count = 0;
  for (auto& entity : world.entities) {
    entity = Entity{};
  }
}

void DiptychActivity::addEntity(World& world, EntityType type, int x, int y, uint8_t id, const char* text) const {
  if (world.count >= world.entities.size()) {
    return;
  }
  world.entities[world.count++] = Entity{type, static_cast<int8_t>(x), static_cast<int8_t>(y), id, text};
}

void DiptychActivity::addPerimeter(World& world) const {
  for (int i = 0; i < GRID; ++i) {
    if (i >= DOOR_LO && i <= DOOR_HI) {
      continue;
    }
    world.tiles[0][i] = Tile::Wall;
    world.tiles[GRID - 1][i] = Tile::Wall;
    world.tiles[i][0] = Tile::Wall;
    world.tiles[i][GRID - 1] = Tile::Wall;
  }
}

void DiptychActivity::addWalls(World& world, std::initializer_list<std::array<int, 2>> points) const {
  for (const auto& point : points) {
    const int x = point[0];
    const int y = point[1];
    if (inside(x, y)) {
      world.tiles[y][x] = Tile::Wall;
    }
  }
}

void DiptychActivity::buildRoom(int rx, int ry, World& light, World& shadow) const {
  clearWorld(light);
  clearWorld(shadow);

  if (rx == 0 && ry == 0) {
    addEntity(light, EntityType::Npc, 5, 7);
    light.tiles[2][2] = Tile::Tree;
    light.tiles[12][12] = Tile::Tree;
    shadow.tiles[3][3] = Tile::Tree;
    shadow.tiles[11][11] = Tile::Tree;
    return;
  }

  if (rx == 1 && ry == 0) {
    for (int y = 0; y < GRID; ++y) {
      light.tiles[y][9] = Tile::Wall;
    }
    addEntity(light, EntityType::Sign, 3, 7, 0, "Back splits Light for five steps.");
    return;
  }

  if (rx == 2 && ry == 0) {
    for (int y = 0; y < GRID; ++y) {
      shadow.tiles[y][9] = Tile::Wall;
    }
    addEntity(light, EntityType::Sign, 3, 7, 0, "Confirm splits Shadow unless someone speaks.");
    return;
  }

  if (rx == 4 && ry == 0) {
    addEntity(light, EntityType::Npc, 7, 6, 1);
    return;
  }

  for (uint8_t i = 0; i < TOTAL_SHARDS; ++i) {
    if (rx != SHARDS[i].rx || ry != SHARDS[i].ry) {
      continue;
    }

    switch (i) {
      case 0:
        addWalls(light, {{4, 4}, {5, 4}, {6, 4}, {4, 10}, {5, 10}, {6, 10}});
        addWalls(shadow, {{10, 7}, {4, 5}, {5, 5}, {6, 5}, {4, 9}, {5, 9}, {6, 9}});
        break;
      case 1:
        addWalls(light, {{10, 7}, {5, 5}, {6, 5}, {7, 5}, {11, 9}, {12, 9}});
        addWalls(shadow, {{8, 7}, {5, 9}, {6, 9}, {7, 9}, {11, 5}, {12, 5}});
        break;
      case 2:
        addWalls(light, {{9, 3}, {5, 8}, {6, 8}, {7, 8}, {11, 6}, {12, 6}});
        addWalls(shadow, {{9, 5}, {5, 6}, {6, 6}, {7, 6}, {11, 8}, {12, 8}});
        break;
      case 3:
        addWalls(light, {{12, 7}, {5, 5}, {6, 5}, {7, 5}, {8, 5}, {4, 10}, {5, 10}, {6, 10}});
        addWalls(shadow, {{9, 7}, {5, 9}, {6, 9}, {7, 9}, {8, 9}, {4, 4}, {5, 4}, {6, 4}});
        break;
      case 4:
        addWalls(light, {{13, 7}, {4, 5}, {5, 5}, {6, 5}, {7, 5}, {4, 10}, {5, 10}, {6, 10}, {7, 10}});
        addWalls(shadow, {{10, 7}, {4, 9}, {5, 9}, {6, 9}, {7, 9}, {4, 4}, {5, 4}, {6, 4}, {7, 4}});
        break;
    }

    if (!isCollected(i)) {
      addEntity(light, EntityType::HalfLight, SHARDS[i].lx, SHARDS[i].ly, i);
      addEntity(shadow, EntityType::HalfShadow, SHARDS[i].sx, SHARDS[i].sy, i);
    }
    return;
  }

  addPerimeter(light);
  addPerimeter(shadow);
}

void DiptychActivity::loadRoom(int rx, int ry, int entryX, int entryY) {
  roomX_ = rx;
  roomY_ = ry;
  lightX_ = shadowX_ = entryX;
  lightY_ = shadowY_ = entryY;
  buildRoom(roomX_, roomY_, light_, shadow_);
  needsFullRefresh_ = true;
}

bool DiptychActivity::isCollected(uint8_t shardId) const { return (shardsMask_ & (1U << shardId)) != 0; }

int DiptychActivity::shardCount() const {
  int count = 0;
  for (int i = 0; i < TOTAL_SHARDS; ++i) {
    if (shardsMask_ & (1U << i)) {
      ++count;
    }
  }
  return count;
}

bool DiptychActivity::isWalkable(Tile tile) const { return tile == Tile::Empty; }

bool DiptychActivity::inside(int x, int y) const { return x >= 0 && y >= 0 && x < GRID && y < GRID; }

int DiptychActivity::snapDoor(int v) const { return std::max(DOOR_LO, std::min(DOOR_HI, v)); }

int DiptychActivity::findEntityAt(const World& world, int x, int y, EntityType type) const {
  for (int i = 0; i < world.count; ++i) {
    const Entity& entity = world.entities[i];
    if (entity.x == x && entity.y == y && (type == EntityType::None || entity.type == type)) {
      return i;
    }
  }
  return -1;
}

bool DiptychActivity::hasBlockingEntity(const World& world, int x, int y) const {
  return findEntityAt(world, x, y, EntityType::Npc) >= 0;
}

void DiptychActivity::removeEntity(World& world, int index) {
  if (index < 0 || index >= world.count) {
    return;
  }
  for (int i = index; i + 1 < world.count; ++i) {
    world.entities[i] = world.entities[i + 1];
  }
  world.entities[--world.count] = Entity{};
}

bool DiptychActivity::tryWalkOrPush(World& world, EntityType halfType, int nx, int ny, int dx, int dy, bool execute) {
  if (!inside(nx, ny)) {
    return false;
  }

  const int halfIndex = findEntityAt(world, nx, ny, halfType);
  if (halfIndex >= 0) {
    const int px = nx + dx;
    const int py = ny + dy;
    if (!inside(px, py) || !isWalkable(world.tiles[py][px])) {
      return false;
    }
    const int blockingIndex = findEntityAt(world, px, py);
    if (blockingIndex >= 0 && blockingIndex != halfIndex) {
      return false;
    }
    if (execute) {
      world.entities[halfIndex].x = static_cast<int8_t>(px);
      world.entities[halfIndex].y = static_cast<int8_t>(py);
    }
    return true;
  }

  return isWalkable(world.tiles[ny][nx]) && !hasBlockingEntity(world, nx, ny);
}

void DiptychActivity::tryTransition(int dx, int dy) {
  int nextRoomX = roomX_;
  int nextRoomY = roomY_;
  int entryX = lightX_;
  int entryY = lightY_;

  if (dx > 0) {
    ++nextRoomX;
    entryX = 1;
    entryY = snapDoor(lightY_);
  } else if (dx < 0) {
    --nextRoomX;
    entryX = GRID - 2;
    entryY = snapDoor(lightY_);
  } else if (dy > 0) {
    ++nextRoomY;
    entryY = 1;
    entryX = snapDoor(lightX_);
  } else if (dy < 0) {
    --nextRoomY;
    entryY = GRID - 2;
    entryX = snapDoor(lightX_);
  }

  World nextLight;
  World nextShadow;
  buildRoom(nextRoomX, nextRoomY, nextLight, nextShadow);

  if (isWalkable(nextLight.tiles[entryY][entryX]) && isWalkable(nextShadow.tiles[entryY][entryX]) &&
      !hasBlockingEntity(nextLight, entryX, entryY) && !hasBlockingEntity(nextShadow, entryX, entryY)) {
    light_ = nextLight;
    shadow_ = nextShadow;
    roomX_ = nextRoomX;
    roomY_ = nextRoomY;
    lightX_ = shadowX_ = entryX;
    lightY_ = shadowY_ = entryY;
    needsFullRefresh_ = true;
    ++steps_;
    message_.clear();
  } else {
    message_ = "The path is blocked.";
  }
}

void DiptychActivity::collectAlignedShard(int shardId, int x, int y) {
  shardsMask_ |= (1U << shardId);
  lightX_ = shadowX_ = x;
  lightY_ = shadowY_ = y;

  removeEntity(light_, findEntityAt(light_, x, y, EntityType::HalfLight));
  removeEntity(shadow_, findEntityAt(shadow_, x, y, EntityType::HalfShadow));

  const int count = shardCount();
  pickupMessage_ = PICKUP_LINES[std::max(0, std::min(TOTAL_SHARDS - 1, count - 1))];
  screen_ = Screen::Pickup;
  ++steps_;
}

void DiptychActivity::updateAlignmentMessage() {
  int lx = -1;
  int ly = -1;
  int sx = -2;
  int sy = -2;
  for (int i = 0; i < light_.count; ++i) {
    if (light_.entities[i].type == EntityType::HalfLight) {
      lx = light_.entities[i].x;
      ly = light_.entities[i].y;
      break;
    }
  }
  for (int i = 0; i < shadow_.count; ++i) {
    if (shadow_.entities[i].type == EntityType::HalfShadow) {
      sx = shadow_.entities[i].x;
      sy = shadow_.entities[i].y;
      break;
    }
  }
  if (lx == sx && ly == sy) {
    message_ = "The halves resonate. Walk onto them.";
  } else {
    message_.clear();
  }
}

void DiptychActivity::moveCoupled(int dx, int dy) {
  const int nlx = lightX_ + dx;
  const int nly = lightY_ + dy;
  const int nsx = shadowX_ + dx;
  const int nsy = shadowY_ + dy;

  if (!inside(nlx, nly) || !inside(nsx, nsy)) {
    tryTransition(dx, dy);
    return;
  }

  int lightHalf = findEntityAt(light_, nlx, nly, EntityType::HalfLight);
  int shadowHalf = findEntityAt(shadow_, nsx, nsy, EntityType::HalfShadow);
  if (lightHalf >= 0 && shadowHalf >= 0 && light_.entities[lightHalf].x == shadow_.entities[shadowHalf].x &&
      light_.entities[lightHalf].y == shadow_.entities[shadowHalf].y &&
      light_.entities[lightHalf].id == shadow_.entities[shadowHalf].id) {
    collectAlignedShard(light_.entities[lightHalf].id, nlx, nly);
    return;
  }

  const bool lightOk = tryWalkOrPush(light_, EntityType::HalfLight, nlx, nly, dx, dy, false);
  const bool shadowOk = tryWalkOrPush(shadow_, EntityType::HalfShadow, nsx, nsy, dx, dy, false);

  if (!lightOk || !shadowOk) {
    if (!lightOk && !shadowOk) {
      message_ = "Blocked in both worlds.";
    } else if (!lightOk) {
      message_ = "Blocked above.";
    } else {
      message_ = "Blocked below.";
    }
    return;
  }

  tryWalkOrPush(light_, EntityType::HalfLight, nlx, nly, dx, dy, true);
  tryWalkOrPush(shadow_, EntityType::HalfShadow, nsx, nsy, dx, dy, true);
  lightX_ = nlx;
  lightY_ = nly;
  shadowX_ = nsx;
  shadowY_ = nsy;
  ++steps_;
  updateAlignmentMessage();
}

void DiptychActivity::moveSplit(int dx, int dy) {
  if (splitSteps_ <= 0) {
    snapSplit();
    return;
  }

  const bool soloLight = splitMode_ == SplitMode::Light;
  World& world = soloLight ? light_ : shadow_;
  int& px = soloLight ? lightX_ : shadowX_;
  int& py = soloLight ? lightY_ : shadowY_;
  const EntityType halfType = soloLight ? EntityType::HalfLight : EntityType::HalfShadow;
  const int nx = px + dx;
  const int ny = py + dy;

  if (!tryWalkOrPush(world, halfType, nx, ny, dx, dy, false)) {
    message_ = soloLight ? "Blocked above." : "Blocked below.";
    return;
  }

  tryWalkOrPush(world, halfType, nx, ny, dx, dy, true);
  px = nx;
  py = ny;
  --splitSteps_;
  ++steps_;

  updateAlignmentMessage();
  if (message_.empty()) {
    char buf[48];
    snprintf(buf, sizeof(buf), "%s split: %d steps.", soloLight ? "Light" : "Shadow", splitSteps_);
    message_ = buf;
  }
  if (splitSteps_ <= 0) {
    message_ = "Tether spent. Move again to snap back.";
  }
}

void DiptychActivity::snapSplit(const char* text) {
  if (splitMode_ == SplitMode::Light) {
    lightX_ = splitAnchorX_;
    lightY_ = splitAnchorY_;
  } else if (splitMode_ == SplitMode::Shadow) {
    shadowX_ = splitAnchorX_;
    shadowY_ = splitAnchorY_;
  }
  splitMode_ = SplitMode::None;
  splitSteps_ = 0;
  message_ = text;
}

void DiptychActivity::toggleLightSplit() {
  if (splitMode_ == SplitMode::Light) {
    snapSplit();
    return;
  }
  if (splitMode_ != SplitMode::None) {
    message_ = "Snap back first.";
    return;
  }
  splitMode_ = SplitMode::Light;
  splitSteps_ = SPLIT_STEPS;
  splitAnchorX_ = lightX_;
  splitAnchorY_ = lightY_;
  message_ = "Light split: 5 steps.";
}

void DiptychActivity::toggleShadowSplit() {
  if (splitMode_ == SplitMode::Shadow) {
    snapSplit();
    return;
  }
  if (splitMode_ != SplitMode::None) {
    message_ = "Snap back first.";
    return;
  }
  splitMode_ = SplitMode::Shadow;
  splitSteps_ = SPLIT_STEPS;
  splitAnchorX_ = shadowX_;
  splitAnchorY_ = shadowY_;
  message_ = "Shadow split: 5 steps.";
}

bool DiptychActivity::startAdjacentDialogueOrSign() {
  static constexpr int dirs[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
  for (const auto& dir : dirs) {
    const int ax = lightX_ + dir[0];
    const int ay = lightY_ + dir[1];
    if (!inside(ax, ay)) {
      continue;
    }
    const int index = findEntityAt(light_, ax, ay);
    if (index < 0) {
      continue;
    }
    const Entity& entity = light_.entities[index];
    if (entity.type == EntityType::Npc) {
      if (entity.id == 1) {
        openSageDialogue();
      } else {
        openWatcherDialogue();
      }
      return true;
    }
    if (entity.type == EntityType::Sign) {
      message_ = entity.text ? entity.text : "...";
      return true;
    }
  }
  return false;
}

void DiptychActivity::openWatcherDialogue() {
  dialogue_.clear();
  pendingVictory_ = false;
  const int count = shardCount();

  if (count >= TOTAL_SHARDS) {
    dialogue_.push_back("All five shards answer your step.");
    dialogue_.push_back("Come near. Let the seam remember how to close.");
    dialogue_.push_back("A scar is not a wound. It is proof that something endured.");
    pendingVictory_ = true;
  } else if (count >= 3) {
    dialogue_.push_back(std::to_string(count) + "/5 shards mended.");
    dialogue_.push_back("The dark no longer hunts the light. It walks beside it.");
    dialogue_.push_back("Do not fear the tether. It is proof you can return.");
    dialogue_.push_back("The broken stars still call from the map.");
  } else if (count >= 1) {
    dialogue_.push_back(std::to_string(count) + "/5 shards mended.");
    dialogue_.push_back("Each shard remembers a shape you have not yet become.");
    dialogue_.push_back("Split, push, return. Even the lost can be guided home.");
  } else {
    dialogue_.push_back("Little traveler, you cast two bodies and one will.");
    dialogue_.push_back("Light is not mercy. Shadow is not sin. Both are doors.");
    dialogue_.push_back("Each world holds half of what was broken.");
    dialogue_.push_back("Push the halves together. Step onto them as one.");
    dialogue_.push_back("Back loosens Light. Confirm loosens Shadow.");
  }

  dialogueLine_ = 0;
  screen_ = Screen::Dialogue;
}

void DiptychActivity::openSageDialogue() {
  dialogue_.clear();
  if (shardCount() >= TOTAL_SHARDS) {
    dialogue_.push_back("The last bell has rung.");
    dialogue_.push_back("Return to the Watcher. Let the silence become whole.");
  } else {
    dialogue_.push_back("A wall is only a question asked in stone.");
    dialogue_.push_back("Answer with patience. Push only what can be brought back.");
    dialogue_.push_back(std::to_string(shardCount()) + "/5 shards carry your name.");
  }
  dialogueLine_ = 0;
  pendingVictory_ = false;
  screen_ = Screen::Dialogue;
}

void DiptychActivity::advanceDialogue() {
  if (dialogueLine_ + 1 < dialogue_.size()) {
    ++dialogueLine_;
    return;
  }
  if (pendingVictory_) {
    screen_ = Screen::Victory;
    pendingVictory_ = false;
  } else {
    screen_ = Screen::Play;
  }
}

void DiptychActivity::loop() {
  if (mappedInput.wasPressed(Button::Back)) {
    backConsumed_ = false;
  }
  if (mappedInput.isPressed(Button::Back) && !backConsumed_ && mappedInput.getHeldTime() >= EXIT_HOLD_MS) {
    backConsumed_ = true;
    finish();
    return;
  }

  if (screen_ == Screen::Intro) {
    if (mappedInput.wasReleased(Button::Confirm)) {
      screen_ = Screen::Play;
      message_ = "Talk to the Watcher. Split, push, mend.";
      requestUpdate();
    }
    return;
  }

  if (screen_ == Screen::Pickup) {
    if (mappedInput.wasReleased(Button::Confirm) || mappedInput.wasReleased(Button::Back)) {
      screen_ = Screen::Play;
      requestUpdate();
    }
    return;
  }

  if (screen_ == Screen::Dialogue) {
    if (mappedInput.wasReleased(Button::Confirm)) {
      advanceDialogue();
      requestUpdate();
    } else if (mappedInput.wasReleased(Button::Back) && !backConsumed_) {
      screen_ = Screen::Play;
      pendingVictory_ = false;
      requestUpdate();
    }
    return;
  }

  if (screen_ == Screen::Victory) {
    if (mappedInput.wasReleased(Button::Confirm)) {
      screen_ = Screen::Play;
      message_ = "Free to explore.";
      requestUpdate();
    } else if (mappedInput.wasReleased(Button::Back) && !backConsumed_) {
      finish();
    }
    return;
  }

  bool updated = false;
  if (mappedInput.wasReleased(Button::Back) && !backConsumed_) {
    toggleLightSplit();
    updated = true;
  } else if (mappedInput.wasReleased(Button::Confirm)) {
    if (splitMode_ == SplitMode::None && startAdjacentDialogueOrSign()) {
      updated = true;
    } else {
      toggleShadowSplit();
      updated = true;
    }
  } else {
    int dx = 0;
    int dy = 0;
    if (mappedInput.wasPressed(Button::Up)) {
      dy = -1;
    } else if (mappedInput.wasPressed(Button::Down)) {
      dy = 1;
    } else if (mappedInput.wasPressed(Button::Left)) {
      dx = -1;
    } else if (mappedInput.wasPressed(Button::Right)) {
      dx = 1;
    }

    if (dx != 0 || dy != 0) {
      if (splitMode_ == SplitMode::None) {
        moveCoupled(dx, dy);
      } else {
        moveSplit(dx, dy);
      }
      updated = true;
    }
  }

  if (updated) {
    requestUpdate();
  }
}

void DiptychActivity::drawDiamond(int cx, int cy, int radius, bool filled, bool black) const {
  for (int dy = -radius; dy <= radius; ++dy) {
    const int w = radius - std::abs(dy);
    if (filled) {
      renderer.fillRect(cx - w, cy + dy, w * 2 + 1, 1, black);
    } else {
      renderer.drawPixel(cx - w, cy + dy, black);
      renderer.drawPixel(cx + w, cy + dy, black);
    }
  }
}

void DiptychActivity::drawShard(int tileX, int tileY, EntityType type, bool black) const {
  const int cx = tileX + TILE / 2;
  const int cy = tileY + TILE / 2;
  drawDiamond(cx, cy, 10, false, black);
  drawDiamond(cx, cy, 8, false, black);

  const int yStart = type == EntityType::HalfLight ? -8 : 0;
  const int yEnd = type == EntityType::HalfLight ? 0 : 8;
  for (int dy = yStart; dy <= yEnd; ++dy) {
    const int w = 8 - std::abs(dy);
    if (w > 0) {
      renderer.fillRect(cx - w, cy + dy, w * 2 + 1, 1, black);
    }
  }
  drawDiamond(cx, cy, 3, true, black);
}

void DiptychActivity::drawPlayer(int x, int y, bool black) const {
  renderer.fillRect(x + 9, y + 4, 7, 7, black);
  renderer.fillRect(x + 7, y + 11, 11, 8, black);
  renderer.fillRect(x + 6, y + 18, 5, 3, black);
  renderer.fillRect(x + 14, y + 18, 5, 3, black);
  renderer.drawLine(x + 4, y + 14, x + 2, y + 22, black);
}

void DiptychActivity::drawNpc(int x, int y, bool black) const {
  renderer.fillRect(x + 8, y + 3, 8, 7, black);
  renderer.drawPixel(x + 10, y + 6, !black);
  renderer.drawPixel(x + 13, y + 6, !black);
  renderer.fillRect(x + 6, y + 10, 13, 12, black);
  renderer.drawLine(x + 20, y + 9, x + 20, y + 23, black);
}

void DiptychActivity::drawWall(int x, int y, bool black) const {
  renderer.fillRect(x, y, TILE, TILE, black);
  renderer.drawLine(x, y + 7, x + TILE - 1, y + 7, !black);
  renderer.drawLine(x, y + 15, x + TILE - 1, y + 15, !black);
  renderer.drawLine(x + 7, y, x + 7, y + 7, !black);
  renderer.drawLine(x + 15, y + 8, x + 15, y + 15, !black);
  renderer.drawLine(x + 11, y + 16, x + 11, y + TILE - 1, !black);
}

void DiptychActivity::drawWorld(const World& world, int playerX, int playerY, int originY, bool shadowWorld,
                                bool frozen) const {
  const bool ink = !shadowWorld;
  renderer.fillRect(VIEW_X, originY, VIEW, VIEW, shadowWorld);
  renderer.drawRect(VIEW_X - 1, originY - 1, VIEW + 2, VIEW + 2, ink);

  for (int y = 0; y < GRID; ++y) {
    for (int x = 0; x < GRID; ++x) {
      const int px = VIEW_X + x * TILE;
      const int py = originY + y * TILE;
      if (world.tiles[y][x] == Tile::Wall) {
        drawWall(px, py, ink);
      } else if (world.tiles[y][x] == Tile::Tree) {
        drawDiamond(px + 12, py + 9, 9, true, ink);
        renderer.fillRect(px + 10, py + 15, 4, 8, ink);
      }
    }
  }

  for (int i = 0; i < world.count; ++i) {
    const Entity& entity = world.entities[i];
    const int px = VIEW_X + entity.x * TILE;
    const int py = originY + entity.y * TILE;
    if (entity.type == EntityType::Npc) {
      drawNpc(px, py, ink);
    } else if (entity.type == EntityType::Sign) {
      renderer.drawRect(px + 4, py + 4, 16, 10, ink);
      renderer.fillRect(px + 11, py + 14, 2, 8, ink);
    } else if (entity.type == EntityType::HalfLight || entity.type == EntityType::HalfShadow) {
      drawShard(px, py, entity.type, ink);
    }
  }

  const int ppx = VIEW_X + playerX * TILE;
  const int ppy = originY + playerY * TILE;
  drawPlayer(ppx, ppy, ink);

  if (frozen) {
    renderer.drawRect(ppx - 3, ppy - 3, TILE + 6, TILE + 6, ink);
  }
}

void DiptychActivity::drawSplitMeter(SplitMode mode, int originY, bool inverse) const {
  const bool active = splitMode_ == mode;
  const int remaining = active ? splitSteps_ : 0;
  const int x = VIEW_X - 27;
  const int y0 = originY + VIEW / 2 - 34;

  if (inverse) {
    renderer.fillRect(x - 9, y0 - 8, 18, 84, true);
    renderer.drawRect(x - 9, y0 - 8, 18, 84, true);
  }

  for (int i = 0; i < SPLIT_STEPS; ++i) {
    drawDiamond(x, y0 + i * 16, 5, i < remaining, !inverse);
  }
}

void DiptychActivity::drawShardMeter() const {
  const int startX = 480 / 2 - TOTAL_SHARDS * 8;
  for (int i = 0; i < TOTAL_SHARDS; ++i) {
    drawDiamond(startX + i * 16, 396, 5, (shardsMask_ & (1U << i)) != 0, true);
  }
}

void DiptychActivity::drawMinimap() const {
  constexpr int CELL = 7;
  constexpr int SIZE = 7;
  const int x0 = VIEW_X + VIEW + 8;
  const int y0 = LIGHT_Y + 8;
  renderer.drawText(SMALL_FONT_ID, x0 + 8, y0 - 8, "MAP", true, EpdFontFamily::BOLD);
  renderer.drawRect(x0 - 1, y0 - 1, SIZE * CELL + 2, SIZE * CELL + 2, true);

  const int half = SIZE / 2;
  for (int my = 0; my < SIZE; ++my) {
    for (int mx = 0; mx < SIZE; ++mx) {
      const int rx = roomX_ + mx - half;
      const int ry = roomY_ + my - half;
      const int px = x0 + mx * CELL;
      const int py = y0 + my * CELL;
      const bool current = mx == half && my == half;
      bool shardHere = false;
      for (int i = 0; i < TOTAL_SHARDS; ++i) {
        shardHere = shardHere || (SHARDS[i].rx == rx && SHARDS[i].ry == ry && !isCollected(i));
      }
      if (current) {
        renderer.fillRect(px + 1, py + 1, CELL - 2, CELL - 2, true);
      } else if (rx == 0 && ry == 0) {
        renderer.drawRect(px + 1, py + 1, CELL - 2, CELL - 2, true);
      } else if (shardHere) {
        drawDiamond(px + 3, py + 3, 3, true, true);
      }
    }
  }
}

void DiptychActivity::drawStatus() const {
  renderer.fillRect(0, STATUS_Y, renderer.getScreenWidth(), renderer.getScreenHeight() - STATUS_Y, false);
  renderer.drawLine(VIEW_X - 8, STATUS_Y - 4, VIEW_X + VIEW + 8, STATUS_Y - 4, true);

  char left[32];
  snprintf(left, sizeof(left), "Shards %d/%d", shardCount(), TOTAL_SHARDS);
  renderer.drawText(SMALL_FONT_ID, 8, STATUS_Y + 4, left, true, EpdFontFamily::BOLD);

  std::string center;
  if (splitMode_ != SplitMode::None) {
    center = splitMode_ == SplitMode::Light ? "Light split" : "Shadow split";
    center += ": " + std::to_string(splitSteps_);
  } else if (!message_.empty()) {
    center = message_;
  } else {
    center = "D-pad | Back=Light | OK=Shadow";
  }
  center = renderer.truncatedText(SMALL_FONT_ID, center.c_str(), 260);
  renderer.drawCenteredText(SMALL_FONT_ID, STATUS_Y + 4, center.c_str(), true);

  char right[32];
  snprintf(right, sizeof(right), "(%d,%d)", roomX_, roomY_);
  const int rw = renderer.getTextWidth(SMALL_FONT_ID, right);
  renderer.drawText(SMALL_FONT_ID, 472 - rw, STATUS_Y + 4, right, true);
}

void DiptychActivity::drawDialogue() const {
  const int x = 32;
  const int y = 280;
  const int w = 416;
  const int h = 220;
  renderer.fillRect(x, y, w, h, false);
  renderer.drawRect(x, y, w, h, true);
  renderer.drawRect(x + 3, y + 3, w - 6, h - 6, true);
  renderer.drawText(UI_12_FONT_ID, x + 18, y + 20, "...", true, EpdFontFamily::BOLD);
  renderer.drawLine(x + 12, y + 42, x + w - 12, y + 42, true);

  if (!dialogue_.empty()) {
    const auto lines = renderer.wrappedText(UI_12_FONT_ID, dialogue_[dialogueLine_].c_str(), w - 40, 5);
    for (size_t i = 0; i < lines.size(); ++i) {
      renderer.drawText(UI_12_FONT_ID, x + 20, y + 62 + static_cast<int>(i) * 26, lines[i].c_str(), true);
    }
  }

  char idx[24];
  snprintf(idx, sizeof(idx), "%u/%u", static_cast<unsigned>(dialogueLine_ + 1), static_cast<unsigned>(dialogue_.size()));
  renderer.drawText(SMALL_FONT_ID, x + w - 76, y + h - 22, idx, true);
  renderer.drawText(SMALL_FONT_ID, x + w - 48, y + h - 22, "OK", true, EpdFontFamily::BOLD);
}

void DiptychActivity::drawPickup() const {
  const int x = 40;
  const int y = 310;
  const int w = 400;
  const int h = 150;
  renderer.fillRect(x, y, w, h, true);
  renderer.drawRect(x + 3, y + 3, w - 6, h - 6, false);
  drawShard(480 / 2 - 12, y + 14, EntityType::HalfLight, false);
  drawShard(480 / 2 - 12, y + 14, EntityType::HalfShadow, false);
  renderer.drawCenteredText(UI_12_FONT_ID, y + 62, "SHARD MENDED", false, EpdFontFamily::BOLD);
  renderer.drawCenteredText(UI_10_FONT_ID, y + 92, pickupMessage_.c_str(), false);
  renderer.drawCenteredText(SMALL_FONT_ID, y + 122, "Confirm", false, EpdFontFamily::BOLD);
}

void DiptychActivity::drawIntro() const {
  renderer.clearScreen();
  renderer.drawCenteredText(UI_12_FONT_ID, 230, "DIPTYCH", true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(UI_10_FONT_ID, 270, "Two canvases. One truth.", true);
  drawDiamond(240, 342, 24, false, true);
  drawDiamond(240, 342, 15, true, true);
  renderer.drawCenteredText(UI_10_FONT_ID, 430, "Confirm to begin", true, EpdFontFamily::BOLD);
}

void DiptychActivity::drawVictory() const {
  renderer.clearScreen();
  renderer.fillRect(0, 0, renderer.getScreenWidth(), renderer.getScreenHeight(), true);
  renderer.drawCenteredText(UI_12_FONT_ID, 220, "The rift closes.", false, EpdFontFamily::BOLD);
  renderer.drawCenteredText(UI_10_FONT_ID, 280, "Five shards. Mended.", false);
  renderer.drawCenteredText(UI_10_FONT_ID, 318, "The worlds become one.", false);
  drawPlayer(214, 386, false);
  drawPlayer(242, 386, false);
  char buf[48];
  snprintf(buf, sizeof(buf), "%lu steps", static_cast<unsigned long>(steps_));
  renderer.drawCenteredText(SMALL_FONT_ID, 500, buf, false);
  renderer.drawCenteredText(UI_10_FONT_ID, 600, "Confirm to explore", false, EpdFontFamily::BOLD);
}

void DiptychActivity::drawPlay() const {
  renderer.clearScreen();
  renderer.drawCenteredText(SMALL_FONT_ID, 6, "DIPTYCH", true, EpdFontFamily::BOLD);
  renderer.drawCenteredText(SMALL_FONT_ID, 18, "~ Light World ~", true, EpdFontFamily::BOLD);
  drawWorld(light_, lightX_, lightY_, LIGHT_Y, false, splitMode_ == SplitMode::Shadow);
  drawSplitMeter(SplitMode::Light, LIGHT_Y, false);
  drawMinimap();

  renderer.drawLine(VIEW_X - 8, 388, VIEW_X + VIEW + 8, 388, true);
  drawShardMeter();
  renderer.drawCenteredText(SMALL_FONT_ID, 397, "~ Shadow World ~", true, EpdFontFamily::BOLD);
  renderer.drawLine(VIEW_X - 8, 407, VIEW_X + VIEW + 8, 407, true);
  drawWorld(shadow_, shadowX_, shadowY_, SHADOW_Y, true, splitMode_ == SplitMode::Light);
  drawSplitMeter(SplitMode::Shadow, SHADOW_Y, true);

  drawStatus();
}

void DiptychActivity::render(RenderLock&&) {
  if (screen_ == Screen::Intro) {
    drawIntro();
    renderer.displayBuffer(needsFullRefresh_ ? HalDisplay::FULL_REFRESH : HalDisplay::FAST_REFRESH);
    needsFullRefresh_ = false;
    return;
  }
  if (screen_ == Screen::Victory) {
    drawVictory();
    renderer.displayBuffer(needsFullRefresh_ ? HalDisplay::FULL_REFRESH : HalDisplay::FAST_REFRESH);
    needsFullRefresh_ = false;
    return;
  }

  drawPlay();
  if (screen_ == Screen::Dialogue) {
    drawDialogue();
  } else if (screen_ == Screen::Pickup) {
    drawPickup();
  }
  renderer.displayBuffer(needsFullRefresh_ ? HalDisplay::FULL_REFRESH : HalDisplay::FAST_REFRESH);
  needsFullRefresh_ = false;
}
