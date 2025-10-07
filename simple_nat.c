/**
 * Simple NAT Tool -  NAT 穿透工具（支持保活）
 * 用法: simple_nat.exe <端口>
 * 示例: simple_nat.exe 5678
 * 基于 https://github.com/paullouisageneau/libjuice/releases/tag/v1.6.2
 * 这个Lib库是为了方便自己用
 */

#include "juice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define msleep(ms) Sleep(ms)
#else
#include <unistd.h>
#define msleep(ms) usleep((ms)*1000)
#endif

// 内置3个STUN服务器
static const char* STUN_SERVERS[][2] = {
    {"stun1.l.google.com", "19302"},
    {"stun.hot-chilli.net", "3478"},
    {"stun.miwifi.com", "3478"}
};

static juice_agent_t *agents[3] = {0};

// 静默回调
static void cb_state(juice_agent_t *a, juice_state_t s, void *p) { (void)a;(void)s;(void)p; }
static void cb_cand(juice_agent_t *a, const char *d, void *p) { (void)a;(void)d;(void)p; }
static void cb_done(juice_agent_t *a, void *p) { (void)a;(void)p; }
static void cb_recv(juice_agent_t *a, const char *d, size_t s, void *p) { (void)a;(void)d;(void)s;(void)p; }

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <port>\n", argv[0]);
        printf("Example: %s 5678\n", argv[0]);
        return -1;
    }

    juice_set_log_level(JUICE_LOG_LEVEL_NONE);
    uint16_t port = (uint16_t)atoi(argv[1]);

    // 创建3个agent，每个使用不同的STUN服务器
    for (int i = 0; i < 3; i++) {
        juice_config_t cfg = {0};
        cfg.stun_server_host = STUN_SERVERS[i][0];
        cfg.stun_server_port = (uint16_t)atoi(STUN_SERVERS[i][1]);
        cfg.bind_address = NULL;
        cfg.local_port_range_begin = port;
        cfg.local_port_range_end = port;
        cfg.cb_state_changed = cb_state;
        cfg.cb_candidate = cb_cand;
        cfg.cb_gathering_done = cb_done;
        cfg.cb_recv = cb_recv;

        agents[i] = juice_create(&cfg);
        if (agents[i]) {
            juice_gather_candidates(agents[i]);
        }
    }

    // 快速轮询等待结果
    for (int t = 0; t < 15; t++) {
        msleep(200);
        for (int i = 0; i < 3; i++) {
            if (agents[i] && juice_get_stun_mapped_addresses(agents[i], NULL, 0, 0) > 0) {
                goto results_ready;
            }
        }
    }
results_ready:

    // 收集所有唯一的公网地址
    char results[10][JUICE_MAX_ADDRESS_STRING_LEN];
    int count = 0;

    for (int i = 0; i < 3; i++) {
        if (!agents[i]) continue;

        char addrs[10][JUICE_MAX_ADDRESS_STRING_LEN];
        int n = juice_get_stun_mapped_addresses(agents[i], (char*)addrs, 
                                                JUICE_MAX_ADDRESS_STRING_LEN, 10);
        
        for (int j = 0; j < n; j++) {
            int dup = 0;
            for (int k = 0; k < count; k++) {
                if (strcmp(addrs[j], results[k]) == 0) {
                    dup = 1;
                    break;
                }
            }
            if (!dup && count < 10) {
                strcpy(results[count++], addrs[j]);
            }
        }
    }

    // 输出结果
    for (int i = 0; i < count; i++) {
        printf("%s\n", results[i]);
    }

    // 保活模式：持续运行，保持NAT映射
    // agents 会自动发送 keepalive，维持映射
    int keepalive_count = 0;
    while (1) {
        msleep(15000);  // 每15秒keepalive
        keepalive_count++;
        printf("keepalive %d\n", keepalive_count);
    }

    // 清理（实际不会执行到）
    for (int i = 0; i < 3; i++) {
        if (agents[i]) juice_destroy(agents[i]);
    }

    return 0;
}


