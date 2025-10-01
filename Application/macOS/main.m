#import <Cocoa/Cocoa.h>
//#import "GameApplication.h"

int main( int argc, const char * argv[] )
{
    NSApplication* application = [NSApplication sharedApplication];
    application.mainMenu = [[NSMenu alloc] init];
    NSString* bundleName = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleName"];
    NSMenu* appMenu = [[NSMenu alloc] initWithTitle:@""];
    NSMenuItem* appMenuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    [appMenu addItemWithTitle:[@"About " stringByAppendingString:bundleName] action:@selector(orderFrontStandardAboutPanel:) keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:[@"Hide " stringByAppendingString:bundleName] action:@selector(hide:) keyEquivalent:@"h"];
    NSMenuItem* hide_other_item = [appMenu addItemWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"]; hide_other_item.keyEquivalentModifierMask = NSEventModifierFlagOption | NSEventModifierFlagCommand;
    [appMenu addItemWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""];
    [appMenu addItem:[NSMenuItem separatorItem]];
    [appMenu addItemWithTitle:[@"Quit " stringByAppendingString:bundleName] action:@selector(terminate:) keyEquivalent:@"q"];
    appMenuItem.submenu = appMenu;
    [application.mainMenu addItem:appMenuItem];
    NSMenu* windowsMenu = [[NSMenu alloc] initWithTitle:@"Window"];
    NSMenuItem* windowsMenuItem = [[NSMenuItem alloc] initWithTitle:@"" action:nil keyEquivalent:@""];
    [windowsMenu addItemWithTitle:NSLocalizedString(@"Minimize", @"") action:@selector(performMiniaturize:) keyEquivalent:@"m"];
    windowsMenuItem.submenu = windowsMenu;
    [application.mainMenu addItem:windowsMenuItem];
    application.windowsMenu = windowsMenu;

    //GameApplication* gameApplication = [[GameApplication alloc] init];
    //application.delegate             = gameApplication;
    return NSApplicationMain(argc, argv);
}
