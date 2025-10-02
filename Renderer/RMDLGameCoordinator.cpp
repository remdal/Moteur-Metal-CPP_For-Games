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
#include <stdio.h>
#include <iostream>
#include <memory>
#include <thread>
#include <sys/sysctl.h>
#include <stdlib.h>

#include "RMDLGameCoordinator.hpp"
#include "RMDLUtilities.h"

#define NUM_ELEMS(arr) (sizeof(arr) / sizeof(arr[0]))

static constexpr uint64_t kPerFrameBumpAllocatorCapacity = 1024;

struct Uniforms
{
    simd::float4x4 modelViewProjectionMatrix;
    simd::float4x4 modelMatrix;
};

static const float cubeVertices[] = {
    // positions          // colors
    -1.0f, -1.0f, -1.0f,  1.0f, 0.0f, 0.0f,
     1.0f, -1.0f, -1.0f,  0.0f, 1.0f, 0.0f,
     1.0f,  1.0f, -1.0f,  0.0f, 0.0f, 1.0f,
    -1.0f,  1.0f, -1.0f,  1.0f, 1.0f, 0.0f,
    -1.0f, -1.0f,  1.0f,  1.0f, 0.0f, 1.0f,
     1.0f, -1.0f,  1.0f,  0.0f, 1.0f, 1.0f,
     1.0f,  1.0f,  1.0f,  1.0f, 1.0f, 1.0f,
    -1.0f,  1.0f,  1.0f,  0.0f, 0.0f, 0.0f
};

static const uint16_t cubeIndices[] = {
    0, 1, 2, 2, 3, 0, // front
    4, 5, 6, 6, 7, 4, // back
    0, 4, 7, 7, 3, 0, // left
    1, 5, 6, 6, 2, 1, // right
    3, 2, 6, 6, 7, 3, // top
    0, 1, 5, 5, 4, 0  // bottom
};

static const uint16_t indices[] = {
    0, 1, 2, 2, 3, 0, /* front */
    4, 5, 6, 6, 7, 4, /* right */
    8, 9, 10, 10, 11, 8, /* back */
    12, 13, 14, 14, 15, 12, /* left */
    16, 17, 18, 18, 19, 16, /* top */
    20, 21, 22, 22, 23, 20, /* bottom */
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
    , m_frameDataBufferIndex(0)
{
    printf("GameCoordinator constructor called\n");
    unsigned int numThreads = std::thread::hardware_concurrency();
    std::cout << "number of Threads = std::thread::hardware_concurrency : " << numThreads << std::endl;
    _pCommandQueue = _pDevice->newCommandQueue();
    setupCamera();
    buildCubeBuffers();
    buildRenderPipelines(assetSearchPath);
    buildComputePipelines(assetSearchPath);
    
    const NS::UInteger nativeWidth = (NS::UInteger)(width / 1.2);
    const NS::UInteger nativeHeight = (NS::UInteger)(height / 1.2);
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
    m_pSkyboxPipelineState->release();
    m_pSkyVertexDescriptor->release();
    _pDevice->release();
    for(uint8_t i = 0; i < MaxFramesInFlight; i++)
    {
        m_frameDataBuffers[i]->release();
    }
}

void GameCoordinator::loadMetal()
{
    NS::Error* pError = nullptr;

    printf("Selected Device: %s\n", _pDevice->name()->utf8String());
    
    for(uint8_t i = 0; i < MaxFramesInFlight; i++)
    {
        static const MTL::ResourceOptions storageMode = MTL::ResourceStorageModeShared;
        m_frameDataBuffers[i] = _pDevice->newBuffer(sizeof(FrameData), storageMode);
        m_frameDataBuffers[i]->setLabel( AAPLSTR( "FrameData" ) );
    }

    MTL::Library* pShaderLibrary = _pDevice->newDefaultLibrary();
    #pragma mark Sky render pipeline setup
    {
        m_pSkyVertexDescriptor = MTL::VertexDescriptor::alloc()->init();
        m_pSkyVertexDescriptor->attributes()->object(VertexAttributePosition)->setFormat( MTL::VertexFormatFloat3 );
        m_pSkyVertexDescriptor->attributes()->object(VertexAttributePosition)->setOffset( 0 );
        m_pSkyVertexDescriptor->attributes()->object(VertexAttributePosition)->setBufferIndex( BufferIndexMeshPositions );
        m_pSkyVertexDescriptor->layouts()->object(BufferIndexMeshPositions)->setStride( 12 );
        m_pSkyVertexDescriptor->attributes()->object(VertexAttributeNormal)->setFormat( MTL::VertexFormatFloat3 );
        m_pSkyVertexDescriptor->attributes()->object(VertexAttributeNormal)->setOffset( 0 );
        m_pSkyVertexDescriptor->attributes()->object(VertexAttributeNormal)->setBufferIndex( BufferIndexMeshGenerics );
        m_pSkyVertexDescriptor->layouts()->object(BufferIndexMeshGenerics)->setStride( 12 );

        MTL::Function* pSkyboxVertexFunction = pShaderLibrary->newFunction( AAPLSTR( "skybox_vertex" ) );
        MTL::Function* pSkyboxFragmentFunction = pShaderLibrary->newFunction( AAPLSTR( "skybox_fragment" ) );

        if ( !pSkyboxVertexFunction )
            std::cout << "Failed to load skybox_vertex shader" << std::endl;
        
        if ( !pSkyboxFragmentFunction )
            std::cout << "Failed to load skybox_fragment shader" << std::endl;

        MTL::RenderPipelineDescriptor* pRenderPipelineDescriptor = MTL::RenderPipelineDescriptor::alloc()->init();
        pRenderPipelineDescriptor->setLabel( AAPLSTR( "Sky" ) );
        pRenderPipelineDescriptor->setVertexDescriptor( m_pSkyVertexDescriptor );
        pRenderPipelineDescriptor->setVertexFunction( pSkyboxVertexFunction );
        pRenderPipelineDescriptor->setFragmentFunction( pSkyboxFragmentFunction );
        pRenderPipelineDescriptor->setDepthAttachmentPixelFormat( MTL::PixelFormat::PixelFormatDepth32Float_Stencil8 );
        pRenderPipelineDescriptor->setStencilAttachmentPixelFormat( MTL::PixelFormat::PixelFormatDepth32Float_Stencil8 );
        m_pSkyboxPipelineState = _pDevice->newRenderPipelineState( pRenderPipelineDescriptor, &pError );
        AAPL_ASSERT_NULL_ERROR( pError, "Failed to create skybox render pipeline state:" );
        pRenderPipelineDescriptor->release();
        pSkyboxVertexFunction->release();
        pSkyboxFragmentFunction->release();
    }
    {
        m_skyMesh = makeSphereMesh(_pDevice, *m_pSkyVertexDescriptor, 20, 20, 150.0 );
    }
}

void GameCoordinator::drawSky( MTL::RenderCommandEncoder* pRenderEncoder )
{
    pRenderEncoder->pushDebugGroup( AAPLSTR( "Draw Sky" ) );
    pRenderEncoder->setRenderPipelineState( m_pSkyboxPipelineState );
    pRenderEncoder->setCullMode( MTL::CullModeFront );
    pRenderEncoder->setVertexBuffer( m_frameDataBuffers[m_frameDataBufferIndex], 0, BufferIndexFrameData );
    pRenderEncoder->setFragmentTexture( m_pSkyMap, TextureIndexBaseColor );
    for (auto& meshBuffer : m_skyMesh.vertexBuffers())
    {
        pRenderEncoder->setVertexBuffer(meshBuffer.buffer(),
                                        meshBuffer.offset(),
                                        meshBuffer.argumentIndex());
    }


    for (auto& submesh : m_skyMesh.submeshes())
    {
        pRenderEncoder->drawIndexedPrimitives(submesh.primitiveType(),
                                              submesh.indexCount(),
                                              submesh.indexType(),
                                              submesh.indexBuffer().buffer(),
                                              submesh.indexBuffer().offset() );
    }
    pRenderEncoder->popDebugGroup();
}

void GameCoordinator::setupCamera()
{
    // Configurer une caméra perspective
    _camera.initPerspectiveWithPosition(
        {0.0f, 0.0f, 5.0f},  // position
        {0.0f, 0.0f, -1.0f}, // direction
        {0.0f, 1.0f, 0.0f},  // up
        M_PI / 3.0f,         // viewAngle (60 degrés - 3.0f)
        1.0f,                // aspectRatio
        0.1f,                // nearPlane
        100.0f               // farPlane
    );
}

void GameCoordinator::buildCubeBuffers()
{
    _pCubeVertexBuffer = _pDevice->newBuffer(cubeVertices, sizeof(cubeVertices), MTL::ResourceStorageModeShared);
    _pCubeIndexBuffer = _pDevice->newBuffer(cubeIndices, sizeof(cubeIndices), MTL::ResourceStorageModeShared);
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

    _rotationAngle += 0.008f;
    if (_rotationAngle > 2 * M_PI)
    {
        _rotationAngle -= 2 * M_PI;
    }
    Uniforms* pUniforms = (Uniforms *)_pUniformBuffer->contents();
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
