// Tirapiedras (Top-down) — C++ port using raylib
// Ported from HTML/CSS/JS single file version

#include "raylib.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <string>
#include <cstdint>

struct V2 { float x{0}, y{0}; };

static inline V2 V(float x=0, float y=0){ return V2{x,y}; }
static inline V2 add(V2 a, V2 b){ return V(a.x+b.x, a.y+b.y); }
static inline V2 sub(V2 a, V2 b){ return V(a.x-b.x, a.y-b.y); }
static inline V2 mul(V2 a, float k){ return V(a.x*k, a.y*k); }
static inline float len(V2 a){ return std::hypot(a.x, a.y); }
static inline V2 norm(V2 a){ float L = len(a); return (L>1e-6f) ? V(a.x/L, a.y/L) : V(0,0); }

struct Rock { V2 pos; float radius; bool picked=false; };
struct Projectile { V2 pos; V2 vel; float radius; bool alive=true; };
struct Enemy { V2 pos; float radius; float speed; int hp; int maxHp; bool alive=true; bool boss=false; };

struct Player {
  V2 pos{0,0}; float radius=14.f; float speed=160.f; V2 target{0,0}; bool hasTarget=false;
  std::vector<float> inventory; int capacity=3; int maxHp=100; int hp=100; int lives=3; float iTimer=0.f; float lastHitAt= -10.f;
};

// Deterministic RNG similar to JS LCG
static uint32_t g_seed = 42u;
static inline float rng(){ g_seed = (g_seed*1664525u + 1013904223u); return (g_seed >> 0) / 4294967296.0f; }
static inline float frand(float a, float b){ return a + (b-a) * rng(); }

// Weighted pick utility
static int weightedPick(const std::vector<int>& w){
  int sum=0; for(int x: w) sum+=x; float t = frand(0.0f, (float)sum);
  for (int i=0;i<(int)w.size();++i){ t -= (float)w[i]; if (t <= 0) return i; }
  return (int)w.size()-1;
}

// Draw helpers
static void RoundRect(float x,float y,float w,float h,float r, Color cFill, Color cStroke){
  // Approx rounded rect: base filled rect + 4 quarter circles + stroke
  float rr = std::min({r, w*0.5f, h*0.5f});
  DrawRectangleV({x+rr,y}, {(float)(w-2*rr), h}, cFill);
  DrawRectangleV({x,y+rr}, {rr, (float)(h-2*rr)}, cFill);
  DrawRectangleV({x+w-rr,y+rr}, {rr, (float)(h-2*rr)}, cFill);
  DrawCircleV({x+rr, y+rr}, rr, cFill);
  DrawCircleV({x+w-rr, y+rr}, rr, cFill);
  DrawCircleV({x+rr, y+h-rr}, rr, cFill);
  DrawCircleV({x+w-rr, y+h-rr}, rr, cFill);
  // Stroke
  if (cStroke.a){
    DrawRectangleLinesEx({x,y,w,h}, 1.0f, cStroke);
  }
}

static void DrawHeart(float x, float y, float s, Color c){
  // Simple heart using 2 circles and a triangle-like bottom
  // This is an approximation; good enough for HUD lives
  Vector2 c1 = {x - s*0.5f, y - s*0.2f};
  Vector2 c2 = {x + s*0.5f, y - s*0.2f};
  DrawCircleV(c1, s*0.5f, c);
  DrawCircleV(c2, s*0.5f, c);
  Vector2 p1 = {x - s, y - s*0.2f};
  Vector2 p2 = {x + s, y - s*0.2f};
  Vector2 p3 = {x, y + s};
  DrawTriangle(p1, p2, p3, c);
  DrawTriangleLines(p1, p2, p3, Fade(BLACK, 0.5f));
}

int main(){
  const int W = 960, H = 540;
  SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
  InitWindow(W, H, "Tirapiedras C++ (raylib)");
  SetTargetFPS(60);

  // World & entities
  std::vector<Rock> rocks; rocks.reserve(256);
  std::vector<Projectile> projectiles; projectiles.reserve(256);
  std::vector<Enemy> enemies; enemies.reserve(256);

  int score = 0; bool gameOver = false;
  int level = 1; int cycle = 0; int enemiesToSpawn = 0; float spawnTimer = 0.f; float spawnInterval = 2.8f; float levelBannerTimer = 2.0f;

  const int rockSizes[3] = {6,10,16};
  const int rockWeights[3] = {5,3,2};
  const int initialRocks = 70;
  float rockSpawnTimer = 0.f; const float rockSpawnInterval = 2.0f;

  auto damageMultiplier = [&](){ return (int)std::pow(2.0f, (float)cycle); };
  auto speedMultiplier = [&](){ return std::pow(1.25f, (float)cycle); };

  Player player; player.pos = {W*0.5f, H*0.5f}; player.target = player.pos;

  auto resetGame = [&](){
    projectiles.clear(); enemies.clear(); score=0; gameOver=false;
    player.pos = {W*0.5f, H*0.5f}; player.target = player.pos; player.hasTarget=false; player.inventory.clear(); player.hp=player.maxHp; player.lives=3; player.iTimer=0; player.lastHitAt = -10.f;
    cycle=0; level=1; enemiesToSpawn=0; spawnTimer=0; spawnInterval=2.8f; levelBannerTimer=2.0f;
  };

  auto startLevel = [&](int n){
    level = n; spawnTimer = 0.f; levelBannerTimer = 2.0f;
    if (level>=1 && level<=4) enemiesToSpawn = (int)std::pow(2, level);
    else if (level==5) enemiesToSpawn = 1;
  };

  auto spawnRock = [&](){
    int idx = weightedPick(std::vector<int>{rockWeights[0],rockWeights[1],rockWeights[2]});
    float radius = (float)rockSizes[idx];
    rocks.push_back(Rock{ V(frand(40.0f, (float)(W-40)), frand(40.0f, (float)(H-40))), radius, false });
  };

  for (int i=0;i<initialRocks;i++) spawnRock();

  auto spawnEnemy = [&](){
    if (level==5){
      float radius = 80.f; float x = frand(80.0f, (float)(W-80)); float y = -radius - 20.f; int maxHp = 400; float speed = 30.f * speedMultiplier();
      enemies.push_back(Enemy{ V(x,y), radius, speed, maxHp, maxHp, true, true });
      return;
    }
    float radius = frand(12.f, 20.f); float x = frand(30.0f, (float)(W-30)); float y = -radius - 14.f; int maxHp = (int)std::round(radius*4.f);
    float t = (radius - 12.f) / (20.f - 12.f); // 0..1
    float speed = (85.f + (45.f - 85.f) * std::clamp(t, 0.f, 1.f)) * speedMultiplier();
    enemies.push_back(Enemy{ V(x,y), radius, speed, maxHp, maxHp, true, false });
  };

  auto near = [&](const Player& pl, const Rock& r, float margin=4.f){
    return std::hypot(pl.pos.x - r.pos.x, pl.pos.y - r.pos.y) <= (pl.radius + r.radius + margin);
  };

  auto throwRockToward = [&](V2 target){
    if (gameOver) return;
    if (player.inventory.empty()) return;
    // pick largest radius
    int idx = 0; for (int i=1;i<(int)player.inventory.size();++i) if (player.inventory[i] > player.inventory[idx]) idx = i;
    float r = player.inventory[idx]; player.inventory.erase(player.inventory.begin()+idx);
    V2 dir = norm(sub(target, player.pos));
    float mass = r*r; float base = 420.f; float speed = base / std::sqrt(std::max(1.f, mass*0.02f));
    V2 spawn = add(player.pos, mul(dir, player.radius + r + 2.f));
    projectiles.push_back(Projectile{spawn, mul(dir, speed), r, true});
  };

  auto drawGrid = [&](){
    ClearBackground(Color{11,18,32,255});
    Color grid = Color{255,255,255,10};
    int step = 40;
    for (int x=0;x<=W;x+=step) DrawLine(x,0,x,H,grid);
    for (int y=0;y<=H;y+=step) DrawLine(0,y,W,y,grid);
  };

  auto drawRock = [&](const Rock& r){
    DrawCircleV({r.pos.x, r.pos.y}, r.radius, Color{139,143,151,255});
    DrawCircleLines((int)r.pos.x, (int)r.pos.y, r.radius, Color{0,0,0,220});
  };

  auto drawProjectile = [&](const Projectile& p){
    DrawCircleV({p.pos.x, p.pos.y}, p.radius, Color{107,114,128,255});
    DrawCircleLines((int)p.pos.x, (int)p.pos.y, p.radius, Color{0,0,0,220});
  };

  auto drawEnemy = [&](const Enemy& e){
    DrawCircleV({e.pos.x, e.pos.y}, e.radius, Color{245,158,11,255});
    DrawCircleLines((int)e.pos.x, (int)e.pos.y, e.radius, Color{0,0,0,230});
    // health bar
    float barW = std::max(24.f, e.radius*2.f), barH=5.f; float x = e.pos.x - barW/2.f; float y = e.pos.y - e.radius - 10.f;
    RoundRect(x-1,y-1,barW+2,barH+2,3, Color{0,0,0,128}, BLANK);
    RoundRect(x,y,barW,barH,3, Color{127,29,29,255}, BLANK);
    float ratio = std::max(0.f, (float)e.hp / (float)e.maxHp);
    RoundRect(x,y,barW*ratio,barH,3, Color{34,197,94,255}, BLANK);
  };

  auto drawPlayer = [&](const Player& pl){
    // flicker when invulnerable
    Color body1 = Color{139,212,255,255};
    Color body2 = Color{56,189,248,255};
    float alpha = 1.0f;
    if (pl.iTimer > 0.f){ if (((int)(GetTime()*10))%2==0) alpha = 0.5f; }
    Color c1 = Fade(body1, alpha); Color c2 = Fade(body2, alpha);
    // shadow
    DrawCircleV({pl.pos.x+2, pl.pos.y+3}, pl.radius, Color{0,0,0,90});
    // gradient approximation: inner lighter circle and outer darker ring
    DrawCircleV({pl.pos.x, pl.pos.y}, pl.radius*0.75f, c1);
    DrawCircleV({pl.pos.x, pl.pos.y}, pl.radius, c2);
    DrawCircleLines((int)pl.pos.x, (int)pl.pos.y, pl.radius, Color{0,0,0,230});
    // target line
    if (player.hasTarget){
      DrawLineEx({pl.pos.x, pl.pos.y}, {player.target.x, player.target.y}, 2.0f, Color{56,189,248,160});
      DrawCircleLines((int)player.target.x, (int)player.target.y, 8.f, Color{56,189,248,160});
    }
  };

  auto drawInventory = [&](){
    float x0=12, y0=12; RoundRect(x0-6,y0-6,170,50,10, Color{17,24,39,230}, Color{255,255,255,20});
    float x=x0;
    for (int i=0;i<player.capacity;i++){
      float r = (i < (int)player.inventory.size()) ? player.inventory[i] : 0.f;
      if (r>0){ DrawCircleV({x+r, y0+r}, r, Color{156,163,175,255}); DrawCircleLines((int)(x+r), (int)(y0+r), r, Color{0,0,0,220}); x += r*2 + 6; }
      else { float er=10; DrawCircleLines((int)(x+er), (int)(y0+er), er, Color{255,255,255,50}); x += er*2 + 6; }
    }
    DrawText("Inventario", (int)x0, (int)(y0+38), 12, Color{226,232,240,190});
  };

  auto drawScore = [&](){
    std::string text = "Puntaje: "+std::to_string(score);
    int pad=8; int w = MeasureText(text.c_str(), 14) + pad*2; int h=26; int x = (W - w - 12), y = 12;
    RoundRect((float)x,(float)y,(float)w,(float)h,10, Color{17,24,39,230}, Color{255,255,255,20});
    DrawText(text.c_str(), x+pad, y+5, 14, Color{226,232,240,230});
  };

  auto drawPlayerHP = [&](){
    float x=12, y=70, w=220, h=16; RoundRect(x-6,y-6,w+12,h+12,10, Color{17,24,39,230}, Color{255,255,255,20});
    RoundRect(x,y,w,h,8, Color{127,29,29,255}, BLANK);
    float ratio = std::max(0.f, (float)player.hp/(float)player.maxHp);
    RoundRect(x,y,w*ratio,h,8, Color{34,197,94,255}, BLANK);
    std::string t = "HP: "+std::to_string(player.hp)+"/"+std::to_string(player.maxHp);
    DrawText(t.c_str(), (int)x+8, (int)y+2, 12, Color{226,232,240,230});
  };

  auto drawLives = [&](){
    float x0=12, y0=100; float gap=8, size=14;
    for (int i=0;i<3;i++) DrawHeart(x0 + i*(size*2+gap), y0, size, (i<player.lives) ? Color{239,68,68,255} : Color{255,255,255,40});
  };

  auto drawLevelBanner = [&](){
    int dmgMult = damageMultiplier(); std::string label = std::string("Nivel ") + std::to_string(level) + (level==5?" (Jefe)":""); std::string sub = std::string("Dificultad x") + std::to_string(dmgMult);
    int pad=8; int w = std::max(MeasureText(label.c_str(), 16), MeasureText(sub.c_str(), 12)) + pad*2; int h=42; int x = (W - w)/2; int y=10;
    RoundRect((float)x,(float)y,(float)w,(float)h,10, Color{17,24,39,230}, Color{255,255,255,20});
    DrawText(label.c_str(), x+pad, y+4, 16, Color{229,231,235,255});
    DrawText(sub.c_str(), x+pad, y+24, 12, Color{226,232,240,220});
    if (levelBannerTimer > 0.f){
      // big banner
      int w2 = MeasureText(label.c_str(), 36);
      DrawRectangle((W-w2)/2-20, (int)(H*0.42f-36), w2+40, 56, Color{0,0,0,90});
      DrawText(label.c_str(), (W-w2)/2, (int)(H*0.42f), 36, Color{229,231,235,255});
    }
  };

  auto drawGameOver = [&](){
    DrawRectangle(0,0,W,H, Color{0,0,0,140});
    const char* msg = "GAME OVER"; int w1 = MeasureText(msg, 32);
    DrawText(msg, (W-w1)/2, H/2 - 10, 32, Color{229,231,235,255});
    const char* msg2 = "Presioná R para reiniciar"; int w2 = MeasureText(msg2, 16);
    DrawText(msg2, (W-w2)/2, H/2 + 20, 16, Color{229,231,235,255});
  };

  startLevel(1);

  float lastTime = (float)GetTime();
  while (!WindowShouldClose()){
    float now = (float)GetTime();
    float dt = now - lastTime; lastTime = now; if (dt>0.033f) dt = 0.033f;

    // Input
    Vector2 mouse = GetMousePosition();
    if (!gameOver){
      if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
        bool pickedAny=false;
        for (auto &r : rocks){
          if (r.picked) continue;
          float dClick = std::hypot(mouse.x - r.pos.x, mouse.y - r.pos.y);
          if (dClick <= r.radius + 6){
            if (near(player, r)){
              if ((int)player.inventory.size() < player.capacity){ r.picked=true; player.inventory.push_back(r.radius); pickedAny=true; break; }
            }
          }
        }
        if (!pickedAny){ player.target = {mouse.x, mouse.y}; player.hasTarget = true; }
      }
      if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_SPACE)){
        throwRockToward(V(mouse.x, mouse.y));
      }
    }
    if (IsKeyPressed(KEY_R)){
      if (gameOver) { resetGame(); startLevel(1); }
    }

    // Update
    if (!gameOver){
      // Player movement
      if (player.hasTarget){
        V2 dir = sub(player.target, player.pos); float d = len(dir);
        if (d < 2.f){ player.hasTarget = false; }
        else { V2 step = mul(norm(dir), player.speed*dt); if (len(step)>d) step = dir; player.pos = add(player.pos, step); }
      }

      // Spawn within level
      if (enemiesToSpawn > 0){
        spawnTimer += dt;
        if (spawnTimer >= spawnInterval){
          spawnTimer = 0.f; spawnEnemy(); enemiesToSpawn--; spawnInterval = 2.4f + frand(-0.4f, 1.2f);
        }
      }

      // Enemies update (chase)
      for (auto &e : enemies){
        V2 dir = norm(sub(player.pos, e.pos)); e.pos = add(e.pos, mul(dir, e.speed*dt));
        if (e.boss){
          for (int i=(int)rocks.size()-1;i>=0;--i){ auto &r = rocks[i]; if (r.picked) continue; float d = std::hypot(e.pos.x - r.pos.x, e.pos.y - r.pos.y); if (d <= e.radius + r.radius){ rocks.erase(rocks.begin()+i); } }
        }
      }

      // Projectiles
      for (auto &p : projectiles){ p.pos = add(p.pos, mul(p.vel, dt)); if (p.pos.x < -50 || p.pos.x > W+50 || p.pos.y < -50 || p.pos.y > H+50) p.alive=false; }

      // Collisions: projectile vs enemy
      for (auto &e : enemies){ if (!e.alive) continue; for (auto &p : projectiles){ if (!p.alive) continue; float d = std::hypot(e.pos.x - p.pos.x, e.pos.y - p.pos.y); if (d <= e.radius + p.radius){ int damage = (int)std::round(p.radius * 3.f); e.hp -= damage; p.alive=false; if (e.hp <= 0){ e.alive=false; score += (e.boss ? 5 : 1); player.hp = std::min(player.maxHp, player.hp + 5); if (frand(0.0f, 1.0f) < 0.5f) { rocks.push_back(Rock{ V(e.pos.x, e.pos.y), (float)rockSizes[2], false }); } } } } }

      // Collision: enemy touches player
      bool canBeHit = (now - player.lastHitAt) > 0.7f;
      for (auto &e : enemies){ if (!e.alive) continue; float d = std::hypot(e.pos.x - player.pos.x, e.pos.y - player.pos.y); if (d <= e.radius + player.radius){ if (canBeHit){ bool isBig = e.boss || e.radius >= 18.f; int base = e.boss ? 40 : (isBig ? 20 : 10); int dmg = base * damageMultiplier(); player.hp -= dmg; player.lastHitAt = now; player.iTimer = 0.6f; V2 dir = norm(sub(player.pos, e.pos)); player.pos = add(player.pos, mul(dir, 12.f)); if (player.hp <= 0){ player.lives -= 1; if (player.lives > 0){ player.hp = player.maxHp; player.pos = {W*0.5f, H*0.5f}; player.hasTarget=false; player.iTimer=1.2f; for (auto &en : enemies){ if (std::hypot(en.pos.x - player.pos.x, en.pos.y - player.pos.y) < 80.f){ en.pos.y -= 120.f; } } } else { gameOver = true; } } } }
      }

      // New rock every 4s
      rockSpawnTimer += dt; if (rockSpawnTimer >= rockSpawnInterval){ rockSpawnTimer = 0.f; spawnRock(); }

      // Cleanup
      projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(), [](const Projectile&p){return !p.alive;}), projectiles.end());
      enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [&](const Enemy&e){ return !e.alive || e.pos.y > H + 120.f; }), enemies.end());
      if (player.iTimer > 0.f) player.iTimer = std::max(0.f, player.iTimer - dt);

      // Level complete
      if (!gameOver && enemiesToSpawn==0 && enemies.empty()){
        if (level < 5) startLevel(level+1);
        else { cycle += 1; startLevel(1); }
      }
    }

    // Draw
    BeginDrawing();
    drawGrid();
    for (auto &r : rocks) if (!r.picked) drawRock(r);
    for (auto &p : projectiles) drawProjectile(p);
    for (auto &e : enemies) drawEnemy(e);
    drawPlayer(player);
    drawInventory();
    drawScore();
    drawPlayerHP();
    drawLives();
    drawLevelBanner();
    if (gameOver) drawGameOver();
    EndDrawing();

    if (levelBannerTimer > 0.f) levelBannerTimer -= (1.0f/60.0f);
  }

  CloseWindow();
  return 0;
}

