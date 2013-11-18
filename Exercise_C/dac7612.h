#ifndef DAC7612_H
#define DAC7612_H

/*
 * DAC7612 Definitions and Macros
 */
#define DAC7612_CS_PORT			PORTB
#define DAC7612_CS_DDR			DDRB
#define DAC7612_CS_PIN			PB0

// instruction bit defines
#define DAC7612_CONVERT			0x80

#define DAC7612_REG_READ		0x40
#define DAC7612_REG_WRITE		0x00
#define DAC7612_REG_16BIT		0x20

// register addresses
#define DAC7612_RESULTLO		0x00
#define DAC7612_RESULTHI		0x01
#define DAC7612_PGAVALID		0x02
#define DAC7612_ADCTRL			0x03
#define DAC7612_GAINMUX			0x04
#define DAC7612_DIGIOSTATE		0x05
#define DAC7612_DIGIOCTRL		0x06
#define DAC7612_REFOSC			0x07
#define DAC7612_SERIFCTRL		0x18
#define DAC7612_ID			0x1F

// register bit defines
#define DAC7612_RESULTLO_OVR	0x01

#define DAC7612_ADCTRL_BIN		0x20
#define DAC7612_ADCTRL_RMB1		0x08
#define DAC7612_ADCTRL_RMB0		0x04
#define DAC7612_ADCTRL_CFD1		0x02
#define DAC7612_ADCTRL_CFD0		0x01

#define DAC7612_GAINMUX_CNVBSY	0x80

#define DAC7612_REFOSC_OSCR		0x20
#define DAC7612_REFOSC_OSCE		0x10
#define DAC7612_REFOSC_REFE		0x08
#define DAC7612_REFOSC_BUFE		0x04
#define DAC7612_REFOSC_R2V		0x02
#define DAC7612_REFOSC_RBG		0x01

#define DAC7612_SERIFCTRL_LSB	0x01
#define DAC7612_SERIFCTRL_2W3W	0x02
#define DAC7612_SERIFCTRL_8051	0x04

#define DAC7612_ID_VALUE		0x01

// gain defines
#define DAC7612_GAIN_1X			0x00
#define DAC7612_GAIN_2X			0x10
#define DAC7612_GAIN_4X			0x20
#define DAC7612_GAIN_5X			0x30
#define DAC7612_GAIN_8X			0x40
#define DAC7612_GAIN_10X		0x50
#define DAC7612_GAIN_16X		0x60
#define DAC7612_GAIN_20X		0x70

// channel defines
#define DAC7612_CH_0_1_DIFF		0x00
#define DAC7612_CH_2_3_DIFF		0x01
#define DAC7612_CH_4_5_DIFF		0x02
#define DAC7612_CH_6_7_DIFF		0x03
#define DAC7612_CH_1_0_DIFF		0x04
#define DAC7612_CH_3_2_DIFF		0x05
#define DAC7612_CH_5_4_DIFF		0x06
#define DAC7612_CH_7_6_DIFF		0x07
#define DAC7612_CH_SINGLE_ENDED	0x08
#define DAC7612_CH_0			0x08
#define DAC7612_CH_1			0x09
#define DAC7612_CH_2			0x0A
#define DAC7612_CH_3			0x0B
#define DAC7612_CH_4			0x0C
#define DAC7612_CH_5			0x0D
#define DAC7612_CH_6			0x0E
#define DAC7612_CH_7			0x0F

#endif
