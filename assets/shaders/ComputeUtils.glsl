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

