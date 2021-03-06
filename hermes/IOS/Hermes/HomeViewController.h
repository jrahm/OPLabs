//
//  HomeViewController.h
//  Hermes
//
//  Created by Sarah Feller on 2/3/15.
//  Copyright (c) 2015 NicolasBroeking. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "CommunicationDelegate.h"
#import "SessionData.h"

@interface HomeViewController : UIViewController <CommunicationDelegate, UIAlertViewDelegate>

@property (strong, nonatomic) SessionData* data;
@property (strong, nonatomic) IBOutlet UIButton *RunTestsButton;
@property (strong, nonatomic) IBOutlet UIButton *AboutTestsButton;

- (IBAction)goToSettings:(id)sender;
-(BOOL) checkLogin;
-(void) appBecameActive;

-(void)startLogin;
-(void)endLogin;

@end
