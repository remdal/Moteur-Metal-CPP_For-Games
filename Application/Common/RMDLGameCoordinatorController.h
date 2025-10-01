#import <QuartzCore/QuartzCore.h>

@interface RMDLGameCoordinatorController : NSObject <CAMetalDisplayLinkDelegate>

- (nonnull instancetype)initWithMetalLayer:(nonnull CAMetalLayer *)metalLayer gameUICanvasSize:(NSUInteger)gameUICanvasSize;

- (void)metalDisplayLink:(nonnull CAMetalDisplayLink *)link needsUpdate:(nonnull CAMetalDisplayLinkUpdate *)update;

- (void)maxEDRValueDidChangeTo:(float)value;

- (void)setBrightness:(float)brightness;

- (void)setEDRBias:(float)edrBias;

@end
