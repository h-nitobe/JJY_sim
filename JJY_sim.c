#include	<stdio.h>
#include	<time.h>
#include	<wiringPi.h>

#define		MARK	2

void sendPulse(int pulse);

int main(void)
{
  	time_t		t = time(NULL);
	time_t		prev = t;
	struct tm	tm;
	int		sec;
	int		pa1 = 0, pa2 = 0;

	wiringPiSetup();

	pinMode(1, PWM_OUTPUT);
	pwmSetMode(PWM_MODE_MS);
	pwmSetRange(10);
	pwmSetClock(48);		// 19.2e6 / 48 = 400kHz
	pwmWrite(1, 0);

	for  ( ;; ) {
		t = time(NULL);
		sec = t % 60;
		localtime_r(&t, &tm);

		if (prev != sec) {
			switch (sec) {
				case 0:
					sendPulse(MARK); 
					printf(" %04d/%02d/%02d %d %02d:%02d:%02d %d ",
						tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
						tm.tm_wday, tm.tm_hour, tm.tm_min, tm.tm_sec, tm.tm_yday+1);
					pa1 = pa2 = 0;
					break;
				case 1:
					sendPulse(((tm.tm_min/10) >> 2) & 1); 
					pa2 += ((tm.tm_min/10) >> 2) & 1;
					break;
				case 2:
					sendPulse(((tm.tm_min/10) >> 1) & 1);
					pa2 += ((tm.tm_min/10) >> 1) & 1;
					break;
				case 3:
					sendPulse(((tm.tm_min/10) >> 0) & 1);
					pa2 += ((tm.tm_min/10) >> 0) & 1;
					break;
				case 4:
					sendPulse(0); break;
				case 5:
					sendPulse(((tm.tm_min%10) >> 3) & 1);
					pa2 += ((tm.tm_min%10) >> 3) & 1;
				       	break;
				case 6:
					sendPulse(((tm.tm_min%10) >> 2) & 1);
					pa2 += ((tm.tm_min%10) >> 2) & 1;
				       	break;
				case 7:
					sendPulse(((tm.tm_min%10) >> 1) & 1);
					pa2 += ((tm.tm_min%10) >> 1) & 1;
				       	break;
				case 8:
					sendPulse(((tm.tm_min%10) >> 0) & 1);
					pa2 += ((tm.tm_min%10) >> 0) & 1;
			 		break;
				case 9: 
					sendPulse(MARK); break;
				case 10:
					sendPulse(0); break;
				case 11: 
					sendPulse(0); break;
				case 12:
					sendPulse(((tm.tm_hour/10) >> 1) & 1);
					pa1 += ((tm.tm_hour/10) >> 1) & 1;
				       	break;
				case 13:
					sendPulse(((tm.tm_hour/10) >> 0) & 1);
					pa1 += ((tm.tm_hour/10) >> 0) & 1;
				       	break;
				case 14: 
					sendPulse(0); break;
				case 15:
					sendPulse(((tm.tm_hour%10) >> 3) & 1); 
					pa1 += ((tm.tm_hour%10) >> 3) & 1;
					break;
				case 16:
					sendPulse(((tm.tm_hour%10) >> 2) & 1);
					pa1 += ((tm.tm_hour%10) >> 2) & 1;
				       	break;
				case 17:
					sendPulse(((tm.tm_hour%10) >> 1) & 1);
					pa1 += ((tm.tm_hour%10) >> 1) & 1;
				       	break;
				case 18:
					sendPulse(((tm.tm_hour%10) >> 0) & 1);
					pa1 += ((tm.tm_hour%10) >> 0) & 1;
				       	break;
				case 19: 
					sendPulse(MARK); break;
				case 20:
					sendPulse(0); break;
				case 21: 
					sendPulse(0); break;
				case 22:
					sendPulse((((tm.tm_yday+1)/100) >> 1) & 1); break;
				case 23:
					sendPulse((((tm.tm_yday+1)/100) >> 0) & 1); break;
				case 24: 
					sendPulse(0); break;
				case 25:
					sendPulse((((tm.tm_yday+1)%100)/10 >> 3) & 1); break;
				case 26:
					sendPulse((((tm.tm_yday+1)%100)/10 >> 2) & 1); break;
				case 27:
					sendPulse((((tm.tm_yday+1)%100)/10 >> 1) & 1); break;
				case 28:
					sendPulse((((tm.tm_yday+1)%100)/10 >> 0) & 1); break;
				case 29: 
					sendPulse(MARK); break;
				case 30:
					sendPulse((((tm.tm_yday+1)%100)%10 >> 3) & 1); break;
				case 31:
					sendPulse((((tm.tm_yday+1)%100)%10 >> 2) & 1); break;
				case 32:
					sendPulse((((tm.tm_yday+1)%100)%10 >> 1) & 1); break;
				case 33:
					sendPulse((((tm.tm_yday+1)%100)%10 >> 0) & 1); break;
				case 34:
					sendPulse(0); break;		// "0"
				case 35: 
					sendPulse(0); break;		// "0"
				case 36:
					sendPulse(pa1%2); break;	// PA1
				case 37:
					sendPulse(pa2%2); break;	// PA2
				case 38:
					sendPulse(0); break;		// "0"
				case 39: 
					sendPulse(MARK); break;		// "MARK"
				case 40:
					sendPulse(0); break;		// "0"
				case 41:
					if (tm.tm_min == 15 || tm.tm_min == 45) {
						sendPulse('J');
					} else {
						sendPulse(((tm.tm_year-100)/10 >> 3) & 1);
					}
					break;
				case 42:
					if (tm.tm_min == 15 || tm.tm_min == 45) {
						sendPulse('J');
					} else {
						sendPulse(((tm.tm_year-100)/10 >> 2) & 1);
					}
					break;
				case 43:
					if (tm.tm_min == 15 || tm.tm_min == 45) {
						sendPulse('Y');
					} else {
						sendPulse(((tm.tm_year-100)/10 >> 1) & 1);
					}
					break;
				case 44:
					if (tm.tm_min == 15 || tm.tm_min == 45) {
						sendPulse(0);
					} else {
						sendPulse(((tm.tm_year-100)/10 >> 0) & 1);
					}
					break;
				case 45:
					if (tm.tm_min == 15 || tm.tm_min == 45) {
						sendPulse('J');
					} else {
						sendPulse(((tm.tm_year-100)%10 >> 3) & 1);
					}
					break;
				case 46:
					if (tm.tm_min == 15 || tm.tm_min == 45) {
						sendPulse('J');
					} else {
						sendPulse(((tm.tm_year-100)%10 >> 2) & 1);
					}
					break;
				case 47:
					if (tm.tm_min == 15 || tm.tm_min == 45) {
						sendPulse('Y');
					} else {
						sendPulse(((tm.tm_year-100)%10 >> 1) & 1);
					}
					break;
				case 48:
					if (tm.tm_min == 15 || tm.tm_min == 45) {
						sendPulse(0);
					} else {
						sendPulse(((tm.tm_year-100)%10 >> 0) & 1);
					}
					break;
				case 49: 
					sendPulse(MARK); break;
				case 50:
					if (tm.tm_min == 15 || tm.tm_min == 45) {
						sendPulse(0);
					} else {
						sendPulse((tm.tm_wday >> 2) & 1);
					}
					break;
				case 51:
					if (tm.tm_min == 15 || tm.tm_min == 45) {
						sendPulse(0);
					} else {
						sendPulse((tm.tm_wday >> 1) & 1);
					}
					break;
				case 52:
					if (tm.tm_min == 15 || tm.tm_min == 45) {
						sendPulse(0);
					} else {
						sendPulse((tm.tm_wday >> 0) & 1);
					}
					break;
				case 53:
					sendPulse(0); break;
				case 54:
					sendPulse(0); break;
				case 55:
					sendPulse(0); break;
				case 56:
					sendPulse(0); break;
				case 57:
					sendPulse(0); break;
				case 58:
					sendPulse(0); break;
				case 59:
					sendPulse(MARK);
					printf("\n");
					fflush(stdout);
					break;
				default:
					break;
			}
			prev = sec;
		}
	}

	return 0;
}

void sendPulse(int pulse)
{
	pwmWrite(1, 5);
	switch (pulse) {
		case 0:
			delay(800);
			break;
		case 1:
			delay(500);
			break;
		case 2:
			delay(200);
			break;
		case 'J':
			pwmWrite(1, 5);
			delay(62);
			pwmWrite(1, 0);
			delay(62);
			pwmWrite(1, 5);
			delay(186);
			pwmWrite(1, 0);
			delay(62);
			pwmWrite(1, 5);
			delay(186);
			pwmWrite(1, 0);
			delay(62);
			pwmWrite(1, 5);
			delay(186);
			break;
		case 'Y':
			pwmWrite(1, 5);
			delay(186);
			pwmWrite(1, 0);
			delay(62);
			pwmWrite(1, 5);
			delay(62);
			pwmWrite(1, 0);
			delay(62);
			pwmWrite(1, 5);
			delay(186);
			pwmWrite(1, 0);
			delay(62);
			pwmWrite(1, 5);
			delay(186);
			break;
	}
	pwmWrite(1, 0);
	if (pulse == 'J' || pulse == 'Y') {
		printf("%c", pulse);
	} else {
		printf("%d", pulse);
	}
	fflush(stdout);

}
