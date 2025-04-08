#include <openvdb/openvdb.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <limits>
#include <random>
#include <fstream>

// Vector3 class for ray tracing
struct Vec3 {
    float x, y, z;
    
    Vec3(float x_ = 0, float y_ = 0, float z_ = 0) : x(x_), y(y_), z(z_) {}
    
    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(float f) const { return Vec3(x * f, y * f, z * f); }
    Vec3 operator/(float f) const { return Vec3(x / f, y / f, z / f); }
    
    float length() const { return std::sqrt(x*x + y*y + z*z); }
    Vec3 normalize() const { float l = length(); return Vec3(x/l, y/l, z/l); }
    float dot(const Vec3& v) const { return x*v.x + y*v.y + z*v.z; }
    Vec3 cross(const Vec3& v) const {
        return Vec3(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x);
    }
    Vec3 componentMul(const Vec3& v) const {
        return Vec3(x*v.x, y*v.y, z*v.z);
    }
    static Vec3 max(const Vec3& a, const Vec3& b) {
        return Vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
    }
    static Vec3 min(const Vec3& a, const Vec3& b) {
        return Vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
    }
};

// Ray class
struct Ray {
    Vec3 origin;
    Vec3 direction;
    
    Ray(const Vec3& o, const Vec3& d) : origin(o), direction(d.normalize()) {}
};

// Camera class
class Camera {
public:
    Camera(const Vec3& pos, const Vec3& lookAt, const Vec3& upVec, float fov, float aspect) {
        position = pos;
        forward = (lookAt - pos).normalize();
        right = forward.cross(upVec).normalize();
        Vec3 up = forward.cross(right).normalize();
        
        float tanFov = std::tan(fov * 0.5f * M_PI / 180.0f);
        this->right = right * (tanFov * aspect);
        this->up = up * tanFov;
    }
    
    Ray getRay(float u, float v) const {
        Vec3 direction = forward + right * (u * 2.0f - 1.0f) + up * (v * 2.0f - 1.0f);
        return Ray(position, direction.normalize());
    }
    
private:
    Vec3 position;
    Vec3 forward, right, up;
};

// Volume renderer class
class VolumeRenderer {
public:
    VolumeRenderer(openvdb::FloatGrid::Ptr grid, const Vec3& lightDir, float stepSize = 0.1f)
        : grid(grid), lightDir(lightDir.normalize()), stepSize(stepSize), accessor(grid->getConstAccessor())
    {
        bounds = grid->evalActiveVoxelBoundingBox();
        t0 = Vec3(bounds.min().x(), bounds.min().y(), bounds.min().z());
        t1 = Vec3(bounds.max().x(), bounds.max().y(), bounds.max().z());
    }
    
    Vec3 trace(const Ray& ray, std::mt19937& rng) const {
        // Find intersection with bounding box
        float tMin, tMax;
        if (!intersectBox(ray, tMin, tMax)) {
            return Vec3(0.0f); // Miss
        }
        
        // Start from first intersection
        float t = tMin;
        Vec3 color(0.0f);
        float transmittance = 1.0f;
        
        // Ray march through volume
        while (t < tMax && transmittance > 0.01f) {
            Vec3 pos = ray.origin + ray.direction * t;
            
            // Get density at current position
            float density = sampleDensity(pos);
            
            if (density > 0.0f) {
                // Calculate light contribution
                float lightDensity = traceShadowRay(pos);
                
                // Beer's law for extinction
                float extinction = density * stepSize;
                transmittance *= std::exp(-extinction);
                
                // Add scattered light contribution
                float phase = 1.0f / (4.0f * M_PI); // Isotropic phase function
                Vec3 scatteredLight = Vec3(1.0f) * phase * lightDensity;
                color = color + scatteredLight * transmittance * extinction;
            }
            
            t += stepSize;
        }
        
        return color;
    }
    
private:
    openvdb::FloatGrid::Ptr grid;
    openvdb::FloatGrid::ConstAccessor accessor;
    openvdb::CoordBBox bounds;
    Vec3 t0, t1;
    Vec3 lightDir;
    float stepSize;
    
    float sampleDensity(const Vec3& worldPos) const {
        openvdb::Vec3d pos(worldPos.x, worldPos.y, worldPos.z);
        return accessor.getValue(grid->transform().worldToIndexCellCentered(pos));
    }
    
    bool intersectBox(const Ray& ray, float& tMin, float& tMax) const {
        Vec3 invDir(1.0f/ray.direction.x, 1.0f/ray.direction.y, 1.0f/ray.direction.z);
        Vec3 tMin3 = (t0 - ray.origin).componentMul(invDir);
        Vec3 tMax3 = (t1 - ray.origin).componentMul(invDir);
        
        Vec3 t1 = Vec3::max(tMin3, tMax3);
        Vec3 t2 = Vec3::min(tMin3, tMax3);
        
        tMin = std::max(std::max(t2.x, t2.y), t2.z);
        tMax = std::min(std::min(t1.x, t1.y), t1.z);
        
        return tMax >= tMin && tMax > 0;
    }
    
    float traceShadowRay(const Vec3& pos) const {
        float transmittance = 1.0f;
        float t = 0.0f;
        
        while (t < 20.0f && transmittance > 0.01f) {
            Vec3 samplePos = pos + lightDir * t;
            float density = sampleDensity(samplePos);
            transmittance *= std::exp(-density * stepSize);
            t += stepSize;
        }
        
        return transmittance;
    }
};

void saveToPPM(const std::string& filename, const std::vector<Vec3>& pixels, int width, int height) {
    std::ofstream file(filename, std::ios::binary);
    file << "P6\n" << width << " " << height << "\n255\n";
    
    for (const auto& color : pixels) {
        unsigned char r = static_cast<unsigned char>(std::min(color.x * 255.0f, 255.0f));
        unsigned char g = static_cast<unsigned char>(std::min(color.y * 255.0f, 255.0f));
        unsigned char b = static_cast<unsigned char>(std::min(color.z * 255.0f, 255.0f));
        file.write(reinterpret_cast<const char*>(&r), 1);
        file.write(reinterpret_cast<const char*>(&g), 1);
        file.write(reinterpret_cast<const char*>(&b), 1);
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <vdb_file>" << std::endl;
        return 1;
    }
    
    // Initialize OpenVDB
    openvdb::initialize();
    
    try {
        // Load VDB file
        openvdb::io::File file(argv[1]);
        file.open();
        
        // Get density grid
        openvdb::GridBase::Ptr baseGrid = file.readGrid("density");
        openvdb::FloatGrid::Ptr densityGrid = openvdb::gridPtrCast<openvdb::FloatGrid>(baseGrid);
        
        // Set up camera
        Vec3 cameraPos(5.0f, 3.0f, 5.0f);
        Vec3 lookAt(0.0f, 0.0f, 0.0f);
        float fov = 60.0f;
        int width = 800;
        int height = 600;
        float aspect = static_cast<float>(width) / height;
        
        Camera camera(cameraPos, lookAt, Vec3(0,1,0), fov, aspect);
        
        // Set up renderer
        Vec3 lightDir(-1.0f, 1.0f, -1.0f);
        VolumeRenderer renderer(densityGrid, lightDir, 0.1f);
        
        // Set up random number generator
        std::random_device rd;
        std::mt19937 rng(rd());
        
        // Render image
        std::vector<Vec3> pixels(width * height);
        
        #pragma omp parallel for collapse(2)
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float u = (x + 0.5f) / width;
                float v = (y + 0.5f) / height;
                
                Ray ray = camera.getRay(u, v);
                pixels[y * width + x] = renderer.trace(ray, rng);
            }
        }
        
        // Save image
        saveToPPM("volume_render.ppm", pixels, width, height);
        std::cout << "Rendered image saved to volume_render.ppm" << std::endl;
        
        file.close();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 