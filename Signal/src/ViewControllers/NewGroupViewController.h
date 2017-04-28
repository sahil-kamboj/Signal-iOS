//
//  Copyright (c) 2017 Open Whisper Systems. All rights reserved.
//

#import "OWSConversationSettingsViewDelegate.h"

NS_ASSUME_NONNULL_BEGIN

@interface NewGroupViewController : UIViewController

@property (nonatomic, weak) id<OWSConversationSettingsViewDelegate> delegate;

@property (nonatomic) BOOL shouldEditGroupNameOnAppear;
@property (nonatomic) BOOL shouldEditAvatarOnAppear;

@end

NS_ASSUME_NONNULL_END
