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
uniform int u_move_objects;
uniform int u_max_bounces;

const float EPSILON = 0.001;
const float FOV = 2.5;
const float PI = 3.14159265358979;

uint seed;

// ------ Pseudo RNG adapted from: https://stackoverflow.com/a/17479300 ------
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
// The generated ray is within hemisphere of normal in a cosine-weighted distribution
// Adapted from https://www.pbr-book.org/3ed-2018/Monte_Carlo_Integration/2D_Sampling_with_Multidimensional_Transformations
vec3 cosineWeightedHemisphere(vec3 normal) {
    // Uniformly sample random point on disk
    vec2 randomDiskPoint;
    vec2 point = vec2(rand(), rand());
    float r = sqrt(point.x);
    float theta = 2.0 * PI * point.y;
    randomDiskPoint = r * vec2(cos(theta), sin(theta));

    // Malley's method - convert disk point into cosine-weighted hemisphere
    float y = sqrt(max(0.0, 1 - dot(randomDiskPoint, randomDiskPoint)));
    vec3 localRay = vec3(randomDiskPoint.x, y, randomDiskPoint.y);

    // Gram-Schmidt process:
    // Convert localRay to worldRay by projecting world axes onto surface normal axes
    // normal  : surface normal's y-axis
    // right   : surface normal's x-axis
    // forward : surface normal's z-axis
    vec3 worldUp = abs(normal.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);
    vec3 right = normalize(cross(worldUp, normal));
    vec3 forward = cross(normal, right);

    return right*localRay.x + forward*localRay.z + normal*localRay.y;
}

// Information about a ray
struct Ray {
    vec3 origin;    // Point of origin
    vec3 direction; // Direction of ray
};

// Information about a ray-surface intersection
struct HitInfo {
    float distanceAlongRay;     // distance along ray to point of intersection
    vec3 normal;                // surface normal
    vec3 objectColour;          // surface colour (albedo)
    vec3 objectLightIntensity;  // intensity of emitted light from surface
    float roughness;            // surface roughness
};

// Objects to be used in scenes
struct Sphere {
    vec3 centre;
    float radius;
    vec3 colour;         // albedo
    vec3 lightIntensity; // intensity of emitted light from sphere
    float roughness;
};
struct Plane {
    vec3 point;
    vec3 normal;
    vec3 colour;         // albedo
    vec3 lightIntensity; // intensity of emitted light from plane           
    float roughness;   
};

// Determines ray-sphere intersection
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

// Determines ray-plane intersection
// Adapted from Week 6 Ray Tracing slides
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

    // Objects in scene
    vec3 lightPosition = vec3(0.0, 0.4, -3.0);
    if (u_move_objects == 2) {
        lightPosition = vec3(0.5*sin(u_time*0.5), 0.4, 0.5*cos(u_time*0.5) - 3.0);
    }
    Sphere lightSphere = Sphere(
        lightPosition,
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
        vec3(-0.3, -0.6, -4.0),
        0.2,
        vec3(0.3, 0.8, 0.3),
        vec3(0.0),
        0.25
    );
    Sphere mirrorSphere = Sphere(
        vec3(0.0, 0.0, -4.5),   // centre
        0.25,                   // radius
        vec3(1.0),              // colour
        vec3(0.0),              // lightIntensity
        0.01                    // roughness
    );

    Plane leftWall = Plane(
        vec3(-1.0, 0.0, 0.0),       // point
        vec3(1.0, 0.0, 0.0),        // normal
        0.8*vec3(0.65, 0.15, 0.15), // colour
        vec3(0.0),                  // lightIntensity
        1.0                         // roughness
    );
    Plane rightWall = Plane(
        vec3(1.0, 0.0, 0.0),
        vec3(-1.0, 0.0, 0.0),
        0.8*vec3(0.15, 0.65, 0.15),
        vec3(0.0),
        1.0
    );
    Plane topWall = Plane(
        vec3(0.0, 0.7, 0.0),
        vec3(0.0, -1.0, 0.0),
        vec3(0.7, 0.7, 0.7),
        vec3(0.0),
        1.0
    );
    Plane bottomWall = Plane(
        vec3(0.0, -1.0, 0.0),
        vec3(0.0, 1.0, 0.0),
        vec3(0.7, 0.7, 0.7),
        vec3(0.0),
        1.0
    );
    Plane backWall = Plane(
        vec3(0.0, 0.0, -6.0),
        vec3(0.0, 0.0, 1.0),
        vec3(0.7, 0.7, 0.7),
        vec3(0.0),
        1.0
    );

    // Find closest intersection
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
    if (intersectsSphere(ray, mirrorSphere, possibleHit) 
        && possibleHit.distanceAlongRay < objectHit.distanceAlongRay) {
        objectHit = possibleHit;
        foundHit = true;
    }

    if (intersectsPlane(ray, leftWall, possibleHit) 
        && possibleHit.distanceAlongRay < objectHit.distanceAlongRay) {
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

// Generates a new ray using BRDF to simulate bouncing off a material
Ray getBRDFRay(Ray ray, HitInfo hit) {
    // For back-face bounces
    vec3 faceNormal = dot(ray.direction, hit.normal) < 0.0 ? hit.normal : -hit.normal;

    vec3 diffuseDirection = cosineWeightedHemisphere(hit.normal);
    vec3 reflectDirection = reflect(ray.direction, hit.normal);
    vec3 brdfRayDirection = normalize(mix(reflectDirection, diffuseDirection, hit.roughness));
    vec3 brdfRayOrigin = ray.origin + hit.distanceAlongRay*ray.direction + faceNormal*EPSILON;

    return Ray(brdfRayOrigin, brdfRayDirection);
}

// Calculates pixel colour for one ray from camera
// Adapted from https://iquilezles.org/articles/simplepathtracing/
vec3 calcPixelColour(Ray cameraRay) {
    Ray currentRay = cameraRay;
    vec3 pixelColour = vec3(0.0);   // Final colour of pixel
    vec3 rayIntensity = vec3(1.0);  // Light intensity of ray

    for (int i = 0; i < u_max_bounces; i++) {
        HitInfo hit;
        bool hasHitObject = worldIntersect(currentRay, hit);

        if (!hasHitObject) {
            // No objects hit, sample pseudo-skybox colour
            vec3 skyColour = vec3(0.2, 0.5, 1.0);
            pixelColour  += rayIntensity * skyColour;
            break;
        }

        // Accumulate pixel colour from light source
        pixelColour += rayIntensity * hit.objectLightIntensity;
        // Attenuate ray after bounce
        rayIntensity *= hit.objectColour;

        // Optimization: Russian Roulette Termination
        if (i > 4) {
            float survivalProbability = max(rayIntensity.r, 
                            max(rayIntensity.g, rayIntensity.b));
            if (rand() > survivalProbability) break;
            // Scale remaining rays up
            rayIntensity /= survivalProbability;
        }

        currentRay = getBRDFRay(currentRay, hit);
    }
    return pixelColour;
}

void main() {
    vec3 colour = vec3(0.0);

    for (int s = 0; s < u_samples_per_pixel; s++) {
        // Deterministic seed
        seed = uint(gl_FragCoord.x) * 1973u
            ^ uint(gl_FragCoord.y) * 9277u
            ^ uint(u_frames * u_samples_per_pixel + s + 1) * 26699u;

        vec2 subPixelJitter = vec2(rand(), rand()) - 0.5;
        vec2 jitteredPixel = gl_FragCoord.xy + subPixelJitter;
        vec2 pos = (jitteredPixel 
            / vec2(u_resolution.x, u_resolution.y)) * 2.0 - 1.0;
        pos.x *= u_resolution.x / u_resolution.y;

        Ray firstRay = Ray(
            u_cameraPosition, normalize(vec3(pos, -FOV))
        );

        colour += calcPixelColour(firstRay);
    }
    colour /= float(u_samples_per_pixel);

    if (u_rendering_mode == 2) {
        // Temporal accumulation
        // Blending previous frame to current frame
        vec3 prevFrame = texture(u_accumulatedTexture, fragmentTexCoord).rgb;
        float blendWeight = 1.0 / float(u_frames + 1);
        colour = mix(prevFrame, colour, blendWeight);
    }

    fragColor = vec4(colour, 1.0);
}