# Suzerain Save Studio

## 第一次使用

1. 把 ZIP 解压到一个文件夹，双击 `SuzerainSaveStudio.exe`。想先试试，可以在启动窗口点“索德兰示例”或“里奇亚示例”。
2. 使用自己的存档时，先在游戏里保存并退出。在选档窗口可输入“里奇亚 自动 第3回合”，再核对文件名。在索德兰的“经济”或“高级字段”中，搜 `GB` 找政府预算，搜 `PW` 找私人资产。也可以粘贴攻略里的字段赋值来查找，例如 `["BaseGame.GovernmentBudget"]=20`。搜索只定位字段。
3. 选中字段，输入目标数值并点“应用修改”。按 **Ctrl+S** 核对原值和新值。第一次可以用“另存为”制作副本。回到游戏主菜单，手动读取刚才保存的文件。

点“外观”可以切换雾蓝、暖纸、林间和午夜。覆盖存档时，工具会在同目录的 `.rizia-backups` 中保存原版，可在编辑器的备份页恢复。

在简体或繁体中文界面，选档器和编辑器的“外观”菜单也能切换字体。自动模式优先使用本机的相应苹方字体，常见 Windows 环境会使用微软雅黑 UI 或微软正黑体 UI。

如果想在游戏运行时微调资源，请查阅[实时面板说明](../live/README.zh-CN.md)。

想支持作者，可从选档窗口底部或编辑器侧栏打开 **赞助作者**，用微信「扫一扫」扫描赞赏码，也可以选择 PayPal 或 USDT。USDT 使用 TRON（TRC20）网络，可在窗口中复制地址。

按 **Ctrl+F8** 开启「按 Ctrl 查看选项结果」，然后关掉面板。鼠标指向选项时，轻按并松开任意一侧的 **Ctrl** 即可查看结果。再按一次或移开鼠标即可收起。移到另一个选项后，再按 Ctrl 查看。这个开关默认关闭，桌面端的「实时模式」也可以设置。

## English quick start

Open **Support the author** in the save picker or editor sidebar. Scan the appreciation code with WeChat to tip the author, or choose PayPal or USDT. For USDT, use the TRON (TRC20) network.

1. Extract the ZIP and open `SuzerainSaveStudio.exe`. Pick a Sordland or Rizia sample to try the editor.
2. To edit your own campaign, save and quit the game first. Try `Rizia auto turn 3` in the picker and check the file name. In Sordland’s **Economy** or **Advanced** tab, search for `GB` to find government budget or `PW` to find personal wealth. You can also paste a guide entry such as `["BaseGame.GovernmentBudget"]=20` to locate the field.
3. Select the field, enter a new value and choose **Apply**. Press **Ctrl+S** to review the changes. **Save as** creates a separate copy. Return to the game's main menu and load the file you just saved.

To check a choice before selecting it, enable **Preview choices with Ctrl** in the **Ctrl+F8** panel. Close the panel, point to a choice and tap either Ctrl key. Tap Ctrl again or move away to hide the tooltip. Point to another choice and tap Ctrl to see its effects. The setting is off by default and is also available in the editor’s **Live mode** dialog.

For in-game adjustments on Windows Steam, the optional plugin requires BepInEx 6 IL2CPP. Keep the DLL beside the editor, choose **Live mode** at startup, and select the game's executable. In a running campaign, press **Ctrl+F8**, choose a resource, preview your edit and confirm.
