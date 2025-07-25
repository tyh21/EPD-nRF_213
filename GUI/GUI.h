#ifndef __GUI_H
#define __GUI_H

#include "Adafruit_GFX.h"

#ifndef PAGE_HEIGHT
#define PAGE_HEIGHT ((__HEAP_SIZE / 50) - 4)
#endif

typedef enum {
    MODE_NONE = 0,
    MODE_CALENDAR = 1,
    MODE_CLOCK = 2,
} display_mode_t;

typedef struct {
    bool bwr;
    uint16_t width;
    uint16_t height;
    uint32_t timestamp;
    int8_t temperature;
    float voltage;
} gui_data_t;

void DrawGUI(gui_data_t *data, buffer_callback draw, display_mode_t mode);

/**
 * @brief 仅绘制并更新时钟的分钟个位数（局部刷新）
 * @param data GUI数据
 * @param write_partial_image 驱动层提供的局部图像写入函数指针
 */
// 最终修正：使用与 EPD_driver.h 完全匹配的5参数函数指针原型
void DrawGUITime(gui_data_t *data, void (*write_partial_image)(uint8_t*, uint16_t, uint16_t, uint16_t, uint16_t));

#endif // __GUI_H
