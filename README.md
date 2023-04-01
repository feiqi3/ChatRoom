# ChatRoom
## Introduction     

This my OS final project, written by C++ and running in unix-like system (Only tested on ubuntu22.04)      

这是我的OS课程设计，运行在Unix命令行上的聊天软件，编译环境：posix系统，gcc10以上，cmake 3.13以上。     
在ubuntu22.04测试通过。     
</br>

-----------    
</br>

使用了许多C++17特性（<small>*还有少量C++20特性和非常多C++98的封建余孽*</small>     
实现非常不优雅，充满了并发问题死锁和各种奇奇怪怪的寄点      
（第一次写网络程序, 第一次用Socket QWQ      

支持一对一聊天和群组聊天（所有人在一个群里的那种）   
</br>


## 运行方法     

```shell  
mkdir build 
cd build 
make -j 
```     

### 运行服务端    

```shell   
cd build/Server     
./ChatRoomOnlineServer   
```    

### 运行客户端  

```shell   
cd build/Client       
./ChatRoomOnlineClient    
```    

## 部署方法   

在Configuration.hpp    
#define SERVER_IP "" *改成你自己的服务器IP*     
constexpr ushort SERVER_PORT = 6666;   *改成自己的服务器端口*      
客户端使用方法：   
当服务端开启后，进入界面，然后输入help看帮助     

## 潜在问题     

客户端如果运行在bash或者某些系统自带的shell上显示可能有点问题    
解决方法是下一个现代的shell-like zsh      
或者去display.cpp里改代码，把某些代码中的fmt::print中的颜色参数改了      

***That's all.***  

 
