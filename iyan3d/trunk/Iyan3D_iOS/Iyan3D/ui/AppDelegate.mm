//
//  AppDelegate.m
//  Iyan3D
//
//  Created by Harishankar Narayanan on 03/01/14.
//  Copyright (c) 2014 Smackall Games. All rights reserved.
//

#import "AppDelegate.h"

#include "SGEngineCommon.h"
#include "SGEngineOGL.h"
#include "SGEngineMTL.h"
#include <SafariServices/SafariServices.h>
#include "Utility.h"
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/machine.h>

#include <sys/time.h>

SceneManager *scenemgr;
@interface AppDelegate ()

@property(nonatomic, assign) BOOL okToWait;
@property(nonatomic, copy) void (^dispatchHandler)(GAIDispatchResult result);

@end

static NSString *const kTrackingId = @"UA-49819146-1";
static NSString *const kAllowTracking = @"allowTracking";
static NSString *const kClient = @"328259754555-buqbocp0ehq7mtflh0lk3j2p82cc4ltm.apps.googleusercontent.com";

@implementation AppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    [FIRApp configure];
    
    NSDictionary *appDefaults = @{kAllowTracking: @(YES)};
    [[NSUserDefaults standardUserDefaults] registerDefaults:appDefaults];
    [GAI sharedInstance].optOut = ![[NSUserDefaults standardUserDefaults] boolForKey:kAllowTracking];
    [GAI sharedInstance].dispatchInterval = 20;
    [GAI sharedInstance].trackUncaughtExceptions = YES;
    self.tracker = [[GAI sharedInstance] trackerWithTrackingId:kTrackingId];

    [[UIApplication sharedApplication] setStatusBarStyle:UIStatusBarStyleLightContent];
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
    loadingViewController = [[LoadingViewControllerPad alloc] initWithNibName:@"LoadingViewControllerPad" bundle:nil];
    
    if([Utility IsPadDevice]) {
        [[UIApplication sharedApplication] setStatusBarHidden:NO];
    } else {
        [[UIApplication sharedApplication] setStatusBarHidden:YES];
    }

    [self.window setRootViewController:loadingViewController];
    [self.window makeKeyAndVisible];
    
    return YES;
}

- (void)application:(UIApplication*)application didReceiveRemoteNotification:(NSDictionary*)userInfo
{
    NSLog(@"Received notification: %@", userInfo);
    [[NSNotificationCenter defaultCenter] postNotificationName:@"renderCompleted" object:nil];
}

- (void)application:(UIApplication *)application didRegisterUserNotificationSettings:(UIUserNotificationSettings *)notificationSettings
{
    [[UIApplication sharedApplication] registerForRemoteNotifications];
}

- (void)application:(UIApplication *)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken
{
    NSString * deviceTokenString = [[[[deviceToken description]
                                      stringByReplacingOccurrencesOfString: @"<" withString: @""]
                                     stringByReplacingOccurrencesOfString: @">" withString: @""]
                                    stringByReplacingOccurrencesOfString: @" " withString: @""];
    NSUserDefaults* standardUserDefaults = [NSUserDefaults standardUserDefaults];
    
    if (standardUserDefaults) {
        [standardUserDefaults setObject:deviceTokenString forKey:@"deviceToken"];
        [standardUserDefaults synchronize];
    }
}

- (void)application:(UIApplication *)application didFailToRegisterForRemoteNotificationsWithError:(NSError *)error
{
    NSLog(@" Error %@ ", error.localizedDescription);
}

-(BOOL) initEngine:(int)type ScreenWidth:(float)width ScreenHeight:(float)height ScreenScale:(float)screenScale renderView:(UIView*) view
{
    scenemgr = new SceneManager(width, height, screenScale, (DEVICE_TYPE)type, [[[NSBundle mainBundle] resourcePath] UTF8String], (__bridge void*)view);
    if(!scenemgr)
        return false;
    return true;
}

+(AppDelegate *)getAppDelegate
{
    return (AppDelegate*)[UIApplication sharedApplication].delegate;
}

-(void*) getSceneManager
{
    return scenemgr;
}

-(bool) isMetalSupported
{
    size_t size;
    cpu_type_t type;
    cpu_subtype_t subtype;
    size = sizeof(type);
    sysctlbyname("hw.cputype", &type, &size, NULL, 0);
    
    size = sizeof(subtype);
    sysctlbyname("hw.cpusubtype", &subtype, &size, NULL, 0);
    if(subtype == CPU_SUBTYPE_ARM64_V8)
        return true;
    return false;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    [[NSNotificationCenter defaultCenter] postNotificationName:@"AppGoneBackground" object:nil userInfo:nil];
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    [self sendHitsInBackground];
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    [GAI sharedInstance].optOut = ![[NSUserDefaults standardUserDefaults] boolForKey:kAllowTracking];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"applicationDidBecomeActive" object:nil userInfo:nil];
}

- (void)sendHitsInBackground
{
    self.okToWait = YES;
    __weak AppDelegate *weakSelf = self;
    __block UIBackgroundTaskIdentifier backgroundTaskId =
    [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{
        weakSelf.okToWait = NO;
    }];
    
    if (backgroundTaskId == UIBackgroundTaskInvalid) {
        return;
    }
    
    self.dispatchHandler = ^(GAIDispatchResult result) {
        // If the last dispatch succeeded, and we're still OK to stay in the background then kick off
        // again.
        if (result == kGAIDispatchGood && weakSelf.okToWait ) {
            [[GAI sharedInstance] dispatchWithCompletionHandler:weakSelf.dispatchHandler];
        } else {
            [[UIApplication sharedApplication] endBackgroundTask:backgroundTaskId];
        }
    };
    [[GAI sharedInstance] dispatchWithCompletionHandler:self.dispatchHandler];
}

- (void)applicationWillTerminate:(UIApplication *)application
{
}

- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
{
    NSString* ext = [[[url absoluteString] pathExtension] lowercaseString];
    NSString* msg = @"There was a problem in loading the file you just imported.";

    NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* documentsDirectory = [paths objectAtIndex:0];
    NSFileManager *fm = [NSFileManager defaultManager];

    if([ext isEqualToString:@"zip"]) {
        
        ZipArchive *zip = [[ZipArchive alloc] init];
        if([zip UnzipOpenFile:[url path]]) {
            if([zip UnzipFileTo:documentsDirectory overWrite:YES]) {
                msg = @"Your file was imported successfully, Please use import option in the Add Menu to import the file into your scene.";
                [fm removeItemAtPath:[url path] error:nil];
            }
        }
        [zip UnzipCloseFile];

    } else if([ext isEqualToString:@"obj"] || [ext isEqualToString:@"3ds"] || [ext isEqualToString:@"fbx"] || [ext isEqualToString:@"dae"] || [ext isEqualToString:@"png"] || [ext isEqualToString:@"jpg"] || [ext isEqualToString:@"jpeg"] || [ext isEqualToString:@"tga"] || [ext isEqualToString:@"bmp"]) {
        msg = @"Your file was imported successfully, Please use import option in the Add Menu to import the file into your scene.";
    }
    
	UIAlertView *message = [[UIAlertView alloc]initWithTitle:@"Information" message:msg delegate:self cancelButtonTitle:@"Ok" otherButtonTitles:nil];
	[message performSelectorOnMainThread:@selector(show) withObject:nil waitUntilDone:YES];

	return YES;
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
}

-(BOOL)iPhone6Plus
{
    if (([UIScreen mainScreen].scale > 2.0)) return YES;
    return NO;
}

@end
