# miniblink-simple-demo

使用miniblink中各种常见需求的单文件demo（Win32平台），列举如下：

* 创建窗口

* JS/CPP互调

* 无边框窗口拖动

* 根据exe文件图标设置窗口图标

* 最大化/最小化

* 获取窗口句柄

* 只允许单实例（互斥量实现）

* 显示 devtools


## 依赖

miniblink 作者 https://github.com/weolar 发布的二进制包（或自行编译）

编译此程序需要其中的 wkedefine.h，执行依赖 node.dll

注意32/64位问题

## 编译

```
g++ -m32 mb_simple_demo.cpp -lcomctl32
```

如果不需要使用无边框窗体，将子类化那几行代码从文件中移除后即可正常编译。


```
cl mb_simple_demo.cpp
```

MSVC 编译，唯一需要的注意的是，文件编码应设置为 UTF8-BOM 或本地编码（GBK），CL不接受无BOM头的utf-8
