# TimeUtils

TimeUtils is an interrupt-based library that provides microsecond timing functionalities on STM32 microcontrollers written in HAL with minimal runtime overhead.

It only needs simple capabilities of "basic timers" and does not rely on "advanced" or "general purpose" timers or "DWT", making it compatible with a wider range of microcontroller series. It only occupies one basic timer.

To use TimeUtils, follow these steps:

1. Enable a timer in interrupt mode.
2. Pass the timer's pointer to the `timeUtils_init` function during startup.
3. Call the `timeUtils_ISR` function inside the interrupt callback of the timer.

The Prescaler (PSC) of the timer will be automatically set to the appropriate value based on the HCLK.

There's also a test function provided if you tend to use.
### Example:

**Initializing and usage:**

    int main(){

        ...

        timeUtils_init(&htim16);

        ...

        while(1){
            ...
            timeUtils_delayMicros(100);
            ...
        }
        
        return 0;
    }

**Callback Routine:**

    void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
        if (htim->Instance == timeUtils_getTimerInstance()) {
            timeUtils_ISR();
        }
    }

### Recommendations:
* Compile the code with high compiler optimizations(-O2 or higher) to improved accuracy and performance.
* Set the system clock to at least 16 MHz to ensure accurate timing.
* Set priority of the timer interrupt to 0 to avoid any interference with TimeUtils' operation.

## **Caution:**
This is a personal project and still under development. thus
**No guarantee is provided.** Please use it at your own discretion, and be aware that there may be bugs or changes in future versions.