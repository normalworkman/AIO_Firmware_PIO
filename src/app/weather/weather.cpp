#include "weather.h"
#include "weather_gui.h"
#include "ESP32Time.h"
#include "sys/app_controller.h"
#include "network.h"
#include "common.h"
#include "ArduinoJson.h"
#include <esp32-hal-timer.h>
#include <map>

#define TIME_API "http://api.m.taobao.com/rest/api3.do?api=mtop.common.gettimestamp"
#define WEATHER_DALIY_API "https://restapi.amap.com/v3/weather/weatherInfo?key=%s&city=%s&extensions=all"
#define WEATHER_NOW_API "https://restapi.amap.com/v3/weather/weatherInfo?key=%s&city=%s&extensions=base"
#define ZHIXIN_WEATHER_API "https://api.seniverse.com/v3/weather/now.json?key=%s&location=%s&language=zh-Hans&unit=c"
#define WEATHER_PAGE_SIZE    2
#define UPDATE_WEATHER       0x01  // 更新天气
#define UPDATE_DALIY_WEATHER 0x02  // 更新每天天气
#define UPDATE_TIME          0x04  // 更新时间

// 天气的持久化配置
#define WEATHER_CONFIG_PATH "/weather.cfg"
struct WT_Config
{
    String tianqi_key[2];                // tianqiapid 的 key
    String tianqi_addr;                  // tianqiapid 的地址（填中文）
    unsigned long weatherUpdataInterval; // 天气更新的时间间隔(s)
    unsigned long timeUpdataInterval;    // 日期时钟更新的时间间隔(s)
    int wea_mode;                        // mode == 0 极简 mode == 1 太空人 
};

static void write_config(WT_Config *cfg)
{
    char tmp[16];
    // 将配置数据保存在文件中（持久化）
    String w_data;
    w_data = w_data + cfg->tianqi_key[0] + "\n";
    w_data = w_data + cfg->tianqi_key[1] + "\n";
    w_data = w_data + cfg->tianqi_addr + "\n";
    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%lu\n", cfg->weatherUpdataInterval);
    w_data += tmp;
    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%lu\n", cfg->timeUpdataInterval);
    w_data += tmp;
    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%d\n", cfg->wea_mode);
    w_data += tmp;
    g_flashCfg.writeFile(WEATHER_CONFIG_PATH, w_data.c_str());
}

static void read_config(WT_Config *cfg)
{
    // 如果有需要持久化配置文件 可以调用此函数将数据存在flash中
    // 配置文件名最好以APP名为开头 以".cfg"结尾，以免多个APP读取混乱
    char info[128] = {0};
    uint16_t size = g_flashCfg.readFile(WEATHER_CONFIG_PATH, (uint8_t *)info);
    info[size] = 0;
    if (size == 0)
    {
        // 默认值
        cfg->tianqi_addr = "东莞";
        cfg->tianqi_key[0] = "Sln0hpEcsjS4ARoFC";
        cfg->tianqi_key[1] = "c51421c9dd619007f4d77a9fc2755c51";
        cfg->wea_mode = 0;
        cfg->weatherUpdataInterval = 900000; // 天气更新的时间间隔900000(900s)
        cfg->timeUpdataInterval = 900000;    // 日期时钟更新的时间间隔900000(900s)
        write_config(cfg);
    }
    else
    {
        // 解析数据
        char *param[6] = {0};
        analyseParam(info, 6, param);
        cfg->tianqi_key[0] = param[0];
        cfg->tianqi_key[1] = param[1];
        cfg->tianqi_addr = param[2];
        cfg->weatherUpdataInterval = atol(param[3]);
        cfg->timeUpdataInterval = atol(param[4]);
        cfg->wea_mode = atol(param[5]);
    }
}

struct WeatherAppRunData
{
    unsigned long preWeatherMillis;        // 上一回更新天气时的毫秒数
    unsigned long preTimeMillis;           // 更新时间计数器
    long long preNetTimestamp;             // 上一次的网络时间戳
    long long errorNetTimestamp;           // 网络到显示过程中的时间误差
    long long preLocalTimestamp;           // 上一次的本地机器时间戳
    unsigned int coactusUpdateFlag;        // 强制更新标志
    int clock_page;
    ESP32Time g_rtc;                       // 用于时间解码
    Weather wea;                           // 保存天气状况
};

static WT_Config cfg_data;
static WeatherAppRunData *run_data = NULL;
static boolean isNight = false;

enum wea_event_Id
{
    UPDATE_NOW,
    UPDATE_NTP,
    UPDATE_DAILY
};

std::map<String, int> weatherMap = {{"qing", 0}, {"yin", 1}, {"yu", 2}, {"yun", 3}, {"bingbao", 4}, {"wu", 5}, {"shachen", 6}, {"lei", 7}, {"xue", 8}, {"unknown", 0}};

String convertToPy(const String& chineseWeather) {
    std::map<String, String> chineseToPy = {
        {"晴", "qing"},
        {"云", "yun"},{"晴间多云", "yun"},{"少云", "yun"},{"多云", "yun"},
        {"阴", "yin"},
        {"霾", "shachen"},{"中度霾", "shachen"},{"重度霾", "shachen"},{"严重霾", "shachen"},
        {"雨", "yu"},{"大雨", "yu"},{"中雨", "yu"},{"小雨", "yu"},{"阵雨", "yu"},{"暴雨", "yu"},{"大暴雨", "yu"},{"特大暴雨", "yu"},
        {"强阵雨", "yu"},{"极端降雨", "yu"},{"毛毛雨/细雨", "yu"},
        {"小雨-中雨", "yu"},{"中雨-大雨", "yu"},{"大雨-暴雨", "yu"},{"暴雨-大暴雨", "yu"},{"大暴雨-特大暴雨", "yu"},
        {"冰雹", "bingbao"},{"冻雨", "bingbao"},
        {"雾", "wu"},{"浓雾", "wu"},{"强浓雾", "wu"},{"轻雾", "wu"},{"大雾", "wu"},{"特强浓雾", "wu"},
        {"浮尘", "shachen"},{"扬沙", "shachen"},{"沙尘暴", "shachen"},{"强沙尘暴", "shachen"},{"龙卷风", "shachen"},
        {"雷", "lei"},{"雷阵雨", "lei"},{"强雷阵雨", "lei"},
        {"雪", "xue"},{"大雪", "xue"},{"中雪", "xue"},{"小雪", "xue"},{"暴雪", "xue"},{"阵雪", "xue"},
        {"雨雪天气", "xue"},{"雨夹雪", "xue"},{"阵雨夹雪", "xue"},
        {"小雪-中雪", "xue"},{"中雪-大雪", "xue"},{"大雪-暴雪", "xue"}
    };

    auto it = chineseToPy.find(chineseWeather);
    if (it != chineseToPy.end()) {
        return it->second;
    } else {
        return "unknown";
    }
}

static void get_weather(void)
{
    if (WL_CONNECTED != WiFi.status())
        return;

    HTTPClient http;
    http.setTimeout(1000);
    char api[128] = {0};
    if (cfg_data.wea_mode == 0)
    {
        snprintf(api, 128, ZHIXIN_WEATHER_API,
             cfg_data.tianqi_key[0].c_str(),
             cfg_data.tianqi_addr.c_str());
        http.begin(api);

        int httpCode = http.GET();
        if (httpCode > 0)
        {
            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
                String payload = http.getString();
                DynamicJsonDocument doc(1024);
                deserializeJson(doc, payload);
                JsonObject sk = doc.as<JsonObject>();
                strcpy(run_data->wea.cityname, sk["results"][0]["location"]["name"].as<String>().c_str());
                run_data->wea.temperature = sk["results"][0]["now"]["temperature"].as<int>();
                run_data->wea.code = sk["results"][0]["now"]["code"].as<int>();
            }
        }
        http.end();
    }
    else if (cfg_data.wea_mode == 1)
    {
        snprintf(api, 128, WEATHER_NOW_API,
             cfg_data.tianqi_key[1].c_str(),
             cfg_data.tianqi_addr.c_str());
        http.begin(api);

        int httpCode = http.GET();
        if (httpCode > 0)
        {
            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
                String payload = http.getString();
                DynamicJsonDocument doc(1024);
                deserializeJson(doc, payload);
                JsonObject sk = doc.as<JsonObject>();
                strcpy(run_data->wea.cityname, sk["lives"][0]["city"].as<String>().c_str());
                run_data->wea.temperature = sk["lives"][0]["temperature"].as<int>();
                // 获取湿度
                run_data->wea.humidity = sk["lives"][0]["humidity"].as<int>();
                strcpy(run_data->wea.windDir, sk["lives"][0]["winddirection"].as<String>().c_str()); 
            }
        }
        http.end();
    }
    
}

static long long get_timestamp()
{
    // 使用本地的机器时钟
    run_data->preNetTimestamp = run_data->preNetTimestamp + (GET_SYS_MILLIS() - run_data->preLocalTimestamp);
    run_data->preLocalTimestamp = GET_SYS_MILLIS();
    return run_data->preNetTimestamp;
}

static long long get_timestamp(String url)
{
    if (WL_CONNECTED != WiFi.status())
        return 0;

    String time = "";
    HTTPClient http;
    http.setTimeout(1000);
    http.begin(url);

    int httpCode = http.GET();
    if (httpCode > 0)
    {
        if (httpCode == HTTP_CODE_OK)
        {
            String payload = http.getString();
            int time_index = (payload.indexOf("data")) + 12;
            time = payload.substring(time_index, payload.length() - 3);
            // 以网络时间戳为准
            run_data->preNetTimestamp = atoll(time.c_str()) + run_data->errorNetTimestamp + TIMEZERO_OFFSIZE;
            run_data->preLocalTimestamp = GET_SYS_MILLIS();
        }
    }
    else
    {
        // 得不到网络时间戳时
        run_data->preNetTimestamp = run_data->preNetTimestamp + (GET_SYS_MILLIS() - run_data->preLocalTimestamp);
        run_data->preLocalTimestamp = GET_SYS_MILLIS();
    }
    http.end();

    return run_data->preNetTimestamp;
}

static void get_daliyWeather(void)
{
    if (WL_CONNECTED != WiFi.status())
        return;

    HTTPClient http;
    http.setTimeout(1000);
    char api[128] = {0};
    snprintf(api, 128, WEATHER_DALIY_API,
            cfg_data.tianqi_key[1].c_str(),
            cfg_data.tianqi_addr.c_str());
    http.begin(api);

    int httpCode = http.GET();
    if (httpCode > 0)
    {
        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
        {
            String payload = http.getString();
            DynamicJsonDocument doc(2048);
            deserializeJson(doc, payload);
            JsonObject sk = doc.as<JsonObject>();
            // 获取温度
            run_data->wea.maxTemp = sk["forecasts"][0]["casts"][0]["daytemp"].as<int>();
            run_data->wea.minTemp = sk["forecasts"][0]["casts"][0]["nighttemp"].as<int>();
            if (isNight == false) 
            {
                strcpy(run_data->wea.windLevel, sk["forecasts"][0]["casts"][0]["daypower"].as<String>().c_str());
                run_data->wea.weather_code = weatherMap[convertToPy(sk["forecasts"][0]["casts"][0]["dayweather"].as<String>())];
            }
            else
            {
                strcpy(run_data->wea.windLevel, sk["forecasts"][0]["casts"][0]["nightpower"].as<String>().c_str());
                run_data->wea.weather_code = weatherMap[convertToPy(sk["forecasts"][0]["casts"][0]["nightweather"].as<String>())];
            }
    
            for (int GDW_i = 1; GDW_i < 4; GDW_i++)
            {
                String weatherDescription;
                if (isNight == false) weatherDescription = sk["forecasts"][0]["casts"][GDW_i]["dayweather"].as<String>();
                else weatherDescription = sk["forecasts"][0]["casts"][GDW_i]["nightweather"].as<String>();
                run_data->wea.daily_code[GDW_i-1] = weatherMap[convertToPy(weatherDescription)];
                run_data->wea.daily_max[GDW_i-1] = sk["forecasts"][0]["casts"][GDW_i]["daytemp"].as<int>();
                run_data->wea.daily_min[GDW_i-1] = sk["forecasts"][0]["casts"][GDW_i]["nighttemp"].as<int>();
                run_data->wea.dayofWeek[GDW_i-1] = sk["forecasts"][0]["casts"][GDW_i]["week"].as<int>() - 1;
            } 
        }
    }
    
    http.end();
}

// 刷新时间
static void UpdateTime_RTC(long long timestamp)
{
    struct TimeStr t;
    run_data->g_rtc.setTime(timestamp / 1000);
    t.month = run_data->g_rtc.getMonth() + 1;
    t.day = run_data->g_rtc.getDay();
    t.hour = run_data->g_rtc.getHour(true);
    t.minute = run_data->g_rtc.getMinute();
    t.second = run_data->g_rtc.getSecond();
    t.weekday = run_data->g_rtc.getDayofWeek();
    if (t.hour >= 18) isNight = true;
    else isNight = false;
    display_time(t, LV_SCR_LOAD_ANIM_NONE);
}

static int weather_init(AppController *sys)
{
    tft->setSwapBytes(true);
    weather_gui_init();
    // 获取配置信息
    read_config(&cfg_data);

    // 初始化运行时参数
    run_data = (WeatherAppRunData *)calloc(1, sizeof(WeatherAppRunData));
    memset((char *)&run_data->wea, 0, sizeof(Weather));
    run_data->preNetTimestamp = 1577808000000; // 上一次的网络时间戳 初始化为2020-01-01 00:00:00
    run_data->errorNetTimestamp = 2;
    run_data->preLocalTimestamp = GET_SYS_MILLIS(); // 上一次的本地机器时间戳
    run_data->clock_page = 0;
    run_data->preWeatherMillis = 0;
    run_data->preTimeMillis = 0;
    // 强制更新
    run_data->coactusUpdateFlag = 0x01;
    strcpy(run_data->wea.cityname, "东莞");

    return 0;
}

static void weather_process(AppController *sys,
                            const ImuAction *act_info)
{
    lv_scr_load_anim_t anim_type = LV_SCR_LOAD_ANIM_NONE;

    if (RETURN == act_info->active)
    {
        sys->app_exit();
        return;
    }
    else if (GO_FORWORD == act_info->active)
    {
        // 间接强制更新
        run_data->coactusUpdateFlag = 0x01;
        delay(500); // 以防间接强制更新后，生产很多请求 使显示卡顿
    }
    else if (TURN_RIGHT == act_info->active)
    {
        anim_type = LV_SCR_LOAD_ANIM_MOVE_RIGHT;
        run_data->clock_page = (run_data->clock_page + 1) % WEATHER_PAGE_SIZE;
    }
    else if (TURN_LEFT == act_info->active)
    {
        anim_type = LV_SCR_LOAD_ANIM_MOVE_LEFT;
        // 以下等效与 clock_page = (clock_page + WEATHER_PAGE_SIZE - 1) % WEATHER_PAGE_SIZE;
        // +3为了不让数据溢出成负数，而导致取模逻辑错误
        run_data->clock_page = (run_data->clock_page + WEATHER_PAGE_SIZE - 1) % WEATHER_PAGE_SIZE;
    }

    // 界面刷新
    if (run_data->clock_page == 0)
    {
        display_weather(run_data->wea, anim_type, cfg_data.wea_mode);

        if (cfg_data.wea_mode == 1)
        {
            if (0x01 == run_data->coactusUpdateFlag || doDelayMillisTime(cfg_data.timeUpdataInterval, &run_data->preTimeMillis, false))
            {
                // 尝试同步网络上的时钟
                sys->send_to(WEATHER_APP_NAME, CTRL_NAME,
                            APP_MESSAGE_WIFI_CONN, (void *)UPDATE_NTP, NULL);
            }
            else if (GET_SYS_MILLIS() - run_data->preLocalTimestamp > 400)
            {
                UpdateTime_RTC(get_timestamp());
            }
        }
        
        if (0x01 == run_data->coactusUpdateFlag || doDelayMillisTime(cfg_data.weatherUpdataInterval, &run_data->preWeatherMillis, false))
        {
            sys->send_to(WEATHER_APP_NAME, CTRL_NAME,
                        APP_MESSAGE_WIFI_CONN, (void *)UPDATE_DAILY, NULL);
            sys->send_to(WEATHER_APP_NAME, CTRL_NAME,
                        APP_MESSAGE_WIFI_CONN, (void *)UPDATE_NOW, NULL);
        }
        
        run_data->coactusUpdateFlag = 0x00; // 取消强制更新标志
        if (cfg_data.wea_mode == 1) display_space();
        delay(60);

    }
    else if (run_data->clock_page == 1)
    {
        // 仅在切换界面时获取一次未来天气
        display_daily(run_data->wea, anim_type);
        display_space();
        delay(60);
    }
}

static void weather_background_task(AppController *sys,
                                    const ImuAction *act_info)
{
    // 本函数为后台任务，主控制器会间隔一分钟调用此函数
    // 本函数尽量只调用"常驻数据",其他变量可能会因为生命周期的缘故已经释放
}

static int weather_exit_callback(void *param)
{
    weather_gui_del();

    // 释放运行数据
    if (NULL != run_data)
    {
        free(run_data);
        run_data = NULL;
    }
    return 0;
}

static void weather_message_handle(const char *from, const char *to,
                                   APP_MESSAGE_TYPE type, void *message,
                                   void *ext_info)
{
    switch (type)
    {
    case APP_MESSAGE_WIFI_CONN:
    {
        int event_id = (int)message;
        switch (event_id)
        {
        case UPDATE_NOW:
        {
            get_weather();
            if (run_data->clock_page == 0)
            {
                display_weather(run_data->wea, LV_SCR_LOAD_ANIM_NONE, cfg_data.wea_mode);
            }
        };
        break;
        case UPDATE_NTP:
        {
            long long timestamp = get_timestamp(TIME_API); // nowapi时间API
            if (run_data->clock_page == 0)
            {
                UpdateTime_RTC(timestamp);
            }
        };
        break;
        case UPDATE_DAILY:
        {
            get_daliyWeather();
            if (run_data->clock_page == 1)
            {
                display_daily(run_data->wea, LV_SCR_LOAD_ANIM_NONE);
            }
        };
        break;
        default:
            break;
        }
    }
    break;
    case APP_MESSAGE_GET_PARAM:
    {
        char *param_key = (char *)message;
        if (!strcmp(param_key, "tianqi_addr"))
        {
            snprintf((char *)ext_info, 32, "%s", cfg_data.tianqi_addr.c_str());
        }
        else if
        (!strcmp(param_key, "wea_mode"))
        {
            snprintf((char *)ext_info, 32, "%d", cfg_data.wea_mode);
        }
    }
    break;
    case APP_MESSAGE_SET_PARAM:
    {
        char *param_key = (char *)message;
        char *param_val = (char *)ext_info;
        
        if (!strcmp(param_key, "tianqi_addr"))
        {
            cfg_data.tianqi_addr = param_val;
        }
        else if (!strcmp(param_key, "wea_mode"))
        {
            cfg_data.wea_mode = atol(param_val);
        }
    }
    break;
    case APP_MESSAGE_READ_CFG:
    {
        read_config(&cfg_data);
    }
    break;
    case APP_MESSAGE_WRITE_CFG:
    {
        write_config(&cfg_data);
    }
    break;
    default:
        break;
    }
}

APP_OBJ weather_app = {WEATHER_APP_NAME, &app_weather, "",
                       weather_init, weather_process, weather_background_task,
                       weather_exit_callback, weather_message_handle};
