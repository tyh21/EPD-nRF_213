#include "fonts.h"
#include "Lunar.h"
#include "GUI.h"
#include <stdio.h>
#include <stdlib.h> // <-- 新增：为了使用 malloc 和 free

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define GFX_printf_styled(gfx, fg, bg, font, ...) \
            GFX_setTextColor(gfx, fg, bg);        \
            GFX_setFont(gfx, font);               \
            GFX_printf(gfx, __VA_ARGS__);

// =========================================================================
// 以下是原始文件中的所有静态数据和辅助函数，保持不变
// Festival, holidays, GetHoliday, GetFestival, DrawBattery, 等等...
// =========================================================================

typedef struct {
    uint8_t month;
    uint8_t day;
    char name[10]; // 3x3+1
} Festival;

static const Festival festivals[] = {
    { 1,  1, "元旦节"}, { 2, 14, "情人节"}, { 3,  8, "妇女节"}, { 3, 12, "植树节"},
    { 4,  1, "愚人节"}, { 5,  1, "劳动节"}, { 5,  4, "青年节"}, { 6,  1, "儿童节"},
    { 7,  1, "建党节"}, { 8,  1, "建军节"}, { 9, 10, "教师节"}, {10,  1, "国庆节"},
    {11,  1, "万圣节"}, {12, 24, "平安夜"}, {12, 25, "圣诞节"},
};

static const Festival festivals_lunar[] = {
    { 1,  1, "春节"  }, { 1, 15, "元宵节"}, { 2,  2, "龙抬头"}, { 5,  5, "端午节"},
    { 7,  7, "七夕节"}, { 7, 15, "中元节"}, { 8, 15, "中秋节"}, { 9,  9, "重阳节"},
    {10,  1, "寒衣节"}, {12,  8, "腊八节"}, {12, 30, "除夕"  },
};

#define HOLIDAY_YEAR 2025
static const uint16_t holidays[] = {
    0x0101, 0x111A, 0x011C, 0x011D, 0x011E, 0x011F, 0x0201, 0x0202,
    0x0202, 0x0203, 0x0204, 0x1208, 0x0404, 0x0405, 0x0406, 0x141B,
    0x0501, 0x0502, 0x0503, 0x0504, 0x0505, 0x051F, 0x0601, 0x0602,
    0x191C, 0x0A01, 0x0A02, 0x0A03, 0x0A04, 0x0A05, 0x0A06, 0x0A07,
    0x0A08, 0x1A0B,
};

static bool GetHoliday(uint8_t mon, uint8_t day, bool *work) {
    for (uint8_t i = 0; i < ARRAY_SIZE(holidays); i++) {
        if (((holidays[i] >> 8) & 0xF) == mon && (holidays[i] & 0xFF) == day) {
            *work = ((holidays[i] >> 12) & 0xF) > 0;
            return true;
        }
    }
    return false;
}

static bool GetFestival(uint16_t year, uint8_t mon, uint8_t day, uint8_t week,
                        struct Lunar_Date *Lunar, char *festival) {
    for (uint8_t i = 0; i < ARRAY_SIZE(festivals_lunar); i++) {
        if (Lunar->Month == festivals_lunar[i].month && Lunar->Date == festivals_lunar[i].day) {
            strcpy(festival, festivals_lunar[i].name);
            return true;
        }
    }
    if (Lunar->Month == 12 && Lunar->Date == 29) {
        struct Lunar_Date nextLunar;
        struct devtm tm = {year, mon, day, 0, 0, 0, week};
        transformTime(transformTimeStruct(&tm) + 86400, &tm);
        LUNAR_SolarToLunar(&nextLunar, tm.tm_year + YEAR0, tm.tm_mon + 1, tm.tm_mday);
        if (nextLunar.Month == 1 && nextLunar.Date == 1) {
            strcpy(festival, "除夕");
            return true;
        }
    }
    if (mon == 5 && week == 0 && day >= 8 && day <= 14) { strcpy(festival, "母亲节"); return true; }
    if (mon == 6 && week == 0 && day >= 15 && day <= 21) { strcpy(festival, "父亲节"); return true; }
    if (mon == 11 && week == 4 && day >= 22 && day <= 28) { strcpy(festival, "感恩节"); return true; }
    for (uint8_t i = 0; i < ARRAY_SIZE(festivals); i++) {
        if (mon == festivals[i].month && day == festivals[i].day) {
            strcpy(festival, festivals[i].name);
            return true;
        }
    }
    uint8_t JQdate;
    if (GetJieQi(year, mon, day, &JQdate) && JQdate == day) {
        uint8_t JQ = (mon - 1) * 2;
        if (day >= 15) JQ++;
        if (JQ == 6) sprintf(festival, "%s节", JieQiStr[JQ]);
        else strcpy(festival, JieQiStr[JQ]);
        return true;
    }
    return false;
}

static void DrawBattery(Adafruit_GFX *gfx, int16_t x, int16_t y, float voltage) {
    uint8_t level = (uint8_t)(voltage * 100 / 4.2);
    GFX_setCursor(gfx, x - 26, y + 9);
    GFX_setFont(gfx, u8g2_font_wqy9_t_lunar);
    GFX_printf(gfx, "%.1fV", voltage);
    GFX_fillRect(gfx, x, y, 20, 10, GFX_WHITE);
    GFX_drawRect(gfx, x, y, 20, 10, GFX_BLACK);
    GFX_fillRect(gfx, x + 20, y + 4, 2, 2, GFX_BLACK);
    GFX_fillRect(gfx, x + 2, y + 2, 16 * level / 100, 6, GFX_BLACK);
}

static void DrawTemperature(Adafruit_GFX *gfx, int16_t x, int16_t y, int8_t temp) {
    GFX_setCursor(gfx, x, y);
    GFX_setFont(gfx, u8g2_font_wqy9_t_lunar);
    GFX_printf(gfx, "%d℃", temp);
}

static void DrawDate(Adafruit_GFX *gfx, int16_t x, int16_t y, tm_t *tm) {
    GFX_setCursor(gfx, x, y);
    GFX_printf_styled(gfx, GFX_BLACK, GFX_WHITE, u8g2_font_helvB14_tn, "%d", tm->tm_year + YEAR0);
    GFX_printf_styled(gfx, GFX_BLACK, GFX_WHITE, u8g2_font_wqy9_t_lunar, "年");
    GFX_printf_styled(gfx, GFX_BLACK, GFX_WHITE, u8g2_font_helvB14_tn, "%02d", tm->tm_mon + 1);
    GFX_printf_styled(gfx, GFX_BLACK, GFX_WHITE, u8g2_font_wqy9_t_lunar, "月");
    GFX_printf_styled(gfx, GFX_BLACK, GFX_WHITE, u8g2_font_helvB14_tn, "%02d", tm->tm_mday);
    GFX_printf_styled(gfx, GFX_BLACK, GFX_WHITE, u8g2_font_wqy9_t_lunar, "日 ");
}

static void DrawDateHeader(Adafruit_GFX *gfx, int16_t x, int16_t y, tm_t *tm, struct Lunar_Date *Lunar, gui_data_t *data) {
    DrawDate(gfx, x, y, tm);
    GFX_setFont(gfx, u8g2_font_wqy9_t_lunar);
    GFX_printf(gfx, "星期%s", Lunar_DayString[tm->tm_wday]);
    GFX_setCursor(gfx, x + 180, y);
    GFX_printf(gfx, "%s%s%s %s%s", Lunar_MonthLeapString[Lunar->IsLeap], Lunar_MonthString[Lunar->Month],
                     Lunar_DateString[Lunar->Date], Lunar_StemStrig[LUNAR_GetStem(Lunar)],
                     Lunar_BranchStrig[LUNAR_GetBranch(Lunar)]);
    GFX_setTextColor(gfx, GFX_RED, GFX_WHITE);
    GFX_printf(gfx, "%s", Lunar_ZodiacString[LUNAR_GetZodiac(Lunar)]);
    GFX_setTextColor(gfx, GFX_BLACK, GFX_WHITE);
    GFX_printf(gfx, "年");
}

static void DrawWeekHeader(Adafruit_GFX *gfx, int16_t x, int16_t y) {
    GFX_fillRect(gfx, x, y, 205, 14, GFX_RED);
    GFX_fillRect(gfx, x + 30, y, 205, 14, GFX_BLACK);
    GFX_setFont(gfx, u8g2_font_wqy9_t_lunar);
    for (int i = 0; i < 7; i++) {
        GFX_setTextColor(gfx, GFX_WHITE, (i > 0 && i < 6) ? GFX_BLACK : GFX_RED);
        GFX_setCursor(gfx, x + 8 + i * 34, y + 10);
        GFX_printf(gfx, "%s", Lunar_DayString[i]);
    }
}

static void DrawMonthDays(Adafruit_GFX *gfx, tm_t *tm, struct Lunar_Date *Lunar) {
    uint8_t firstDayWeek = get_first_day_week(tm->tm_year + YEAR0, tm->tm_mon + 1);
    uint8_t monthMaxDays = thisMonthMaxDays(tm->tm_year + YEAR0, tm->tm_mon + 1);
    uint8_t monthDayRows = 1 + (monthMaxDays - (7 - firstDayWeek) + 6) / 7;
    for (uint8_t i = 0; i < monthMaxDays; i++) {
        uint16_t year = tm->tm_year + YEAR0;
        uint8_t month = tm->tm_mon + 1;
        uint8_t day = i + 1;
        int16_t week = (firstDayWeek + i) % 7;
        bool weekend = (week  == 0) || (week == 6);
        int16_t x = 10 + week * 34;
        int16_t y = (monthDayRows > 5 ? 14 : 16) + (firstDayWeek + i) / 7 * (monthDayRows > 5 ? 14 : 16);
        if (day == tm->tm_mday) {
            GFX_fillCircle(gfx, x + 11, y + (monthDayRows > 5 ? 2 : 3)+30, 8, GFX_RED);
            GFX_setTextColor(gfx, GFX_WHITE, GFX_RED);
        } else {
            GFX_setTextColor(gfx, weekend ? GFX_RED : GFX_BLACK, GFX_WHITE);
        }
        GFX_setFont(gfx, u8g2_font_helvB14_tn);
        GFX_setCursor(gfx, x + (day < 10 ? 6 : 2), y + 42);
        GFX_printf(gfx, "%d", day);
        char festival[10] = {0};
        if (GetFestival(year, month, day, week, Lunar, festival)) {
            if (day != tm->tm_mday) GFX_setTextColor(gfx, GFX_RED, GFX_WHITE);
            GFX_setCursor(gfx, strlen(festival) > 6 ? x - 6 : x, y + 12);
            GFX_printf(gfx, "%s", festival);
        } else {
            GFX_setCursor(gfx, x, y + 12);
            if (Lunar->Date == 1) GFX_printf(gfx, "%s", Lunar_MonthString[Lunar->Month]);
            else GFX_printf(gfx, "%s", Lunar_DateString[Lunar->Date]);
        }
        bool work = false;
        if (year == HOLIDAY_YEAR && GetHoliday(month, day, &work)) {
            if (day == tm->tm_mday) {
                GFX_fillCircle(gfx, x + 20, y-2, 4, GFX_WHITE);
                GFX_drawCircle(gfx, x + 20, y-2, 4, GFX_RED);
            }
            GFX_setTextColor(gfx, work ? GFX_BLACK : GFX_RED, GFX_WHITE);
            GFX_setCursor(gfx, x + 25, y + 6);
            GFX_printf(gfx, "%s", work ? "班" : "休");
        }
    }
}

static void DrawCalendar(Adafruit_GFX *gfx, tm_t *tm, struct Lunar_Date *Lunar, gui_data_t *data) {
    DrawDateHeader(gfx, 6, 25, tm, Lunar, data);
    DrawWeekHeader(gfx, 4, 28);
    DrawMonthDays(gfx, tm, Lunar);
}

static void Draw7Number(Adafruit_GFX *gfx, int n, unsigned int xLoc, unsigned int yLoc, char cS, unsigned int fC, unsigned int bC, char nD) {
    unsigned int num=abs(n),i,t,w,col,h,a,b,j=1,d=0,S2=5*cS,S3=2*cS,S4=7*cS,x1=cS+1,x2=S3+S2+1,y1=yLoc+x1,y3=yLoc+S3+S4+1;
    unsigned int seg[7][3]={{x1,yLoc,1},{x2,y1,0},{x2,y3+x1,0},{x1,(2*y3)-yLoc,1},{0,y3+x1,0},{0,y1,0},{x1,y3,1}};
    unsigned char nums[12]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F,0x00,0x40};
    char c=(c=abs(cS))>10?10:(c<1)?1:c;
    char cnt=(cnt=abs(nD))>10?10:(cnt<1)?1:cnt;
    for (xLoc+=cnt*(d=S2+(3*S3)+2);cnt>0;cnt--){
      for (i=(num>9)?num%10:((!cnt)&&(n<0))?11:((nD<0)&&(!num))?10:num,xLoc-=d,num/=10,j=0;j<7;++j){
        col=(nums[i]&(1<<j))?fC:bC;
        if (seg[j][2])for(w=S2,t=seg[j][1]+S3,h=seg[j][1]+cS,a=xLoc+seg[j][0]+cS,b=seg[j][1];b<h;b++,a--,w+=2)GFX_drawFastHLine(gfx,a,b,w,col);
        else for(w=S4,t=xLoc+seg[j][0]+S3,h=xLoc+seg[j][0]+cS,b=xLoc+seg[j][0],a=seg[j][1]+cS;b<h;b++,a--,w+=2)GFX_drawFastVLine(gfx,b,a,w,col);
        for (;b<t;b++,a++,w-=2)seg[j][2]?GFX_drawFastHLine(gfx,a,b,w,col):GFX_drawFastVLine(gfx,b,a,w,col);
      }
    }
}

static void DrawTime(Adafruit_GFX *gfx, tm_t *tm, int16_t x, int16_t y, uint16_t cS, uint16_t nD) {
    Draw7Number(gfx, tm->tm_hour, x, y, cS, GFX_BLACK, GFX_WHITE, nD);
    x += (nD*(10*cS+2)-2*cS) + 2*cS;
    GFX_fillRect(gfx, x, y + 4.5*cS+1, 2*cS, 2*cS, GFX_BLACK);
    GFX_fillRect(gfx, x, y + 13.5*cS+3, 2*cS, 2*cS, GFX_BLACK);
    x += 4*cS;
    Draw7Number(gfx, tm->tm_min, x, y, cS, GFX_BLACK, GFX_WHITE, nD);
}

static void DrawClock(Adafruit_GFX *gfx, tm_t *tm, struct Lunar_Date *Lunar, gui_data_t *data) {
    DrawDate(gfx, 10, 24, tm);
    GFX_setCursor(gfx, 140, 22);
    GFX_setFont(gfx, u8g2_font_wqy9_t_lunar);
    GFX_printf(gfx, "星期%s", Lunar_DayString[tm->tm_wday]);
    GFX_setCursor(gfx, 0,60);
    GFX_printf(gfx, "%s%s%s", Lunar_MonthLeapString[Lunar->IsLeap], Lunar_MonthString[Lunar->Month],
        Lunar_DateString[Lunar->Date]);
    DrawBattery(gfx, 220, 14, data->voltage);
    DrawTemperature(gfx,20,80, data->temperature);
    GFX_drawFastHLine(gfx, 10, 25, 230, GFX_BLACK);
    DrawTime(gfx, tm, 50, 35, 4, 2);
    GFX_drawFastHLine(gfx, 10, 127, 230, GFX_BLACK);
    GFX_setCursor(gfx, 6, 40);
    GFX_setFont(gfx, u8g2_font_wqy9_t_lunar);
    GFX_printf(gfx, "%s%s%s年", Lunar_StemStrig[LUNAR_GetStem(Lunar)], Lunar_BranchStrig[LUNAR_GetBranch(Lunar)],
        Lunar_ZodiacString[LUNAR_GetZodiac(Lunar)]);
    uint8_t day = 0;
    uint8_t JQday = GetJieQiStr(tm->tm_year + YEAR0, tm->tm_mon + 1, tm->tm_mday, &day);
    if (day == 0) {
        GFX_setCursor(gfx, 6, 110);
        GFX_printf(gfx, "%s", JieQiStr[JQday % 24]);
    } else {
        GFX_setCursor(gfx, 6, 95);
        GFX_printf(gfx, "离%s", JieQiStr[JQday % 24]);
        GFX_setCursor(gfx, 6, 110);
        GFX_printf(gfx, "还有%d天", day);
    }
}

static void Draw7Number_To_Buffer(uint8_t *buffer, int n, char cS, uint16_t buf_w) {
    unsigned int num=abs(n), i, w, h, x, y, j=1;
    unsigned int S2=5*cS, S3=2*cS, S4=7*cS, x1=cS+1, x2=S3+S2+1, y1=cS+1, y3=cS+S3+S4+2;
    unsigned int seg[7][3]={{x1,0,1},{x2,y1,0},{x2,y3,0},{x1,y3+y1-1,1},{0,y3,0},{0,y1,0},{x1,y1+S4+1,1}};
    unsigned char nums[10]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F};

    // 清空缓冲区为白色 (0xFF)
    memset(buffer, 0xFF, (buf_w * (20 * cS + 4) + 7) / 8);

    i = num % 10; // 只取个位数
    for (j=0; j<7; ++j) {
        if (nums[i] & (1 << j)) { // 如果该段需要点亮 (黑色)
            if (seg[j][2]) { // 水平段
                for (y = seg[j][1]; y < seg[j][1] + S3; y++) {
                    for (x = seg[j][0]; x < seg[j][0] + S2; x++) {
                        // 设置像素点为黑色
                        buffer[(x + y * buf_w) / 8] &= ~(0x80 >> (x % 8));
                    }
                }
            } else { // 垂直段
                for (y = seg[j][1]; y < seg[j][1] + S4; y++) {
                    for (x = seg[j][0]; x < seg[j][0] + S3; x++) {
                        // 设置像素点为黑色
                        buffer[(x + y * buf_w) / 8] &= ~(0x80 >> (x % 8));
                    }
                }
            }
        }
    }
}


// =========================================================================
//                             主要修改区域
// =========================================================================

/**
 * @brief 仅绘制并更新时钟的分钟个位数（局部刷新）
 * @param data GUI数据
 * @param write_partial_image 驱动层提供的局部图像写入函数指针
 */
void DrawGUITime(gui_data_t *data, void (*write_partial_image)(uint8_t*, uint16_t, uint16_t, uint16_t, uint16_t))
{
    tm_t tm = {0};
    transformTime(data->timestamp, &tm);

    char cS_time = 4;
    // 注意：这里的宽度和高度计算需要与 Draw7Number 的逻辑匹配
    // 为了简化，我们使用一个稍大一点的固定尺寸来确保能容纳
    uint16_t digit_width = 9 * cS_time + 2;   // 38
    uint16_t digit_height = 20 * cS_time + 4; // 84

    uint8_t *local_buffer = (uint8_t *)malloc((digit_width * digit_height + 7) / 8);
    if (local_buffer == NULL) {
        return;
    }

    // *** 核心修正：不再使用 GFX 库，而是调用我们自己的绘图函数 ***
    Draw7Number_To_Buffer(local_buffer, tm.tm_min, cS_time, digit_width);

    // 调用驱动函数，将我们自己绘制好的缓冲区数据写入屏幕
    write_partial_image(local_buffer, 188, 35, digit_width, digit_height);

    // 释放内存
    free(local_buffer);
}

/**
 * @brief 绘制完整的用户界面（全屏刷新）
 * @param data GUI数据
 * @param draw GFX库的分页绘图回调
 * @param mode 显示模式（日历或时钟）
 */
void DrawGUI(gui_data_t *data, buffer_callback draw, display_mode_t mode)
{
    tm_t tm = {0};
    struct Lunar_Date Lunar;

    transformTime(data->timestamp, &tm);

    Adafruit_GFX gfx;

    if (data->bwr)
      GFX_begin_3c(&gfx, data->width, data->height, PAGE_HEIGHT);
    else
      GFX_begin(&gfx, data->width, data->height, PAGE_HEIGHT);
      
    GFX_setRotation(&gfx, GFX_ROTATE_270);

    GFX_firstPage(&gfx);
    do {
        GFX_fillScreen(&gfx, GFX_WHITE);

        LUNAR_SolarToLunar(&Lunar, tm.tm_year + YEAR0, tm.tm_mon + 1, tm.tm_mday);

        switch (mode) {
            case MODE_CALENDAR:
                DrawCalendar(&gfx, &tm, &Lunar, data);
                break;
            case MODE_CLOCK:
                DrawClock(&gfx, &tm, &Lunar, data);
                break;
            default:
                break;
        }
    } while(GFX_nextPage(&gfx, draw));
      
    GFX_end(&gfx);
}
