//
//  IPAddress.h
//  Test_AEC
//
//  Created by ddp on 16/6/30.
//  Copyright © 2016年 ddp. All rights reserved.
//

#ifndef IPAddress_h
#define IPAddress_h

#define MAXADDRS 32
extern char *if_names[MAXADDRS];
extern char *ip_names[MAXADDRS];
extern char *hw_addrs[MAXADDRS];
extern unsigned long ip_addrs[MAXADDRS];
void InitAddresses();
void FreeAddresses();
void GetIPAddresses();
void GetHWAddresses();

char* deviceGetAddress();

#endif /* IPAddress_h */
