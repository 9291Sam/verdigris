struct Ray
{
    vec3 origin;
    vec3 direction;
};

vec3 Ray_at(in Ray self, in float t)
{
    return self.origin + self.direction * t;
}