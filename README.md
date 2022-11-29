# coroutine_ucontext

## 简介 
* 使用ucontext 搭建linux轻量级 协程测试
* task 为 协程任务
* exector 线程调度器
* main下简单测试
* 添加mutex、condition_variable
* 添加channel

##编译环境
* 使用的是gcc 支持 C++17 (推荐gcc 8.5以上)
* makefile生成的是 libcoroutine.a 静态库

##快速体验
* mkdir build
make
生成静态库

* cd main 
make
生成测试可执行文件
