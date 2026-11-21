# Suzerain Save Studio 游戏内实时面板

适用于 Windows x64 Steam 版《Suzerain》和 BepInEx 6 IL2CPP。此技术预览使用游戏内的 Dialogue System 变量接口。安装后可直接在当前回合读写数值。离线存档修改器仍可单独使用。

| 战役 | 可修改的数值 |
| --- | --- |
| 索德兰 | 政府预算、私人资产 |
| 里奇亚 | 预算、权威、能源 |

![游戏内面板布局预览，使用模拟数据离线渲染，示例为王国预算从 18 改为 30](https://raw.githubusercontent.com/lingjieheti/suzerain-save-studio/v0.13.0/docs/images/live-panel-preview.png)

上图为离线布局预览。面板内可用 Tab 和 Shift+Tab 移动焦点、Enter 操作选中的按钮、Esc 取消。

点面板的外观选项即可切换 **暖纸、林间、雾蓝、午夜**。桌面编辑器与游戏内面板共用主题偏好，重新打开面板时会读取最近的选择。正在输入或等待确认的数值会保留。

## 安装和使用

1. 退出游戏，解压实时版 ZIP，使 EXE 和 DLL 位于同一目录。
2. 打开 `SuzerainSaveStudio.exe`，在启动窗口点 **实时模式**。选择游戏目录里的 `Suzerain.exe`，程序会把插件复制到已有的 BepInEx。若提示已有其他版本，请先将旧版 `SuzerainSaveStudio.Live.dll` 移出 `BepInEx/plugins`，再重新安装。
3. 启动游戏并读取战役，在游戏里按 **Ctrl+F8** 显示面板。选择字段，输入整数，点 **预览修改**，核对后点 **确认写入游戏**。面板会回读当前战役的数值。
4. 里奇亚库存遵循当前战役的资源上限。游戏内编辑作用于运行中的回合。需要保留这些修改时，通过游戏正常保存进度。

也可以将 `SuzerainSaveStudio.Live.dll` 手动放入游戏的 `BepInEx/plugins/SuzerainSaveStudio/`。游戏需已用 BepInEx 6 IL2CPP 成功运行过一次。已有基于该宿主的翻译插件时，可直接使用同一个宿主。卸载时先退出游戏，然后只移除上述目录中的 `SuzerainSaveStudio.Live.dll`。

## 验证范围

此插件已使用本地 Windows 安装的 BepInEx 6.0.0-be.755 和游戏互操作程序集编译，零警告、零错误。44 项模拟战役状态检查与 25 项离线面板交互检查通过。尚未在真实战役中验证写入和游戏 HUD 的刷新时机。若显示「等待读取战役存档」，请先从游戏主菜单读取可玩战役。数值只能在对应战役的字段和当前回合就绪后写入。

## 从源码构建

使用 .NET 6 SDK，或已装 .NET 6 目标包的兼容 SDK。在仓库根目录运行以下命令，将路径替换为自己的游戏安装目录。

```powershell
.\live\build-live.ps1 -GameRoot 'D:\SteamLibrary\steamapps\common\Suzerain'
```

插件位于 `build/live/SuzerainSaveStudio.Live.dll`。构建只引用本机的游戏和宿主，不会将游戏程序集打入插件。
