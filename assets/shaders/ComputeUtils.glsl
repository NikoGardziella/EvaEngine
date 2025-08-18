// ---------- config ----------
#define MAX_TEXTURES            32
#define MAX_COLLISION_ENTITIES  64
#define CHUNK_SIZE              32
#define SUBTILES_PER_TILE       4
#define GRID_WIDTH              3
#define GRID_HEIGHT             3
#define ACTIVE_SLOTS            (GRID_WIDTH * GRID_HEIGHT)  // 9

bool pointInRotatedBox(vec2 p, vec2 c, vec2 halfSize, float rot);

bool pointInRotatedBox(vec2 p, vec2 c, vec2 halfSize, float rot)
{
    vec2 d = p - c;
    float cs = cos(rot), sn = sin(rot);
    vec2 q = vec2(cs*d.x + sn*d.y, -sn*d.x + cs*d.y);
    return (abs(q.x) <= halfSize.x) && (abs(q.y) <= halfSize.y);
}

bool isProjectile(uint type) { return type == 0u; }
bool isPlayer(uint type)     { return type == 1u; }
bool isVehicle(uint type)    { return type == 2u; }

