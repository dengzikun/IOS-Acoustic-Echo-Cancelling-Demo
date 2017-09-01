//
//  NetworkManager.h
//  Test_AEC
//
//  Created by ddp on 16/7/4.
//  Copyright © 2016年 ddp. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NetworkManager : NSObject

-(void)startWithTargetIP:(NSString *)targetIp;
-(void)stop;


-(uint16_t)getWritePointer:(uint8_t**)returnedWritePointer;
-(void)didWriteLength:(uint16_t)length;

-(uint16_t)getReadPointer:(uint8_t**)returnedReadPointer;
-(void)didReadLength:(uint16_t)length;



-(double)getLostRate;
-(uint32_t) getTotalDrop;
-(uint32_t) getTotalRcvCount;

@end

extern NetworkManager *network;
