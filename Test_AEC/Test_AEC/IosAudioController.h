//
//  IosAudioController.h
//  Test_AEC
//
//  Created by ddp on 16/6/28.
//  Copyright © 2016年 ddp. All rights reserved.
//

#import <Foundation/Foundation.h>
#import<AudioToolbox/AudioToolbox.h>

@interface IosAudioController : NSObject

-(void)start;
-(void)stop;

-(AudioComponentInstance)getAudioUnit;

-(BOOL)isAECOn;
-(void)setAECOn: (BOOL)isOn;

-(BOOL)isAGCOn;
-(void)setAGCOn: (BOOL)isOn;

@end
