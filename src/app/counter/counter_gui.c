#include "counter_gui.h"
#include "lvgl.h"
#include "counter_image.h"

LV_FONT_DECLARE(lv_font_ibmplex_115);
LV_FONT_DECLARE(lv_font_ibmplex_64);
LV_FONT_DECLARE(ch_font20);

static lv_obj_t *scr_0 = NULL;
static lv_obj_t *scr_1 = NULL;
static lv_style_t default_style;
static lv_style_t smallNum;
static lv_style_t bigNum;
// static lv_style_t chFont;
static lv_obj_t *circ = NULL;
static lv_obj_t *minuteLabel = NULL, *secondLabel = NULL;

static lv_obj_t *start_src = NULL;
static lv_obj_t *startImg = NULL;
static lv_obj_t *success_src = NULL;
static lv_obj_t *successImg = NULL;
const void *successImageMap[] = {&success_image_00, &success_image_01};
// 淡粉色 天蓝色 紫罗兰 草绿色 珊瑚橙 湛蓝色 玫瑰红 翡翠绿
static unsigned int colourSel[8] = {0xFFC0CB, 0x87CEEB, 0xEE82EE, 0x7CFC00, 
                                    0xFF7F50, 0x00BFFF, 0xFF1493, 0x00FF7F};

void counter_gui_init(void)
{ 
    lv_style_init(&default_style);
    lv_style_set_bg_color(&default_style, lv_color_hex(0x000000));

    // lv_style_init(&chFont);
    // lv_style_set_text_opa(&chFont, LV_OPA_COVER);
    // lv_style_set_text_color(&chFont, lv_color_hex(0xffffff));
    // lv_style_set_text_font(&chFont, &ch_font20);

    lv_style_init(&smallNum);
    lv_style_set_text_opa(&smallNum, LV_OPA_COVER);
    lv_style_set_text_color(&smallNum, lv_color_hex(0xffffff));
    lv_style_set_text_font(&smallNum, &lv_font_ibmplex_64);

    lv_style_init(&bigNum);
    lv_style_set_text_opa(&bigNum, LV_OPA_COVER);
    lv_style_set_text_color(&bigNum, lv_color_hex(0xffffff));
    lv_style_set_text_font(&bigNum, &lv_font_ibmplex_115);
}


static void display_counter_scr_init(int flag, lv_scr_load_anim_t anim_type)
{
    lv_obj_t *act_obj = lv_scr_act(); // 获取当前活动页
    if ((act_obj == scr_0 && flag == 0) || (act_obj == scr_1 && flag == 1))
        return;

    counter_gui_del();
    lv_obj_clean(act_obj); // 清空此前页面

    if (flag == 0)
    {
        scr_0 = lv_obj_create(NULL);
        lv_obj_add_style(scr_0, &default_style, LV_STATE_DEFAULT);

        circ = lv_arc_create(scr_0);
        lv_obj_remove_style(circ, NULL, LV_PART_KNOB);
        lv_obj_set_style_arc_width(circ, 8, LV_PART_MAIN);
        lv_obj_set_style_arc_width(circ, 8, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(circ, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR); 
        lv_obj_set_style_arc_color(circ, lv_color_hex(colourSel[2]), LV_PART_MAIN);      
        lv_obj_set_style_arc_rounded(circ, false, LV_PART_INDICATOR);
        lv_obj_set_style_arc_rounded(circ, false, LV_PART_MAIN);

        lv_obj_set_size(circ, 200, 200);            // 设置进度条的大小
        lv_obj_align(circ, LV_ALIGN_CENTER, 0, 0);  // 将进度条居中对齐
        lv_arc_set_bg_angles(circ, 135, 55);        // 设置进度条的背景角度（0到360度）
        lv_arc_set_angles(circ, 135, 135);          // 设置进度条的起始和结束角度

        minuteLabel = lv_label_create(scr_0);
        lv_obj_add_style(minuteLabel, &bigNum, LV_STATE_DEFAULT);
        lv_label_set_recolor(minuteLabel, true);

        secondLabel = lv_label_create(scr_0);
        lv_obj_add_style(secondLabel, &smallNum, LV_STATE_DEFAULT);
        lv_label_set_recolor(secondLabel, true);

        lv_obj_align(minuteLabel, LV_ALIGN_CENTER, 0,  15);
        lv_obj_align(secondLabel, LV_ALIGN_CENTER, 10, 50);

    }
    else if (flag == 1)
    {
        scr_1 = lv_obj_create(NULL);
        lv_obj_add_style(scr_1, &default_style, LV_STATE_DEFAULT);

        circ = lv_arc_create(scr_1);
        lv_obj_remove_style(circ, NULL, LV_PART_KNOB);
        lv_obj_set_style_arc_width(circ, 8, LV_PART_MAIN);
        lv_obj_set_style_arc_width(circ, 8, LV_PART_INDICATOR);
        lv_obj_set_style_arc_color(circ, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR); 
        lv_obj_set_style_arc_color(circ, lv_color_hex(colourSel[2]), LV_PART_MAIN);      
        lv_obj_set_style_arc_rounded(circ, false, LV_PART_INDICATOR);
        lv_obj_set_style_arc_rounded(circ, false, LV_PART_MAIN);

        lv_obj_set_size(circ, 200, 200);            // 设置进度条的大小
        lv_obj_align(circ, LV_ALIGN_CENTER, 0, 0);  // 将进度条居中对齐
        lv_arc_set_bg_angles(circ, 135, 55);        // 设置进度条的背景角度（0到360度）
        lv_arc_set_angles(circ, 135, 135);          // 设置进度条的起始和结束角度

        startImg = lv_img_create(scr_1);
        lv_obj_align(startImg, LV_ALIGN_CENTER, -2, 20);

    }
    else
    {
        
    }

}

void display_counter_scr(struct Counter cnt, lv_scr_load_anim_t anim_type)
{
    
    display_counter_scr_init(cnt.mode, anim_type);
    // fresh the arc
    lv_arc_set_value(circ, cnt.progress); 
    if (cnt.mode == 0)
    {
        // fresh the time
        lv_label_set_text_fmt(minuteLabel, "#ffa500 %02d#", cnt.min);
        lv_label_set_text_fmt(secondLabel, "%02d", cnt.second);
        if (LV_SCR_LOAD_ANIM_NONE != anim_type)
        {
            lv_scr_load_anim(scr_0, anim_type, 300, 300, false);
        }
        else
        {
            lv_scr_load(scr_0);
        }
        
    }
    else if (cnt.mode == 1)
    {
        lv_img_set_src(startImg, &start_image);
        if (LV_SCR_LOAD_ANIM_NONE != anim_type)
        {
            lv_scr_load_anim(scr_1, anim_type, 300, 300, false);
        }
        else
        {
            lv_scr_load(scr_1);
        }
    }
    else
    {

    }
}

static void display_start_init(lv_scr_load_anim_t anim_type)
{
    lv_obj_t *act_obj = lv_scr_act(); // 获取当前活动页
    if (act_obj == start_src)
        return;

    counter_gui_del();
    lv_obj_clean(act_obj); // 清空此前页面

    start_src = lv_obj_create(NULL);
    lv_obj_add_style(start_src, &default_style, LV_STATE_DEFAULT);

    circ = lv_arc_create(start_src);
    lv_obj_remove_style(circ, NULL, LV_PART_KNOB);
    lv_obj_set_style_arc_width(circ, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_width(circ, 8, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(circ, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR); 
    lv_obj_set_style_arc_rounded(circ, false, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(circ, false, LV_PART_MAIN);

    lv_obj_set_size(circ, 200, 200);            // 设置进度条的大小
    lv_obj_align(circ, LV_ALIGN_CENTER, 0, 0);  // 将进度条居中对齐
    lv_arc_set_bg_angles(circ, 135, 55);        // 设置进度条的背景角度（0到360度）
    lv_arc_set_angles(circ, 135, 135);          // 设置进度条的起始和结束角度

    startImg = lv_img_create(start_src);
    lv_obj_align(startImg, LV_ALIGN_CENTER, -2, 20);
}

void display_start(lv_scr_load_anim_t anim_type)
{
    display_start_init(anim_type);
    lv_img_set_src(startImg, &start_image);

    if (LV_SCR_LOAD_ANIM_NONE != anim_type)
    {
        lv_scr_load_anim(start_src, anim_type, 300, 300, false);
    }
    else
    {
        lv_scr_load(start_src);
    }
}

static void display_success_init(lv_scr_load_anim_t anim_type)
{
    
    lv_obj_t *act_obj = lv_scr_act(); // 获取当前活动页
    if (act_obj == success_src)
        return;

    counter_gui_del();
    lv_obj_clean(act_obj); // 清空此前页面

    success_src = lv_obj_create(NULL);
    lv_obj_add_style(success_src, &default_style, LV_STATE_DEFAULT);
    successImg = lv_img_create(success_src);
    lv_obj_align(successImg, LV_ALIGN_CENTER, 0, 0);

}

void display_success(lv_scr_load_anim_t anim_type)
{
    display_success_init(anim_type);
    static int _successIndex = 0;
    lv_img_set_src(successImg, successImageMap[_successIndex]);
    _successIndex = (_successIndex + 1) % 2;

    if (LV_SCR_LOAD_ANIM_NONE != anim_type)
    {
        lv_scr_load_anim(success_src, anim_type, 300, 300, false);
    }
    else
    {
        lv_scr_load(success_src);
    }
}

void counter_src0_del(void)
{
    if (NULL != scr_0)
    {
        lv_obj_clean(scr_0);
        scr_0 = NULL;
        circ = NULL;
        minuteLabel = NULL;
        secondLabel = NULL;
    }
    if (NULL != scr_1)
    {
        lv_obj_clean(scr_1);
        scr_1 = NULL;
        circ = NULL;
        startImg = NULL;
    }
}

void counter_start_del(void)
{
    if (NULL != start_src)
    {
        lv_obj_clean(start_src);
        start_src = NULL;
        startImg = NULL;
        circ = NULL;
    }
}

void couner_success_del(void)
{
    if (NULL != success_src)
    {
        lv_obj_clean(success_src);
        success_src = NULL;
        successImg = NULL;
    }
}

void counter_gui_del(void)
{
    counter_src0_del();
    counter_start_del();
    couner_success_del();
    
    // 手动清除样式，防止内存泄漏
    // lv_style_reset(&default_style);
}