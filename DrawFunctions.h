#pragma once
#include <DXE.h>
#include <Renderer/Texture.h>
#include "Maths/Maths.h"
#include "UIWidgets.h"

namespace Draw {


    struct Pixel { uint8_t r, g, b, a; };

    inline Pixel sample(const std::vector<uint8_t>& img, int W, int H, double x, double y) {
        if (x < 0 || y < 0 || x >= W - 1 || y >= H - 1) return { 0, 0, 0, 255 };
        int x0 = int(floor(x));
        int y0 = int(floor(y));
        double fx = x - x0;
        double fy = y - y0;

        auto getP = [&](int xi, int yi) -> Pixel {
            size_t idx = (yi * W + xi) * 4;
            return { img[idx], img[idx + 1], img[idx + 2], img[idx + 3] };
            };

        Pixel p00 = getP(x0, y0);
        Pixel p10 = getP(x0 + 1, y0);
        Pixel p01 = getP(x0, y0 + 1);
        Pixel p11 = getP(x0 + 1, y0 + 1);

        auto lerp = [](double a, double b, double t) { return a + t * (b - a); };

        Pixel out;
        out.r = (uint8_t)lerp(lerp(p00.r, p10.r, fx), lerp(p01.r, p11.r, fx), fy);
        out.g = (uint8_t)lerp(lerp(p00.g, p10.g, fx), lerp(p01.g, p11.g, fx), fy);
        out.b = (uint8_t)lerp(lerp(p00.b, p10.b, fx), lerp(p01.b, p11.b, fx), fy);
        out.a = (uint8_t)lerp(lerp(p00.a, p10.a, fx), lerp(p01.a, p11.a, fx), fy);;
        return out;
    }

    inline void RectangleToRing(DXE::Texture* texture) {
        int width = texture->Width();
        int height = texture->Height();
        int channels = texture->Channels();
        auto& pixels = texture->Pixels();

        std::vector<uint8_t> output(width * height * channels, 0);

        double maxRadius = 0.5*(height-1); // thickness = input height

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                double dx = x - width/2.f;
                double dy = y - height/2.f;
                double r = std::sqrt(dx * dx + dy * dy);
                double theta = std::atan2(dy, dx);
                if (theta < 0) theta += 2 * DXM::Pi;

                size_t idx = (y * width + x) * 4;

                if (r >= 0 && r <= maxRadius) {
                    // Map polar (r, theta) back to input rectangle coordinates
                    double inX = (theta / (2 * DXM::Pi)) * (width-1);
                    double inY = (r / maxRadius) * (height-1);

                    Pixel color = sample(texture->Pixels(), width, height, inX, inY);

         
                    output[idx + 0] = color.r;
                    output[idx + 1] = color.g;
                    output[idx + 2] = color.b;
                    output[idx + 3] = color.a;
                }
                else {
                    output[idx + 0] = 0;
                    output[idx + 1] = 0;
                    output[idx + 2] = 0;
                    output[idx + 3] = 0;
                }
            }
        }
        texture->Pixels() = output;
    }

    inline void Leaf(DXE::Texture* texture) {

        int width = texture->Width();
        int height = texture->Height();
        int channels = texture->Channels();
        auto& pixels = texture->Pixels();

        auto leafFunction = [](DXM::Vector2& p) {

            float x = p.y;
            float y = p.x;
            float fullness = -.65f;
            int number = 3;

            float pi_2 = DXM::Pi / 2.f;
            float z = y / x;


            bool sign_x = (x > 0) - (x < 0);
            bool sign_y = (y > 0) - (y < 0);
            if (p.x == 0.0f) { z = sign_y; }

            float A = std::atan2f(x, y) / pi_2;
            float V = A * (float)number / (4.f);

            float T = 1.f - 4.f * fabs(V - 0.5f - floorf(V - 0.5f) - 0.5f);

            float value = -(x * x + y * y) - (fullness - (fullness + 1.f) * T);

            return std::max(value, 0.0f);
            };


        for (int Y = 0; Y < height; Y++) {
            for (int X = 0; X < width; X++) {

                int pixelIndex = (Y * width + X) * channels;
                auto& pixel = pixels[pixelIndex];
                // Access components
                unsigned char& r = pixels[pixelIndex + 0];
                unsigned char& g = pixels[pixelIndex + 1];
                unsigned char& b = pixels[pixelIndex + 2];
                unsigned char& a = pixels[pixelIndex + 3];

                DXM::Vector2 p = { (float)X / (width - 1), (float)Y / (height - 1) };
                p = 2.f * p - DXM::Vector2(1.f, 1.f);

                bool condition = true;
                float value = leafFunction(p);
                uint32_t col = std::clamp((int)(255 * value), 0, 255);

                if (condition) {
                    r = col;
                    g = col;
                    b = col;
                    a = (col > 0) ? 255 : 0;
                }

            }
        }
        texture->UpdateTexture();
    }
    inline void Crescent(DXE::Texture* texture, float time = 0.f, float fullness = 0.75f, float bias = 0.05, float noise_scale = 0.015f, int noise_freq_x = 3, int noise_freq_y = 2) {

        int width = texture->Width();
        int height = texture->Height();
        int channels = texture->Channels();
        auto& pixels = texture->Pixels();

        for (int Y = 0; Y < height; Y++) {
            for (int X = 0; X < width; X++) {
                //access rgba components
                int pixelIndex = (Y * width + X) * channels;
                auto& pixel = pixels[pixelIndex];
                unsigned char& r = pixels[pixelIndex + 0];
                unsigned char& g = pixels[pixelIndex + 1];
                unsigned char& b = pixels[pixelIndex + 2];
                unsigned char& a = pixels[pixelIndex + 3];

                // scale coords to -1 <= 0 <= 1
                DXM::Vector2 p = { (float)X / (width - 1), (float)Y / (height - 1) };
                p = 2.f * p - DXM::Vector2(1.f, 1.f);
                float x = p.x;
                float y = p.y;

                // add coord noise distortion
                x += noise_scale * std::sin(DXM::Pi * noise_freq_x * (x + 2 * time));
                y += noise_scale * std::sin(DXM::Pi * noise_freq_y * (y + 2 * time));

                // crecent shape
                float z1 = std::max(0.f, 1.f - (x + fullness) * (x + fullness) - y * y);
                float z2 = std::max(0.f, 1.f - (x - fullness) * (x - fullness) + y * y);
                float denom = (1 - fullness * fullness) * (1 - fullness * fullness);
                float value = (z1 * z2) / denom - bias;

                // write pixel values to texture
                uint32_t col = std::clamp((int)(255 * value), 0, 255);
                r = col;
                g = col;
                b = col;
                a = 255;

            }
        }
        texture->UpdateTexture();
    }
    inline void UnevenCapsule(DXE::Texture* texture, float time = 0.f, DXM::Vector2 pa = { -0.5,0 }, DXM::Vector2 pb = { 0.5,0 }, float ra = 0.02f, float rb = 0.4f, float bias = 0.05, float noise_scale = 0.015f, int noise_freq_x = 3, int noise_freq_y = 2) {

        auto sdUnevenCapsule = [](DXM::Vector2 p, DXM::Vector2 pa, DXM::Vector2 pb, float ra = 0.1f, float rb = 0.1f) {
            p -= pa;
            pb -= pa;
            float h = pb.Dot(pb);
            DXM::Vector2  q = DXM::Vector2(p.Dot(DXM::Vector2(pb.y, -pb.x)), p.Dot(pb)) / h;

            //-----------

            q.x = fabs(q.x);

            float b = ra - rb;
            DXM::Vector2  c = DXM::Vector2(sqrt(h - b * b), b);

            float k = c.x * q.y - c.y * q.x;
            float m = c.Dot(q);
            float n = q.Dot(q);

            if (k < 0.0) return sqrt(h * (n)) - ra;
            else if (k > c.x) return sqrt(h * (n + 1.0f - 2.0f * q.y)) - rb;
            return m - ra;
            };

        int width = texture->Width();
        int height = texture->Height();
        int channels = texture->Channels();
        auto& pixels = texture->Pixels();

        for (int Y = 0; Y < height; Y++) {
            for (int X = 0; X < width; X++) {
                //access rgba components
                int pixelIndex = (Y * width + X) * channels;
                auto& pixel = pixels[pixelIndex];
                unsigned char& r = pixels[pixelIndex + 0];
                unsigned char& g = pixels[pixelIndex + 1];
                unsigned char& b = pixels[pixelIndex + 2];
                unsigned char& a = pixels[pixelIndex + 3];

                // scale coords to -1 <= 0 <= 1
                DXM::Vector2 p = { (float)X / (width - 1), (float)Y / (height - 1) };
                p = 2.f * p - DXM::Vector2(1.f, 1.f);

                p.x += noise_scale * std::sin(DXM::Pi * noise_freq_x * (p.x + 2 * time));
                p.y += noise_scale * std::sin(DXM::Pi * noise_freq_y * (p.y + 2 * time));
                float value = std::max(0.f, -sdUnevenCapsule(p, pa, pb, ra, rb));

                // write pixel values to texture
                uint32_t col = std::clamp((int)(255 * value), 0, 255);
                r = col;
                g = col;
                b = col;
                a = 255;

            }
        }
        texture->UpdateTexture();
    }

    inline void OpenRingSharp(DXE::Texture* texture, float time = 0.f, float opening = 0.5, float radius = 0.5, float thickness = 0.3, float noise_scale = 0.001f, int noise_freq_x = 4, int noise_freq_y = 3) {

        auto sdRing = [](DXM::Vector2 p, float opening, float r, float thickness) {

            // Apply 2D rotation (matrix multiplication)

            DXM::Vector2 pre_rot = { -std::sin(opening), -std::cos(opening), };
            p =  {
                pre_rot.x * p.x - pre_rot.y * p.y,
                pre_rot.y * p.x + pre_rot.x * p.y
            };
       


            p.x = abs(p.x);

            DXM::Vector2 n = { std::cos(opening),std::sin(opening) };
            DXM::Vector2 p_rot = {
            n.x * p.x - n.y * p.y,
            n.y * p.x + n.x * p.y
            };
            p = p_rot;

            // Compute distances
            float sign_x = (p.x > 0) - (p.x < 0);
            float d1 = std::fabs(p.Length() - r) - thickness * 0.5f;
            float d2 = DXM::Vector2(p.x, std::max(0.0, fabs(r - p.y) - thickness * 0.5)).Length() * sign_x;

            return std::max(d1, d2);
            };


        int width = texture->Width();
        int height = texture->Height();
        int channels = texture->Channels();
        auto& pixels = texture->Pixels();

        for (int Y = 0; Y < height; Y++) {
            for (int X = 0; X < width; X++) {
                //access rgba components
                int pixelIndex = (Y * width + X) * channels;
                auto& pixel = pixels[pixelIndex];
                unsigned char& r = pixels[pixelIndex + 0];
                unsigned char& g = pixels[pixelIndex + 1];
                unsigned char& b = pixels[pixelIndex + 2];
                unsigned char& a = pixels[pixelIndex + 3];

                // scale coords to -1 <= 0 <= 1
                DXM::Vector2 p = { (float)X / (width - 1), (float)Y / (height - 1) };
                p = 2.f * p - DXM::Vector2(1.f, 1.f);

                p.x += noise_scale * std::sin(DXM::Pi * noise_freq_x * (p.x + 2 * time));
                p.y += noise_scale * std::sin(DXM::Pi * noise_freq_y * (p.y + 2 * time));
                float value = -sdRing(p, 2*time*DXM::Pi*opening, radius, thickness);

                // write pixel values to texture
                uint32_t col = std::clamp((int)(255 * value), 0, 255);
                r = col;
                g = col;
                b = col;
                a = 255;

            }
        }
        texture->UpdateTexture();
    }

    inline void OpenRingRounded(DXE::Texture* texture, float time = 0.f, float opening = 0.5, float ra = 0.7, float rb = 0.2, float noise_scale = 0.001f, int noise_freq_x = 4, int noise_freq_y = 3) {

        auto sdRingRounded = [](DXM::Vector2 p, float opening, float ra, float rb) {


            DXM::Vector2 pre_rot = { -std::sin(opening), -std::cos(opening), };
            DXM::Vector2 p_rot = {
                pre_rot.x * p.x - pre_rot.y * p.y,
                pre_rot.y * p.x + pre_rot.x * p.y
            };
            p = p_rot;


            // Apply 2D rotation (matrix multiplication)
            p.x = abs(p.x);

            DXM::Vector2 n = { std::sin(opening), std::cos(opening), };

            return ((n.y * p.x > n.x * p.y) ? (p - n * ra).Length() : fabs((p.Length() - ra))) - rb;

            };


        int width = texture->Width();
        int height = texture->Height();
        int channels = texture->Channels();
        auto& pixels = texture->Pixels();

        for (int Y = 0; Y < height; Y++) {
            for (int X = 0; X < width; X++) {
                //access rgba components
                int pixelIndex = (Y * width + X) * channels;
                auto& pixel = pixels[pixelIndex];
                unsigned char& r = pixels[pixelIndex + 0];
                unsigned char& g = pixels[pixelIndex + 1];
                unsigned char& b = pixels[pixelIndex + 2];
                unsigned char& a = pixels[pixelIndex + 3];

                // scale coords to -1 <= 0 <= 1
                DXM::Vector2 p = { (float)X / (width - 1), (float)Y / (height - 1) };
                p = 2.f * p - DXM::Vector2(1.f, 1.f);

                p.x += noise_scale * std::sin(DXM::Pi * noise_freq_x * (p.x + 2 * time));
                p.y += noise_scale * std::sin(DXM::Pi * noise_freq_y * (p.y + 2 * time));
                float value = -sdRingRounded(p, 2 * time * DXM::Pi * opening, ra, rb);

                // write pixel values to texture
                uint32_t col = std::clamp((int)(255 * value), 0, 255);
                r = col;
                g = col;
                b = col;
                a = 255;

            }
        }
        texture->UpdateTexture();
    }

}

class IFrameGenerator {
public:
    virtual ~IFrameGenerator() = default;
    virtual const char* GetName() const = 0;
    virtual bool IsLooping() = 0;
    virtual void Generate(DXE::Texture* tex, double t) = 0;
    virtual bool DrawImGui() = 0; //draw parameters in ImGui
};

class SlashTrailGenerator : public IFrameGenerator {
public:
    struct Parameters {
        DXM::Vector2 pa = { -0.5f,0.f };
        DXM::Vector2 pb = { 0.5f,0.f };
        float ra = 0.0f;
        float rb = 0.4f;
        float bias = 0.05;
        float noise_scale = 0.00f;
        int noise_freq_x = 2;
        int noise_freq_y = 3;
        float brightness = 2.f;
        bool circular = false;
    } S;

    const char* GetName() const override { return "Slash Trail"; }
    bool IsLooping() override { return false; }
    void Generate(DXE::Texture* tex, double t) override {
        SlashTrail(tex, t, S);
    }

    bool DrawImGui() override {
        bool changing = false;
        ImGui::TextUnformatted("Slash Trail Parameters");


        changing |= ImGui::SliderFloat("RA", &S.ra, 0.0f, 1.0f);
        changing |= ImGui::SliderFloat("RB", &S.rb, 0.0f, 2.0f);
        changing |= ImGui::SliderFloat("Bias", &S.bias, 0.0f, 0.5f);
        changing |= ImGui::SliderFloat("Noise Scale", &S.noise_scale, 0.0f, 1.0f);
        changing |= ImGui::SliderInt("Noise Freq X", &S.noise_freq_x, 1, 10);
        changing |= ImGui::SliderInt("Noise Freq Y", &S.noise_freq_y, 1, 10);
        changing |= ImGui::Checkbox("Circular", &S.circular);
  

        changing |= Widgets::XYPad("Start Point", S.pa, -1.0f, 1.0f, -1.0f, 1.0f);
        changing |= Widgets::XYPad("End Point", S.pb, -1.0f, 1.0f, -1.0f, 1.0f);

        return changing;
    }

    void SlashTrail(DXE::Texture* texture, double time, Parameters s) {
        DXM::Vector2 pa = s.pa;
        DXM::Vector2 pb = s.pb;
        float ra = s.ra;
        float rb = s.rb;
        float bias = s.bias;
        float noise_scale = s.noise_scale;
        int noise_freq_x = s.noise_freq_x;
        int noise_freq_y = s.noise_freq_y;


        auto sdUnevenCapsule = [](DXM::Vector2 p, DXM::Vector2 pa, DXM::Vector2 pb, float ra, float rb) {

            p -= pa;
            pb -= pa;
            float h = pb.Dot(pb);


            DXM::Vector2  q = DXM::Vector2(p.Dot(DXM::Vector2(pb.y, -pb.x)), p.Dot(pb)) / h;

            //-----------

            q.x = fabs(q.x);

            float b = ra - rb;
            DXM::Vector2  c = DXM::Vector2(sqrt(h - b * b), b);

            float k = c.x * q.y - c.y * q.x;
            float m = c.Dot(q);
            float n = q.Dot(q);

            if (k < 0.0) return sqrt(h * (n)) - ra;
            else if (k > c.x) return sqrt(h * (n + 1.0f - 2.0f * q.y)) - rb;
            return m - ra;
            };

        int width = texture->Width();
        int height = texture->Height();
        int channels = texture->Channels();
        auto& pixels = texture->Pixels();

        for (int Y = 0; Y < height; Y++) {
            for (int X = 0; X < width; X++) {
                //access rgba components
                int pixelIndex = (Y * width + X) * channels;
                auto& pixel = pixels[pixelIndex];
                unsigned char& r = pixels[pixelIndex + 0];
                unsigned char& g = pixels[pixelIndex + 1];
                unsigned char& b = pixels[pixelIndex + 2];
                unsigned char& a = pixels[pixelIndex + 3];

                // scale coords to -1 <= 0 <= 1
                DXM::Vector2 p = { (float)X / (width - 1), (float)Y / (height - 1) };
                p = 2.f * p - DXM::Vector2(1.f, 1.f);

                p.x += noise_scale * std::sin(DXM::Pi * noise_freq_x * (p.x + 2 * time));
                p.y += noise_scale * std::sin(DXM::Pi * noise_freq_y * (p.y + 2 * time));

                //time = std::clamp(time, 0.f, 1.f);
                double u = (1 - time);
                double t = 1 - u * u * u * u;
                DXM::Vector2 end_pos = pa * (1 - t) + (t)*pb;
                float end_rad = ra * (1 - t) + (t)*rb;
                float value = std::max(0.f, -sdUnevenCapsule(p, pa, end_pos, ra, end_rad));


                // brighten
                if (value > 0) { value = std::clamp(8.f * value, 0.f, 1.f); }
                uint8_t col = std::clamp((int)(255 * value), 0, 255);

                r = col;
                g = col;
                b = col;
                a = col;

            }
        }

        if (s.circular) { Draw::RectangleToRing(texture); }
   
    }
};

class LightningBeamGenerator : public IFrameGenerator {
public:
    struct Parameters {
        int speed = 1;
        
        float freq = 1.75f;
        float amps = 0.125f;
        float offset = 0.f;

        float angle = 0.f;
        float height = 0.782;


        float noise_scale_x = 0.175f;
        float noise_scale_y = 0.325f;
        float noise_freq_x = 24.f;
        float noise_freq_y = 48.f;

        float brightness = 2.f;
        float bias = 0.01f;
        bool inverted = false;
        bool circular = false;
    } S;

    const char* GetName() const override { return "Lightning Beam"; }
    bool IsLooping() override { return true; }
    void Generate(DXE::Texture* tex, double t) override {
        LightningBeam(tex, t, S);
    }

    bool DrawImGui() override {
        bool changing = false;
        ImGui::TextUnformatted("Lightning Beam Parameters");

        changing |= ImGui::SliderInt("Speed", &S.speed, -10, 10);
        changing |= ImGui::SliderFloat("Frequency", &S.freq, -10.f, 10.f);
        changing |= ImGui::SliderFloat("Amps", &S.amps, 0.f, 1.f);
        changing |= ImGui::SliderFloat("Offset", &S.offset, -1.f, 1.f);
        changing |= ImGui::SliderFloat("Angle", &S.angle, 0.f, DXM::Pi);

        changing |= ImGui::SliderFloat("Noise Scale X", &S.noise_scale_x, -1.f, 1.f);
        changing |= ImGui::SliderFloat("Noise Frequency X", &S.noise_freq_x, 0.f, 100.f);

        changing |= ImGui::SliderFloat("Noise Scale Y", &S.noise_scale_y, -1.f, 1.f);
        changing |= ImGui::SliderFloat("Noise Frequency Y", &S.noise_freq_y, 0.f, 100.f);

        changing |= ImGui::SliderFloat("Height", &S.height, 0.f, 1.f);
        changing |= ImGui::SliderFloat("Bias", &S.bias, 0.f, 1.f);
        changing |= ImGui::SliderFloat("Brightness", &S.brightness, 0.f, 10.f);
        changing |= ImGui::Checkbox("Invert", &S.inverted);
        changing |= ImGui::Checkbox("Circular", &S.circular);

        return changing;

    }

    void LightningBeam(DXE::Texture* texture, double time, Parameters s) {

        auto triangleWave = [](float x) { 
            float X = x - std::floor(x);
            return fabs(4 * X - 2.f) - 1.f;
            };
        auto circleEnvelope = [](float x, float y, float h) {return h-(x*x+y*y);};
        auto lineSlope = [](float x, float y, float h) {return (h - 1.f) + (2.f/(1.f + fabs(x+y)) - 1.f);};
        auto rotateXY = [](float x, float y, float alpha) {
            float cos = std::cos(alpha);
            float sin = std::sin(alpha);
            return DXM::Vector2(x*cos-y*sin,x*sin+y*cos);
            };

        int width = texture->Width();
        int height = texture->Height();
        int channels = texture->Channels();
        auto& pixels = texture->Pixels();

        for (int Y = 0; Y < height; Y++) {
            for (int X = 0; X < width; X++) {
                //access rgba components
                int pixelIndex = (Y * width + X) * channels;
                auto& pixel = pixels[pixelIndex];
                unsigned char& r = pixels[pixelIndex + 0];
                unsigned char& g = pixels[pixelIndex + 1];
                unsigned char& b = pixels[pixelIndex + 2];
                unsigned char& a = pixels[pixelIndex + 3];

                // scale coords to -1 <= 0 <= 1
                DXM::Vector2 p = { (float)X / (width - 1), (float)Y / (height - 1) };
                p = 2.f * p - DXM::Vector2(1.f, 1.f);



                double t = time;
                DXM::Vector2 p_rot = rotateXY(p.x, p.y, s.angle);
                p_rot.y += s.offset;

                float noise_x = 1 + s.noise_scale_x * sin(s.noise_freq_x * (p_rot.x));
                float noise_y = 1.f/(1 - s.noise_scale_y * cos(s.noise_freq_y * (p_rot.y)));

                float pos_x = triangleWave(s.freq * p_rot.x * noise_x + s.speed * time);
                float pos_y = (2.f / s.amps) * p_rot.y * noise_y;

                float env = std::max(0.f, circleEnvelope(p.x, p.y, s.height));

                float polarity = (s.inverted) ? -1.f : 1.f;

                float value = polarity*lineSlope(pos_x,pos_y,s.height)*env - s.bias;
                // brighten
                if (value > 0) { value = std::clamp(s.brightness * value, 0.f, 1.f); }
                uint8_t col = std::clamp((int)(255 * value), 0, 255);

                r = col;
                g = col;
                b = col;
                a = col;

            }
        }

        if (s.circular) { Draw::RectangleToRing(texture); }

    }
};