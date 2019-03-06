#ifndef acromegaly_CONFIG_H
#define acromegaly_CONFIG_H

#define USE_TICK_GENERATOR false
#define DEBUG 1

/** Value used to convert ticks count to more recognizable unit, broadcasted by status service.
 * In this case, it's fraction 1*10e-6, where one unit of multiplied ticks equals 1um.
 */
#define TICK_TO_HEIGHT_MULTI 525

/**
 * 
 */
#define BASE_HEIGHT 834000 

#define TICKS_UPPER_LIMIT 816
#define TICK_LOWER_LIMIT 0

#endif
