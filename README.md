# 仓库说明

> 这是一个普普通通大三学生的计算机网络课程和网络技术与应用课程作业仓库

## Net_Lab1

本次实验在Windows系统下，利用C/C++实现了一个**基于流式Socket套字节的多人聊天程序，实现了多个客户端和服务端之间的同时聊天功能**，程序界面为命令行界面。实验中所用到的主要技术如下：
+ 重新设计了传输消息的格式，消息格式规定了不同客户端和服务端；
+ 创建线程解决服务端和不同客户端之间的消息交互，创建线程解决服务端和客户端同时收发消息避免阻塞；
+ 设计了以服务端为主体，通过服务端进行不同客户端之间的消息通信的消息交互模式；
