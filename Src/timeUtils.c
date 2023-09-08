
//#define TIME_UTILS_TEST_ENABLED

#include "timeUtils.h"

#ifdef TIME_UTILS_TEST_ENABLED

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

extern UART_HandleTypeDef huart2;

extern TIM_HandleTypeDef htim16;

//extern ADC_HandleTypeDef hadc1;

static size_t strlenn(const char *str);

void timeUtils_debug(const char *format, ...);

void timeUtils_test(void);

#endif

static volatile uint32_t timeUtils_ticks = 0;
static volatile uint32_t *timeUtils_timerCNT_Ptr = NULL;
static uint32_t timeUtils_maxCnt = UINT16_MAX - 1000;
static TIM_HandleTypeDef *timeUtils_timer = NULL;
static uint32_t timeUtils_counterPeriod = UINT16_MAX;

timeUtils_StatusTypeDef timeUtils_Init(TIM_HandleTypeDef *timer) {
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

#ifdef TIME_UTILS_TEST_ENABLED
//            HAL_ADC_Start(&hadc1);
            HAL_Delay(100);
//            uint32_t adcVal = HAL_ADC_GetValue(&hadc1);
            srandom(50);
            timeUtils_debug("starting... adc: %lu\n", 50);
            htim16.Instance->PSC = (SystemCoreClock / 1000000) - 1;
            htim16.Instance->EGR = TIM_EGR_UG;
            HAL_TIM_Base_Start(&htim16);
#endif
            if (HAL_TIM_Base_Start_IT(timeUtils_timer) == HAL_OK)
                return TIME_UTILS_OK;
        }
    }
    return TIME_UTILS_ERROR;
}

TIM_TypeDef *timeUtils_getTimerInstance(void) {
    return timeUtils_timer->Instance;
}

void timeUtils_IncISR(void) {
    timeUtils_ticks += timeUtils_counterPeriod;
}

volatile uint32_t timeUtils_getMicros(void) {
    return timeUtils_ticks + *timeUtils_timerCNT_Ptr;
}

void timeUtils_delayMicros(uint32_t delay) {
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

void timeUtils_delayMillis(uint32_t delay) {
    uint32_t wait = (delay > ((uint32_t) 4294967U)) ? UINT32_MAX : (delay * 1000);
    timeUtils_delayMicros(wait);
}

uint32_t timeUtils_deltaTime16(uint32_t now, uint32_t before) {
    return (before <= now) ? (now - before) : (UINT16_MAX - before + now);
}

uint32_t timeUtils_deltaTime32(uint32_t now, uint32_t before) {
    return (before <= now) ? (now - before) : (UINT32_MAX - before + now);
}

#ifdef TIME_UTILS_TEST_ENABLED

static size_t strlenn(const char *str) {
    size_t len = 0;
    while (*(str + len) != '\0')
        len++;
    return len;
}

void timeUtils_debug(const char *format, ...) {
    uint32_t len;
    char tmpStr[300];
    va_list arguments;

    va_start(arguments, format);
    len = vsnprintf(tmpStr, strlenn(format), format, arguments);
    va_end(arguments);

    HAL_UART_Transmit(&huart2, (uint8_t *) tmpStr, len, 100);
}

void timeUtils_test(void) {
    uint32_t maxDiff;
    uint32_t maxTdiff;
    double maxError;
    double maxTerror;
    double diffSum;
    double tDiffSum;
    double diffErSum;
    double tDiffErSum;
    uint32_t testIterations = 100;

    uint32_t start;
    uint32_t end;
    uint32_t realMicros;

/*
    maxDiff = 0;
    maxTdiff = 0;
    maxError = 0;
    maxTerror = 0;
    diffSum = 0;
    tDiffSum = 0;
    diffErSum = 0;
    tDiffErSum = 0;

    {
        for (uint32_t i = 0; i < testIterations; i++) {
            uint32_t randomNum = random() % 1000000;
            uint32_t wait = randomNum / 1000 + 1;
            htim16.Instance->CNT = 0;
            start = timeUtils_getMicros();
            HAL_Delay(wait);
            end = timeUtils_getMicros();
            realMicros = htim16.Instance->CNT;

            wait *= 1000;
            uint32_t deltaMicros = end - start;
            uint32_t diff = (realMicros > deltaMicros) ? (realMicros - deltaMicros) : (deltaMicros - realMicros);
            uint32_t tdiff = (realMicros > wait) ? (realMicros - wait) : (wait - realMicros);
            double error = (double) diff * 100 / realMicros;
            double tError = (double) tdiff * 100 / wait;
            if (error >= 0.1) {
                timeUtils_debug(
                              "[1] [D] i: %lu, diff: %lu,\nwait: %lu, micros: %lu, real micros: %lu, error: %.3lf %%,\nstart: %lu, end: %lu\n"
                              "--------------\n", i,
                              diff, wait,
                              timeUtils_deltaTime32(end, start),
                              realMicros,
                              error, start, end);

            }
            if (tError >= 0.1) {
                timeUtils_debug(
                              "[1] [T] i: %lu, Tdiff: %lu,\nwait: %lu, micros: %lu, real micros: %lu, error: %.3lf %%,\nstart: %lu, end: %lu\n"
                              "--------------\n", i,
                              tdiff, wait,
                              timeUtils_deltaTime32(end, start),
                              realMicros,
                              tError, start, end);
            }

            if (diff > maxDiff)
                maxDiff = diff;
            if (tdiff > maxTdiff)
                maxTdiff = tdiff;
            if (error > maxError)
                maxError = error;
            if (tError > maxTerror)
                maxTerror = tError;
            diffSum += diff;
            tDiffSum += tdiff;
            diffErSum += error;
            tDiffErSum += tError;

        }

        timeUtils_debug(
                      "1] Testing overall micros using HAL_Delay()\nmax diff: %lu, max Tdiff: %lu, max error: %.3lf %%, max Terror : %.3lf %%\n"
                      "diff avg: %.3lf, tdiff avg: %.3lf, diff error avg: %.3lf %%, tdiff error avg: %.3lf %%\n"
                      "--------------\n",
                      maxDiff, maxTdiff, maxError, maxTerror, diffSum / testIterations, tDiffSum / testIterations, diffErSum / testIterations,
                      tDiffErSum / testIterations);
    }
    HAL_Delay(2000);
    maxDiff = 0;
    maxTdiff = 0;
    maxError = 0;
    maxTerror = 0;
    diffSum = 0;
    tDiffSum = 0;
    diffErSum = 0;
    tDiffErSum = 0;

    {
        for (uint32_t i = 0; i < testIterations; i++) {
            uint32_t randomNum = random() % 1000000;
            uint32_t wait = randomNum / 1000 + 1;
            htim16.Instance->CNT = 0;
            start = timeUtils_getMicros();
            timeUtils_delayMillis(wait);
            end = timeUtils_getMicros();
            realMicros = htim16.Instance->CNT;

            wait *= 1000;
            uint32_t deltaMicros = end - start;
            uint32_t diff = (realMicros > deltaMicros) ? (realMicros - deltaMicros) : (deltaMicros - realMicros);
            uint32_t tdiff = (realMicros > wait) ? (realMicros - wait) : (wait - realMicros);
            double error = (double) diff * 100 / realMicros;
            double tError = (double) tdiff * 100 / wait;
            if (error >= 0.1) {
                timeUtils_debug(
                              "[2] [D] i: %lu, diff: %lu,\nwait: %lu, micros: %lu, real micros: %lu, error: %.3lf %%,\nstart: %lu, end: %lu\n"
                              "--------------\n", i,
                              diff, wait,
                              timeUtils_deltaTime32(end, start),
                              realMicros,
                              error, start, end);

            }
            if (tError >= 0.1) {
                timeUtils_debug(
                              "[2] [T] i: %lu, Tdiff: %lu,\nwait: %lu, micros: %lu, real micros: %lu, error: %.3lf %%,\nstart: %lu, end: %lu\n"
                              "--------------\n", i,
                              tdiff, wait,
                              timeUtils_deltaTime32(end, start),
                              realMicros,
                              tError, start, end);
            }
            if (diff > maxDiff)
                maxDiff = diff;
            if (tdiff > maxTdiff)
                maxTdiff = tdiff;
            if (error > maxError)
                maxError = error;
            if (tError > maxTerror)
                maxTerror = tError;
            diffSum += diff;
            tDiffSum += tdiff;
            diffErSum += error;
            tDiffErSum += tError;

        }

        timeUtils_debug(
                      "2] Testing delayMillis()\nmax diff: %lu, max Tdiff: %lu, max error: %.3lf %%, max Terror : %.3lf %%\n"
                      "diff avg: %.3lf, tdiff avg: %.3lf, diff error avg: %.3lf %%, tdiff error avg: %.3lf %%\n"
                      "--------------\n",
                      maxDiff, maxTdiff, maxError, maxTerror, diffSum / testIterations, tDiffSum / testIterations, diffErSum / testIterations,
                      tDiffErSum / testIterations);
    }
    HAL_Delay(2000);*/
    maxDiff = 0;
    maxTdiff = 0;
    maxError = 0;
    maxTerror = 0;
    diffSum = 0;
    tDiffSum = 0;
    diffErSum = 0;
    tDiffErSum = 0;
    {
        for (uint32_t i = 0; i < testIterations; i++) {
            uint32_t randomNum = (random() % 100) + 1;
            uint32_t wait = randomNum;
            htim16.Instance->CNT = 0;
            start = timeUtils_getMicros();
            timeUtils_delayMicros(wait);
            end = timeUtils_getMicros();
            realMicros = htim16.Instance->CNT;

            uint32_t deltaMicros = end - start;
            uint32_t diff = (realMicros > deltaMicros) ? (realMicros - deltaMicros) : (deltaMicros - realMicros);
            uint32_t tdiff = (realMicros > wait) ? (realMicros - wait) : (wait - realMicros);
            double error = (double) diff * 100 / realMicros;
            double tError = (double) tdiff * 100 / wait;
            if (error >= 0.1) {
                timeUtils_debug(
                        "[3] [D] i: %lu, diff: %lu,\nwait: %lu, micros: %lu, real micros: %lu, error: %.3lf %%,\nstart: %lu, end: %lu\n"
                        "--------------\n", i,
                        diff, wait,
                        timeUtils_deltaTime32(end, start),
                        realMicros,
                        error, start, end);
            }
            if (tError >= 0.1) {
                timeUtils_debug(
                        "[3] [T] i: %lu, Tdiff: %lu,\nwait: %lu, micros: %lu, real micros: %lu, error: %.3lf %%,\nstart: %lu, end: %lu\n"
                        "--------------\n", i,
                        tdiff, wait,
                        timeUtils_deltaTime32(end, start),
                        realMicros,
                        tError, start, end);
            }
            if (diff > maxDiff)
                maxDiff = diff;
            if (tdiff > maxTdiff)
                maxTdiff = tdiff;
            if (error > maxError)
                maxError = error;
            if (tError > maxTerror)
                maxTerror = tError;
            diffSum += diff;
            tDiffSum += tdiff;
            diffErSum += error;
            tDiffErSum += tError;
        }

        timeUtils_debug(
                "3] Testing delayMicros()\nmax diff: %lu, max Tdiff: %lu, max error: %.3lf %%, max Terror : %.3lf %%\n"
                "diff avg: %.3lf, tdiff avg: %.3lf, diff error avg: %.3lf %%, tdiff error avg: %.3lf %%\n"
                "--------------\n",
                maxDiff, maxTdiff, maxError, maxTerror, diffSum / testIterations, tDiffSum / testIterations,
                diffErSum / testIterations,
                tDiffErSum / testIterations);
    }
    timeUtils_debug("3 Phases of tests done.\n");
    HAL_Delay(100000);

}

#endif