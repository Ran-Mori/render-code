#ifndef STATE_MACHINE_LAB_H
#define STATE_MACHINE_LAB_H

#include <memory>
#include <string>

class Geometry;
class Shader;

class StateMachineLab
{
public:
    StateMachineLab();
    ~StateMachineLab();

    StateMachineLab(const StateMachineLab&) = delete;
    StateMachineLab& operator=(const StateMachineLab&) = delete;

    void render();
    void resize(int width, int height);

    void nextScenario();
    void previousScenario();
    void selectScenario(int index);
    void resetScenario();

    int scenarioIndex() const;
    const char* scenarioName() const;
    std::string windowTitle() const;

private:
    enum class Scenario
    {
        Baseline = 0,
        VaoLeak = 1,
        BlendLeak = 2,
        ScissorLeak = 3,
        PolygonModeLeak = 4,
        ExplicitRestore = 5
    };

    struct UniformLocations
    {
        int offset = -1;
        int scale = -1;
        int color = -1;
    };

    void establishKnownFrameState() const;
    void renderCurrentScenario();
    void setObjectUniforms(
        float offsetX,
        float offsetY,
        float scale,
        float red,
        float green,
        float blue,
        float alpha
    ) const;
    void drawLeftTriangle(float alpha = 1.0f) const;
    void drawRightRectangle(float alpha = 1.0f, bool bindOwnVao = true) const;
    void announceScenario() const;
    void printStateSnapshot() const;

    int framebufferWidth_ = 960;
    int framebufferHeight_ = 640;
    Scenario scenario_ = Scenario::Baseline;
    bool needsStateSnapshot_ = true;
    UniformLocations uniforms_;
    std::unique_ptr<Shader> shader_;
    std::unique_ptr<Geometry> triangle_;
    std::unique_ptr<Geometry> rectangle_;
};

#endif
