using System;
using Il2CppInterop.Runtime;
using PixelCrushers.DialogueSystem;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.Events;
using UnityEngine.InputSystem;
using UnityEngine.InputSystem.UI;
using UnityEngine.SceneManagement;
using UnityEngine.UI;

namespace SuzerainSaveStudio.Live;

internal sealed class ChoiceHover
{
    private enum Kind { Dialogue, ReadingPrologue, DetachedDialogue, Decision, Form, Carousel, Decree, SignBill, VetoBill }

    private readonly struct Choice
    {
        public readonly Kind Kind;
        public readonly Component Owner;
        public readonly IntPtr Binding, Entry;
        public readonly Response Response;
        public readonly string Script;

        public Choice(Kind kind, Component owner, IntPtr binding, string script, Response response = null)
        {
            Kind = kind;
            Owner = owner;
            Binding = binding;
            Response = response;
            Entry = response?.destinationEntry?.Pointer ?? IntPtr.Zero;
            Script = script;
        }

        public bool Matches(Choice? other) => other is Choice current && Owner == current.Owner &&
            Kind == current.Kind && Binding == current.Binding && Entry == current.Entry && Script == current.Script;
    }

    private EventSystem legacySystem;
    private PointerEventData legacyPointer;
    private Il2CppSystem.Collections.Generic.List<RaycastResult> legacyHits;
    private KeyCode pressedKey;
    private Choice? pressedChoice, shownChoice;
    private int scene, lastPhysicalFrame = -1, lastReleaseFrame = -1, chordFrame = -1;
    private Vector2 lastMouse;
    private bool hasMouse, chineseText;
    private GUIContent heading, body;
    private GUIStyle headingStyle, bodyStyle;
    private bool warned;
    private float retryAt;

    internal void OnGUI(bool enabled, bool chinese, PanelAppearance appearance, bool glass)
    {
        if (!enabled || !Application.isFocused)
        {
            Reset();
            return;
        }
        var ev = Event.current;
        if (ev == null || Time.realtimeSinceStartup < retryAt) return;

        try
        {
            if ((pressedChoice.HasValue || shownChoice.HasValue) && SceneManager.GetActiveScene().handle != scene)
                Reset();
            if (ev.rawType == EventType.KeyDown || ev.rawType == EventType.KeyUp)
            {
                // Unity doesn't guarantee a useful mousePosition on a keyboard event. The
                // Input System raycast is current; the legacy backend uses the last GUI position.
                var pointer = hasMouse ? lastMouse : ev.mousePosition;
                bool ctrl = ev.keyCode == KeyCode.LeftControl || ev.keyCode == KeyCode.RightControl;
                if (!ctrl || ev.shift || ev.alt || ev.command || HasOtherKey(Keyboard.current, ev.keyCode))
                {
                    CancelChord();
                    return;
                }
                if (ev.rawType == EventType.KeyDown) Begin(ev.keyCode, pointer);
                else End(ev.keyCode, pointer);
                return;
            }
            if (ev.rawType == EventType.MouseDown || ev.rawType == EventType.MouseUp || ev.rawType == EventType.ScrollWheel)
            {
                if (pressedKey != KeyCode.None || ev.control) CancelChord();
                return;
            }
            if (ev.type != EventType.Repaint) return;
            lastMouse = ev.mousePosition;
            hasMouse = true;
            PollKeyboard(lastMouse);
            if (!pressedChoice.HasValue && !shownChoice.HasValue) return;

            var current = AtMouse(lastMouse);
            if (pressedChoice is Choice pressed && !pressed.Matches(current)) CancelChord();
            if (shownChoice is not Choice shown) return;
            if (!shown.Matches(current))
            {
                Hide();
                return;
            }
            if (body == null || chinese != chineseText) Read(shown, chinese);
            Draw(lastMouse, appearance, glass);
        }
        catch (Exception error)
        {
            Reset();
            retryAt = Time.realtimeSinceStartup + 1f;
            if (!warned)
            {
                warned = true;
                BepInEx.Logging.Logger.CreateLogSource("Suzerain Save Studio Hover")
                    .LogWarning("Could not read this choice preview. " + error.Message);
            }
        }
    }

    private void PollKeyboard(Vector2 pointer)
    {
        int frame = Time.frameCount;
        var keyboard = Keyboard.current;
        if (keyboard == null || lastPhysicalFrame == frame) return;
        lastPhysicalFrame = frame;
        // Some IMGUI backends omit standalone modifiers. Input System edge flags also
        // preserve a short press and release delivered between consecutive repaints.
        var left = keyboard.leftCtrlKey;
        var right = keyboard.rightCtrlKey;
        bool leftPress = left.wasPressedThisFrame, leftRelease = left.wasReleasedThisFrame;
        bool rightPress = right.wasPressedThisFrame, rightRelease = right.wasReleasedThisFrame;
        if (!leftPress && !rightPress && pressedKey == KeyCode.None) return;
        if ((left.isPressed || leftPress) && (right.isPressed || rightPress))
        {
            CancelChord();
            return;
        }
        var side = leftPress ? KeyCode.LeftControl : rightPress ? KeyCode.RightControl : pressedKey;
        if (HasOtherKey(keyboard, side))
        {
            CancelChord();
            return;
        }
        if (leftPress) Begin(KeyCode.LeftControl, pointer);
        if (rightPress) Begin(KeyCode.RightControl, pointer);
        if (leftRelease) End(KeyCode.LeftControl, pointer);
        if (rightRelease) End(KeyCode.RightControl, pointer);
    }

    private static bool HasOtherKey(Keyboard keyboard, KeyCode side)
    {
        if (keyboard == null) return false;
        var allowed = side == KeyCode.LeftControl ? Key.LeftCtrl : Key.RightCtrl;
        var keys = keyboard.allKeys;
        for (int i = 0; i < keys.Count; i++)
        {
            var key = keys[i];
            if (key.keyCode != allowed && (key.isPressed || key.wasPressedThisFrame || key.wasReleasedThisFrame))
                return true;
        }
        return false;
    }

    private void Begin(KeyCode side, Vector2 pointer)
    {
        if (chordFrame == Time.frameCount || lastReleaseFrame == Time.frameCount) return;
        if (pressedKey != KeyCode.None)
        {
            if (pressedKey != side) CancelChord();
            return;
        }
        var target = AtMouse(pointer);
        if (!target.HasValue) return;
        pressedChoice = target;
        pressedKey = side;
        scene = SceneManager.GetActiveScene().handle;
    }

    private void End(KeyCode side, Vector2 pointer)
    {
        if (pressedKey != side) return;
        lastReleaseFrame = Time.frameCount;
        var pressed = pressedChoice;
        pressedKey = KeyCode.None;
        pressedChoice = null;
        if (pressed is not Choice choice || !choice.Matches(AtMouse(pointer))) return;
        bool alreadyShown = choice.Matches(shownChoice);
        Hide();
        if (!alreadyShown) shownChoice = choice;
    }

    private void CancelChord()
    {
        pressedKey = KeyCode.None;
        pressedChoice = null;
        chordFrame = Time.frameCount;
    }

    private Choice? AtMouse(Vector2 pointer)
    {
        if (pointer.x < 0 || pointer.y < 0 || pointer.x >= Screen.width || pointer.y >= Screen.height) return null;
        var hit = HitAt(pointer);
        return hit == null || !hit.activeInHierarchy ? null : BoundChoice(hit);
    }

    private void Hide()
    {
        shownChoice = null;
        heading = body = null;
    }

    internal void Reset()
    {
        CancelChord();
        Hide();
        hasMouse = false;
    }

    private GameObject HitAt(Vector2 guiMouse)
    {
        var system = EventSystem.current;
        if (system == null || system.currentInputModule == null) return null;
        var module = system.currentInputModule.TryCast<InputSystemUIInputModule>();
        if (module != null)
        {
            var mouse = Mouse.current;
            return mouse == null ? null : module.GetLastRaycastResult(mouse.deviceId).gameObject;
        }

        if (legacySystem != system)
        {
            legacySystem = system;
            legacyPointer = new PointerEventData(system);
            legacyHits = new Il2CppSystem.Collections.Generic.List<RaycastResult>();
        }
        legacyPointer.position = new Vector2(guiMouse.x, Screen.height - guiMouse.y);
        legacyHits.Clear();
        system.RaycastAll(legacyPointer, legacyHits);
        return legacyHits.Count == 0 ? null : legacyHits[0].gameObject;
    }

    private static T Owner<T>(GameObject hit) where T : Component =>
        hit.GetComponentInParent(Il2CppType.Of<T>())?.Cast<T>();

    private static Choice? FromResponse(GameObject hit, Response response, Button button)
    {
        if (button == null || !button.IsInteractable() || response?.enabled == false) return null;
        var reader = Owner<PrologueEpiloguePanel>(hit);
        if (reader == null) return new Choice(Kind.DetachedDialogue, button, response?.Pointer ?? IntPtr.Zero, null, response);
        var kind = reader.State == PrologueEpiloguePanel.PrologueEpilogueState.Prologue
            ? Kind.ReadingPrologue : Kind.Dialogue;
        return new Choice(kind, button, response?.Pointer ?? IntPtr.Zero, null, response);
    }

    private static Choice? BoundChoice(GameObject hit)
    {
        // Only the topmost hit and its ancestors count. A modal may obscure another choice.
        var dialogue = Owner<TemplateConversationResponse>(hit);
        if (dialogue != null)
            return FromResponse(hit, dialogue.response, dialogue.responseButton);

        var standard = Owner<StandardUIResponseButton>(hit);
        if (standard != null)
            return standard.isVisible && standard.isClickable ? FromResponse(hit, standard.response, standard.button) : null;

        var classic = Owner<UnityUIResponseButton>(hit);
        if (classic != null)
            return classic.visible && classic.clickable ? FromResponse(hit, classic.response, classic.button) : null;

        var control = Owner<Selectable>(hit);
        if (control != null && !control.IsInteractable()) return null;

        var decision = Owner<TemplateDecisionOptionButton>(hit);
        if (decision != null)
            return new Choice(Kind.Decision, decision, decision.currentDecisionOption?.Pointer ?? IntPtr.Zero, decision.currentDecisionOption?.Instruction);

        var multiple = Owner<TemplateMultipleChoiceOption>(hit);
        if (multiple != null)
            return new Choice(Kind.Form, multiple, multiple.currentChoiceData?.Pointer ?? IntPtr.Zero, multiple.currentChoiceData?.MultipleChoiceOptionProperties?.Instruction);

        var carousel = Owner<TemplateCarouselChoiceOption>(hit);
        if (carousel != null)
            return new Choice(Kind.Carousel, carousel, carousel.selectedChoiceData?.Pointer ?? IntPtr.Zero, carousel.selectedChoiceData?.CarouselChoiceOptionProperties?.Instruction);

        var prologue = Owner<TemplateSkipPrologueOption>(hit);
        if (prologue != null)
            return new Choice(Kind.Carousel, prologue, prologue.selectedOption?.Pointer ?? IntPtr.Zero, prologue.selectedOption?.CarouselChoiceOptionProperties?.Instruction);

        var decree = Owner<TemplateDecree>(hit);
        if (decree != null)
            return new Choice(Kind.Decree, decree, decree.currentDecreeData?.Pointer ?? IntPtr.Zero, decree.currentDecreeData?.DecreeProperties?.OnDecreeEnact);

        // The details page also contains unrelated controls. Only the bound sign button is a choice.
        var details = Owner<DecreeDetailsPage>(hit);
        if (details != null && details.signButton != null && Owner<Button>(hit) == details.signButton)
            return new Choice(Kind.Decree, details, details.currentDecreeData?.Pointer ?? IntPtr.Zero, details.currentDecreeData?.DecreeProperties?.OnDecreeEnact);

        var bill = Owner<BillPanel>(hit);
        return bill == null ? null : BillChoice(bill, Owner<Button>(hit));
    }

    private static Choice? BillChoice(BillPanel panel, Button button)
    {
        var bill = panel.currentBillData?.BillProperties;
        if (button == null || !button.IsInteractable() || bill == null) return null;
        // In both campaigns the sign button is bound by its persistent click event.
        // BillPanel does not expose a signButton field. Check the actual target without invoking it.
        var click = button.onClick;
        if (click == null) return null;
        bool sign = false, veto = false;
        for (int i = 0, count = click.GetPersistentEventCount(); i < count; i++)
        {
            if (click.GetPersistentTarget(i) != panel ||
                click.GetPersistentListenerState(i) == UnityEventCallState.Off) continue;
            var method = click.GetPersistentMethodName(i);
            sign |= method == nameof(BillPanel.SignBill);
            veto |= method == nameof(BillPanel.VetoBill);
        }
        if (sign == veto || veto != (button == panel.vetoButton)) return null;
        return sign ? new Choice(Kind.SignBill, button, panel.currentBillData.Pointer, bill.SignVariables) :
            new Choice(Kind.VetoBill, button, panel.currentBillData.Pointer, bill.VetoVariables);
    }

    private void Read(Choice choice, bool chinese)
    {
        var entry = choice.Response?.destinationEntry;
        bool readingPrologue = choice.Kind == Kind.ReadingPrologue;
        if (choice.Kind == Kind.DetachedDialogue && entry != null)
        {
            // A response canvas can live outside its reader panel. Resolve the bound entry's
            // conversation from game data, using the exact titles of the two campaign prologues.
            var conversationTitle = DialogueManager.masterDatabase?.GetConversation(entry.conversationID)?.Title;
            readingPrologue = conversationTitle is "Sordland/Prologue" or "Rizia/Prologue";
        }
        string summary = choice.Kind is Kind.Dialogue or Kind.DetachedDialogue or Kind.ReadingPrologue
            ? ChoicePath.Preview(choice.Response, chinese, readingPrologue)
            : ChoiceEffects.Preview(choice.Script, chinese, true);
        string title = choice.Kind switch
        {
            Kind.Form => chinese ? "确认选择后" : "When confirmed",
            Kind.Carousel => chinese ? "当前选项 · 确认后" : "Current choice · when confirmed",
            Kind.Decree => chinese ? "颁布后" : "When enacted",
            Kind.SignBill => chinese ? "签署后" : "When signed",
            Kind.VetoBill => chinese ? "否决后" : "When vetoed",
            _ => chinese ? "选项结果" : "Choice effects"
        };
        heading = new GUIContent(title);
        body = new GUIContent(summary);
        chineseText = chinese;
    }

    private void Draw(Vector2 mouse, PanelAppearance theme, bool glass)
    {
        float scale = Math.Clamp(Screen.height / 900f, 0.9f, 2f);
        float margin = 8 * scale, pad = 13 * scale, gap = 4 * scale, offset = 17 * scale;
        headingStyle ??= new GUIStyle { alignment = TextAnchor.MiddleLeft, clipping = TextClipping.Clip };
        bodyStyle ??= new GUIStyle { wordWrap = true, alignment = TextAnchor.UpperLeft, clipping = TextClipping.Clip };
        headingStyle.fontSize = (int)Math.Round(13 * scale);
        bodyStyle.fontSize = (int)Math.Round(16 * scale);
        headingStyle.normal.textColor = theme.Accent;
        bodyStyle.normal.textColor = theme.Text;
        float maxWidth = Math.Min(350 * scale, Screen.width - 2 * margin);
        if (maxWidth <= 2 * pad) return;
        float width = Math.Min(maxWidth, Math.Max(headingStyle.CalcSize(heading).x, bodyStyle.CalcSize(body).x) + 2 * pad + 2);
        float textWidth = width - 2 * pad;
        float headingHeight = headingStyle.CalcSize(heading).y;
        float bodyHeight = Math.Max(bodyStyle.CalcHeight(body, textWidth), bodyStyle.fontSize + 2);
        float height = Math.Min(Screen.height - 2 * margin, 2 * pad + headingHeight + gap + bodyHeight);
        if (height <= 2 * pad) return;
        float x = mouse.x + offset, y = mouse.y + offset;
        if (x + width > Screen.width - margin) x = mouse.x - width - offset;
        if (y + height > Screen.height - margin) y = mouse.y - height - offset;
        x = Math.Clamp(x, margin, Math.Max(margin, Screen.width - margin - width));
        y = Math.Clamp(y, margin, Math.Max(margin, Screen.height - margin - height));
        var box = new Rect(x, y, width, height);

        int depth = GUI.depth;
        var color = GUI.color;
        var ink = GUI.contentColor;
        try
        {
            GUI.depth = -849;
            GUI.color = GUI.contentColor = Color.white;
            // Draw-only primitives have no control IDs, raycast targets or pointer handlers.
            if (glass)
                GUI.DrawTexture(new Rect(x, y + 2 * scale, width, height), Texture2D.whiteTexture,
                    ScaleMode.StretchToFill, true, 0, new Color(0, 0, 0, .13f), 0, 13 * scale);
            theme.DrawGlass(box, 13 * scale, scale, glass);
            headingStyle.Draw(new Rect(x + pad, y + pad, textWidth, headingHeight), heading, false, false, false, false);
            float bodyY = y + pad + headingHeight + gap;
            bodyStyle.Draw(new Rect(x + pad, bodyY, textWidth, Math.Max(0, y + height - pad - bodyY)), body,
                false, false, false, false);
        }
        finally
        {
            GUI.depth = depth;
            GUI.color = color;
            GUI.contentColor = ink;
        }
    }

}
