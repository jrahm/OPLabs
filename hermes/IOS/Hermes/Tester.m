//
//  Tester.m
//  Hermes
//
//  Created by Nicolas Charles Herbert Broeking on 1/22/15.
//  Copyright (c) 2015 NicolasBroeking. All rights reserved.
//

#import "Tester.h"
#import "TestState.h"
#import "Communication.h"
#import "TestSettings.h"
#import "PerformanceTester.h"

@interface Tester()
@property(strong, nonatomic) TestState *stateMachine;
@property(strong, nonatomic) PerformanceTester* tester;

-(void) tearDownRunLoop;
@end

@implementation Tester
@synthesize thread;
@synthesize timer;
@synthesize stateMachine;
@synthesize tester;

+(Tester*) getTester{
    static Tester *sharedInstance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        sharedInstance = [[Tester alloc] init];
    });
    return sharedInstance;
}
//Set all our values to false or nil on creation
-(instancetype)init
{
    if( self = [super init])
    {
        started = false;
        shouldRun = false;
        thread = nil;
        timer = nil;
        stateMachine = [TestState getStateMachine];
        
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(runTest:) name:@"NotifyStartTest" object:nil];
    }
    return self;
}

//Request to start a test
-(void)startTest
{
    [stateMachine setState:PREPARING];
    [[Communication getComm] startTest];
}


//Main Thread Run Loop
-(void)threadMain
{
    NSLog(@"Tester Start");
    //Every time an event loop is kicked we exit our run loop. So we want to make sure we reenter it every time.
    //RunMode is a blocking call until a system event happens
    
    
    //Add a mach port to prevent the run loop from closing if there is no event
    //I know this is ugly but the documentation says there is no other way
    NSPort *port = [NSMachPort port];
    [[NSRunLoop currentRunLoop] addPort:port forMode:NSDefaultRunLoopMode];
    
    while(shouldRun)
    {
        [[NSRunLoop currentRunLoop] runMode: NSDefaultRunLoopMode beforeDate:[NSDate distantFuture]];
    }
    @synchronized(self){
        
        //Reseting member variables
        thread = nil;
        started = false;
        shouldRun = false;
    }
    NSLog(@"Tester Thread Closed");
}

//Start the Communication thread
-(void)start{
    
    @synchronized(self){
        
        if( started)
        {
            NSLog(@"Tester already started");
            return;
        }
        shouldRun = true;
        started = true;
        
        //Start the run loop
        thread = [[NSThread alloc] initWithTarget:self selector:@selector(threadMain) object:nil];
        [thread setName:@"Tester"];
        [thread start];
    }
}

//Request the subsystem to be stopped no matter what
-(void) forceStop{
    @synchronized(self){
        if(!started)
        {
            NSLog(@"Thread is already stopped");
            return;
        }
        
        shouldRun = false;
        [self performSelector:@selector(tearDownRunLoop) onThread:thread withObject:nil waitUntilDone:false];
    }

}
//Kill the communication thread cleanly
-(void)stop{
    @synchronized(self){
        if(!started)
        {
            NSLog(@"Thread is already stopped");
            return;
        }
        
        if( [stateMachine getState] != IDLE){
            NSLog(@"We cant shut down the tester because we are running a test");
            return;
        }
        shouldRun = false;
        [self performSelector:@selector(tearDownRunLoop) onThread:thread withObject:nil waitUntilDone:false];
    }
}

//We want to run a test on our own threads so
-(void) runTest :(NSNotification*) notification{
    NSLog(@"Starting a test");
    [self performSelector:@selector(runTestHelper:) onThread:thread withObject:notification waitUntilDone:NO];
    
}
//Called on the tester thread to run a test
-(void) runTestHelper :(NSNotification*) notification
{
    TestSettings *settings = (TestSettings*)[notification object];
    if( settings == NULL){
        NSLog(@"Tester doesn't have settings");
        return;
    }
    NSLog(@"Results ID = %d", (int)[settings mobileResultID]);
    
    tester = [[PerformanceTester alloc] init:settings];
    
    [tester runTests: self];
}

//Called by the subsystem when a test is complete
-(void)testComplete: (TestResults*)results :(TestSettings*)settings{
    //Once we have the reslts we need to report them
    
    NSLog(@"testComplete in Tester");
    [stateMachine setMobileResults:results];
    
    NSMutableDictionary *allInfo = [[NSMutableDictionary alloc] init];
    
    [allInfo setValue:results forKey:@"results"];
    [allInfo setValue:settings forKey:@"settings"];
    
    NSLog(@"Completed a Performance test");
    
    [results print];
    
    [[TestState getStateMachine] setState:COMPLETED];
    [[NSNotificationCenter defaultCenter] postNotificationName:@"TestComplete" object:results userInfo:allInfo];
}

//This method is used to add something to the run loops queue.
//This method purposly does nothing except wake up the run loop
//To give it a chance to exit
-(void)tearDownRunLoop
{
    NSLog(@"Trying to kill tester");
    //When this method exits the run loop will close
}

//Returns is running under a NSMutex
-(BOOL)isRunning
{
    @synchronized(self)
    {
        return started;
    }
}
@end
