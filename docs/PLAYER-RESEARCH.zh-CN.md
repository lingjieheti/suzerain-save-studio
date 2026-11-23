# 玩家反馈与同类工具对照

首次调研于 2026 年 11 月 21 日，11 月 23 日补充字段前缀搜索案例。以下记录可以回看原帖、产品说明和公开实现。样本来自公开讨论，能说明玩家遇到了哪些问题，无法估计问题在全体玩家中的发生率。

## 玩家的操作遇到了什么问题

| 玩家要做的事 | 玩家实际遇到的问题 | 本次改动 |
| --- | --- | --- |
| 每次开档都能找到同一组变量 | 有玩家点名预算、预算上限和权威，希望自行收藏任意字段。作者随后回复已加入置顶。[玩家的具体需求](https://www.reddit.com/r/suzerain/comments/1luxuyg/comment/n23cjhw/) · [作者回复](https://www.reddit.com/r/suzerain/comments/1luxuyg/comment/n23ioml/) | 用原始键记住收藏，同一战役换档、重启后仍可使用。 |
| 找到正确的资源数值 | 有人搜索预算和财富时找到了决策成本。另有人想改里奇亚每回合能源，试着改了预算修正总量，报告没有得到想要的效果。[索德兰讨论](https://www.reddit.com/r/suzerain/comments/1d1t6z8/) · [里奇亚讨论](https://www.reddit.com/r/suzerain/comments/1fvr88m/) | 在里奇亚区分当前库存、上限和回合净变化，支持按用途查找。 |
| 在同一回合多签几项建设政令 | 提问者说明了自己为什么关心资源上限。这个目标涉及库存和上限两个数值。[玩家解释](https://www.reddit.com/r/suzerain/comments/1cgsw77/) | 将资源上限和当前库存放在一起，便于核对。 |
| 在游戏里补一点资源，知道能不能花 | 有玩家向实时修改器作者追问，修改后顶栏未刷新，能否马上支付。WeMod 的玩家也点名需要里奇亚能源与权威。[实时面板讨论](https://www.reddit.com/r/suzerain/comments/1t660o3/meet_wolknerprotocol_a_live_state_editor_for/) · [WeMod 玩家反馈](https://community.wemod.com/t/suzerain-cheats-and-trainer-for-steam/378190) | 在已有预览、确认和回读流程中加入相对加减。 |

## 用攻略中的实际输入复测

在后续的 0.16.0 测试中，我们用攻略里真正出现的输入检查了两项操作。

- **从攻略找到那个字段。** 玩家示例包含方括号、引号和等号，有时连着建议设置的数值。0.15.1 把这些符号当成普通搜索词，复制整段找不到政府预算。0.16.0 接受 Lua 写法和存档 JSON 里的转义写法，按完整键定位，示例中的数值只用于搜索上下文。普通关键词仍可查找多个相关项。[Steam 玩家攻略](https://steamcommunity.com/sharedfiles/filedetails/?id=3373743028) · [玩家给出的完整赋值示例](https://www.reddit.com/r/suzerain/comments/17t03ed/)
- **从多个存档中选对一个。** 玩家需要分辨进行中、自动和回合存档，再在游戏中加载对应文件。我们用同战役不同回合的文件检验选择器，并参考了 Apple 对即时筛选和搜索范围的说明。[Steam 存档指南](https://steamcommunity.com/sharedfiles/filedetails/?id=3341333208) · [Apple 搜索设计指南](https://developer.apple.com/design/human-interface-guidelines/searching)

### 按攻略前缀找一组字段

Steam 攻略在人物关系一节让玩家复制 `[\"RiziaDLC.Relations_`，从结果中挑选具体人物。Reddit 的玩家回复也沿用了这段写法。0.18.1 能找到完整的键，粘贴这段带左括号的前缀却没有结果。新版按键的开头筛选，在里奇亚示例中可以找到 `Relations_Hugo` 和 `Relations_Vina`。[Steam 攻略](https://steamcommunity.com/sharedfiles/filedetails/?id=3204074586) · [Reddit 玩家回复](https://www.reddit.com/r/suzerain/comments/1hbn4bx/)

2026 年 10 月，另一位 Windows 玩家询问如何找到帕莱斯的外交关系，回复建议先搜 `negotiatingpower` 再找具体国家。这个例子说明玩家也会用部分键名查找外交字段。双方没有报告修改后的游戏结果。[玩家原帖](https://www.reddit.com/r/suzerain/comments/1wxmfpx/save_editing_in_rizia/)

## 五款工具的公开做法

| 工具 | 与本次决策相关的设计 |
| --- | --- |
| yijinyang 的桌面编辑器 | 作者提供搜索和备份，玩家提出可收藏任意键，仓库可见收藏配置。[源码](https://github.com/yijinyang/Suzerain-Save-Editor) · [原始讨论](https://www.reddit.com/r/suzerain/comments/1luxuyg/suzerain_save_editor/) |
| Morgan McNabb 的桌面编辑器 | 支持按名称、ID、说明检索，具有修改标记和即时输入校验。[作者指南](https://github.com/morgan-mcnabb/Suzerain-Save-Editor/blob/main/USER_GUIDE.md) |
| ralfarishi 的网页编辑器 | 支持单字段撤销。Studio 已有预览逐项撤回，本次优先改善反复找字段的操作。[项目介绍](https://github.com/ralfarishi/suzerain-save-editor) |
| indoctrinatedrecluse 的桌面编辑器 | 作者建议从主菜单重新读档，尤其在修改 Active 存档时。本版在保存成功后直接给出这个操作提示。[作者说明](https://github.com/indoctrinatedrecluse/Suzerain-save-editor#suzerain-save-editor) |
| WeMod 的 Suzerain 工具 | 官方将“增加”和“设定”作为不同操作，列有索德兰两项和里奇亚三项资源。Studio 的快捷加减先产生草稿，方便玩家核对目标值。[官方更新记录](https://community.wemod.com/t/suzerain-cheats-and-trainer-for-steam/378190) |

## 收益与结局的验证范围

里奇亚存档包含预算、权威、能源各自的修正汇总，以及独立的预计能源收入。只读样本可以确认字段和值的类型。修改汇总能否改变后续每回合收益，还需要跟踪游戏结算。民意也有玩家想查看，但单个意见字段与选举结局之间的关系仍需独立验证。[字段记录](FIELD-EVIDENCE.md) · [玩家希望查看民意的讨论](https://www.reddit.com/r/suzerain/comments/12gl5af/)
