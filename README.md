# Suzerain Save Studio

<a name="languages"></a>

**简体中文** · [繁體中文](docs/intro/zh-TW.md) · [English](docs/intro/en.md) · [日本語](docs/intro/ja.md) · [한국어](docs/intro/ko.md) · [Deutsch](docs/intro/de.md) · [Français](docs/intro/fr.md) · [Español](docs/intro/es.md) · [Português (Brasil)](docs/intro/pt-BR.md) · [Русский](docs/intro/ru.md) · [Türkçe](docs/intro/tr.md)

我以前玩《Suzerain》主要靠改存档，每次都要读两遍档，觉得有点麻烦，就给自己做了这个修改器。现在既可以改存档，也可以直接在游戏里调资源。开启选项预览后，把鼠标放在选项上，轻按一下 **Ctrl** 就能看到已知的直接效果。还有一些顺手做的小功能，欢迎大家[提提建议](https://github.com/lingjieheti/suzerain-save-studio/issues)～

支持 Windows 10 / 11 x64。解压后双击即可编辑存档，也可以先打开内置示例试试。

在选档窗口底部或编辑器左侧点 **赞助作者**，可离线查看微信赞赏码、PayPal 和 USDT 收款码。USDT 使用 **TRON（TRC20）** 网络。

**[下载 v0.21.1 Windows 完整包](https://github.com/lingjieheti/suzerain-save-studio/releases/download/v0.21.1/SuzerainSaveStudio-0.21.1-Live-Preview-win-x64.zip)** · [发布说明与 SHA-256](https://github.com/lingjieheti/suzerain-save-studio/releases/tag/v0.21.1) · [♡ 赞助作者](SPONSOR.md)

![液态玻璃外观的 Windows 编辑器，里奇亚内置示例显示预算、权威和能源](docs/images/rizia-workspace.png)

[按钮反光动图](docs/images/glass-hover.gif) · [四套配色与宽窄窗口](docs/images/appearances.png) · [存档选择器](docs/images/save-picker.png) · [游戏内面板的模拟预览](docs/images/live-glass-preview.png)

## 快速上手

完整包内含桌面编辑器和可选的 **Ctrl+F8 游戏内实时面板**。解压后运行 EXE 即可编辑存档。若要使用实时面板，在启动窗口选择“实时模式”。安装要求、可编辑的五项资源与验证范围见[实时面板使用说明](live/README.zh-CN.md)。

1. 在游戏中保存并退出。打开选档器就可以输入“里奇亚 自动 第3回合”。即使存档还在扫描，结果出现时也会按已输入的条件筛选。同一回合有多个存档时，可再输入“修改时间”列显示的日期或时分，例如 `18:45`。选中后核对文件名，也可以点“手动打开”。首次使用可以先选“索德兰示例”或“里奇亚示例”。
2. 选择左侧分类，按中文名称或原始字段名查找项目。也可以在“高级字段”粘贴攻略中的整段代码，例如 `["BaseGame.GovernmentBudget"]=20`，直接定位政府预算。选中后输入自己想要的数值，再点“应用修改”。
3. 打开“修改预览”，核对原值与新值。可以单独撤回某项，也可以撤销或重做。选择“另存为”输出副本，或保存到原档。回到游戏主菜单，手动读取刚修改的存档。

## 常用数值和收藏

- **收藏字段**。选中常用或高级字段，在右侧点“收藏字段”。换同一战役的存档后，到“高级字段”点“只看收藏”，可继续编辑这些数值。索德兰和里奇亚各用一份收藏。
- **查里奇亚资源**。在高级字段搜索“能源”时，当前库存会排在收入修正项前面。要看每回合的变化，可以搜索“每回合能源”。净变化是存档记下的汇总值，游戏结算时可能重新计算。
- **选择前查看结果**。按 **Ctrl+F8**，开启「按 Ctrl 查看选项结果」，再关掉面板。将鼠标指向选项，轻按一下左或右 **Ctrl**，就能看到「私人资产 +2」这样的变化。再按一次或移开鼠标，小窗就会收起。换一个选项后可再次按 Ctrl 查看。此功能默认关闭，剧情结果尚不明确时会显示「暂无可确认的直接效果」。[查看游戏内提示实拍](docs/images/choice-preview.jpg)
- **在游戏里微调**。按 **Ctrl+F8**，选中资源后可点 **−5、−1、+1、+5**，看清目标值再预览和确认。如果操作期间游戏已更新该资源，请按面板提示重新核对。

[查看收藏界面](docs/images/field-favorites.png) · [粘贴攻略找字段](docs/images/guide-search.png) · [修改前后对照](docs/images/change-review.png) · [玩家反馈与五款同类工具对照](docs/PLAYER-RESEARCH.zh-CN.md)

## 选择外观

在简体或繁体中文界面，打开 **外观** 即可选择中文字体。选择 **自动** 时，编辑器先查找电脑上的苹方简体或苹方繁体，再选用对应的 Windows 中文界面字体。也可手动选择本机已有的 Noto Sans。选档器和编辑器会记住同一项选择。数字列保持等宽，方便逐位核对原值和新值。[查看 Windows 字体实机对比](docs/images/chinese-fonts.png)

想先看看苹方的效果，可以在 iPhone 或 Mac 的浏览器打开里奇亚样张。页面会检测浏览器能否使用设备上的苹方，再显示切换按钮。[下载离线字体样张](https://github.com/lingjieheti/suzerain-save-studio/releases/download/v0.16.2/PingFang-preview.html)

打开里奇亚存档后，可以在同一行比较预算、权威和能源。大字是现有库存，旁边的小字显示每回合变化与上限。游戏内的 Ctrl+F8 面板使用游戏自带字体显示中文和英文。

默认配色是 **雾蓝**。侧栏和文件工具栏会映出背景的颜色。沿按钮边缘移动鼠标，反光会跟随指针，按下时向内收。数值和修改预览使用实色底，长时间对照也清楚。选档器和编辑器的 **外观** 菜单提供 **雾蓝、暖纸、林间、午夜** 四套配色，也可以单独关闭 **液态玻璃**。关闭 Windows 动画效果后，桌面端保留静态玻璃。开启高对比度或关闭系统透明效果时，桌面端使用实色。

## 功能

| 使用场景 | 可以做什么 |
| --- | --- |
| 索德兰本体 | 51 项常用字段，涵盖经济、政治、人物、外交与军事。预算与私人资产均有 +5 快捷操作。 |
| 里奇亚 DLC | 63 项常用字段，涵盖资源、军备、家族与外交。可将库存设为 99 或 999，预览中可核对联动上限。 |
| 找字段 | 常用页与高级页支持多词检索、结果计数和待保存筛选。可用中文释义或原始键搜索。中文词库覆盖已检查的 4,097 个高级字段。 |
| 修改与复用 | 在预览中逐项撤回。字段 CSV 的 `Original` 列保留存档原值，`Current` 列反映修改预览，包括自动调高的里齐亚资源上限。也可导入或导出绑定当前战役的修改方案。 |
| 保存与恢复 | 覆盖与恢复前都会备份当前文件。备份页按完整存档名区分相似档位。可以打开备份查看，并另存到备份目录之外。 |
| 界面与语言 | 窄窗口切换为图标导航，支持高 DPI 和 11 种界面语言。部分较长说明及少见报错使用英文。 |

`Ctrl+L` 重开选档器时会选中正在编辑的存档，列表仍按最近修改时间排序。`Ctrl+O` 手动打开，`Ctrl+F` 搜索，`Ctrl+S` 打开修改预览，按 ↑ / ↓ 逐项核对原值和新值。编辑历史支持撤销与重做。备份保存在原档同目录的 `.rizia-backups` 文件夹。

## 兼容性与验证

已对 15 个索德兰和 6 个里奇亚存档进行只读验证，并在 Windows 桌面使用合成数据测试保存与恢复。已检查的真实存档版本为 `3.1.0.1.175` 和 `3.1.0.1.137`。桌面编辑器改动数值后重新进游戏读档的效果仍待验证。内置示例用于练习编辑流程。[字段依据](docs/FIELD-EVIDENCE.md) · [语言覆盖](docs/LANGUAGES.md)

在真实索德兰战役副本中，0.13.1 完成了政府预算 **16 → 17 → 16** 和个人财富 **1 → 2 → 1** 两组测试。四次操作后，游戏原生顶栏和 Ctrl+F8 面板显示的数值均一致。里奇亚的预算、权威和能源已通过模拟验证，仍待真实战役实测。[验证记录](VALIDATION.md)

## English quick start

Start typing `Rizia auto turn 3` as soon as the save picker opens. Results use your latest search when the scan finishes. Add a local date or time from the **Modified** column, such as `18:45`, to distinguish saves from the same turn. Paste a guide entry such as `["BaseGame.GovernmentBudget"]=20` into **Advanced** to find that exact field. Enter your preferred amount in the inspector to make the edit.

Bookmark any available field in its inspector and find it again under **Advanced** with the favorites filter. Sordland and Rizia keep separate lists. In the live panel, use **−5, −1, +1, +5** to prepare a change, then preview and confirm it.

To see what a choice changes, enable **Preview choices with Ctrl** in the **Ctrl+F8** panel, close it, then point to the choice and tap either Ctrl key. Tap again or move away to dismiss the preview.

The desktop editor uses equal-width digits in numeric columns and inputs. The Ctrl+F8 panel uses the game's own font for English and Chinese, with bold headings and larger resource values. [View the new panel with simulated campaign data](docs/images/live-glass-preview.png)

New installations open in **Fjord**. Move the pointer along a glass button to see the highlight follow its edge, then press to see it shift inward. Rizia’s three resource balances share one solid strip, with income and caps alongside each balance. Under **Appearance**, choose **Paper**, **Forest**, **Fjord** or **Midnight**, or turn off **Liquid Glass**. Turning off Windows animations keeps the desktop glass still. High contrast or disabled transparency gives it a solid finish.

Download and extract the Windows x64 ZIP above. Run the executable, open a discovered save or try either built-in sample. Select a field, enter a value and apply it. In **Review changes**, inspect each original and proposed value before saving. To use the optional in-game resource panel, choose **Live mode** on startup. It requires an existing BepInEx 6 IL2CPP installation. After installation, open a campaign and press **Ctrl+F8**. In a live Sordland campaign, v0.13.1 changed the government budget from 16 to 17 and personal wealth from 1 to 2, and restored both original values. The game's top bar and the panel agreed after all four writes. Rizia’s three resources have been tested in simulation only. [HUD verification and test coverage](VALIDATION.md)

## 从源码构建

在 Windows 上使用 x64 MinGW-w64 GCC、windres 和 PowerShell。发布程序使用 GCC 13.2.0 构建。以下命令采用 Strawberry Perl 附带工具链的默认位置。

```powershell
.\build.ps1 -Toolchain C:\Strawberry\c\bin -CompilerKind GCC
.\test.ps1 -Compiler C:\Strawberry\c\bin\g++.exe
```

程序位于 `build/SuzerainSaveStudio.exe`。测试默认使用合成数据。向 `test.ps1` 传入 `-SaveDirectory <目录>` 可额外运行只读存档检查。按[实时面板说明](live/README.zh-CN.md)编译插件后，运行 `python tools/package_release.py build/dist` 即可生成带中英入门说明的完整包与 SHA-256 校验文件。

本项目是独立的非官方玩家工具，与 Torpor Games 无隶属关系。应用源码使用 [MIT 许可](LICENSE)，编译运行库的许可见 [第三方说明](src/THIRD-PARTY-NOTICES.txt)。
