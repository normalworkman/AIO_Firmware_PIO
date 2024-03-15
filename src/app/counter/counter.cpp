#include "counter.h"
#include "counter_gui.h"
#include "sys/app_controller.h"
#include "common.h"

#define COUNTER_APP_NAME "Counter"
#define COUNTER_CONFIG_PATH "/counter.cfg"
#define COUNTER_PAGE_SIZE 6 // 5分钟 10分钟 20分钟 30分钟 1个小时 2个小时

// 动态数据，APP的生命周期结束也需要释放它
struct CounterAppRunData
{
    long long initTimestamp;
    long long preTimestamp;
    unsigned int counterTime;
    /**
     * @brief counter功能的状态指示
     * counter == -1 first time entry
     * counter == 0 idle状态
     * counter == 1 running状态
     * counter == 2 counter完成状态
     */
    int counterState;
    Counter cnt;
};

struct CounterAppForeverData
{
    int mainColour;
};

static void write_config(CounterAppForeverData *cfg)
{
    char tmp[16];
    // 将配置数据保存在文件中（持久化）
    String w_data;
    memset(tmp, 0, 16);
    snprintf(tmp, 16, "%lu\n", cfg->mainColour);
    w_data += tmp;
    g_flashCfg.writeFile(COUNTER_CONFIG_PATH, w_data.c_str());
}

static void read_config(CounterAppForeverData *cfg)
{
    // 如果有需要持久化配置文件 可以调用此函数将数据存在flash中
    // 配置文件名最好以APP名为开头 以".cfg"结尾，以免多个APP读取混乱
    char info[128] = {0};
    uint16_t size = g_flashCfg.readFile(COUNTER_CONFIG_PATH, (uint8_t *)info);
    info[size] = 0;
    if (size == 0)
    {
        cfg->mainColour = 2; // 默认为紫罗兰
        write_config(cfg);
    }
    else
    {
        // 解析数据
        char *param[1] = {0};
        analyseParam(info, 1, param);
        cfg->mainColour = atol(param[0]);
    }
}

// 保存APP运行时的参数信息，理论上关闭APP时推荐在 xxx_exit_callback 中释放掉
static CounterAppRunData *run_data = NULL;
static unsigned int CountLimit[6] = {5, 10, 20, 30, 60, 120};

// 当然你也可以添加恒定在内存中的少量变量（退出时不用释放，实现第二次启动时可以读取）
// 考虑到所有的APP公用内存，尽量减少 forever_data 的数据占用
static CounterAppForeverData forever_data;

static int counter_init(AppController *sys)
{
    // 初始化运行时的参数
    counter_gui_init();
    // 获取配置信息
    read_config(&forever_data);
    // 初始化运行时参数
    run_data = (CounterAppRunData *)calloc(1, sizeof(CounterAppRunData));
    memset((char *)&run_data->cnt, 0, sizeof(Counter));
    run_data->initTimestamp = 0;
    run_data->preTimestamp = 0;
    run_data->counterTime = 0;
    run_data->counterState = -1;
    run_data->cnt.page = 0;
    run_data->cnt.progress = 100;
    run_data->cnt.colour = forever_data.mainColour;
    // 使用 forever_data 中的变量，任何函数都可以用
    // Serial.print(forever_data.val1);

    // 如果有需要持久化配置文件 可以调用此函数将数据存在flash中
    // 配置文件名最好以APP名为开头 以".cfg"结尾，以免多个APP读取混乱
    // char info[128] = {0};
    // uint16_t size = g_flashCfg.readFile("/example.cfg", (uint8_t *)info);
    // // 解析数据
    // // 将配置数据保存在文件中（持久化）
    // g_flashCfg.writeFile("/example.cfg", "value1=100\nvalue2=200");
    
    return 0;
}

static void counter_process(AppController *sys,
                            const ImuAction *act_info)
{
    lv_scr_load_anim_t anim_type = LV_SCR_LOAD_ANIM_NONE;

    if (RETURN == act_info->active)
    {
        if (run_data->counterState == 1)
        {
            run_data->counterState = 0;
            run_data->initTimestamp = 0;
            run_data->preTimestamp = 0;
            run_data->counterTime = 0;
        }
        else
        {
            sys->app_exit(); // 退出APP
            return;
        }
    }
    else if (GO_FORWORD == act_info->active)
    {
        // counter restart
        if (run_data->counterState == 2)
        {
            run_data->counterState = 0;
            run_data->initTimestamp = 0;
            run_data->preTimestamp = 0;
            run_data->counterTime = 0;
        }
        else if (run_data->counterState == 0)
        {
            // 开始counter计数 if counterState == 1
            run_data->counterState = 1;
            // 更新时间
            run_data->initTimestamp = GET_SYS_MILLIS();
            run_data->preTimestamp = GET_SYS_MILLIS();
            run_data->counterTime = CountLimit[run_data->cnt.page] * 60;
        }
    }

    // page selection if counterState == 0 or counterState == -1
    if (TURN_RIGHT == act_info->active)
    {
        if (run_data->counterState == -1 || run_data->counterState == 0)
        {
            run_data->cnt.page = (run_data->cnt.page + 1 + run_data->counterState) % COUNTER_PAGE_SIZE;
            run_data->counterState = 0;
        }
        else if (run_data->counterState == 1)
        {
            run_data->cnt.mode = (run_data->cnt.mode + 1) % 2;
        }
        
    }
    else if (TURN_LEFT == act_info->active)
    {
        if (run_data->counterState == -1 || run_data->counterState == 0)
        {
            run_data->cnt.page = (run_data->cnt.page + COUNTER_PAGE_SIZE - 1) % COUNTER_PAGE_SIZE;
            run_data->counterState = 0;
        }
        else if (run_data->counterState == 1)
        {
            run_data->cnt.mode = (run_data->cnt.mode + 1) % 2;
        }
    }

    if (run_data->counterState == -1)
    {
        display_start(anim_type);
    }
    else if (run_data->counterState == 2)
    {
        display_success(anim_type);
        delay(600);
    }
    // from idle to counter mode 
    else if (run_data->counterState == 1)
    {
        if (GET_SYS_MILLIS() - run_data->initTimestamp <= CountLimit[run_data->cnt.page] * 60000)
        {
            if (GET_SYS_MILLIS() - run_data->preTimestamp >= 1000)
            {
                run_data->preTimestamp = GET_SYS_MILLIS();
                run_data->counterTime -= 1;
                run_data->cnt.min = run_data->counterTime / 60;
                run_data->cnt.second = run_data->counterTime % 60;
                run_data->cnt.progress = (run_data->counterTime * 5 ) / (CountLimit[run_data->cnt.page] * 3);
                // 每秒刷新一下时间
                display_counter_scr(run_data->cnt, anim_type);
            }
        }
        else
        {
            // counter success
            run_data->counterState = 2;
        }        
    }
    else
    {
        // 保持倒数值不变
        run_data->cnt.min = CountLimit[run_data->cnt.page];
        run_data->cnt.second = 0;
        run_data->cnt.progress = 100;
        display_counter_scr(run_data->cnt, anim_type);
    }

}

static void example_background_task(AppController *sys,
                                    const ImuAction *act_info)
{
    // 本函数为后台任务，主控制器会间隔一分钟调用此函数
    // 本函数尽量只调用"常驻数据",其他变量可能会因为生命周期的缘故已经释放

    // 发送请求。如果是wifi相关的消息，当请求完成后自动会调用 example_message_handle 函数
    // sys->send_to(EXAMPLE_APP_NAME, CTRL_NAME,
    //              APP_MESSAGE_WIFI_CONN, (void *)run_data->val1, NULL);

    // 也可以移除自身的后台任务，放在本APP可控的地方最合适
    // sys->remove_backgroud_task();

    // 程序需要时可以适当加延时
    // delay(300);
}

static int counter_exit_callback(void *param)
{
    counter_gui_del();

    // 释放资源
    if (NULL != run_data)
    {
        free(run_data);
        run_data = NULL;
    }
    return 0;
}

static void example_message_handle(const char *from, const char *to,
                                   APP_MESSAGE_TYPE type, void *message,
                                   void *ext_info)
{
    // 目前主要是wifi开关类事件（用于功耗控制）
    switch (type)
    {
    case APP_MESSAGE_WIFI_CONN:
    {
        // todo
    }
    break;
    case APP_MESSAGE_WIFI_AP:
    {
        // todo
    }
    break;
    case APP_MESSAGE_WIFI_ALIVE:
    {
        // wifi心跳维持的响应 可以不做任何处理
    }
    break;
    case APP_MESSAGE_GET_PARAM:
    {
        char *param_key = (char *)message;
        if (!strcmp(param_key, "mainColour"))
        {
            snprintf((char *)ext_info, 32, "%lu", forever_data.mainColour);
        }
        else
        {
            snprintf((char *)ext_info, 32, "%s", "NULL");
        }
    }
    break;
    case APP_MESSAGE_SET_PARAM:
    {
        char *param_key = (char *)message;
        char *param_val = (char *)ext_info;
        
        if (!strcmp(param_key, "mainColour"))
        {
            forever_data.mainColour = atol(param_val);
        }
    }
    break;
    case APP_MESSAGE_READ_CFG:
    {
        read_config(&forever_data);
    }
    break;
    case APP_MESSAGE_WRITE_CFG:
    {
        write_config(&forever_data);
    }
    break;
    default:
        break;
    }
}

APP_OBJ counter_app = {COUNTER_APP_NAME, &app_counter, "",
                       counter_init, counter_process, example_background_task,
                       counter_exit_callback, example_message_handle};
