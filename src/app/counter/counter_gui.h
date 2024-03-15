#ifndef APP_CIOUNTER_GUI_H
#define APP_CIOUNTER_GUI_H

struct Counter
{
    int min;       // 倒计时分钟
    int second;    // 倒计时秒
    int progress;  // 进度条
    int page;      // 不同的page对应不同的倒计时间长度
    int mode;      // mode == 0 显示倒计时数字 mode == 1 显示向日葵图案
    int colour;    // 不同的 main part colour
};

#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"
#define ANIEND                      \
    while (lv_anim_count_running()) \
        lv_task_handler(); //等待动画完成

    void counter_gui_init(void);
    void display_counter_scr(struct Counter cnt, lv_scr_load_anim_t anim_type);
    void counter_gui_del(void);
    void display_start(lv_scr_load_anim_t anim_type);
    void display_success(lv_scr_load_anim_t anim_type);

#ifdef __cplusplus
} /* extern "C" */
#endif


#ifdef __cplusplus
extern "C"
{
#endif

#include "lvgl.h"
    extern const lv_img_dsc_t app_counter;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif