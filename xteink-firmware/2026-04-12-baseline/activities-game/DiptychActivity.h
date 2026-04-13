#pragma once

#include <array>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

#include "../Activity.h"

class DiptychActivity final : public Activity {
 public:
  explicit DiptychActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Diptych", renderer, mappedInput) {}

  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;
  bool preventAutoSleep() override;

 private:
  static constexpr int GRID = 15;
  static constexpr int TILE = 24;
  static constexpr int TOTAL_SHARDS = 5;
  static constexpr int SPLIT_STEPS = 5;
  static constexpr int MAX_ENTITIES = 4;

  enum class Screen : uint8_t { Intro, Play, Dialogue, Pickup, Victory };
  enum class Tile : uint8_t { Empty, Wall, Tree };
  enum class EntityType : uint8_t { None, Npc, Sign, HalfLight, HalfShadow };
  enum class SplitMode : uint8_t { None, Light, Shadow };

  struct Entity {
    EntityType type = EntityType::None;
    int8_t x = 0;
    int8_t y = 0;
    uint8_t id = 0;
    const char* text = nullptr;
  };

  struct World {
    Tile tiles[GRID][GRID] = {};
    std::array<Entity, MAX_ENTITIES> entities{};
    uint8_t count = 0;
  };

  Screen screen_ = Screen::Intro;
  World light_{};
  World shadow_{};

  int roomX_ = 0;
  int roomY_ = 0;
  int lightX_ = 7;
  int lightY_ = 7;
  int shadowX_ = 7;
  int shadowY_ = 7;

  SplitMode splitMode_ = SplitMode::None;
  int splitSteps_ = 0;
  int splitAnchorX_ = 7;
  int splitAnchorY_ = 7;

  uint8_t shardsMask_ = 0;
  uint32_t steps_ = 0;
  bool pendingVictory_ = false;
  bool backConsumed_ = false;
  bool needsFullRefresh_ = true;

  std::string message_;
  std::string pickupMessage_;
  std::vector<std::string> dialogue_;
  size_t dialogueLine_ = 0;

  void resetGame();
  void buildRoom(int rx, int ry, World& light, World& shadow) const;
  void loadRoom(int rx, int ry, int entryX, int entryY);

  void clearWorld(World& world) const;
  void addEntity(World& world, EntityType type, int x, int y, uint8_t id = 0, const char* text = nullptr) const;
  void addPerimeter(World& world) const;
  void addWalls(World& world, std::initializer_list<std::array<int, 2>> points) const;

  bool isCollected(uint8_t shardId) const;
  int shardCount() const;
  bool isWalkable(Tile tile) const;
  bool inside(int x, int y) const;
  int snapDoor(int v) const;
  int findEntityAt(const World& world, int x, int y, EntityType type = EntityType::None) const;
  bool hasBlockingEntity(const World& world, int x, int y) const;
  void removeEntity(World& world, int index);

  bool tryWalkOrPush(World& world, EntityType halfType, int nx, int ny, int dx, int dy, bool execute);
  void moveCoupled(int dx, int dy);
  void moveSplit(int dx, int dy);
  void tryTransition(int dx, int dy);
  void snapSplit(const char* text = "The tether snaps back.");
  void toggleLightSplit();
  void toggleShadowSplit();
  void collectAlignedShard(int shardId, int x, int y);
  void updateAlignmentMessage();

  bool startAdjacentDialogueOrSign();
  void openWatcherDialogue();
  void openSageDialogue();
  void advanceDialogue();

  void drawIntro() const;
  void drawVictory() const;
  void drawPlay() const;
  void drawWorld(const World& world, int playerX, int playerY, int originY, bool shadowWorld, bool frozen) const;
  void drawStatus() const;
  void drawMinimap() const;
  void drawShardMeter() const;
  void drawSplitMeter(SplitMode mode, int originY, bool inverse) const;
  void drawDialogue() const;
  void drawPickup() const;

  void drawDiamond(int cx, int cy, int radius, bool filled, bool black) const;
  void drawShard(int tileX, int tileY, EntityType type, bool black) const;
  void drawPlayer(int x, int y, bool black) const;
  void drawNpc(int x, int y, bool black) const;
  void drawWall(int x, int y, bool black) const;
};
