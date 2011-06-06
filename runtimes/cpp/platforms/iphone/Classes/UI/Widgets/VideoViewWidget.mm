/* Copyright (C) 2011 MoSync AB
 
 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License, version 2, as published by
 the Free Software Foundation.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; see the file COPYING.  If not, write to the Free
 Software Foundation, 59 Temple Place - Suite 330, Boston, MA
 02111-1307, USA.
 */

// default rectangle values for the video player
#define DEFAULT_RECT_X        0
#define DEFAULT_RECT_Y        0
#define DEFAULT_RECT_WIDTH  100
#define DEFAULT_RECT_HEIGHT 100

#import "VideoViewWidget.h"
#include <helpers/CPP_IX_WIDGET.h>
#include <helpers/cpp_defs.h>
#include "Platform.h"

@implementation VideoViewWidget

/**
 * Init function.
 */
- (id)init {
    if(!view) {
        moviePlayerController = [[MPMoviePlayerController alloc] init];
        CGRect viewRect =CGRectMake(DEFAULT_RECT_X, DEFAULT_RECT_Y, DEFAULT_RECT_WIDTH, DEFAULT_RECT_HEIGHT);
        UIView* smallView = [[[UIView alloc] initWithFrame:viewRect] retain];
        
        [moviePlayerController.view setFrame: smallView.bounds];
        view = smallView;
        [view addSubview:moviePlayerController.view];

        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(moviePlaybackComplete:)
                                                     name:MPMoviePlayerPlaybackDidFinishNotification
                                                   object:moviePlayerController];
        
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(moviePlaybackStateChanged:)
                                                     name:MPMoviePlayerPlaybackStateDidChangeNotification
                                                   object:moviePlayerController];
        
    }

    return [super init];
}

/**
 * Release the objects and remove the observers.
 */
- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:MPMoviePlayerPlaybackDidFinishNotification
                                                  object:moviePlayerController];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self
                                                    name:MPMoviePlayerPlaybackStateDidChangeNotification
                                                  object:moviePlayerController];
    [super dealloc];
}

/**
 * Sets a video property.
 * @param key The property of the video widget that should be set.
 * @param value The value of the property.
 * @return MAW_RES_OK if the property was set, or an error code otherwise.
 */
- (int)setPropertyWithKey: (NSString*)key toValue: (NSString*)value {
    if([key isEqualToString:@"path"]) {
          // TO DO handle local video files
        NSString *filepath   =   [[NSBundle mainBundle] pathForResource:@"mymovie" ofType:@"m4v"];
        NSLog(@"File path = %@", filepath);
            if(filepath) {
                NSURL    *fileURL    =   [NSURL fileURLWithPath:filepath];
                [moviePlayerController setContentURL:fileURL];
            }
        } else if([key isEqualToString:@"url"]) {
        NSURL    *webURL    =   [NSURL URLWithString:value]; 
        [moviePlayerController setContentURL:webURL];
    } else if([key isEqualToString:@"action"]) {
        [self handleAction:value];
    }  else {
        return [super setPropertyWithKey:key toValue:value];;
    }
    
    return MAW_RES_OK;	
}

/**
 * Returns a property value of the video widget.
 * @param key The property of the video widget. 
 * @return The value for the given property.
 */
- (NSString*)getPropertyWithKey: (NSString*)key {
	if([key isEqualToString:@"duration"]) {
        // return the maximum value for the slider
        return[[NSString alloc] initWithFormat:@"%f", [moviePlayerController duration]];
	} else {
		return [super getPropertyWithKey:key];
	}
}

/**
 * Handle and video widget action(play, pause or stop the video).
 * @param One of the VideoWidgetAction.
 */
-(void)handleAction:(NSString*) value {
    int newStateValue = [value intValue];
    switch (newStateValue) {
        case MAW_VIDEO_WIDGET_ACTION_PLAY:
            [moviePlayerController play];
            break;
        case MAW_VIDEO_WIDGET_ACTION_PAUSE:
            [moviePlayerController pause];
            break;
        case MAW_VIDEO_WIDGET_ACTION_STOP:
            [moviePlayerController stop];
            break;    
        default:
            break;
    }
}

/**
 * This delegate method is invoked when the movie has finished playing.
 * @param The current movie player object.
 */
- (void)moviePlaybackComplete:(NSNotification *)notification {
    // check if an error occurred
    NSError *error = [[notification userInfo] objectForKey:@"error"];
    if(!error) {
        
        // send the event
        MAEvent event;
        event.type = EVENT_TYPE_WIDGET;
        
        MAWidgetEventData *eventData = new MAWidgetEventData;
        eventData->eventType = MAW_EVENT_VIDEO_STATE_CHANGED;
        eventData->videoWidgetInfo = MAW_VIDEO_WIDGET_FINISHED;
        eventData->widgetHandle = handle;
        
        event.data = (int)eventData;
        Base::gEventQueue.put(event);
    }
}

/**
 * This delegate method is invoked when the playback state has changed.
 * @param The current movie player object.
 */
- (void)moviePlaybackStateChanged:(NSNotification *)notification {
    // check if an error occurred
    NSError *error = [[notification userInfo] objectForKey:@"error"];
    if(!error) {
        
        int playbackStateEvent = 0;
        BOOL sendEvent = TRUE;
        MPMoviePlaybackState playbackState = [moviePlayerController playbackState];
        
        // check the type of the event(only the events from VideoWidgetEvent should be handled)
        switch (playbackState) {
            case MPMoviePlaybackStatePlaying:
                playbackStateEvent = MAW_VIDEO_WIDGET_PLAYING;
                break;
            case MPMoviePlaybackStatePaused:
                playbackStateEvent = MAW_VIDEO_WIDGET_PAUSED;
                break;
            case MPMoviePlaybackStateStopped:
                playbackStateEvent = MAW_VIDEO_WIDGET_STOPPED;
                break;
            case MPMoviePlaybackStateInterrupted:
                playbackStateEvent = MAW_VIDEO_WIDGET_INTERRUPTED;
                break;    
            default:
                //don't handle the other playback states
                sendEvent = FALSE;
                break;
        }
        
        if(sendEvent) {
            // send the event
            MAEvent event;
            event.type = EVENT_TYPE_WIDGET;
        
            MAWidgetEventData *eventData = new MAWidgetEventData;
            eventData->eventType = MAW_EVENT_VIDEO_STATE_CHANGED;
            eventData->widgetHandle = handle;
            eventData->videoWidgetInfo = playbackStateEvent;
            
            event.data = (int)eventData;
            Base::gEventQueue.put(event);
        }
    }
}

@end
