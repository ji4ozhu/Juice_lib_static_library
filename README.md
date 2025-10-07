这个是个备用库，方便下次直接放在自己的项目里使用。
使用 https://github.com/paullouisageneau/libjuice/releases/tag/v1.6.2
改成了 Windows C++ 的 Lib 和 DLL
里面有 x86/x64/Arm64 3个版本。
还有头文件和示例


This is a backup library that can be used directly in your own project next time. 
I changed to Windows C++ with https://github.com/paullouisageneau/libjuice/releases/tag/v1.6.2 
and 3 versions of x86/x64/Arm64 in the lib and DLL. 
There are also header files and examples


❌ 不可行的流程：
你的程序绑定 0.0.0.0:5678  ← 端口被占用
    ↓
simple_nat.exe 5678  ← 尝试绑定同一端口
    ↓
ERROR: 端口已被占用！

// 你的程序
int main() {
    // 1. 先初始化 NAT Manager
    nat_manager_init(NULL);
    
    // 2. 添加端口进行 NAT 穿透
    int mapping_id = nat_manager_add_port("0.0.0.0", 5678);
    
    // 3. 等待打洞完成
    Sleep(3000);
    
    // 4. 获取公网地址
    nat_mapping_t mapping;
    nat_manager_get_mapping(mapping_id, &mapping);
    printf("公网地址: %s:%u\n", mapping.public_ip, mapping.public_port);
    
    // 5. 现在你可以使用这个端口
    //    NAT 映射已建立，自动保活
    //    你的业务逻辑...
    
    // 注意：不要再自己创建 UDP socket 绑定 5678
    //      因为 NAT Manager 已经绑定了这个端口
    
    // 6. 程序退出前清理
    nat_manager_cleanup();
}



方案 2：让你的程序使用不同的端口
你的UDP服务: 0.0.0.0:5678 (本地业务)
    +
NAT工具打洞: 0.0.0.0:5679 (NAT穿透)
    ↓
告诉客户端连接: 59.50.87.159:5679
    ↓
路由器转发到: 0.0.0.0:5679
    ↓
你的程序监听端口转发: 5679 → 5678


方案 3：Socket 共享（复杂，不推荐）
Windows 支持 SO_REUSEADDR，但：
⚠️ 实现复杂
⚠️ 容易出错
⚠️ libjuice 不支持外部创建的 socket
推荐做法
将 NAT Manager 集成到你的程序，这样：
✅ NAT Manager 内部创建并管理 UDP socket
✅ 自动进行 NAT 穿透和保活
✅ 你的程序通过 NAT Manager 的接口发送/接收数据
✅ 一切在同一个进程内完成
