# TimeUtils

An Interrupt-based library for providing micro-second timing functionalities on STM32 microcontrollers written in HAL

It only needs simple capabilities of "basic timers". It uses neither "advanced" or "general purpose" timers nor "DWT" which are not available on all microcontroller series. Just occupies one basic timer.

All you need to do is just enabling a timer in interrupt mode, passing its pointer to the timeUtils_Init function on startup and calling the timeUtils_IncISR function inside the interrupt callback of the timer.

The PSC of the timer will be automatically set to the appropriate value based on the HCLK.
### Example:

**initializing:**

    int main(){

        ...

        timeUtils_Init(&htim16);

        ...

        while(1){
            ...
        }
        
        return 0;
    }

**Callback Routine:**

    void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
        if (htim->Instance == timeUtils_getTimerInstance()) {
            timeUtils_IncISR();
        }
    }

### Recommendations:
* Compile the code with high compiler optimizations(O2 or higher) for more accuracy
* Set the system clock to at least 16 MHz
* Set priority of the timer interrupt to 0 to avoid any disturbance of the TimeUtils activity

## **Caution:**
This is a personal project and still under development. thus
**No guarantee is provided.**