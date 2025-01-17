#import <Foundation/Foundation.h>

#import "library/objective-c/EnvoyAliases.h"
#import "library/objective-c/EnvoyConfiguration.h"
#import "library/objective-c/EnvoyEventTracker.h"
#import "library/objective-c/EnvoyHTTPCallbacks.h"
#import "library/objective-c/EnvoyHTTPFilter.h"
#import "library/objective-c/EnvoyHTTPFilterFactory.h"
#import "library/objective-c/EnvoyHTTPStream.h"
#import "library/objective-c/EnvoyKeyValueStore.h"
#import "library/objective-c/EnvoyLogger.h"
#import "library/objective-c/EnvoyNativeFilterConfig.h"
#import "library/objective-c/EnvoyNetworkMonitor.h"
#import "library/objective-c/EnvoyStringAccessor.h"

#import "library/common/types/c_types.h"

NS_ASSUME_NONNULL_BEGIN

#pragma mark - EnvoyEngine

/// Wrapper layer for calling into Envoy's C/++ API.
@protocol EnvoyEngine

/**
 Create a new instance of the engine.

 @param onEngineRunning Closure called when the engine finishes its async startup and begins
 running.
 @param logger Logging interface.
 @param eventTracker Event tracking interface.
 @param networkMonitoringMode Configure how the engines observe network reachability.
 */
- (instancetype)initWithRunningCallback:(nullable void (^)())onEngineRunning
                                 logger:(nullable void (^)(NSString *))logger
                           eventTracker:(nullable void (^)(EnvoyEvent *))eventTracker
                  networkMonitoringMode:(int)networkMonitoringMode;
/**
 Run the Envoy engine with the provided configuration and log level.

 @param config The EnvoyConfiguration used to start Envoy.
 @param logLevel The log level to use when starting Envoy.
 @return A status indicating if the action was successful.
 */
- (int)runWithConfig:(EnvoyConfiguration *)config logLevel:(NSString *)logLevel;

/**
 Run the Envoy engine with the provided yaml string and log level.

 @param yaml The configuration template with which to start Envoy.
 @param config The EnvoyConfiguration used to start Envoy.
 @param logLevel The log level to use when starting Envoy.
 @return A status indicating if the action was successful.
 */
- (int)runWithTemplate:(NSString *)yaml
                config:(EnvoyConfiguration *)config
              logLevel:(NSString *)logLevel;

/**
 Opens a new HTTP stream attached to this engine.

 @param callbacks Handler for observing stream events.
 @param explicitFlowControl Whether explicit flow control will be enabled for the stream.
 */
- (id<EnvoyHTTPStream>)startStreamWithCallbacks:(EnvoyHTTPCallbacks *)callbacks
                            explicitFlowControl:(BOOL)explicitFlowControl;

/**
 Increments a counter with the given count.

 @param elements Elements of the counter stat.
 @param count Amount to add to the counter.
 @return A status indicating if the action was successful.
 */
- (int)recordCounterInc:(NSString *)elements tags:(EnvoyTags *)tags count:(NSUInteger)count;

/**
 Set a gauge of a given string of elements with the given value.

 @param elements Elements of the gauge stat.
 @param value Value to set to the gauge.
 @return A status indicating if the action was successful.
 */
- (int)recordGaugeSet:(NSString *)elements tags:(EnvoyTags *)tags value:(NSUInteger)value;

/**
 Add the gauge with the given string of elements and by the given amount.

 @param elements Elements of the counter stat.
 @param amount Amount to add to the gauge.
 @return A status indicating if the action was successful.
 */
- (int)recordGaugeAdd:(NSString *)elements tags:(EnvoyTags *)tags amount:(NSUInteger)amount;

/**
 Subtract from the gauge with the given string of elements and by the given amount.

 @param elements Elements of the gauge stat.
 @param amount Amount to subtract from the gauge.
 @return A status indicating if the action was successful.
 */
- (int)recordGaugeSub:(NSString *)elements tags:(EnvoyTags *)tags amount:(NSUInteger)amount;

/**
 Add another recorded duration to the timer histogram with the given string of elements.
 @param elements Elements of the histogram stat.
 @param durationMs The duration in milliseconds to record in the histogram distribution
 @return A status indicating if the action was successful.
 */
- (int)recordHistogramDuration:(NSString *)elements
                          tags:(EnvoyTags *)tags
                    durationMs:(NSUInteger)durationMs;

/**
 Add another recorded value to the histogram with the given string of elements.
 @param elements Elements of the histogram stat.
 @param value Amount to record as a new value for the histogram distribution.
 @return A status indicating if the action was successful.
 */
- (int)recordHistogramValue:(NSString *)elements tags:(EnvoyTags *)tags value:(NSUInteger)value;

/**
 Attempt to trigger a stat flush.
 */
- (void)flushStats;

/**
 Retrieve the value of all active stats. Note that this function may block for some time.
 @return The list of active stats and their values, or empty string of the operation failed
 */
- (NSString *)dumpStats;

- (void)terminate;

- (void)resetConnectivityState;

@end

#pragma mark - EnvoyEngineImpl

// Concrete implementation of the `EnvoyEngine` interface.
@interface EnvoyEngineImpl : NSObject <EnvoyEngine>

@property (nonatomic, copy, nullable) void (^onEngineRunning)();

@end

NS_ASSUME_NONNULL_END
