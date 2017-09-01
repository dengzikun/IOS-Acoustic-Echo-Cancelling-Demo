//
//  NetworkManager.m
//  Test_AEC
//
//  Created by ddp on 16/7/4.
//  Copyright © 2016年 ddp. All rights reserved.
//

#import "NetworkManager.h"
#import "NetworkProcess.hpp"

NetworkManager *network;


@interface NetworkManager(){
    unique_ptr<NetworkProcess> process;
    CBuffer *_buffer;
}

@end

@implementation NetworkManager


-(void)startWithTargetIP:(NSString *)targetIp{
    string ip = [targetIp UTF8String];
    process.reset(new NetworkProcess(ip));
}

-(void)stop{
    process.reset();
}

-(void)dealloc{
    [self stop];
}


-(uint16_t)getWritePointer:(uint8_t**)returnedWritePointer{
    return process->getWritePointer(returnedWritePointer);
}

-(void)didWriteLength:(uint16_t)length{
    process->didWriteLength(length);
}

-(uint16_t)getReadPointer:(uint8_t**)returnedReadPointer{
    return process->getReadPointer(returnedReadPointer);
}

-(void)didReadLength:(uint16_t)length{
    process->didReadLength(length);
}


-(uint32_t) getTotalDrop{
    if(!process)return 0;
    return process->GetTotalDrop();
}

-(double)getLostRate{
    if(!process)return 0;
    return process->GetLostRate();
}

-(uint32_t) getTotalRcvCount{
    if(!process)return 0;
    return process->GetTotalRcvCount();
}

@end
