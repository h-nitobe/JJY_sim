//
//    JJY_sim3.c
//
//    JJY シミュレータ
//
//    2021/07/01 Ver.3.00 Written by H.Nitobe
//
//    JJY TimeCode                struct tm
//    00 MARKER                   tm_sec  0-59
//    01 Minutes 40 00-59         tm_min  0-59
//    02 Minutes 20               tm_hour 0-23
//    03 Minutes 10               tm_mday 1-31
//    04 FILLER '0'               tm_mon  0-11
//    05 Minutes 8                tm_year year-1900
//    06 Minutes 4                tm_wday 0-6
//    07 Minutes 2                tm_yday 0-365*
//    08 Minutes 1                tm_isdst (-1)
//    09 POSITION MARKER
//    10 FILEER '0'
//    11 FILLER '0'
//    12 Hour 20 00-23
//    13 Hour 10
//    14 FILLER '0'
//    15 Hour 8
//    16 Hour 4
//    17 Hour 2
//    18 Hour 1
//    19 POSITION MARKER
//    20 FILLER '0'
//    21 FILLER '0'
//    22 DayOfYear 200 001-366
//    23 DayOfYear 100
//    24 FILLER '0'
//    25 DayOfYear 80
//    26 DayOfYear 40
//    27 DayOfYear 20
//    28 DayOfYear 10
//    29 POSITION MARKER
//    30 DayOfYear 8
//    31 DayOfYear 4
//    32 DayOfYear 2
//    35 FILLER '0'
//    36 PARITY 1 '0/1'
//    37 PARITY 2 '0/1'
//    38 SUMMER 1 '0'
//    39 POSITION MARKER
//    40 SUMMER 2 '0'
//    41 Year 80    00-99
//    42 Year 40
//    43 Year 20
//    44 Year 10
//    45 Year 8
//    46 Year 4
//    47 Year 2
//    48 Year 1
//    49 POSITION MARKER
//    50 DayOfWeek 4 0-6
//    51 DayOfWeek 2
//    52 DayOfWeek 1
//    53 LEAP SECOND 1 '0'
//    54 LEAP SECOND 2 '0'
//    55 FILLER '0'
//    56 FILLER '0'
//    57 FILLER '0'
//    58 FILLER '0'
//    59 POSSITION MARKER
//    
//    標準電波（電波時計）の運用状況
//    https://jjy.nict.go.jp/jjy/trans/index.html
//
//    通常時（毎時１５分、４５分以外）のタイムコード（例）
//    https://jjy.nict.go.jp/jjy/trans/timecode1.html
//
//    呼び出し符号送出時（毎時１５分、４５分）のタイムコード（例）
//    https://jjy.nict.go.jp/jjy/trans/timecode2.html
//
//    man ctime
//    https://linuxjm.osdn.jp/html/LDP_man-pages/man3/ctime.3.html
//

#include    <stdio.h>
#include    <time.h>
#include    <sys/time.h>
#include    <wiringPi.h>
#include    <pthread.h>

#define        CLK     7    // WiringPi:7 / GPIO:4  / Phisical:7

#define        MARK    2    // マーク・ポジション
#define        SUMM    4    // サマータイム
#define        LEAP    6    // うるう秒
#define        FILL    8    // フィラー

#define        DEBUG1        // 送信データ表示
//#define        DEBUG2    // 処理時間表示
#define        OFFSET        // オフセット有り　5分進める
#define        PTHREAD       // pthread を使うぞ

//　関数プロトタイプ宣言
void setMin(struct tm *tm);
void setHour(struct tm *tm);
void setDays(struct tm *tm);
void setSum(int pos, int sta, int end);
void setDigit(int dig, int sta, int end);
void sendPulse(int pos);
int  getDigit(int sta, int end);
time_t decodeTimeCode(void);
void ydayToMonDay(int yday, int leap, int *mon, int *day);

//    グローバル変数（ファイル内）
static char    sec[60] = {              // 送信データ　60秒分
    MARK,0,0,0,FILL,0,0,0,0,MARK,       // MA,M40,M20,M10,FI,M8,M4,M2,M1,P1
    FILL,FILL,0,0,FILL,0,0,0,0,MARK,    // FI,FI,H20,H10,FI,H8,H4,H2,H1,P2
    FILL,FILL,0,0,FILL,0,0,0,0,MARK,    // FI,FI,D200,D100,FI,D80,D40,D20,D10,P3
    0,0,0,0,FILL,FILL,0,0,SUMM,MARK,    // D8,D4,D2,D1,FI,FI,PA,PA,SU,P4
    SUMM,0,0,0,0,0,0,0,0,MARK,          // SU,Y80,Y40,Y20,Y10,Y8,Y4,Y2,Y1,P5
    0,0,0,LEAP,LEAP,FILL,FILL,FILL,FILL,MARK, // W4,W2,W1,LE,LE,FI,FI,FI,FI,P6
};
int main(void)
{
#ifdef DEBUG2
    struct timeval    now;
    long              elapse;
#endif
    time_t     t;
    int        currSec, currMin, currHour, currDay;
    int        prevSec, prevMin, prevHour, prevDay;
    struct tm  tm;
#ifdef PTHREAD
    pthread_t  thread;
#endif

    wiringPiSetup();

// 13.3kHz駆動 3倍波が 40kHz 
    gpioClockSet(CLK, 13333);    // 19.2e6 / 1440 = 13.333..kHz
//    pinMode(CLK, GPIO_CLOCK);
    digitalWrite(CLK, LOW);
    pinMode(CLK, OUTPUT);
    setPadDrive(0, 7);         // ドライブ能力　グループ0(GPIO 0-27) 16mA
    

    t = time(NULL);            // 初回時刻設定
#ifdef OFFSET    // 5分進める    
    t = t + 300;
#endif
    localtime_r(&t, &tm);
    prevSec = currSec = tm.tm_sec;
    prevMin = currMin = tm.tm_min;
    prevHour = currHour = tm.tm_hour;
    prevDay = currDay = tm.tm_yday + 1;    // ＊＊注意＊＊　tm:0-364 JJY:1-365
    setMin(&tm);
    setHour(&tm);
    setDays(&tm);
#ifdef PTHREAD                // pthread 使ってみよっかなぁ
    pthread_create(&thread, NULL, 
            (void *)sendPulse, (void *)currSec);
#endif

    for  ( ;; ) {
        t = time(NULL);          // 現在時刻取得（ 協定世界時(UTC)）
#ifdef OFFSET                    // 5分進める
        t = t + 300;
#endif
        localtime_r(&t, &tm);    // 現在時刻取得（ローカルタイム JST）
        currSec  = tm.tm_sec;
        currMin  = tm.tm_min;
        currHour = tm.tm_hour;
        currDay  = tm.tm_yday + 1;    // ＊＊注意＊＊　tm: 0-364 JJY:1-365

        if (prevSec != currSec) {
#ifdef PTHREAD                   // pthread 使ってみよっかなぁ
            pthread_join(thread,NULL);    // 前回のをjoinしないとメモリリーク
            pthread_create(&thread, NULL, 
                    (void *)sendPulse, (void *)currSec);
#else
            sendPulse(currSec);  // データ送信 何はともあれ
#endif
#ifdef    DEBUG2
            gettimeofday(&now, NULL);
            elapse = now.tv_usec;
#endif
            if (currSec == 0) {  // TimeCode設定はマーカのあと

#ifdef DEBUG1
                printf(" %02d %03d %d %02d:%02d ",
                    tm.tm_year - 100, currDay, tm.tm_wday, 
                    currHour, currMin);
#endif
                if (prevMin != currMin) {
                    setMin(&tm);
                    prevMin  = currMin;
                }
                if (prevHour != currHour) {
                    setHour(&tm);
                    prevHour = currHour;
                }
                if (prevDay != currDay) {
                    setDays(&tm);
                    prevDay = currDay;
                }
            }
            prevSec = currSec;
#ifdef DEBUG2
            gettimeofday(&now, NULL);
            elapse = now.tv_usec - elapse;
            printf(" %ld %ld %ld\n", now.tv_sec, now.tv_usec, elapse);
#endif
        }
        
    }

    return 0;
}

void setMin(struct tm *tm)    // min エンコード　15/45 16/46 分処理
{
    setDigit(tm->tm_min / 10, 1, 3);    // 1-3: min 40,20,10 

    setDigit(tm->tm_min % 10, 5, 8);    // 5-8:    min 8,4,2,1

    setSum(37, 1, 8);            // 37:  sum 1-8 

    if (tm->tm_min == 15 || tm->tm_min == 45) {    // 15min, 45min JJY
#if 1
        sec[41] = sec[45] = 'N';    // NI
        sec[42] = sec[46] = 'T';    // TO
        sec[43] = sec[47] = 'B';    // BE
#else
        sec[41] = sec[42] = sec[45] = sec[46] = 'J';
        sec[43] = sec[47] = 'Y';
#endif
        sec[40] = sec[44] = sec[48] = '_';
    } 
    if (tm->tm_min == 16 || tm->tm_min == 46) {    // 16min, 46min year
        sec[40] = SUMM;
        setDigit((tm->tm_year % 100) / 10, 41, 44);
        setDigit((tm->tm_year % 100) % 10, 45, 48);
    }
}

void setHour(struct tm *tm)    // hour エンコード
{
    setDigit(tm->tm_hour / 10, 12, 13);    // 12-13: hour 20, 10

    setDigit(tm->tm_hour % 10, 15, 18);    // 15-18: hour 8, 4, 2, 1

    setSum(36, 12, 18);             // 36;    sum 12-18
}

void setDays(struct tm *tm)    // yday, year, wday エンコード
{
    setDigit((tm->tm_yday + 1) / 100, 22, 23);    // yday 200, 100

    setDigit(((tm->tm_yday + 1) % 100) / 10, 25, 28);    // yday 80,40,20,10
    
    setDigit(((tm->tm_yday + 1) % 100) % 10, 30, 33);    // yday 8, 4, 2, 1

    setDigit((tm->tm_year % 100) / 10, 41, 44);    // year 80, 40, 20, 10    

    setDigit((tm->tm_year % 100) % 10, 45, 48);    // year 8, 4, 2, 1

    setDigit(tm->tm_wday, 50, 52);            // wday 2, 1
}

void setSum(int pos, int sta, int end)    // チェックサム設定
{
    int    sum = 0;

    for ( ; sta <= end; sta++) {
        sum += sec[sta];
    }
    sec[pos] = sum & 1;
}

void setDigit(int dig, int sta, int end)    // 桁～タイムコード変換
{
    for ( ; sta <= end; sta++) {
        sec[sta] = (dig >> (end - sta)) & 1;
    }
}

void sendPulse(int pos)    // タイムコード送出
{
    int        pulse = sec[pos];

    switch (pulse) {
        case 0: case FILL: case SUMM: case LEAP:
            pinMode(CLK, GPIO_CLOCK);
            delay(800);
            pinMode(CLK, OUTPUT);
            break;
        case 1:
            pinMode(CLK, GPIO_CLOCK);
            delay(500);
            pinMode(CLK, OUTPUT);
            break;
        case MARK:
            pinMode(CLK, GPIO_CLOCK);
            delay(200);
            pinMode(CLK, OUTPUT);
            break;
        case 'N':            // NI
            pinMode(CLK, OUTPUT);
            delay(300);
            pinMode(CLK, GPIO_CLOCK);
            delay(150);
            pinMode(CLK, OUTPUT);
            delay(50);
            pinMode(CLK, GPIO_CLOCK);
            delay(50);
            pinMode(CLK, OUTPUT);
            delay(150);
            pinMode(CLK, GPIO_CLOCK);
            delay(50);
            pinMode(CLK, OUTPUT);
            delay(50);
            pinMode(CLK, GPIO_CLOCK);
            delay(50);
            pinMode(CLK, OUTPUT);    // pause 150
            break;
        case 'T':            // TO
            pinMode(CLK, GPIO_CLOCK);
            delay(150);
            pinMode(CLK, OUTPUT);
            delay(150);
            pinMode(CLK, GPIO_CLOCK);
            delay(150);
            pinMode(CLK, OUTPUT);
            delay(50);
            pinMode(CLK, GPIO_CLOCK);
            delay(150);
            pinMode(CLK, OUTPUT);
            delay(50);
            pinMode(CLK, GPIO_CLOCK);
            delay(150);
            pinMode(CLK, OUTPUT);    // pause 150
            break;
        case 'B':            // BE
            pinMode(CLK, GPIO_CLOCK);
            delay(150);
            pinMode(CLK, OUTPUT);
            delay(50);
            pinMode(CLK, GPIO_CLOCK);
            delay(50);
            pinMode(CLK, OUTPUT);
            delay(50);
            pinMode(CLK, GPIO_CLOCK);
            delay(50);
            pinMode(CLK, OUTPUT);
            delay(150);
            pinMode(CLK, GPIO_CLOCK);
            delay(50);
            pinMode(CLK, OUTPUT);    // pause 450
            break;
        case 'J':
            pinMode(CLK, GPIO_CLOCK);
            delay(62);
            pinMode(CLK, OUTPUT);
            delay(62);
            pinMode(CLK, GPIO_CLOCK);
            delay(186);
            pinMode(CLK, OUTPUT);
            delay(62);
            pinMode(CLK, GPIO_CLOCK);
            delay(186);
            pinMode(CLK, OUTPUT);
            delay(62);
            pinMode(CLK, GPIO_CLOCK);
            delay(186);
            pinMode(CLK, OUTPUT);    // pause 186
            break;
        case 'Y':
            pinMode(CLK, GPIO_CLOCK);
            delay(186);
            pinMode(CLK, OUTPUT);
            delay(62);
            pinMode(CLK, GPIO_CLOCK);
            delay(62);
            pinMode(CLK, OUTPUT);
            delay(62);
            pinMode(CLK, GPIO_CLOCK);
            delay(186);
            pinMode(CLK, OUTPUT);
            delay(62);
            pinMode(CLK, GPIO_CLOCK);
            delay(186);
            pinMode(CLK, OUTPUT);
            break;
        case '_':
            pinMode(CLK, OUTPUT);
            break;
    }
#ifdef DEBUG1
    if (pulse == 'J' || pulse == 'Y' 
            || pulse == 'N' || pulse == 'T'
            || pulse == 'B' || pulse == '_') {
        printf("%c", pulse);
    } else {
        printf("%d", pulse);
    }
    if (pos == 59) {
        printf("\n");
    }
    fflush(stdout);
#endif
}

int getDigit(int sta, int end)    // タイムコード～桁変換
{
    int    digit = 0;

    for ( ; sta <= end; sta++) {
        digit += sec[sta] << (end - sta);
    }

    return digit;
}

time_t decodeTimeCode(void)    // タイムコード～time_t変換
{
    struct tm    tm;
    int        yday, mon, day;

    tm.tm_year = 100 + getDigit(41, 44) * 10 + getDigit(45, 48);
    // JJY yday: 1～365　tm_yday: 0～364 なので -1 する
    yday = getDigit(22, 23) * 100 + getDigit(25, 28) * 10
            + getDigit(30, 33) - 1;
    // 1901～2099の間は !(year & 3) が閏年
    ydayToMonDay(yday, !(tm.tm_year & 3), &mon, &day);
    tm.tm_mon = mon;
    tm.tm_mday = day;
    tm.tm_hour = getDigit(12, 13) * 10 + getDigit(15, 18);
    tm.tm_min  = getDigit( 1,  3) * 10 + getDigit( 5,  8);
    tm.tm_sec  = 59;

    return mktime(&tm);
}

// mktime() は yday は無視、mon mday を与えなければならないので自前で処理
void ydayToMonDay(int yday, int leap, int *mon, int *day)
{
    int    i;
    int    ymon[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    if (leap) {        // 閏年は2月は29日だぞ
        ymon[1] += 1;
    }

    yday++;
    for (i = 0; i < 12; i++) {
        if (ymon[i] < yday) {
            yday = yday - ymon[i];
        } else {
            *day = yday;
            break;
        }
    }
    *mon = i;
}

// cc -Wall JJY_sim3.c -oJJY_sim3 -lwiringPi -lpthread
// sudo ./JJY_sim3
