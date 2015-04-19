//
//  TestResults.m
//  Hermes
//
//  Created by Nicolas Broeking on 4/14/15.
//  Copyright (c) 2015 NicolasBroeking. All rights reserved.
//

#import "TestResults.h"

@implementation TestResults
@synthesize dns, latency, packetloss, throughputUpload, throughputDownload, packetlossUnderLoad, latencyUnderLoad, mobileIdentifier, valid, routerIdentifier;

-(instancetype)init :(NSDictionary*)json{
    if( self = [super init]){
        
        @try {
            dns = [[json objectForKey:@"dns_respons_avg"] doubleValue];
            latency = [[json objectForKey:@"latency_avg"] doubleValue];
            throughputDownload = [[json objectForKey:@"download_throughputs_avg"] doubleValue];
            throughputUpload = [[json objectForKey:@"upload_throughputs_avg"] doubleValue];
            packetloss = [[json objectForKey:@"packet_loss"] doubleValue];
            packetlossUnderLoad = [[json objectForKey:@"packet_loss_under_load"] doubleValue];
            latencyUnderLoad = [[json objectForKey:@"download_latencies_avg"] doubleValue];
            valid = true;
        }
        @catch (NSException *exception) {
            self = [self init];
        }
    
        NSLog(@"Results are: ");
        [self print];
    }
    return self;
}
-(instancetype)init{
    if (self = [super init]) {
        
        valid = false;
        dns = 0.0;
        latency = 0.0;
        packetloss = 0.0;
        throughputUpload = 0.0;
        throughputDownload = 0.0;
        latencyUnderLoad = 0.0;
        packetlossUnderLoad = 0.0;
    }
    return self;
}

-(void) print {
    NSLog( @"%@",[[NSString alloc] initWithFormat:@"state=finished&dns_response_avg=%f&packet_loss=%f&latency_avg=%f&upload_throughputs=%f&download_throughputs=%f&packet_loss_under_load=%f&throughput_latency=%f",dns,packetloss,latency,throughputUpload, throughputDownload, packetlossUnderLoad, latencyUnderLoad]);
}
-(NSString *)getPost{
    
    if( !valid){
        return @"state=error";
    }
    
    return [[NSString alloc] initWithFormat:@"state=finished&dns_response_avg=%f&packet_loss=%f&latency_avg=%f&upload_throughputs=%f&download_throughputs=%f&packet_loss_under_load=%f&throughput_latency=%f",dns,packetloss,latency,throughputUpload, throughputDownload, packetlossUnderLoad, latencyUnderLoad];
    
}
@end
