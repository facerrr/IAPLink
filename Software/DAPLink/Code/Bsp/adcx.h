#ifndef __ADCX_H__
#define __ADCX_H__

#define ADC_NUM 		    2
#define ADC_CH_MOD          0
#define ADC_CH_DOWNLOAD     1

typedef struct
{
    unsigned char u8ADCSampleCMPFlag;
    unsigned short int adc_result[ADC_NUM];
    unsigned short int adc_average[ADC_NUM];
}strADC;


void ADCx_Init(void);
void DMA_Init(void);
unsigned short int ADC_GetAvgValue(unsigned char ch);
unsigned short int ADC_GetRawValue(unsigned char ch);
void ADCx_Func(void);

#endif  // __ADCX_H__