
#pragma once
#include <functional>

#include "dm_defs.h"
#include "epi_color.h"

struct PassInfo
{
    int32_t width_;
    int32_t height_;
};

constexpr int32_t kRenderLayerHUD = 1;
constexpr int32_t kRenderWorldMax = 8;

enum WorldLayer
{
    kWorldLayerSky = 0,
    kWorldLayerSolid,
    kWorldLayerTransparent, // Transparent - additive renders on this layer
    kWorldLayerAlpha, // Sub-layer of Transparent - non-additive alpha renders on this layer
    kWorldLayerWeapon,
    kWorldLayerMax
};

struct WorldRender
{
    bool       active_;
    bool       used_;
    WorldLayer current_layer_;
    int32_t    layers_[kWorldLayerMax];
};

typedef std::function<void()> FrameFinishedCallback;

class RenderBackend
{
  public:
    // Setup the GL matrices for drawing 2D stuff.
    virtual void SetupMatrices2D() = 0;

    // Setup the GL matrices for drawing 2D stuff within the "world" rendered by
    // HUDRenderWorld
    virtual void SetupWorldMatrices2D() = 0;

    // Setup the GL matrices for drawing 3D stuff.
    virtual void SetupMatrices3D() = 0;

    virtual void SetClearColor(RGBAColor color) = 0;

    virtual void StartFrame(int32_t width, int32_t height) = 0;

    virtual void SwapBuffers() = 0;

    virtual void FinishFrame() = 0;

    void OnFrameFinished(FrameFinishedCallback callback)
    {
        on_frame_finished_.push_back(callback);
    }

    virtual WorldRender *BeginWorldRender()   = 0;
    virtual WorldRender *CurrentWorldRender() = 0;
    virtual void         FinishWorldRender()  = 0;

    virtual void         SetWorldLayer(WorldLayer layer, bool clear_depth = false)  = 0;
    virtual int32_t      GetCurrentSokolLayer()  = 0;

    virtual void Resize(int32_t width, int32_t height) = 0;

    virtual void Shutdown() = 0;

    virtual void SoftInit();

    virtual void Init();

    virtual void GetPassInfo(PassInfo &info) = 0;

    virtual void CaptureScreen(int32_t width, int32_t height, int32_t stride, uint8_t *dest) = 0;

    int64_t GetFrameNumber()
    {
        return frame_number_;
    }

    int32_t GetMaxTextureSize() const
    {
        return max_texture_size_;
    }

  protected:
    int32_t max_texture_size_ = 0;
    int64_t frame_number_;

    std::vector<FrameFinishedCallback> on_frame_finished_;
};

extern RenderBackend *render_backend;