#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTLFX_PRIVATE_IMPLEMENTATION

#include <Foundation/Foundation.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <Metal/Metal.hpp>
#include <MetalFX/MetalFX.hpp>

#include <simd/simd.h>
#include <utility>
#include <variant>
#include <vector>
#include <cmath>

#include "RMDLGameCoordinator.hpp"

#define NUM_ELEMS(arr) (sizeof(arr) / sizeof(arr[0]))

static constexpr uint64_t kPerFrameBumpAllocatorCapacity = 1024; // 1 KiB

struct Uniforms
{
    simd::float4x4 modelViewProjectionMatrix;
    simd::float4x4 modelMatrix;
};

static const float cubeVertices[] = {
    // positions          // colors
    -1.0f, -1.0f, -1.0f,  1.0f, 0.0f, 0.0f, // 0
     1.0f, -1.0f, -1.0f,  0.0f, 1.0f, 0.0f, // 1
     1.0f,  1.0f, -1.0f,  0.0f, 0.0f, 1.0f, // 2
    -1.0f,  1.0f, -1.0f,  1.0f, 1.0f, 0.0f, // 3
    -1.0f, -1.0f,  1.0f,  1.0f, 0.0f, 1.0f, // 4
     1.0f, -1.0f,  1.0f,  0.0f, 1.0f, 1.0f, // 5
     1.0f,  1.0f,  1.0f,  1.0f, 1.0f, 1.0f, // 6
    -1.0f,  1.0f,  1.0f,  0.0f, 0.0f, 0.0f  // 7
};

static const uint16_t cubeIndices[] = {
    0, 1, 2, 2, 3, 0, // front
    4, 5, 6, 6, 7, 4, // back
    0, 4, 7, 7, 3, 0, // left
    1, 5, 6, 6, 2, 1, // right
    3, 2, 6, 6, 7, 3, // top
    0, 1, 5, 5, 4, 0  // bottom
};

GameCoordinator::GameCoordinator(MTL::Device* pDevice,
                                 MTL::PixelFormat layerPixelFormat,
                                 NS::UInteger width,
                                 NS::UInteger height,
                                 NS::UInteger gameUICanvasSize,
                                 const std::string& assetSearchPath)
    : _layerPixelFormat(layerPixelFormat)
    , _pDevice(pDevice->retain())
    , _frame(0)
    , _maxEDRValue(1.0f)
    , _brightness(500)
    , _edrBias(0)
    , _highScore(0)
    , _prevScore(0)
    , _pCubeVertexBuffer(nullptr)
    , _pCubeIndexBuffer(nullptr)
    , _pUniformBuffer(nullptr)
    , _pCubePipelineState(nullptr)
    , _pDepthState(nullptr)
    , _rotationAngle(0.0f)
{
    printf("GameCoordinator constructor called\n");
    _pCommandQueue = _pDevice->newCommandQueue();
    setupCamera();
    buildCubeBuffers();
    buildRenderPipelines(assetSearchPath);
    buildComputePipelines(assetSearchPath);
    
    const NS::UInteger nativeWidth = (NS::UInteger)(width/1.2);
    const NS::UInteger nativeHeight = (NS::UInteger)(height/1.2);
    printf("GameCoordinator constructor completed\n");
}

GameCoordinator::~GameCoordinator()
{
    if (_pCubeVertexBuffer) _pCubeVertexBuffer->release();
    if (_pCubeIndexBuffer) _pCubeIndexBuffer->release();
    if (_pUniformBuffer) _pUniformBuffer->release();
    if (_pCubePipelineState) _pCubePipelineState->release();
    if (_pDepthState) _pDepthState->release();
    _pCommandQueue->release();
    _pDevice->release();
}

void GameCoordinator::setupCamera()
{
    // Configurer une caméra perspective
    _camera.initPerspectiveWithPosition(
        {0.0f, 0.0f, 5.0f},  // position
        {0.0f, 0.0f, -1.0f}, // direction
        {0.0f, 1.0f, 0.0f},  // up
        M_PI / 3.0f,         // viewAngle (60 degrés)
        1.0f,                // aspectRatio
        0.1f,                // nearPlane
        100.0f               // farPlane
    );
}

void GameCoordinator::buildCubeBuffers()
{
    // Créer le buffer de vertices
    _pCubeVertexBuffer = _pDevice->newBuffer(cubeVertices, sizeof(cubeVertices), MTL::ResourceStorageModeShared);
    
    // Créer le buffer d'indices
    _pCubeIndexBuffer = _pDevice->newBuffer(cubeIndices, sizeof(cubeIndices), MTL::ResourceStorageModeShared);
    
    // Créer le buffer d'uniforms
    _pUniformBuffer = _pDevice->newBuffer(sizeof(Uniforms), MTL::ResourceStorageModeShared);
}

void GameCoordinator::buildRenderPipelines(const std::string& shaderSearchPath)
{
    NS::Error* pError = nullptr;
    std::string metallibPath = shaderSearchPath + "/Shaders.metallib";
    NS::String* nsPath = NS::String::string(metallibPath.c_str(), NS::StringEncoding::UTF8StringEncoding);
    MTL::Library* pLibrary = _pDevice->newLibrary(nsPath, &pError);
    if ( !pLibrary )
    {
        __builtin_printf( "%s", pError->localizedDescription()->utf8String() );
        assert( false );
    }
    MTL::Function* pVertexFn = pLibrary->newFunction(NS::String::string("vertex_main", NS::StringEncoding::UTF8StringEncoding));
    MTL::Function* pFragmentFn = pLibrary->newFunction(NS::String::string("fragment_main", NS::StringEncoding::UTF8StringEncoding));
    MTL::RenderPipelineDescriptor* pDesc = MTL::RenderPipelineDescriptor::alloc()->init();
    pDesc->setVertexFunction(pVertexFn);
    pDesc->setFragmentFunction(pFragmentFn);
    pDesc->colorAttachments()->object(0)->setPixelFormat( MTL::PixelFormat::PixelFormatBGRA8Unorm_sRGB );
    pDesc->setDepthAttachmentPixelFormat( MTL::PixelFormat::PixelFormatDepth16Unorm );

    MTL::VertexDescriptor* pVertexDesc = MTL::VertexDescriptor::alloc()->init();

    // Position (attribute 0)
    pVertexDesc->attributes()->object(0)->setFormat(MTL::VertexFormatFloat3);
    pVertexDesc->attributes()->object(0)->setOffset(0);
    pVertexDesc->attributes()->object(0)->setBufferIndex(0);

    // Couleur (attribute 1)
    pVertexDesc->attributes()->object(1)->setFormat(MTL::VertexFormatFloat3);
    pVertexDesc->attributes()->object(1)->setOffset(12); // 3 floats * 4 bytes = 12
    pVertexDesc->attributes()->object(1)->setBufferIndex(0);

    // Layout du buffer
    pVertexDesc->layouts()->object(0)->setStride(24); // 6 floats * 4 bytes = 24
    pVertexDesc->layouts()->object(0)->setStepRate(1);
    pVertexDesc->layouts()->object(0)->setStepFunction(MTL::VertexStepFunctionPerVertex);

    pDesc->setVertexDescriptor(pVertexDesc);

    _pCubePipelineState = _pDevice->newRenderPipelineState(pDesc, &pError);
    if (!_pCubePipelineState) {
        __builtin_printf( "%s", pError->localizedDescription()->utf8String() );
        assert( false );
    }
    MTL::DepthStencilDescriptor* pDepthDesc = MTL::DepthStencilDescriptor::alloc()->init();
    pDepthDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
    pDepthDesc->setDepthWriteEnabled(true);
    _pDepthState = _pDevice->newDepthStencilState(pDepthDesc);
    
    pDepthDesc->release();
    pVertexDesc->release();
    pDesc->release();
    pVertexFn->release();
    pFragmentFn->release();
    pLibrary->release();
}

void GameCoordinator::buildComputePipelines(const std::string& shaderSearchPath)
{
    // Build any compute pipelines
}

void GameCoordinator::buildRenderTextures(NS::UInteger nativeWidth, NS::UInteger nativeHeight,
                                          NS::UInteger presentWidth, NS::UInteger presentHeight)
{
}

void GameCoordinator::moveCamera(simd::float3 translation)
{
    simd::float3 newPosition = _camera.position() + translation;
    _camera.setPosition(newPosition);
}

void GameCoordinator::rotateCamera(float deltaYaw, float deltaPitch)
{
    _camera.rotateOnAxis({0.0f, 1.0f, 0.0f}, deltaYaw);
    _camera.rotateOnAxis(_camera.right(), deltaPitch);
}

void GameCoordinator::setCameraAspectRatio(float aspectRatio)
{
    _camera.setAspectRatio(aspectRatio);
}

void GameCoordinator::draw(CA::MetalDrawable* pDrawable, double targetTimestamp)
{
    assert(pDrawable);
    NS::AutoreleasePool* pPool = NS::AutoreleasePool::alloc()->init();
    
    MTL::CommandBuffer* pCmd = _pCommandQueue->commandBuffer();

    _rotationAngle += 0.01f;
    if (_rotationAngle > 2 * M_PI)
    {
        _rotationAngle -= 2 * M_PI;
    }
    Uniforms* pUniforms = (Uniforms*)_pUniformBuffer->contents();
    simd::float4x4 modelMatrix =
    {
        simd::float4{ cosf(_rotationAngle), 0.0f, sinf(_rotationAngle), 0.0f },
        simd::float4{ 0.0f, 1.0f, 0.0f, 0.0f },
        simd::float4{ -sinf(_rotationAngle), 0.0f, cosf(_rotationAngle), 0.0f },
        simd::float4{ 0.0f, 0.0f, 0.0f, 1.0f }
    };
    RMDLCameraUniforms cameraUniforms = _camera.uniforms();
    pUniforms->modelMatrix = modelMatrix;
    pUniforms->modelViewProjectionMatrix = cameraUniforms.viewProjectionMatrix * modelMatrix;
    
    MTL::RenderPassDescriptor* pRenderPass = MTL::RenderPassDescriptor::renderPassDescriptor();
    pRenderPass->colorAttachments()->object(0)->setTexture(pDrawable->texture());
    pRenderPass->colorAttachments()->object(0)->setLoadAction(MTL::LoadActionClear);
    pRenderPass->colorAttachments()->object(0)->setClearColor(MTL::ClearColor::Make(0.1, 0.1, 0.1, 1.0));
    pRenderPass->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionStore);

    MTL::RenderCommandEncoder* pRenderEnc = pCmd->renderCommandEncoder(pRenderPass);
    
    if (pRenderEnc && _pCubePipelineState && _pDepthState)
    {
        pRenderEnc->setRenderPipelineState(_pCubePipelineState);
        pRenderEnc->setDepthStencilState(_pDepthState);
        pRenderEnc->setVertexBuffer(_pCubeVertexBuffer, 0, 0);
        pRenderEnc->setVertexBuffer(_pUniformBuffer, 0, 1);
        pRenderEnc->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle,
                                          NUM_ELEMS(cubeIndices),
                                          MTL::IndexTypeUInt16,
                                          _pCubeIndexBuffer,
                                          0);
    }
    pRenderEnc->endEncoding();
    //pRenderEnc->release();
    pCmd->presentDrawable(pDrawable);
    pCmd->commit();
#ifdef CAPTURE
    pCapMan->stopCapture();
#endif
    pPool->release();
}
