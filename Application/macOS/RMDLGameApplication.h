/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                    +       +                      */
/*      RMDLGameApplication.h                    +++     +++                         */
/*      Yuly_Ook                                     +       +                       */
/*                                                    +       +                      */
/*                                                  +           +                    */
/*      Created by Rémy Gralinger on 01/10/2025.     + + + + + +                     */
/*      Copyright © 2025 Laboitederemdal. All rights reserved.                       */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#import "Cocoa/Cocoa.h"

@interface RMDLGameApplication : NSObject <NSApplicationDelegate, NSWindowDelegate>

- (void)setBrightness:(id)sender;

- (void)setEDRBias:(id)sender;

@end
