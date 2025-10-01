#ifndef RMDLGAMECOORDINATOR_HPP
#define RMDLGAMECOORDINATOR_HPP

#include <Foundation/Foundation.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <Metal/Metal.hpp>
#include <MetalFX/MetalFX.hpp>

#include <string>
#include <unordered_map>

#include <../includes/NonCopyable.h>
#include <../../Renderer/RMDLCamera.hpp>

class GameCoordinator : public NonCopyable
{
public:
    GameCoordinator(MTL::Device* pDevice,
                    MTL::PixelFormat layerPixelFormat,
                    NS::UInteger width,
                    NS::UInteger height,
                    NS::UInteger gameUICanvasSize,
                    const std::string& assetSearchPath);
    
    ~GameCoordinator();

    // Set up the scene.
    void buildRenderPipelines(const std::string& shaderSearchPath);
    void buildComputePipelines(const std::string& shaderSearchPath);
    void buildRenderTextures(NS::UInteger nativeWidth, NS::UInteger nativeHeight,
                             NS::UInteger presentWidth, NS::UInteger presentHeight);
    void loadGameTextures(const std::string& textureSearchPath);
    void buildMetalFXUpscaler(NS::UInteger inputWidth, NS::UInteger inputHeight,
                              NS::UInteger outputWidth, NS::UInteger outputHeight);

    // Call every frame to produce the animations.
    void presentTexture(MTL::RenderCommandEncoder* pRenderEnc, MTL::Texture* pTexture);
    void draw(CA::MetalDrawable* pDrawable, double targetTimestamp);

    void setMaxEDRValue(float value)     { _maxEDRValue = value; }
    void setBrightness(float brightness) { _brightness = brightness; }
    void setEDRBias(float edrBias)       { _edrBias = edrBias; }

    enum class HighScoreSource {
        Local,
        Cloud
    };
    
    void setHighScore(int highScore, HighScoreSource scoreSource);
    int highScore() const                { return _highScore; }

    void moveCamera(simd::float3 translation);
    void rotateCamera(float deltaYaw, float deltaPitch);
    void setCameraAspectRatio(float aspectRatio);
    
private:
    MTL::PixelFormat                    _layerPixelFormat;
    MTL::Device*                        _pDevice;
    MTL::CommandQueue*                  _pCommandQueue;
    // Animation data:
    int _frame;

    float          _maxEDRValue;
    float          _brightness;
    float          _edrBias;
    
    int            _highScore;
    int            _prevScore;

    RMDLCamera _camera;
    MTL::Buffer* _pCubeVertexBuffer;
    MTL::Buffer* _pCubeIndexBuffer;
    MTL::Buffer* _pUniformBuffer;
    MTL::RenderPipelineState* _pCubePipelineState;
    MTL::DepthStencilState* _pDepthState;
    float _rotationAngle;
    void buildCubeBuffers();
    void setupCamera();
};

#endif /* RMDLGAMECOORDINATOR_HPP */
