#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#ifdef __cplusplus
extern "C"{
#endif

#include "main.h"

//#define TIME_UTILS_TEST_ENABLED

typedef enum {
    TIME_UTILS_OK = 0x00U,
    TIME_UTILS_ERROR = 0x01U
} timeUtils_StatusTypeDef;

timeUtils_StatusTypeDef timeUtils_init(TIM_HandleTypeDef *timer);

TIM_TypeDef *timeUtils_getTimerInstance(void);

void timeUtils_ISR(void);

volatile uint32_t timeUtils_getMicros(void);

void timeUtils_delayMicros(uint32_t delay);

void timeUtils_delayMillis(uint32_t delay);

uint32_t timeUtils_deltaTime16(uint32_t now, uint32_t before);

uint32_t timeUtils_deltaTime32(uint32_t now, uint32_t before);

#ifdef TIME_UTILS_TEST_ENABLED

void timeUtils_test(TIM_HandleTypeDef *test_timer, UART_HandleTypeDef  *test_uart);

#endif

#ifdef __cplusplus
}
#endif

#endif //TIME_UTILS_H
