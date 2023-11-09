# TI - The IM

## 项目配置
安装这些软件：
- Git
- CMake
- GCC / Clang / MSYS2
- Visual Studio Code

从Gitee克隆这个项目。
```shell
git clone https://gitee.com/douexpectaname/ti.git
```

用VSC打开这个文件夹。
```shell
code ./ti
```

配置VSC的C/C++环境。
1. 安装[C/C++插件](vscode:extension/ms-vscode.cpptools)
2. 选择工具链。参考：[Get started with the CMake Tools Visual Studio Code extension on Linux](https://code.visualstudio.com/docs/cpp/cmake-linux#_select-a-kit)

## 编译
用Shift-Ctrl-P（⇧⌘P在Mac）打开命令窗口。用CMake: Build指令编译项目。或者，点击下方Build按钮。

![build-button](https://code.visualstudio.com/assets/docs/cpp/cpp/cmake-build.png)