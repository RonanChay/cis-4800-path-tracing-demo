#shader vertex
#version 330 core
layout(location = 0) in vec4 position;
layout(location = 1) in vec4 raw_normal;
layout(location = 2) in vec4 raw_colour;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

out vec2 fragmentTexCoord;

void main() {
    gl_Position = vec4(position.xy, 0.0, 1.0);
    fragmentTexCoord = position.xy * 0.5 + 0.5;
}

#shader fragment
#version 330 core

out vec4 fragColor;
in vec2 fragmentTexCoord;

uniform int u_frames;
uniform float u_time;
uniform vec3 u_cameraPosition;
uniform sampler2D u_accumulatedTexture;
uniform vec2 u_resolution;
uniform int u_samples_per_pixel;
uniform int u_rendering_mode;
uniform int u_max_bounces;

const float EPSILON = 0.001;
const float FOV = 2.5;
const float PI = 3.14159265358979;

uint seed;

// ------ Pseudo RNG taken from: https://stackoverflow.com/a/17479300 ------
// A single iteration of Bob Jenkins' One-At-A-Time hashing algorithm.
uint hash( uint x ) {
    x += ( x << 10u );
    x ^= ( x >>  6u );
    x += ( x <<  3u );
    x ^= ( x >> 11u );
    x += ( x << 15u );
    return x;
}
// Construct a float with half-open range [0:1] using low 23 bits.
// All zeroes yields 0.0, all ones yields the next smallest representable value below 1.0.
float floatConstruct( uint m ) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32
    m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
    m |= ieeeOne;                          // Add fractional part to 1.0
    float  f = uintBitsToFloat( m );       // Range [1:2]
    return f - 1.0;                        // Range [0:1]
}
// Pseudo-random value in half-open range [0:1].
float rand() { 
    seed = hash(seed);
    return floatConstruct(seed); 
}
// ------------------------------------------------------------------------

// Generate ray in a random direction after bouncing on a surface
// The generated ray is within hemisphere of normal
// Taken from https://stackoverflow.com/questions/69510208/path-tracing-cosine-hemisphere-sampling-and-emissive-objects
vec3 cosineWeightedHemisphere(vec3 norm) {
    vec4 randNum = vec4(rand(), rand(), rand(), rand());
    float r = pow(randNum.w, 1.0 / (1.0));
    float angle = randNum.y * PI*2.0;
    float sr = sqrt(1.0 - r * r);
    vec3 ph = vec3(sr * cos(angle), sr * sin(angle), r);
    vec3 tangent = normalize(randNum.zyx + vec3(rand(), rand(), rand()) - 1.0);
    vec3 bitangent = cross(norm, tangent);
    tangent = cross(norm, bitangent);
    return normalize(tangent * ph.x + bitangent * ph.y + norm * ph.z);
}

// Information about a ray
struct Ray {
    vec3 origin;
    vec3 direction;
};

// Information about a ray-surface intersection
struct HitInfo {
    float distanceAlongRay;
    vec3 normal;
    vec3 objectColour;
    vec3 objectLightIntensity;
    float roughness;
};

struct Sphere {
    vec3 centre;
    float radius;
    vec3 colour;
    vec3 lightIntensity;
    float roughness;
};
struct Plane {
    vec3 point;
    vec3 normal;
    vec3 colour;
    vec3 lightIntensity;
    float roughness;
};

// Determines ray-sphere intersection
// Returns points where ray enters and leaves sphere
// Adapted from https://iquilezles.org/articles/intersectors/
bool intersectsSphere(in Ray ray, in Sphere sphere, out HitInfo hit) {
    vec3 OC = ray.origin - sphere.centre;   // Distance between ray origin and sphere centre
    float tca = dot(OC, ray.direction);     // t_ca = Projection of OC onto ray direction
    vec3 d = OC - tca * ray.direction;      // d = perpendicular distance between ray direction and sphere centre
    float thc = pow(sphere.radius, 2) - dot(d, d);  // thc = half of chord length ray makes with sphere
    if (thc < 0.0) 
        return false;        // no intersection
    thc = sqrt(thc);
    float t0 = -tca-thc;    // Distance from ray to sphere
    if (t0 < EPSILON) return false; // Floating point precision error

    // Ray intersects with sphere - update hit information
    hit.distanceAlongRay = t0;
    vec3 intersectionPoint = ray.origin + t0*ray.direction;
    hit.normal = normalize(intersectionPoint - sphere.centre);
    hit.objectColour = sphere.colour;
    hit.objectLightIntensity = sphere.lightIntensity;
    hit.roughness = sphere.roughness;
    return true;
}

bool intersectsPlane(in Ray ray, in Plane plane, out HitInfo hit) {
    if (dot(ray.direction, plane.normal) == 0.0) return false;    // Plane perpendicular to ray
    float t = (dot((plane.point - ray.origin), plane.normal)) / dot(ray.direction, plane.normal);
    if (t < EPSILON) return false;  // Plane does not intersect with ray

    // Ray intersects with plane - update hit information
    hit.distanceAlongRay = t;
    hit.normal = plane.normal;
    hit.objectColour = plane.colour;
    hit.objectLightIntensity = plane.lightIntensity;
    hit.roughness = plane.roughness;
    return true;
}

// Find object in world hit by ray, if any
bool worldIntersect(in Ray ray, out HitInfo objectHit) {
    objectHit.distanceAlongRay = 1.0e9;
    bool foundHit = false;
    HitInfo possibleHit;

    // Scene model definition
    Sphere lightSphere = Sphere(
        // vec3(0.0, 0.4, -3.0),
        vec3(0.5*sin(u_time), 0.4, 0.5*cos(u_time) - 3.0),
        0.1,
        vec3(0.0),
        vec3(200.0),
        0.0
    );
    Sphere roughSphere = Sphere(
        vec3(0.3, -0.6, -4.0),
        0.2,
        vec3(0.8, 0.3, 0.3),
        vec3(0.0),
        1.0
    );
    Sphere shinySphere = Sphere(
        vec3(-0.4, -0.6, -4.0),
        0.2,
        vec3(0.3, 0.8, 0.3),
        vec3(0.0),
        0.5
    );
    Sphere mirrorSphere = Sphere(
        vec3(0.0, 0.0, -4.0),
        0.25,
        vec3(1.0),
        vec3(0.0),
        0.01
    );

    Plane leftWall = Plane(
        vec3(-1.0, 0.0, 0.0),
        vec3(1.0, 0.0, 0.0),
        0.8*vec3(0.8, 0.1, 0.1),
        vec3(0.0),
        1.0
    );
    Plane rightWall = Plane(
        vec3(1.0, 0.0, 0.0),
        vec3(-1.0, 0.0, 0.0),
        0.8*vec3(0.1, 0.8, 0.1),
        vec3(0.0),
        1.0
    );
    Plane topWall = Plane(
        vec3(0.0, 0.7, 0.0),
        vec3(0.0, -1.0, 0.0),
        vec3(0.7, 0.4, 0.6),
        vec3(0.0),
        1.0
    );
    Plane bottomWall = Plane(
        vec3(0.0, -1.0, 0.0),
        vec3(0.0, 1.0, 0.0),
        vec3(0.7, 0.4, 0.6),
        vec3(0.0),
        1.0
    );
    Plane backWall = Plane(
        vec3(0.0, 0.0, -6.0),
        vec3(0.0, 0.0, 1.0),
        vec3(0.7, 0.4, 0.6),
        vec3(0.0),
        1.0
    );

    if (intersectsSphere(ray, lightSphere, possibleHit) && possibleHit.distanceAlongRay < objectHit.distanceAlongRay) {
        objectHit = possibleHit;
        foundHit = true;
    }
    if (intersectsSphere(ray, roughSphere, possibleHit) && possibleHit.distanceAlongRay < objectHit.distanceAlongRay) {
        objectHit = possibleHit;
        foundHit = true;
    }
    if (intersectsSphere(ray, shinySphere, possibleHit) && possibleHit.distanceAlongRay < objectHit.distanceAlongRay) {
        objectHit = possibleHit;
        foundHit = true;
    }
    if (intersectsSphere(ray, mirrorSphere, possibleHit) && possibleHit.distanceAlongRay < objectHit.distanceAlongRay) {
        objectHit = possibleHit;
        foundHit = true;
    }

    if (intersectsPlane(ray, leftWall, possibleHit) && possibleHit.distanceAlongRay < objectHit.distanceAlongRay) {
        objectHit = possibleHit;
        foundHit = true;
    }
    if (intersectsPlane(ray, rightWall, possibleHit) && possibleHit.distanceAlongRay < objectHit.distanceAlongRay) {
        objectHit = possibleHit;
        foundHit = true;
    }
    if (intersectsPlane(ray, topWall, possibleHit) && possibleHit.distanceAlongRay < objectHit.distanceAlongRay) {
        objectHit = possibleHit;
        foundHit = true;
    }
    if (intersectsPlane(ray, bottomWall, possibleHit) && possibleHit.distanceAlongRay < objectHit.distanceAlongRay) {
        objectHit = possibleHit;
        foundHit = true;
    }
    if (intersectsPlane(ray, backWall, possibleHit) && possibleHit.distanceAlongRay < objectHit.distanceAlongRay) {
        objectHit = possibleHit;
        foundHit = true;
    }

    return foundHit;
}

Ray getBRDFRay(Ray ray, HitInfo hit) {
    vec3 faceNormal = dot(ray.direction, hit.normal) < 0.0
                      ? hit.normal : -hit.normal;

    vec3 diffuseDirection = cosineWeightedHemisphere(hit.normal);
    vec3 reflectDirection = reflect(ray.direction, hit.normal);
    vec3 brdfRayDirection = normalize(mix(reflectDirection, diffuseDirection, hit.roughness));
    vec3 brdfRayOrigin = ray.origin + hit.distanceAlongRay*ray.direction + faceNormal*EPSILON;

    return Ray(brdfRayOrigin, brdfRayDirection);
}

vec3 calcPixelColour(Ray cameraRay) {
    Ray currentRay = cameraRay;
    vec3 pixelColour = vec3(0.0);
    vec3 rayIntensity = vec3(1.0);

    for (int i = 0; i < u_max_bounces; i++) {
        HitInfo hit;
        bool hasHitObject = worldIntersect(currentRay, hit);

        if (!hasHitObject) {
            vec3 skyColour = vec3(0.2, 0.5, 1.0);
            pixelColour  += rayIntensity * skyColour;
            break;
        }

        pixelColour += rayIntensity * hit.objectLightIntensity;
        rayIntensity *= hit.objectColour;

        // Optimization: Russian Roulette Termination
        if (i > 4) {
            float survivalProbability = max(rayIntensity.r,
                                        max(rayIntensity.g, rayIntensity.b));
            if (rand() > survivalProbability) break;
            rayIntensity /= survivalProbability;
        }

        currentRay = getBRDFRay(currentRay, hit);
    }
    return pixelColour;
}

void main() {
    vec3 colour = vec3(0.0);

    for (int s = 0; s < u_samples_per_pixel; s++) {
        seed = uint(gl_FragCoord.x) * 1973u
            ^ uint(gl_FragCoord.y) * 9277u
            ^ uint(u_frames * u_samples_per_pixel + s + 1) * 26699u;

        vec2 subPixelJitter = vec2(rand(), rand()) - 0.5;
        vec2 jitteredPixel = gl_FragCoord.xy + subPixelJitter;
        vec2 pos = (jitteredPixel / vec2(u_resolution.x, u_resolution.y)) * 2.0 - 1.0;
        pos.x *= u_resolution.x / u_resolution.y;

        Ray firstRay = Ray(u_cameraPosition, normalize(vec3(pos, -FOV)));

        colour += calcPixelColour(firstRay);
    }
    colour /= float(u_samples_per_pixel);

    if (u_rendering_mode == 2) {
        vec3 previousFrameAverage = texture(u_accumulatedTexture, fragmentTexCoord).rgb;
        float blendWeight = 1.0 / float(u_frames + 1);
        colour = mix(previousFrameAverage, colour, blendWeight);
    }

    fragColor = vec4(colour, 1.0);
}