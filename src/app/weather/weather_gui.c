#include "weather_gui.h"
#include "weather_image.h"
#include "lvgl.h"

LV_FONT_DECLARE(lv_font_ibmplex_115);
LV_FONT_DECLARE(lv_font_ibmplex_64);
LV_FONT_DECLARE(ch_font20);
static lv_style_t default_style;
static lv_style_t chFont_style;
static lv_style_t numberSmall_style;
static lv_style_t numberBig_style;
static lv_style_t btn_style;
static lv_style_t bar_style;
// page 0 今日天气
static lv_obj_t *scr_1 = NULL;
static lv_obj_t *weatherImg = NULL;
static lv_obj_t *cityLabel = NULL;
static lv_obj_t *btn = NULL, *btnLabel = NULL;
static lv_obj_t *txtLabel = NULL;
static lv_obj_t *clockLabel_1 = NULL, *clockLabel_2 = NULL;
static lv_obj_t *dateLabel = NULL;
static lv_obj_t *tempImg = NULL, *tempBar = NULL, *tempLabel = NULL;
static lv_obj_t *humiImg = NULL, *humiBar = NULL, *humiLabel = NULL;
static lv_obj_t *spaceImg = NULL;
// page 1 未来三日天气
static lv_obj_t *scr_2 = NULL;
static lv_obj_t *dailyImg[3] = {NULL};
static lv_obj_t *btn_daily[6] = {NULL};
static lv_obj_t *btnLabel_daily[6] = {NULL};
static lv_obj_t *dayLabel[3] = {NULL};
//
static lv_obj_t *loading_scr = NULL;
static lv_obj_t *loading_label = NULL;

// 天气图标路径的映射关系
const void *weaImage_map[] = {&weather_0, &weather_9, &weather_14, &weather_5, &weather_25,
                              &weather_30, &weather_26, &weather_11, &weather_23};
// 太空人图标路径的映射关系
const void *manImage_map[] = {&man_0, &man_1, &man_2, &man_3, &man_4, &man_5, &man_6, &man_7, &man_8, &man_9};
static const char weekDayCh[7][4] = {"日", "一", "二", "三", "四", "五", "六"};
static const char airQualityCh[6][10] = {"优", "良", "轻度", "中度", "重度", "严重"};
static const char weekDayAbbr[7][5] = {"MON", "TUE", "WED", "THUR", "FRI~", "SAT~", "SUN~"};

// 极简天气图标映射关系
const void *imageMap[] = {"S:/weather/Sunny_100.bin", "S:/weather/Clear_150.bin", "S:/weather/Clear_150.bin", "S:/weather/Clear_150.bin",
                           "S:/weather/Cloudy_101.bin","S:/weather/PartlyCloudy_103.bin","S:/weather/PartlyCloudy_153.bin",
                           "S:/weather/PartlyCloudy_153.bin", "S:/weather/PartlyCloudy_153.bin",
                           "S:/weather/Overcast_104.bin","S:/weather/ShowerRain_300.bin","S:/weather/Thundershower_302.bin",
                           "S:/weather/ThundershowerWithHail_304.bin", "S:/weather/LightRain_305.bin",
                           "S:/weather/ModerateRain_306.bin","S:/weather/HeavyRain_307.bin","S:/weather/Storm_310.bin",
                           "S:/weather/HeavyStorm_311.bin", "S:/weather/SevereStorm_312.bin",
                           "S:/weather/FreezingRain_313.bin", "S:/weather/Sleet_404.bin", "S:/weather/SnowFlurry_407.bin", "S:/weather/LightSnow_400.bin", 
                           "S:/weather/Snowstorm_403.bin","S:/weather/ModerateSnow_401.bin","S:/weather/HeavySnow_402.bin",
                           "S:/weather/Dust_504.bin", "S:/weather/Sand_503.bin", "S:/weather/Duststorm_507.bin",
                           "S:/weather/Sandstorm_508.bin", "S:/weather/Foggy_501.bin", "S:/weather/Haze_502.bin",
                           "S:/weather/Windy_32.bin", "S:/weather/Blustery_33.bin", "S:/weather/Hurricane_34.bin",
                           "S:/weather/TropicalStorm_35.bin", "S:/weather/Tornado_36.bin", "S:/weather/Cold_901.bin",
                           "S:/weather/Hot_900.bin", "S:/weather/Unknown_999.bin"};

const int mapIndex[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                         10, 11, 12, 13, 14, 15, 16, 17,
                         18, 19, 20, 21, 22, 23, 24, 25,
                         26, 27, 28, 29, 30, 31, 32, 33,
                         34, 35, 36, 37, 38, 99};

void weather_gui_init(void)
{
    lv_style_init(&default_style);
    lv_style_set_bg_color(&default_style, lv_color_hex(0x000000));

    lv_style_init(&chFont_style);
    lv_style_set_text_opa(&chFont_style, LV_OPA_COVER);
    lv_style_set_text_color(&chFont_style, lv_color_hex(0xffffff));
    lv_style_set_text_font(&chFont_style, &ch_font20);

    lv_style_init(&numberSmall_style);
    lv_style_set_text_opa(&numberSmall_style, LV_OPA_COVER);
    lv_style_set_text_color(&numberSmall_style, lv_color_hex(0xffffff));
    lv_style_set_text_font(&numberSmall_style, &lv_font_ibmplex_64);

    lv_style_init(&numberBig_style);
    lv_style_set_text_opa(&numberBig_style, LV_OPA_COVER);
    lv_style_set_text_color(&numberBig_style, lv_color_hex(0xffffff));
    lv_style_set_text_font(&numberBig_style, &lv_font_ibmplex_115);

    lv_style_init(&btn_style);
    lv_style_set_border_width(&btn_style, 0);
    lv_style_init(&bar_style);
    lv_style_set_bg_color(&bar_style, lv_color_hex(0x000000));
    lv_style_set_border_width(&bar_style, 2);
    lv_style_set_border_color(&bar_style, lv_color_hex(0xFFFFFF));
    lv_style_set_pad_top(&bar_style, 1); // 指示器到背景四周的距离
    lv_style_set_pad_bottom(&bar_style, 1);
    lv_style_set_pad_left(&bar_style, 1);
    lv_style_set_pad_right(&bar_style, 1);
}

void display_daily_init(lv_scr_load_anim_t anim_type)
{
    //页面初始化
    lv_obj_t *act_obj = lv_scr_act(); // 获取当前活动页
    if (act_obj == scr_2)
    {
        return;
    }
    weather_gui_release();
    lv_obj_clean(act_obj); // 清空此前页面
    // 创建page 1界面
    scr_2 = lv_obj_create(NULL);
    lv_obj_add_style(scr_2, &default_style, LV_STATE_DEFAULT);
    // 太空人图标
    spaceImg = lv_img_create(scr_2);
    // 创建一个按钮对象，设置按钮对象的位置 & 大小
    for (int i = 0; i < 3; i ++)
    {
        // 在屏幕上创建三个img图像
        dailyImg[i] = lv_img_create(scr_2);
        // 最高温度(ORANGE)与最低温度(GREEN)显示
        btn_daily[2*i] = lv_btn_create(scr_2);
        lv_obj_add_style(btn_daily[2*i], &btn_style, LV_STATE_DEFAULT);
        lv_obj_set_size(btn_daily[2*i], 50, 25);
        lv_obj_set_style_bg_color(btn_daily[2*i], lv_palette_main(LV_PALETTE_ORANGE), LV_STATE_DEFAULT);
        btnLabel_daily[2*i] = lv_label_create(btn_daily[2*i]);
        lv_obj_add_style(btnLabel_daily[2*i], &chFont_style, LV_STATE_DEFAULT);
        lv_obj_align(btnLabel_daily[2*i], LV_ALIGN_CENTER, 0, 0);
        btn_daily[(2*i) + 1] = lv_btn_create(scr_2);
        lv_obj_add_style(btn_daily[(2*i) + 1], &btn_style, LV_STATE_DEFAULT);
        lv_obj_set_size(btn_daily[(2*i) + 1], 50, 25);
        lv_obj_set_style_bg_color(btn_daily[(2*i) + 1], lv_palette_main(LV_PALETTE_GREEN), LV_STATE_DEFAULT);
        btnLabel_daily[(2*i) + 1] = lv_label_create(btn_daily[(2*i) + 1]);
        lv_obj_add_style(btnLabel_daily[(2*i) + 1], &chFont_style, LV_STATE_DEFAULT);
        lv_obj_align(btnLabel_daily[(2*i) + 1], LV_ALIGN_CENTER, 0, 0);
        // 日期显示
        dayLabel[i] = lv_label_create(scr_2);
        lv_obj_add_style(dayLabel[i], &chFont_style, LV_STATE_DEFAULT);
    }

    // 温度与日期位置
    lv_obj_set_pos(btn_daily[0], 60,  40);
    lv_obj_set_pos(btn_daily[1], 60,  65);
    lv_obj_set_pos(btn_daily[2], 190, 40);
    lv_obj_set_pos(btn_daily[3], 190, 65);
    lv_obj_set_pos(btn_daily[4], 60,  170);
    lv_obj_set_pos(btn_daily[5], 60,  195);
    lv_obj_set_pos(dayLabel[0],  10,  65);
    lv_obj_set_pos(dayLabel[1],  135, 65);
    lv_obj_set_pos(dayLabel[2],  10,  195);
    // 绘制图形
    lv_obj_align(dailyImg[0],  LV_ALIGN_TOP_LEFT,     0,  10);    // 天气位置
    lv_obj_align(dailyImg[1],  LV_ALIGN_TOP_RIGHT,   -30, 10);    // 天气位置
    lv_obj_align(dailyImg[2],  LV_ALIGN_BOTTOM_LEFT,  0, -45);    // 天气位置
    // 太空人图标位置
    lv_obj_align(spaceImg,     LV_ALIGN_BOTTOM_RIGHT, -20, -20);
}

void display_daily(struct Weather weaInfo, lv_scr_load_anim_t anim_type)
{
    display_daily_init(anim_type);
    // 刷新天气 & 温度 & 日期
    for (int i = 0; i < 3; i ++)
    {
        lv_img_set_src(dailyImg[i], weaImage_map[weaInfo.daily_code[i]]);
        lv_label_set_text_fmt(btnLabel_daily[2*i], "%2d°C", weaInfo.daily_max[i]);
        lv_label_set_text_fmt(btnLabel_daily[(2*i)+1], "%2d°C", weaInfo.daily_min[i]);
        lv_label_set_text(dayLabel[i], weekDayAbbr[weaInfo.dayofWeek[i]]);
    }
    
    // 保证活动界面没有问题
    if (LV_SCR_LOAD_ANIM_NONE != anim_type)
    {
        lv_scr_load_anim(scr_2, anim_type, 300, 300, false);
    }
    else
    {
        lv_scr_load(scr_2);
    }
}

void display_weather_new_init(lv_scr_load_anim_t anim_type)
{
    //页面初始化
    lv_obj_t *act_obj = lv_scr_act(); // 获取当前活动页
    if (act_obj == scr_1)
    {
        return;
    }
    weather_gui_release();
    lv_obj_clean(act_obj); // 清空此前页面

    // 在屏幕上创建一个父对象
    scr_1 = lv_obj_create(NULL);
    lv_obj_add_style(scr_1, &default_style, LV_STATE_DEFAULT);

    // 在屏幕上创建一个img图像
    weatherImg = lv_img_create(scr_1);

    cityLabel = lv_label_create(scr_1);
    lv_obj_add_style(cityLabel, &chFont_style, LV_STATE_DEFAULT);
    txtLabel = lv_label_create(scr_1);
    lv_obj_add_style(txtLabel, &numberSmall_style, LV_STATE_DEFAULT);
    tempLabel= lv_label_create(scr_1);
    lv_obj_add_style(tempLabel, &chFont_style, LV_STATE_DEFAULT);
    
}

void display_weather_init(lv_scr_load_anim_t anim_type)
{
    //页面初始化
    lv_obj_t *act_obj = lv_scr_act(); // 获取当前活动页
    if (act_obj == scr_1)
    {
        return;
    }
    weather_gui_release();
    lv_obj_clean(act_obj); // 清空此前页面
    // 在屏幕上创建一个父对象
    scr_1 = lv_obj_create(NULL);
    lv_obj_add_style(scr_1, &default_style, LV_STATE_DEFAULT);

    // 在屏幕上创建一个img图像
    weatherImg = lv_img_create(scr_1);
    // 太空人图标
    spaceImg = lv_img_create(scr_1);
    // 创建一个按钮对象，设置按钮对象的位置 & 大小
    btn = lv_btn_create(scr_1);
    lv_obj_add_style(btn, &btn_style, LV_STATE_DEFAULT);
    lv_obj_set_pos(btn, 20, 15);
    lv_obj_set_size(btn, 105, 25);
    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_ORANGE), LV_STATE_DEFAULT);
    // 城市图标
    cityLabel = lv_label_create(btn);
    lv_obj_add_style(cityLabel, &chFont_style, LV_STATE_DEFAULT);
    lv_obj_align(cityLabel, LV_ALIGN_CENTER, 0, 0);
    // 设置文本框
    txtLabel = lv_label_create(scr_1);
    lv_obj_add_style(txtLabel, &chFont_style, LV_STATE_DEFAULT);
    // lvgl8之前版本，模式一旦设置 LV_LABEL_LONG_SCROLL_CIRCULAR
    // 宽度恒定等于当前文本的长度，所以下面先设置以下长度
    lv_label_set_text(txtLabel, "最低气温12°C, ");
    lv_obj_set_size(txtLabel, 120, 30);
    lv_label_set_long_mode(txtLabel, LV_LABEL_LONG_SCROLL_CIRCULAR);
    // 时钟显示
    clockLabel_1 = lv_label_create(scr_1);
    lv_obj_add_style(clockLabel_1, &numberBig_style, LV_STATE_DEFAULT);
    lv_label_set_recolor(clockLabel_1, true);
    clockLabel_2 = lv_label_create(scr_1);
    lv_obj_add_style(clockLabel_2, &numberSmall_style, LV_STATE_DEFAULT);
    lv_label_set_recolor(clockLabel_2, true);
    // 日期显示
    dateLabel = lv_label_create(scr_1);
    lv_obj_add_style(dateLabel, &chFont_style, LV_STATE_DEFAULT);
    // 温度显示
    tempImg = lv_img_create(scr_1);
    lv_img_set_src(tempImg, &temp);
    lv_img_set_zoom(tempImg, 180);
    tempBar = lv_bar_create(scr_1);
    lv_obj_add_style(tempBar, &bar_style, LV_STATE_DEFAULT);
    lv_bar_set_range(tempBar, -50, 50); // 设置进度条表示的温度为-50~50
    lv_obj_set_size(tempBar, 60, 12);
    lv_obj_set_style_bg_color(tempBar, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
    lv_bar_set_value(tempBar, 10, LV_ANIM_ON);
    tempLabel = lv_label_create(scr_1);
    lv_obj_add_style(tempLabel, &chFont_style, LV_STATE_DEFAULT);
    // 湿度显示
    humiImg = lv_img_create(scr_1);
    lv_img_set_src(humiImg, &humi);
    lv_img_set_zoom(humiImg, 180);
    humiBar = lv_bar_create(scr_1);
    lv_obj_add_style(humiBar, &bar_style, LV_STATE_DEFAULT);
    lv_bar_set_range(humiBar, 0, 100);
    lv_obj_set_size(humiBar, 60, 12);
    lv_obj_set_style_bg_color(humiBar, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
    lv_bar_set_value(humiBar, 49, LV_ANIM_ON);
    humiLabel = lv_label_create(scr_1);
    lv_obj_add_style(humiLabel, &chFont_style, LV_STATE_DEFAULT);
    // 绘制图形
    lv_obj_align(weatherImg,   LV_ALIGN_TOP_RIGHT,    -10,  10);    // 天气位置
    lv_obj_align(txtLabel,     LV_ALIGN_TOP_LEFT,      10,  50);    // 文本框位置
    // 温度条 & 湿度条
    lv_obj_align(tempImg,      LV_ALIGN_LEFT_MID,      10,  70);
    lv_obj_align(tempBar,      LV_ALIGN_LEFT_MID,      35,  70);
    lv_obj_align(tempLabel,    LV_ALIGN_LEFT_MID,      103, 70);
    lv_obj_align(humiImg,      LV_ALIGN_LEFT_MID,      0,   100);
    lv_obj_align(humiBar,      LV_ALIGN_LEFT_MID,      35,  100);
    lv_obj_align(humiLabel,    LV_ALIGN_LEFT_MID,      103, 100);
    // 太空人图标位置
    lv_obj_align(spaceImg,     LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    // 时钟显示 & 日期显示
    lv_obj_align(clockLabel_1, LV_ALIGN_LEFT_MID,      0,   10);
    lv_obj_align(clockLabel_2, LV_ALIGN_LEFT_MID,      165, 9);
    lv_obj_align(dateLabel,    LV_ALIGN_LEFT_MID,      10,  32);

    // 创建一个全屏的Loading界面
    if (loading_scr == NULL) {
        loading_scr = lv_obj_create(scr_1);
        lv_obj_set_size(loading_scr, LV_HOR_RES, LV_VER_RES);
        lv_obj_set_style_bg_color(loading_scr, lv_color_hex(0x000000), LV_STATE_DEFAULT);  

        loading_label = lv_label_create(loading_scr);
        lv_obj_add_style(loading_label, &chFont_style, LV_STATE_DEFAULT);
        lv_label_set_text(loading_label, "Weather Loading ...");
        lv_obj_align(loading_label, LV_ALIGN_CENTER, 0, 0);
    }

    // 将Loading界面移动到顶层
    lv_obj_move_foreground(loading_scr);

}

void display_weather(struct Weather weaInfo, lv_scr_load_anim_t anim_type, int mode)
{
    if (mode == 0)
    {
        display_weather_new_init(anim_type);
        const void *path = NULL;
        if (weaInfo.code < 39)
        {
            path = imageMap[mapIndex[weaInfo.code]];
        }
        else
        {
            path = imageMap[39];
        }

        lv_img_set_src(weatherImg, path);
        lv_label_set_text(cityLabel, weaInfo.cityname);
        lv_label_set_text_fmt(txtLabel, "%d", weaInfo.temperature);
        lv_label_set_text_fmt(tempLabel, "°C");
        lv_obj_align(cityLabel, LV_ALIGN_BOTTOM_MID, -45, -30);
        lv_obj_align(weatherImg, LV_ALIGN_CENTER, 0, -30);
        lv_obj_align_to(txtLabel, cityLabel, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
        lv_obj_align_to(tempLabel, txtLabel, LV_ALIGN_OUT_RIGHT_MID, 10, 0);    
        
    }
    else if (mode == 1)
    {
        display_weather_init(anim_type);
        lv_label_set_text(cityLabel, weaInfo.cityname);
        lv_img_set_src(weatherImg, weaImage_map[weaInfo.weather_code]);
        // 下面这行代码可能会出错
        lv_label_set_text_fmt(txtLabel, "最低气温%d°C, 最高气温%d°C, %s风%s级.   ",
                            weaInfo.minTemp, weaInfo.maxTemp, weaInfo.windDir, weaInfo.windLevel);

        lv_bar_set_value(tempBar, weaInfo.temperature, LV_ANIM_ON);
        lv_label_set_text_fmt(tempLabel, "%2d°C", weaInfo.temperature);
        lv_bar_set_value(humiBar, weaInfo.humidity, LV_ANIM_ON);
        lv_label_set_text_fmt(humiLabel, "%d%%", weaInfo.humidity);
    }
    else
    {

    }

    // 保证活动界面没有问题
    if (LV_SCR_LOAD_ANIM_NONE != anim_type)
    {
        lv_scr_load_anim(scr_1, anim_type, 300, 300, false);
    }
    else
    {
        lv_scr_load(scr_1);
    }

}

void display_time(struct TimeStr timeInfo, lv_scr_load_anim_t anim_type)
{
    display_weather_init(anim_type);
    // 使用#ffa500对"52"进行颜色标注
    lv_label_set_text_fmt(clockLabel_1, "%02d#ffa500 %02d#", timeInfo.hour, timeInfo.minute);
    lv_label_set_text_fmt(clockLabel_2, "%02d", timeInfo.second);
    lv_label_set_text_fmt(dateLabel, "%2d月%2d日  星期%s", timeInfo.month, timeInfo.day,
                          weekDayCh[timeInfo.weekday]);
    
    // 删除Loading界面
    if (loading_scr != NULL) {
        lv_obj_del(loading_scr);
        loading_scr = NULL;
        loading_label = NULL;
    }

    if (LV_SCR_LOAD_ANIM_NONE != anim_type)
    {
        lv_scr_load_anim(scr_1, anim_type, 300, 300, false);
    }
    else
    {
        lv_scr_load(scr_1);
    }
}

void weather_gui_release(void)
{
    if (scr_1 != NULL)
    {
        lv_obj_clean(scr_1);
        scr_1 = NULL;
        weatherImg = NULL;
        cityLabel = NULL;
        btn = NULL;
        btnLabel = NULL;
        txtLabel = NULL;
        clockLabel_1 = NULL;
        clockLabel_2 = NULL;
        dateLabel = NULL;
        tempImg = NULL;
        tempBar = NULL;
        tempLabel = NULL;
        humiImg = NULL;
        humiBar = NULL;
        humiLabel = NULL;
        spaceImg = NULL;
        loading_scr = NULL;
        loading_label = NULL;
    }

    if (scr_2 != NULL)
    {
        lv_obj_clean(scr_2);
        scr_2 = NULL;
        for (int i = 0; i < 6; i++) {
            btn_daily[i] = NULL;
            btnLabel_daily[i] = NULL;
        }
        for (int i = 0; i < 3; i++)
        {
            dailyImg[i] = NULL;
            dayLabel[i] = NULL;
        }
        spaceImg = NULL;
    }
}

void weather_gui_del(void)
{
    weather_gui_release();
}

void display_space(void)
{
    static int _spaceIndex = 0;
    if (NULL != scr_1 || NULL != scr_2)
    {
        lv_img_set_src(spaceImg, manImage_map[_spaceIndex]);
        _spaceIndex = (_spaceIndex + 1) % 10;
    }
}