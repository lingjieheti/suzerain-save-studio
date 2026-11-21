using System;
using System.Globalization;
using Il2CppInterop.Runtime.Attributes;
using UnityEngine;

namespace SuzerainSaveStudio.Live
{
    public sealed class LivePanel : MonoBehaviour
    {
        private const float MaxWidth = 526, MinWidth = 316, Height = 354, Inset = 17;
        private static readonly Color Paper = new Color32(249,247,241,255);
        private static readonly Color Card = new Color32(255,253,248,255);
        private static readonly Color Line = new Color32(214,208,196,255);
        private static readonly Color Ink = new Color32(49,43,37,255);
        private static readonly Color Muted = new Color32(102,94,83,255);
        private static readonly Color Clay = new Color32(154,68,42,255);
        private static readonly Color ClayHover = new Color32(134,55,34,255);
        private static readonly Color Blush = new Color32(248,231,218,255);
        private static readonly Color Hover = new Color32(240,235,224,255);
        private static readonly Color Pine = new Color32(55,95,68,255);
        private static readonly Color MossTint = new Color32(232,240,229,255);
        private static readonly Color Warning = new Color32(134,62,41,255);
        private enum Feedback { Info, Success, Warning }

        private bool visible, hotkeyHeld, chinese;
        private Snapshot snapshot;
        private string key = "", draft = "", noticeZh = "", noticeEn = "";
        private bool confirming, draftEdited;
        private Snapshot previewBasis;
        private float nextPollAt;
        private int proposed, original;
        private float width;
        private Feedback feedback;
        private string focused = "", activate = "";
        private GUIStyle title, number, fieldName, body, caption, rightCaption, action, onClay, input;
        private Font panelFont, displayFont;

        public LivePanel(IntPtr ptr) : base(ptr) { }

        public void Awake()
        {
            var lang = Application.systemLanguage;
            chinese = lang == SystemLanguage.Chinese || lang == SystemLanguage.ChineseSimplified ||
                      lang == SystemLanguage.ChineseTraditional;
        }

        public void OnApplicationFocus(bool focused)
        {
            if (!focused) hotkeyHeld = false;
        }

        public void OnDestroy()
        {
            if (panelFont != null) UnityEngine.Object.Destroy(panelFont);
            if (displayFont != null) UnityEngine.Object.Destroy(displayFont);
        }

        public void OnGUI()
        {
            var ev = Event.current;
            if (ev == null) return;
            if (ev.rawType == EventType.KeyUp && ev.keyCode == KeyCode.F8) hotkeyHeld = false;
            if (ev.type == EventType.KeyDown && ev.keyCode == KeyCode.F8 && ev.control && !ev.alt && !ev.shift)
            {
                if (!hotkeyHeld) {
                    hotkeyHeld = true;
                    visible = !visible;
                    if (visible) Refresh();
                    else confirming = false;
                }
                ev.Use();
            }
            if (!visible) return;
            if (ev.type == EventType.KeyDown && ev.keyCode == KeyCode.Escape)
            {
                if (confirming) { confirming = false; focused = "value"; GUI.FocusControl("value"); }
                else visible = false;
                ev.Use();
                if (!visible) return;
            }
            if (ev.type == EventType.Layout && Time.realtimeSinceStartup >= nextPollAt) Poll();

            if (Screen.width <= 0 || Screen.height <= 0) return;
            float scale = Math.Min(Math.Max(1f, Math.Min(1.8f, Screen.height / 900f)),
                Math.Min(Screen.width / (MinWidth+16), Screen.height / (Height+16)));
            width = Math.Min(MaxWidth, Screen.width/scale-16);
            var area = new Rect(Math.Max(8f,Screen.width/scale-width-16),8,width,Height);
            var previousMatrix = GUI.matrix;
            int previousDepth = GUI.depth;
            var previousColor = GUI.color;
            var previousContent = GUI.contentColor;
            var previousBackground = GUI.backgroundColor;
            bool previousEnabled = GUI.enabled;
            try
            {
                GUI.matrix = Matrix4x4.Scale(new Vector3(scale,scale,1f));
                GUI.depth = -850;
                GUI.color = Color.white;
                GUI.contentColor = Color.white;
                GUI.backgroundColor = Color.white;
                GUI.enabled = true;
                EnsureStyles();
                GUI.BeginGroup(area);
                try { Keyboard(ev); DrawPanel(); }
                finally { GUI.EndGroup(); }
                if ((ev.isMouse || ev.type == EventType.ScrollWheel) && area.Contains(ev.mousePosition)) ev.Use();
            }
            finally
            {
                activate = "";
                GUI.matrix = previousMatrix;
                GUI.depth = previousDepth;
                GUI.color = previousColor;
                GUI.contentColor = previousContent;
                GUI.backgroundColor = previousBackground;
                GUI.enabled = previousEnabled;
            }
        }

        [HideFromIl2Cpp]
        private string T(string zh, string en) { return chinese ? zh : en; }

        [HideFromIl2Cpp]
        private string FieldName(string id)
        {
            switch (id)
            {
                case "BaseGame.GovernmentBudget": return T("政府预算","Government budget");
                case "BaseGame.PersonalWealth": return T("个人财富","Personal wealth");
                case "RiziaDLC.Resources_Budget": return T("王国预算","Royal budget");
                case "RiziaDLC.Resources_Authority": return T("权威","Authority");
                case "RiziaDLC.Resources_Energy": return T("能源","Energy");
                default: return id;
            }
        }

        [HideFromIl2Cpp]
        private void Select(LiveField field)
        {
            key = field == null ? "" : field.Key;
            draft = field == null ? "" : field.Value.ToString(CultureInfo.InvariantCulture);
            draftEdited = false;
            confirming = false;
            previewBasis = null;
            focused = "";
            GUI.FocusControl("");
        }

        [HideFromIl2Cpp]
        private void Refresh()
        {
            nextPollAt = Time.realtimeSinceStartup + 1f;
            snapshot = null;
            confirming = false;
            try
            {
                snapshot = LiveGame.Read();
                var field = snapshot?.Find(key);
                if (field == null && snapshot != null && snapshot.Fields.Count > 0) field = snapshot.Fields[0];
                Select(field);
                if (snapshot?.CanEdit == true)
                    Notice("已从当前游戏读取。","Read from the current game.",snapshot.Status);
                else Notice("","");
            }
            catch (Exception error)
            {
                Select(null);
                Notice("读取失败。","Read failed.",error.Message,Feedback.Warning);
            }
        }

        [HideFromIl2Cpp]
        private void Poll()
        {
            nextPollAt = Time.realtimeSinceStartup + 1f;
            var previous = snapshot;
            var latest = LiveGame.Read();
            var field = latest?.Find(key);
            snapshot = latest;
            if (previous != null && previous.SameSession(latest) && field != null)
            {
                // A running game can spend resources while this panel is open.
                // Keep a user's input and the exact snapshot on which they previewed.
                if (!draftEdited && !confirming)
                    draft = field.Value.ToString(CultureInfo.InvariantCulture);
                return;
            }
            if (previous?.CanEdit != true && latest?.CanEdit != true) return;
            if (field == null && latest?.Fields.Count > 0) field = latest.Fields[0];
            Select(field);
            Notice("游戏状态已更新。","Game state updated.");
        }

        [HideFromIl2Cpp]
        private void Notice(string zh, string en, string details = "", Feedback tone = Feedback.Info)
        {
            feedback = tone;
            if (!String.IsNullOrEmpty(details))
            {
                int split = details.IndexOf(" / ",StringComparison.Ordinal);
                zh += " " + (split < 0 ? details : details.Substring(0,split));
                en += " " + (split < 0 ? details : details.Substring(split+3));
            }
            noticeZh = zh; noticeEn = en;
        }

        [HideFromIl2Cpp]
        private void Preview(LiveField field)
        {
            if (field == null || snapshot == null || !snapshot.CanEdit) return;
            if (!int.TryParse(draft,NumberStyles.Integer,CultureInfo.InvariantCulture,out proposed)) {
                Notice("请输入有效的整数。","Enter a valid whole number.","",Feedback.Warning); return;
            }
            if (proposed == field.Value) { Notice("新值与当前值相同。","The value is unchanged."); return; }
            if ((key != "BaseGame.GovernmentBudget" && proposed < 0) ||
                (field.Cap.HasValue && proposed > field.Cap.Value)) {
                Notice("数值超出该资源的可用范围。","The number is outside this resource’s range.","",Feedback.Warning); return;
            }
            original = field.Value;
            previewBasis = snapshot;
            confirming = true;
            focused = "cancel";
            Notice("","");
            GUI.FocusControl("");
        }

        [HideFromIl2Cpp]
        private void Confirm()
        {
            if (!confirming) return;
            confirming = false;
            var expected = previewBasis;
            var writeKey = key;
            bool accepted = false;
            string reason = "";
            try { accepted = LiveGame.TrySet(expected,writeKey,proposed,out reason); }
            catch (Exception error) { reason = error.Message; }
            Refresh();
            var actual = snapshot?.Find(writeKey);
            if (!accepted) {
                Notice("未确认写入。","Write unconfirmed.",reason,Feedback.Warning);
            } else if (expected != null && expected.SameSession(snapshot) &&
                       actual != null && actual.Value == proposed) {
                Notice("回读确认已写入 "+proposed+"。","Verified in game: "+proposed+".","",Feedback.Success);
            } else {
                string read = actual == null ? "—" : actual.Value.ToString(CultureInfo.InvariantCulture);
                Notice("写入后未能验证。回读值 "+read+"。","Could not verify the write. Read-back value: "+read+".","",Feedback.Warning);
            }
        }

        [HideFromIl2Cpp]
        private void Keyboard(Event ev)
        {
            if (ev.type != EventType.KeyDown || ev.alt || ev.control) return;
            if (ev.keyCode == KeyCode.Tab)
            {
                var order = new System.Collections.Generic.List<string> { "language", "close", "refresh" };
                if (confirming) { order.Add("cancel"); order.Add("confirm"); }
                else if (snapshot?.CanEdit == true)
                {
                    foreach (var field in snapshot.Fields) order.Add(field.Key);
                    order.Add("value"); order.Add("preview");
                }
                int index = order.IndexOf(focused);
                if (index < 0) index = ev.shift ? 0 : -1;
                focused = order[(index+(ev.shift?-1:1)+order.Count)%order.Count];
                GUI.FocusControl(focused == "value" ? "value" : "");
                ev.Use();
                return;
            }
            bool enter = ev.keyCode == KeyCode.Return || ev.keyCode == KeyCode.KeypadEnter;
            if ((enter || (ev.keyCode == KeyCode.Space && focused != "value")) && focused != "")
            {
                activate = focused == "value" ? "preview" : focused;
                ev.Use();
            }
        }

        [HideFromIl2Cpp]
        private void EnsureStyles()
        {
            if (title != null) return;
            panelFont = Font.CreateDynamicFontFromOSFont(new[] { "Segoe UI", "Microsoft YaHei" },16);
            displayFont = Font.CreateDynamicFontFromOSFont(new[] { "Georgia", "SimSun", "Microsoft YaHei" },25);
            title = Style(25,Ink,displayFont);
            number = Style(24,Ink,displayFont);
            fieldName = Style(13,Muted,panelFont);
            body = Style(16,Ink,panelFont);
            caption = Style(13,Muted,panelFont);
            caption.wordWrap = true;
            rightCaption = Style(13,Muted,panelFont,TextAnchor.MiddleRight);
            action = Style(14,Ink,panelFont,TextAnchor.MiddleCenter);
            onClay = Style(14,Color.white,panelFont,TextAnchor.MiddleCenter);
            input = Style(20,Ink,panelFont);
            input.padding = new RectOffset(9,8,0,0);
            input.contentOffset = Vector2.zero;
        }

        [HideFromIl2Cpp]
        private static GUIStyle Style(int size,Color color,Font font,TextAnchor align = TextAnchor.MiddleLeft)
        {
            var style = new GUIStyle();
            style.fontSize = size;
            style.font = font;
            style.alignment = align;
            style.clipping = TextClipping.Clip;
            style.normal.textColor = color;
            style.hover.textColor = color;
            style.active.textColor = color;
            style.focused.textColor = color;
            return style;
        }

        [HideFromIl2Cpp]
        private static void Fill(Rect r,Color fill,float radius = 0)
        {
            GUI.DrawTexture(r,Texture2D.whiteTexture,ScaleMode.StretchToFill,true,0,fill,0,radius);
        }

        [HideFromIl2Cpp]
        private static void PaperBox(Rect r,Color fill,Color edge,float radius,int border = 1)
        {
            Fill(r,edge,radius);
            Fill(new Rect(r.x+border,r.y+border,r.width-2*border,r.height-2*border),fill,
                Math.Max(0,radius-border));
        }

        [HideFromIl2Cpp]
        private bool Press(Rect rect,string id,string text,GUIStyle style)
        {
            if (Event.current.type == EventType.MouseDown && rect.Contains(Event.current.mousePosition))
            {
                focused = id;
                GUI.FocusControl("");
            }
            bool hit = GUI.Button(rect,text,style);
            return hit || activate == id;
        }

        [HideFromIl2Cpp]
        private bool Button(Rect r,string id,string text,bool primary = false)
        {
            bool hovered = r.Contains(Event.current.mousePosition);
            bool focus = focused == id;
            Color face = primary ? (hovered ? ClayHover : Clay) : (hovered ? Hover : Card);
            PaperBox(r,face,focus ? Ink : (primary ? face : Line),7,focus ? 2 : 1);
            return Press(r,id,text,primary ? onClay : action);
        }

        [HideFromIl2Cpp]
        private string LocalStatus(string status)
        {
            if (String.IsNullOrEmpty(status)) return "";
            int split = status.IndexOf(" / ", StringComparison.Ordinal);
            return split < 0 ? status : chinese ? status.Substring(0,split) : status.Substring(split+3);
        }

        [HideFromIl2Cpp]
        private void DrawPanel()
        {
            float content = width-2*Inset;
            bool ready = snapshot?.CanEdit == true;
            bool known = snapshot?.Story=="StoryPack_Main" || snapshot?.Story=="StoryPack_Rizia";
            PaperBox(new Rect(0,0,width,Height),Paper,Line,13);
            GUI.Label(new Rect(Inset,10,content-129,37),T("实时编辑","Live editor"),title);
            if (Button(new Rect(width-Inset-133,15,70,29),"language",T("English","简体中文")))
                chinese = !chinese;
            if (Button(new Rect(width-Inset-57,15,57,29),"close",T("关闭","Close")))
            {
                visible = false; confirming = false; focused = ""; return;
            }
            string campaign = known
                ? (snapshot.Story=="StoryPack_Main"?T("索德兰","Sordland"):T("里奇亚","Rizia"))+
                    "  ·  "+T("回合 ","Turn ")+snapshot.Turn+"  ·  "+T("步骤 ","Step ")+snapshot.Step
                : T("等待战役","Waiting for a campaign");
            Fill(new Rect(Inset,59,7,7),ready ? Pine : Muted,3.5f);
            GUI.Label(new Rect(Inset+14,49,content-97,26),campaign,caption);
            if (Button(new Rect(width-Inset-74,49,74,26),"refresh",T("刷新","Refresh"))) Refresh();
            Fill(new Rect(Inset,83,content,1),Line);

            bool hasFields = snapshot != null && snapshot.Fields.Count > 0;
            if (hasFields) DrawFields(content,ready);
            if (!ready || !hasFields)
            {
                float top = hasFields ? 171 : 96;
                PaperBox(new Rect(Inset,top,content,280-top),Card,Line,9);
                GUI.Label(new Rect(Inset+14,top+9,content-28,30),
                    hasFields ? T("只读","Read only") : T("当前没有可编辑的战役","No editable campaign"),body);
                string detail = LocalStatus(snapshot?.Status);
                GUI.Label(new Rect(Inset+14,top+41,content-28,48),
                    detail != "" ? detail : T("请进入索德兰或里奇亚战役。","Open a Sordland or Rizia campaign."),caption);
            }
            else if (snapshot.Find(key) is LiveField selected) DrawEditor(selected,content);
            DrawFooter(content);
        }

        [HideFromIl2Cpp]
        private void DrawFields(float content,bool ready)
        {
            int count = snapshot.Fields.Count;
            float cardWidth = (content-7*(count-1))/count;
            for (int i=0;i<count;i++)
            {
                var field = snapshot.Fields[i];
                Rect r = new Rect(Inset+i*(cardWidth+7),95,cardWidth,64);
                bool selected = field.Key == key;
                bool enabled = !confirming && ready;
                bool focus = enabled && focused == field.Key;
                Color fill = selected ? Blush : enabled && r.Contains(Event.current.mousePosition) ? Hover : Card;
                PaperBox(r,fill,focus ? Ink : (selected ? Clay : Line),8,focus ? 2 : 1);
                if (enabled && Press(r,field.Key,"",action) && !selected)
                {
                    Select(field);
                    focused = field.Key;
                    Notice("","");
                }
                GUI.Label(new Rect(r.x+10,r.y+4,r.width-20,22),FieldName(field.Key),fieldName);
                var numRect = new Rect(r.x+10,r.y+26,r.width-20,33);
                string value = field.Value.ToString(CultureInfo.InvariantCulture);
                int normalSize = number.fontSize;
                if (value.Length > 5)
                {
                    float measured = number.CalcSize(new GUIContent(value)).x;
                    if (measured > numRect.width)
                        number.fontSize = Math.Max(11,(int)(normalSize*numRect.width/measured));
                }
                GUI.Label(numRect,value,number);
                number.fontSize=normalSize;
            }
        }

        [HideFromIl2Cpp]
        private void DrawEditor(LiveField selected,float content)
        {
            PaperBox(new Rect(Inset,170,content,110),Card,confirming ? Clay : Line,9);
            float x = Inset+13, inside = content-26;
            if (confirming)
            {
                GUI.Label(new Rect(x,178,inside,24),T("确认修改","Confirm change")+"  ·  "+FieldName(key),body);
                GUI.Label(new Rect(x,204,inside,26),
                    original.ToString(CultureInfo.InvariantCulture)+"   →   "+proposed.ToString(CultureInfo.InvariantCulture),body);
                float confirmW = Math.Min(166,inside*.60f);
                if (Button(new Rect(x+inside-confirmW-84,237,77,32),"cancel",T("取消","Cancel")))
                {
                    confirming=false; focused="preview";
                }
                if (Button(new Rect(x+inside-confirmW,237,confirmW,32),"confirm",
                    T("确认写入游戏","Confirm write to game"),true)) Confirm();
                return;
            }

            GUI.Label(new Rect(x,182,inside*.55f,23),T("新值","New value"),body);
            if (selected.Cap.HasValue)
                GUI.Label(new Rect(x+inside*.5f,182,inside*.5f,23),
                    T("资源上限 ","Resource cap ")+selected.Cap.Value,rightCaption);
            float buttonW = Math.Min(145,inside*.45f);
            Rect edit = new Rect(x,215,inside-buttonW-8,42);
            if (Event.current.type==EventType.MouseDown && edit.Contains(Event.current.mousePosition)) focused="value";
            bool editing = focused == "value";
            PaperBox(edit,Paper,editing ? Clay : Line,6,editing ? 2 : 1);
            GUI.SetNextControlName("value");
            var settings = GUI.skin.settings;
            var oldCursor = settings.cursorColor;
            var oldSelection = settings.selectionColor;
            string entered;
            try
            {
                settings.cursorColor=Ink;
                settings.selectionColor=new Color32(224,178,145,204);
                entered=GUI.TextField(edit,draft,12,input);
            }
            finally { settings.cursorColor=oldCursor; settings.selectionColor=oldSelection; }
            if (entered != draft)
            {
                draft=entered;
                draftEdited=draft!=selected.Value.ToString(CultureInfo.InvariantCulture);
                Notice("","");
            }
            if (Button(new Rect(x+inside-buttonW,215,buttonW,42),"preview",T("预览修改","Preview change"),true))
                Preview(selected);
        }

        [HideFromIl2Cpp]
        private void DrawFooter(float content)
        {
            var details = T(noticeZh,noticeEn);
            if (details != "")
            {
                bool signal = feedback != Feedback.Info;
                if (signal)
                    Fill(new Rect(Inset,286,content,39),feedback==Feedback.Success?MossTint:Blush,6);
                Color old = caption.normal.textColor;
                caption.normal.textColor = feedback==Feedback.Success ? Pine : feedback==Feedback.Warning ? Warning : Muted;
                GUI.Label(new Rect(Inset+(signal?9:0),286,content-(signal?18:0),39),details,caption);
                caption.normal.textColor = old;
            }
            Fill(new Rect(Inset,329,content,1),Line);
            GUI.Label(new Rect(Inset,330,content,21),T("Ctrl+F8  显示或隐藏面板","Ctrl+F8  Show or hide panel"),rightCaption);
        }
    }
}
