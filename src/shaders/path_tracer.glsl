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

const int MAX_BOUNCES = 16;
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
vec3 randomRayDirection(vec3 normal) {
    float cosTheta    = rand() * 2.0 - 1.0;   // Uniform in [-1, 1].
    float azimuthAngle = rand() * 2.0 * PI;
    float sinTheta    = sqrt(1.0 - cosTheta * cosTheta);
    vec3 delta = vec3(
        sinTheta * cos(azimuthAngle),
        sinTheta * sin(azimuthAngle),
        cosTheta
    );
    return normalize(normal + delta);
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

// Determines ray-sphere intersection
// Returns points where ray enters and leaves sphere
// Adapted from https://iquilezles.org/articles/intersectors/
bool intersectsSphere(in Ray ray, in Sphere sphere, out HitInfo hit) {
    vec3 OC = ray.origin - sphere.centre;   // Distance between ray origin and sphere centre
    float tca = dot(OC, ray.direction);   // t_ca = Projection of OC onto ray direction
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

// Find object in world hit by ray, if any
bool worldIntersect(in Ray ray, out HitInfo objectHit) {
    objectHit.distanceAlongRay = 1.0e9;
    bool foundHit = false;
    HitInfo possibleHit;

    // Scene model definition
    Sphere lightSphere = Sphere(
        vec3(0.0, 0.3, -3.0),
        0.2,
        vec3(0.0),
        vec3(30.0),
        0.0
    );
    // Sphere lightSphere2 = Sphere(
    //     vec3(-0.2, 0.3, -3.0),
    //     0.2,
    //     vec3(0.0),
    //     vec3(10.0),
    //     0.0
    // );
    Sphere roughSphere = Sphere(
        vec3(0.3, -0.3, -4.0),
        0.3,
        vec3(0.8, 0.3, 0.3),
        vec3(0.0),
        0.9
    );
    Sphere shinySphere = Sphere(
        vec3(-0.4, -0.3, -4.0),
        0.3,
        vec3(0.3, 0.8, 0.3),
        vec3(0.0),
        0.2
    );
    Sphere floorSphere = Sphere(
        vec3(0.0, -101.0, -3.0),
        100.0,
        vec3(0.8),
        vec3(0.0),
        1.0
    );
    
    if (intersectsSphere(ray, lightSphere, possibleHit) && possibleHit.distanceAlongRay < objectHit.distanceAlongRay) {
        objectHit = possibleHit;
        foundHit = true;
    }
    // if (intersectsSphere(ray, lightSphere2, possibleHit) && possibleHit.distanceAlongRay < objectHit.distanceAlongRay) {
    //     objectHit = possibleHit;
    //     foundHit = true;
    // }
    if (intersectsSphere(ray, roughSphere, possibleHit) && possibleHit.distanceAlongRay < objectHit.distanceAlongRay) {
        objectHit = possibleHit;
        foundHit = true;
    }
    if (intersectsSphere(ray, shinySphere, possibleHit) && possibleHit.distanceAlongRay < objectHit.distanceAlongRay) {
        objectHit = possibleHit;
        foundHit = true;
    }
    if (intersectsSphere(ray, floorSphere, possibleHit) && possibleHit.distanceAlongRay < objectHit.distanceAlongRay) {
        objectHit = possibleHit;
        foundHit = true;
    }

    return foundHit;
}

Ray getBRDFRay(Ray ray, HitInfo hit) {
    vec3 diffuseDirection = randomRayDirection(hit.normal);
    vec3 specularDirection = reflect(ray.direction, hit.normal);
    vec3 brdfRayDirection = normalize(mix(specularDirection, diffuseDirection, hit.roughness));
    vec3 brdfRayOrigin = ray.origin + hit.distanceAlongRay*ray.direction + hit.normal*EPSILON;

    return Ray(brdfRayOrigin, brdfRayDirection);
}

vec3 calcPixelColour(Ray cameraRay) {
    Ray currentRay = cameraRay;
    vec3 pixelColour = vec3(0.0);
    vec3 rayIntensity = vec3(1.0);

    for (int i = 0; i < MAX_BOUNCES; i++) {
        HitInfo hit;
        bool hitObject = worldIntersect(currentRay, hit);

        if (!hitObject) {
            // float skyBlendFactor = 0.5 * (currentRay.direction.y + 1.0);
            // vec3 skyRadiance = mix(vec3(1.0, 0.95, 0.8),
            //            vec3(0.2, 0.5, 1.0),
            //            skyBlendFactor);
            vec3 skyColour = vec3(0.2, 0.5, 1.0);
            pixelColour  += rayIntensity * skyColour;
            break;
        }

        pixelColour += rayIntensity * hit.objectLightIntensity;
        rayIntensity *= hit.objectColour;

        // Optimization: Russian Roulette Termination
        if (i > 2) {
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
    const int SAMPLES_PER_FRAME = 6;
    vec3 colour = vec3(0.0);

    for (int s = 0; s < SAMPLES_PER_FRAME; s++) {
        seed = uint(gl_FragCoord.x) * 1973u
            ^ uint(gl_FragCoord.y) * 9277u
            ^ uint(u_frames * SAMPLES_PER_FRAME + s) * 26699u;

        vec2 subPixelJitter = vec2(rand(), rand()) - 0.5;
        vec2 jitteredPixel = gl_FragCoord.xy + subPixelJitter;
        vec2 pos = (jitteredPixel / vec2(u_resolution.x, u_resolution.y)) * 2.0 - 1.0;
        pos.x *= u_resolution.x / u_resolution.y;

        Ray firstRay = Ray(
            u_cameraPosition,
            normalize(vec3(pos, -FOV))
        );

        colour += calcPixelColour(firstRay);
    }
    colour /= float(SAMPLES_PER_FRAME);

    vec3 previousFrameAverage = texture(u_accumulatedTexture, fragmentTexCoord).rgb;
    float blendWeight = 1.0 / float(u_frames + 1);
    vec3 updatedAverage = mix(previousFrameAverage, colour, blendWeight);

    fragColor = vec4(updatedAverage, 1.0);
}