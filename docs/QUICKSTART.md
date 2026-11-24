# Suzerain Save Studio

## 第一次使用

1. 把 ZIP 解压到一个文件夹，双击 `SuzerainSaveStudio.exe`。选档窗口打开后可直接输入文件名、战役或回合，扫描完成时会显示匹配结果。想先试试，可以在启动窗口点“索德兰示例”或“里奇亚示例”。
2. 使用自己的存档时，先在游戏里保存并退出。可在选档窗口输入“里奇亚 自动 第3回合”，也可输入“修改时间”列的日期或时分，例如 `18:45`。选中后核对文件名。在索德兰的“经济”或“高级字段”中，搜 `GB` 找政府预算，搜 `PW` 找私人资产。也可以粘贴攻略里的字段赋值来查找，例如 `["BaseGame.GovernmentBudget"]=20`。搜索只定位字段。
3. 搜到字段后，按 **↓** 进入结果列表，用方向键选中一项，再按 **Enter**。数字会全选，可以直接输入新值并按 **Enter** 加入待保存项。对于“是／否”字段，焦点会落在当前选项。按 **Ctrl+S** 进入待修改列表，用 **↑ / ↓** 逐项对照原值和新值，核对完再选择“保存到原档”或“另存为”。回到游戏主菜单，手动读取刚才保存的文件。

想找人物关系时，打开里奇亚示例的“高级字段”，按 **Ctrl+F**，粘贴 `[\"RiziaDLC.Relations_`。结果会列出维娜、雨果等人的关系字段。

要重做刚撤销的字段修改，可以点窗口底部的“重做”。如果光标还在搜索框，也可以先按 **↓** 进入结果列表，再按 **Ctrl+Y**。

点“外观”可以切换雾蓝、暖纸、林间和午夜。开启“液态玻璃”时，指针沿按钮移动会带动边缘高光。取消勾选即可在当前配色下使用实色界面。覆盖存档时，工具会在同目录的 `.rizia-backups` 中保存原版，可在编辑器的备份页恢复。

在简体或繁体中文界面，选档器和编辑器的“外观”菜单也能切换字体。自动模式优先使用本机的相应苹方字体，常见 Windows 环境会使用微软雅黑 UI 或微软正黑体 UI。

如果想在游戏运行时微调资源，请查阅[实时面板说明](../live/README.zh-CN.md)。

想支持作者，可从选档窗口底部或编辑器侧栏打开 **赞助作者**，用微信「扫一扫」扫描赞赏码，也可以选择 PayPal 或 USDT。USDT 使用 TRON（TRC20）网络，可在窗口中复制地址。

按 **Ctrl+F8** 开启「按 Ctrl 查看选项结果」，然后关掉面板。鼠标指向选项时，轻按并松开任意一侧的 **Ctrl** 即可查看结果。再按一次或移开鼠标即可收起。移到另一个选项后，再按 Ctrl 查看。这个开关默认关闭，桌面端的「实时模式」也可以设置。

## English quick start

Open **Appearance** to choose a color theme. Uncheck **Liquid Glass** for solid navigation and controls in the same color theme.

Open **Support the author** in the save picker or editor sidebar. Scan the appreciation code with WeChat to tip the author, or choose PayPal or USDT. For USDT, use the TRON (TRC20) network.

1. Extract the ZIP and open `SuzerainSaveStudio.exe`. Pick a Sordland or Rizia sample to try the editor.
2. To edit your own campaign, save and quit the game first. Try `Rizia auto turn 3` in the picker. To find a save by its local modification time, enter a date or a time such as `18:45` from the **Modified** column. Check the file name before opening. In Sordland’s **Economy** or **Advanced** tab, search for `GB` to find government budget or `PW` to find personal wealth. You can also paste a guide entry such as `["BaseGame.GovernmentBudget"]=20` to locate the field.
3. From search, press **↓** to enter the results. Select a field with the arrow keys and press **Enter**. For a number, type over the selected value and press **Enter** to stage it. For a yes/no field, focus moves to the current choice. Press **Ctrl+S** to focus the change list. Use **Up / Down** to compare the original and proposed values, then choose **Save to original** or **Save as**. Return to the game's main menu and load the file you just saved.

To find character relationships, open the Rizia sample’s **Advanced** tab, press **Ctrl+F**, and paste `[\"RiziaDLC.Relations_`. The results include the relationship fields for Vina and Hugo.

To redo a field change, click **Redo** at the bottom of the editor. From the search box, you can also press **↓** to focus the results, then **Ctrl+Y**.

To check a choice before selecting it, enable **Preview choices with Ctrl** in the **Ctrl+F8** panel. Close the panel, point to a choice and tap either Ctrl key. Tap Ctrl again or move away to hide the tooltip. Point to another choice and tap Ctrl to see its effects. The setting is off by default and is also available in the editor’s **Live mode** dialog.

For in-game adjustments on Windows Steam, the optional plugin requires BepInEx 6 IL2CPP. Keep the DLL beside the editor, choose **Live mode** at startup, and select the game's executable. In a running campaign, press **Ctrl+F8**, choose a resource, preview your edit and confirm.
