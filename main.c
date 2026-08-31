/******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the PSOC™ Control C1 MCU: VADC EMUX
*              for ModusToolbox.
*
* Related Document: See README.md
*
******************************************************************************
* (c) 2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is
* owned by Infineon Technologies AG or one of its affiliates ("Infineon")
* and is protected by and subject to worldwide patent protection, worldwide
* copyright laws, and international treaty provisions. Therefore, you may use
* this Software only as provided in the license agreement accompanying the
* software package from which you obtained this Software. If no license
* agreement applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
*
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE
* IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
* INCLUDING, BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF
* THIRD-PARTY RIGHTS AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A
* SPECIFIC USE/PURPOSE OR MERCHANTABILITY.
* Infineon reserves the right to make changes to the Software without notice.
* You are responsible for properly designing, programming, and testing the
* functionality and safety of your intended application of the Software, as
* well as complying with any legal requirements related to its use. Infineon
* does not guarantee that the Software will be free from intrusion, data theft
* or loss, or other breaches ("Security Breaches"), and Infineon shall have
* no liability arising out of any Security Breaches. Unless otherwise
* explicitly approved by Infineon, the Software may not be used in any
* application where a failure of the Product or any consequences of the use
* thereof can reasonably be expected to result in personal injury.
*****************************************************************************/

#include "cybsp.h"
#include "cy_utils.h"
#include "cy_retarget_io.h"
#include "cycfg_peripherals.h"
#include "cy_vadc.h"
#include "cy_gpio.h"
#include <stdio.h>

/*******************************************************************************
* Defines
*******************************************************************************/

/* Define macro to enable/disable printing of debug messages */
#define ENABLE_DEBUG_PRINT              (0)

#define ADC_CONVERSION_EVENT_HANDLER        IRQ_Hdlr_17
#define INTERRUPT_PRIORITY_NODE_ID          IRQ17_IRQn


/* Group 0 channel 1 pin is measured and converted */
#define CHANNEL_NUMBER    (1U)
#define VADC_GROUP_PTR    (VADC_G0)

/* Register result */
#define RES_REG_NUMBER    (0)

/* ADC Conversion rate (ms) */
#define TICK_PERIOD (1000U)

static volatile uint32_t g_systick_fired = 0U;
static volatile uint32_t g_adc_latest = 0U;

/*******************************************************************************
* Function Name: ADC_CONVERSION_EVENT_HANDLER
********************************************************************************
* Summary:
* This is the interrupt handler function for the ADC end of conversion.
* The ADC result is retrieved after conversion inside this function.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void ADC_CONVERSION_EVENT_HANDLER(void)
{
    Cy_VADC_RESULT_SIZE_t result;

    /* Read the result register */
    result = Cy_VADC_GROUP_GetResult(VADC_GROUP_PTR, RES_REG_NUMBER);

    /* Acknowledge the interrupt */
    Cy_VADC_GROUP_QueueClearReqSrcEvent(VADC_GROUP_PTR);

    /* Keep ISR short: publish latest sample and return. */
    g_adc_latest = (uint32_t)result;
}

/*******************************************************************************
* Function Name: SysTick_Handler
********************************************************************************
* Summary:
* This is the interrupt handler function for the SysTick timer interrupt.
* It counts the time elapsed in milliseconds since the timer started.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/

void SysTick_Handler(void)
{
    g_systick_fired++;

    /* Start one conversion per SysTick so SysTick cannot be starved by ADC ISR. */
    Cy_VADC_GROUP_QueueTriggerConversion(VADC_GROUP_PTR);
}

/*******************************************************************************
 * Function Name: main
 ********************************************************************************
 * Summary:
 * This is the main function. It initializes the VADC with EMUX configuration
 * and starts periodic ADC conversions using SysTick.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int
 *
*******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    #if ENABLE_DEBUG_PRINT
    /* Initialize retarget-io to use the debug UART port */
    cy_retarget_io_init(CYBSP_DEBUG_UART_HW);

    printf("Initialization done\r\n");
    #endif

    /* Initialize SysTick timer for EMUX channel updates (1000ms period). */
    if (SysTick_Config(SystemCoreClock / 1000U) == 1U)
    {
        CY_ASSERT(0);
    }

    /* Explicitly enable SysTick interrupt */
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
    Cy_SCU_SetInterruptControl(IRQ17_IRQn, CY_SCU_IRQCTRL_VADC0_G0SR0_IRQ17);


    /* Ensure SysTick can preempt ADC ISR if both are pending. */
    NVIC_SetPriority(SysTick_IRQn, 1U);
    NVIC_SetPriority(INTERRUPT_PRIORITY_NODE_ID, 2U);

    NVIC_EnableIRQ(INTERRUPT_PRIORITY_NODE_ID);

    for (;;)
    {
        #if ENABLE_DEBUG_PRINT
            static uint32_t last_systick = 0U;
            if (g_systick_fired != last_systick)
            {
                printf("ADC=%lu\r\n", (unsigned long)g_adc_latest);
                last_systick = g_systick_fired;
            }
        #endif
    }
}

/* [] END OF FILE */
