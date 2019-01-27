#include "Global.h"
#include "i2c.h"

struct rtc_time_struct {
	uint8_t	seconds;
	uint8_t minutes;
	uint8_t	hours;
	uint8_t	day;
	uint8_t	date;
	uint8_t	month;
	uint8_t	year;
	uint8_t alarms[7];
	uint8_t control;
	uint8_t status;
	uint8_t aging;
	uint8_t temp_hi;
	uint8_t temp_lo;
} __packed;

static struct rtc_time_struct rtc, rtcw;



BaseType_t is_rtc_time_valid(struct rtc_time_struct *rtc)
{
	if (rtc->year < 0x19) return pdFAIL;
	else {//return pdPASS;
		if (rtc->month < 0x01) return pdFAIL;
		else {//return pdPASS;
			if (rtc->date < 0x12) return pdFAIL;
			else {//return pdPASS;
				if (rtc->hours < 0x14) return pdFAIL;
				else {//return pdPASS;
					if (rtc->minutes < 0x31) return pdFAIL;
					else return pdPASS;
				}
			}
		}
	}
}

__STATIC_INLINE uint8_t bin2bcd(uint16_t arg)
{
	return (uint8_t)(((arg/10)<<4) + arg%10);
}

void ble_update_rtc(const struct ble_date_time *arg)
{
	uint16_t pyear = arg->year - 2000;
	uint8_t centure = (uint8_t)(pyear/100);
	
	pyear = arg->year - 2000;
	centure = (uint8_t)(pyear/100);
	rtcw.year = bin2bcd(pyear%100);
	rtcw.month = (centure << 7) + bin2bcd(arg->month);
	rtcw.date = bin2bcd(arg->day);
	rtcw.hours = bin2bcd(arg->hours);
	rtcw.minutes = bin2bcd(arg->minutes);
	rtcw.seconds = bin2bcd(arg->seconds);
//	rtcw.alarms[0] = rtc.alarms[0];
	if (arg->year != 2020) LED_WORK_ON;
	i2c(RTC_WRITE, (uint8_t)0, offsetof(rtc_time_struct, alarms), &rtcw);
	i2c(RTC_READ, (uint8_t)0, offsetof(rtc_time_struct, aging), &rtc);
	if (rtcw.year != 0x20)  LED_SD_BUSY_ON;
	if (rtc.year != 0x20) LED_ERR_ON;
}

void RTCProc(void *Param)
{
	i2c(RTC_READ, (uint8_t)0, offsetof(rtc_time_struct, aging), &rtc);

	rtc.status = 0;

	if (is_rtc_time_valid(&rtc) == pdFAIL) {
		rtc.seconds = 0x00;
		rtc.minutes = 0x31;
		rtc.hours = 0x14;
		rtc.day = 0x6;
		rtc.date = 0x12;
		rtc.month = 0x01;
		rtc.year = 0x19;
		i2c(RTC_WRITE, (uint8_t)0, offsetof(rtc_time_struct, alarms), &rtc);
	}

	i2c(RTC_WRITE, (uint8_t)offsetof(struct rtc_time_struct, status), sizeof(rtc.status), &rtc.status);

	for (;;) {
		static uint8_t seconds;

		seconds = rtc.seconds;

		i2c(RTC_READ, (uint8_t)0, offsetof(rtc_time_struct, alarms), &rtc);

		if (seconds != rtc.seconds) {
			struct ble_date_time ble = {
			
				.year =		(uint16_t)(2000 + ((rtc.month & 0x80) >> 7)*100 + ((rtc.year & 0xF0) >> 4)*10 + (rtc.year & 0x0F)),
				.month =	(uint8_t)(((rtc.month & 0x10) >> 4)*10 + (rtc.month & 0x0F)),
				.day =		(uint8_t)(((rtc.date & 0xF0) >> 4)*10 + (rtc.date & 0x0F)),
				.hours =	(uint8_t)(((rtc.hours & 0x30) >> 4)*10 + (rtc.hours & 0x0F)),
				.minutes =	(uint8_t)(((rtc.minutes & 0xF0) >> 4)*10 + (rtc.minutes & 0x0F)),
				.seconds =	(uint8_t)(((rtc.seconds & 0xF0) >> 4)*10 + (rtc.seconds & 0x0F))
			};
			if (RTC_Queue != NULL) {
				xQueueSendToBack(RTC_Queue, &ble, 0);
			}
		}
		vTaskDelay(MS_TO_TICK(50));
	}
}
