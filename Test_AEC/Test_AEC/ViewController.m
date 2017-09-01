//
//  ViewController.m
//  Test_AEC
//
//  Created by ddp on 16/6/28.
//  Copyright © 2016年 ddp. All rights reserved.
//

#import "ViewController.h"
#import "IosAudioController.h"
#import "IPAddress.h"

#import "NetworkManager.h"

@interface ViewController ()

@property (weak, nonatomic) IBOutlet UILabel *labelIP;
@property (weak, nonatomic) IBOutlet UITextField *targetIP;
@property (weak, nonatomic) IBOutlet UISwitch *AECSwitch;

@property (weak, nonatomic) IBOutlet UILabel *lostRateLabel;

@property(strong, nonatomic) IosAudioController *iosAudio;

-(void) timerDidTick:(NSTimer*) theTimer;

@end



@implementation ViewController



- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    
//    UITapGestureRecognizer *singleFingerTap =
//    [[UITapGestureRecognizer alloc] initWithTarget:self
//                                            action:@selector(handleSingleTap:)];
//    [self.view addGestureRecognizer:singleFingerTap];
    
    network = [[NetworkManager alloc]init];
    
    
    char *ip = deviceGetAddress();
    
    self.labelIP.text = [NSString stringWithFormat:@"本机IP: %s", ip];
    self.iosAudio = [[IosAudioController alloc]init];
    [self.AECSwitch setOn:[self.iosAudio isAECOn]];
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)handleSingleTap:(UITapGestureRecognizer *)recognizer {
    [self.view endEditing:YES];
}


- (IBAction)OnStart:(id)sender {
    
    NSString *ip = self.targetIP.text;
    
    [network startWithTargetIP:ip];
    
    [self.iosAudio start];
    NSTimer *timer = [NSTimer timerWithTimeInterval:2.0 target:self selector:@selector(timerDidTick:) userInfo:nil repeats:YES];
    
    [[NSRunLoop mainRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
}



- (IBAction)OnStop:(id)sender {
    [self.iosAudio stop];
    [network stop];
}



- (IBAction)OnAECSwitch:(id)sender {
    
    UISwitch *switchButton = (UISwitch*)sender;
    [self.iosAudio setAECOn:[switchButton isOn]];
}


-(void) timerDidTick:(NSTimer*) theTimer{
    self.lostRateLabel.text = [NSString stringWithFormat:@"丢包率: %.2f%% 总丢包数: %u Rcv: %u", [network getLostRate] * 100, [network getTotalDrop], [network getTotalRcvCount]];
}

@end
