#include "timeUtils.h"

#ifdef TIME_UTILS_TEST_ENABLED

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

UART_HandleTypeDef *timeUtils_test_uart;

static size_t timeUtils_strlen(const char *str);

void timeUtils_debug(const char *format, ...);

#endif

static volatile uint32_t timeUtils_ticks = 0;
static volatile uint32_t *timeUtils_timerCNT_Ptr = NULL;
static uint32_t timeUtils_maxCnt = UINT16_MAX - 1000;
static TIM_HandleTypeDef *timeUtils_timer = NULL;
static uint32_t timeUtils_counterPeriod = UINT16_MAX;

timeUtils_StatusTypeDef timeUtils_init(TIM_HandleTypeDef *timer) {
    uint32_t counterPeriod;
    if ((timer != NULL) && (IS_TIM_INSTANCE(timer->Instance))) {
        counterPeriod = timer->Instance->ARR;
        timeUtils_timer = timer;
        timeUtils_timerCNT_Ptr = (uint32_t *) &(timeUtils_timer->Instance->CNT);
        if (counterPeriod > 0) {
            timeUtils_counterPeriod = counterPeriod;
            timeUtils_maxCnt = (counterPeriod * 197) / 200; //  subtracting 1.5 % of the ARR as a safety margin to make
                                                            //  sure that overflow doesn't happen when delay function is
                                                            //  executing
            timeUtils_ticks = 0;
            timeUtils_timer->Instance->PSC = (HAL_RCC_GetHCLKFreq() / 1000000) - 1;
            timeUtils_timer->Instance->SR &= ~(1);  // resetting the first bit (UIF) to avoid interrupt occurrence immediately
            timeUtils_timer->Instance->EGR = TIM_EGR_UG;


            if (HAL_TIM_Base_Start_IT(timeUtils_timer) == HAL_OK)
                return TIME_UTILS_OK;
        }
    }
    return TIME_UTILS_ERROR;
}

TIM_TypeDef *timeUtils_getTimerInstance(void) {
    return timeUtils_timer->Instance;
}

void timeUtils_ISR(void) {
    timeUtils_ticks += timeUtils_counterPeriod;
}

volatile uint32_t timeUtils_getMicros(void) {
    return timeUtils_ticks + *timeUtils_timerCNT_Ptr;
}

void timeUtils_delayMicros(const uint32_t delay) {
    uint32_t wait = delay;
    uint32_t start;
    uint32_t cnt;
    uint32_t tmpWait;

    do {
        start = timeUtils_getMicros();
        tmpWait = wait;

        if (tmpWait > timeUtils_maxCnt)
            tmpWait = timeUtils_maxCnt;

        //TODO
        // ---------  NOT THREAD SAFE!!! FROM HERE --------------- //
        cnt = *timeUtils_timerCNT_Ptr;
        if ((cnt > timeUtils_maxCnt) || (tmpWait > (timeUtils_maxCnt - cnt))) {
            *timeUtils_timerCNT_Ptr = 0U;
            //TODO
            // --------- TO HERE ------------ //
            timeUtils_ticks += cnt;  // this is thread safe because the previous statement prevents the timer to overflow
                                     // and as result no interrupt will occur
        }

        while (timeUtils_deltaTime32(timeUtils_getMicros(), start) < tmpWait);
        wait -= tmpWait;
    } while (wait != 0);
}

void timeUtils_delayMillis(const uint32_t delay) {
    uint32_t wait = (delay > ((uint32_t) 4294967U)) ? UINT32_MAX : (delay * 1000);
    timeUtils_delayMicros(wait);
}

uint32_t timeUtils_deltaTime16(const uint32_t now, const uint32_t before) {
    return (before <= now) ? (now - before) : (UINT16_MAX - before + now);
}

uint32_t timeUtils_deltaTime32(const uint32_t now, const uint32_t before) {
    return (before <= now) ? (now - before) : (UINT32_MAX - before + now);
}

#ifdef TIME_UTILS_TEST_ENABLED

static size_t timeUtils_strlen(const char *str) {
    size_t len = 0;
    while (*(str + len) != '\0')
        len++;
    return len;
}

void timeUtils_test(TIM_HandleTypeDef *test_timer, UART_HandleTypeDef *test_uart) {
    timeUtils_test_uart = test_uart;
    timeUtils_debug("starting...\n");
    test_timer->Instance->PSC = (SystemCoreClock / 1000000) - 1;
    test_timer->Instance->EGR = TIM_EGR_UG;
    HAL_TIM_Base_Start(test_timer);


    uint32_t maxDiff;
    uint32_t maxError;
    uint32_t diffSum;
    uint32_t diffErSum;
    uint32_t testIterations = 100;

    uint32_t start;
    uint32_t end;
    uint32_t realMicros;

    maxDiff = 0;
    maxError = 0;
    diffSum = 0;
    diffErSum = 0;

    for (uint32_t i = 0; i < testIterations; i++) {
        uint32_t randomNum = (random() % 100);
        uint32_t wait = randomNum;
        if (wait == 0) wait = 1;
        test_timer->Instance->CNT = 0;
        start = timeUtils_getMicros();
        timeUtils_delayMicros(wait);
        end = timeUtils_getMicros();
        realMicros = test_timer->Instance->CNT;
        uint32_t diff = (realMicros > wait) ? (realMicros - wait) : (wait - realMicros);
        uint32_t error = (diff * 1000) / wait;

        if (error >= 1) {
            timeUtils_debug(
                    "[D] i: %lu, diff: %lu,\nwait: %lu, micros: %lu, real micros: %lu, error: %lu /1000,\nstart: %lu, end: %lu\n"
                    "--------------\n", i,
                    diff, wait,
                    timeUtils_deltaTime32(end, start),
                    realMicros,
                    error, start, end);
        }

        if (diff > maxDiff)
            maxDiff = diff;

        if (error > maxError)
            maxError = error;

        diffSum += diff;
        diffErSum += error;
    }

    timeUtils_debug(
            "3] Testing delayMicros()\nmax diff: %lu, max error : %lu /1000\n"
            "diff avg: %lu, diff error avg: %lu /1000\n"
            "--------------\n",
            maxDiff, maxError,
            diffSum / testIterations,
            diffErSum / testIterations);

    timeUtils_debug("3 Phases of tests done.\n");
    HAL_Delay(100000);

}

void timeUtils_debug(const char *format, ...) {
    uint32_t len;
    char tmpStr[300];
    va_list arguments;

    va_start(arguments, format);
    len = vsnprintf(tmpStr, timeUtils_strlen(format), format, arguments);
    va_end(arguments);

    HAL_UART_Transmit(timeUtils_test_uart, (uint8_t *) tmpStr, len, 100);
}

#endif