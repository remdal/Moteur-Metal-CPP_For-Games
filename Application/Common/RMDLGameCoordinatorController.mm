#include "RMDLGameCoordinatorController.h"
#include <../../Renderer/RMDLGameCoordinator.hpp>

#import <QuartzCore/QuartzCore.h>

#include <memory>

static void* renderWorker( void* _Nullable obj )
{
    pthread_setname_np("RenderThread");
    CAMetalDisplayLink* metalDisplayLink = (__bridge CAMetalDisplayLink *)obj;
    [metalDisplayLink addToRunLoop:[NSRunLoop currentRunLoop]
                           forMode:NSDefaultRunLoopMode];
    [[NSRunLoop currentRunLoop] run];
    return nil;
}

@implementation RMDLGameCoordinatorController
{
    std::unique_ptr< GameCoordinator > _pGameCoordinator;
    CAMetalLayer*                      _metalLayer;
    CAMetalDisplayLink*                _metalDisplayLink;
}

- (nonnull instancetype)initWithMetalLayer:(nonnull CAMetalLayer *)metalLayer gameUICanvasSize:(NSUInteger)gameUICanvasSize
{
    self = [super init];
    if (self)
    {
        _metalLayer = metalLayer;
        NSString* shaderPath = NSBundle.mainBundle.resourcePath;
        _pGameCoordinator = std::make_unique< GameCoordinator >((__bridge MTL::Device *)_metalLayer.device,
                                                                (MTL::PixelFormat)_metalLayer.pixelFormat,
                                                                _metalLayer.drawableSize.width,
                                                                _metalLayer.drawableSize.height,
                                                                gameUICanvasSize,
                                                                shaderPath.UTF8String);
        _metalDisplayLink = [[CAMetalDisplayLink alloc] initWithMetalLayer:_metalLayer];
        _metalDisplayLink.delegate = self;
        _metalDisplayLink.preferredFrameRateRange = CAFrameRateRangeMake(30, 120, 60);
        int res = 0;
        pthread_attr_t attr;
        res = pthread_attr_init( &attr );
        NSAssert( res == 0, @"Unable to initialize thread attributes." );
        res = pthread_attr_setschedpolicy( &attr, SCHED_RR );
        NSAssert( res == 0, @"Unable to set thread attribute scheduler policy." );
        struct sched_param param = { .sched_priority = 45 };
        res = pthread_attr_setschedparam( &attr, &param );
        NSAssert( res == 0, @"Unable to set thread attribute priority." );
        res = pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
        NSAssert( res == 0, @"Unable set set thread attribute to run detached." );
        pthread_t tid;
        res = pthread_create( &tid, &attr, renderWorker, (__bridge void *)_metalDisplayLink );
        NSAssert( res == 0, @"Unable to create render thread" );
        pthread_attr_destroy( &attr );
    }
    return (self);
}

- (void)dealloc
{
    self->_metalDisplayLink = nil;
    _pGameCoordinator.reset();
}

- (void)renderThreadLoop
{
    [_metalDisplayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
    [[NSRunLoop currentRunLoop] run];
}

- (void)metalDisplayLink:(nonnull CAMetalDisplayLink *)link needsUpdate:(nonnull CAMetalDisplayLinkUpdate *)update
{
    @autoreleasepool {
        id<CAMetalDrawable> drawable = update.drawable;
        if (!drawable)
        {
            NSLog(@"No drawable available");
            return ;
        }
        if (_pGameCoordinator)
        {   @try {
                CA::MetalDrawable* metalDrawable = (__bridge CA::MetalDrawable*)drawable;
                _pGameCoordinator->draw(metalDrawable, CACurrentMediaTime());
            } @catch (NSException *exception) {
                NSLog(@"Error in draw: %@", exception);
            }
        } else {
            NSLog(@"Game coordinator is null");
        }
    }
}

- (void)maxEDRValueDidChangeTo:(float)value
{
    _pGameCoordinator->setMaxEDRValue(value);
}

- (void)setBrightness:(float)brightness
{
    _pGameCoordinator->setBrightness(brightness);
}

- (void)setEDRBias:(float)edrBias
{
    _pGameCoordinator->setEDRBias(edrBias);
}

- (void)moveCameraX:(float)x Y:(float)y Z:(float)z
{
    if (_pGameCoordinator) {
        _pGameCoordinator->moveCamera(simd::float3{x, y, z});
    }
}

- (void)rotateCameraYaw:(float)yaw Pitch:(float)pitch
{
    if (_pGameCoordinator) {
        _pGameCoordinator->rotateCamera(yaw, pitch);
    }
}

- (void)updateCameraAspectRatio:(float)aspectRatio
{
    if (_pGameCoordinator) {
        _pGameCoordinator->setCameraAspectRatio(aspectRatio);
    }
}

@end
