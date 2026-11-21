// Minimal Unity API for driving the production OnGUI method without rendering or a game installation.
using System;
using System.Collections.Generic;

namespace Il2CppInterop.Runtime.Attributes
{
    internal sealed class HideFromIl2CppAttribute : Attribute { }
}

namespace UnityEngine
{
    public class Object
    {
        public static void Destroy(object target) { }
        public static void DontDestroyOnLoad(object target) { }
    }

    public class GameObject
    {
        public bool activeSelf;
        public readonly Transform transform = new();
        public GameObject(string name) { }
        public void SetActive(bool active) => activeSelf = active;
        public T AddComponent<T>() where T : new() => new();
    }

    public class Transform { public void SetParent(Transform parent, bool worldPositionStays) { } }
    public class RectTransform : Transform { public Vector2 anchorMin, anchorMax, pivot, anchoredPosition, sizeDelta; }
    public class CanvasRenderer { public bool cullTransparentMesh; }
    public enum RenderMode { ScreenSpaceOverlay }
    public class Canvas { public RenderMode renderMode; public int sortingOrder; }
    public class MonoBehaviour { public MonoBehaviour(IntPtr ptr) { } }
    public enum SystemLanguage { English, Chinese, ChineseTraditional, ChineseSimplified }
    public static class Application { public static SystemLanguage systemLanguage; }
    public enum EventType { Layout, Repaint, KeyDown, KeyUp, MouseDown, MouseUp, ScrollWheel, Used }
    public enum KeyCode { None, F8, Escape, Tab, Space, Return, KeypadEnter }
    public enum TextAnchor { MiddleCenter, MiddleLeft, MiddleRight }
    public enum FontStyle { Normal, Bold }
    public enum TextClipping { Clip }
    public enum ScaleMode { StretchToFill }

    public class GUIContent
    {
        public readonly string text;
        public GUIContent(string value) => text = value;
    }
    public class Font { public static Font CreateDynamicFontFromOSFont(string[] names, int size) => new(); }
    public record struct Color32(byte R, byte G, byte B, byte A)
    {
        public static implicit operator Color(Color32 color) => new(color.R, color.G, color.B, color.A);
    }
    public record struct Color(byte R, byte G, byte B, byte A)
    {
        public static Color white => new(255, 255, 255, 255);
        public static Color clear => default;
    }
    public record struct Vector2(float x, float y) { public static Vector2 zero => default; }
    public record struct Vector3(float x, float y, float z);
    public record struct Matrix4x4(float ScaleFactor)
    {
        public static Matrix4x4 Scale(Vector3 scale) => new(scale.x);
    }
    public class RectOffset { public RectOffset(int left, int right, int top, int bottom) { } }
    public record struct Rect(float x, float y, float width, float height)
    {
        public bool Contains(Vector2 point) =>
            point.x >= x && point.x < x + width && point.y >= y && point.y < y + height;
    }
    public static class Screen { public static int width = 1920, height = 1080; }
    public static class Time { public static float realtimeSinceStartup; }
    public static class Texture2D { public static readonly object whiteTexture = new(); }
    public class GUIStyleState { public Color textColor; }
    public class GUIStyle
    {
        public Font font;
        public int fontSize;
        public FontStyle fontStyle;
        public bool wordWrap;
        public readonly GUIStyleState normal = new(), hover = new(), active = new(), focused = new();
        public RectOffset padding;
        public TextClipping clipping;
        public TextAnchor alignment;
        public Vector2 contentOffset;
        public Vector2 CalcSize(GUIContent content) => new(content.text.Length * fontSize * .57f, fontSize);
    }
    public class GUISettings { public Color cursorColor, selectionColor; }
    public class GUISkin { public readonly GUISettings settings = new(); }
    public class Event
    {
        public static Event current = new();
        public EventType rawType, type;
        public KeyCode keyCode;
        public bool control, shift, alt;
        public Vector2 mousePosition;
        public bool isMouse => type == EventType.MouseDown || type == EventType.MouseUp;
        public void Use() => type = EventType.Used;
    }

    public static class GUI
    {
        public static bool enabled = true;
        public static readonly GUISkin skin = new();
        public static Color color, backgroundColor, contentColor;
        public static Matrix4x4 matrix;
        public static int depth;

        // The harness clicks recorded button coordinates and inspects the next repaint.
        public static Rect Group;
        public static float GroupScale;
        public static readonly Dictionary<string, Rect> Buttons = new();
        public static readonly List<string> Labels = new();
        public static readonly Dictionary<string, (Font Font, FontStyle Weight, int Size, Rect Bounds)> LabelTypes = new();
        public static readonly Dictionary<string, (Font Font, FontStyle Weight)> ButtonTypes = new();
        public static readonly List<Color> Fills = new();
        public static string TypedText, DisplayedDraft;

        public static void DrawTexture(Rect rect, object texture, ScaleMode mode, bool blend, float aspect,
            Color fill, float border, float radius) => Fills.Add(fill);
        public static void FocusControl(string name) { }
        public static void SetNextControlName(string name) { }
        public static void BeginGroup(Rect area)
        {
            Group = area;
            GroupScale = matrix.ScaleFactor;
            Buttons.Clear();
            Labels.Clear();
            LabelTypes.Clear();
            ButtonTypes.Clear();
            Fills.Clear();
            DisplayedDraft = null;
        }
        public static void EndGroup() { }
        public static void Label(Rect rect, string text, GUIStyle style)
        {
            Labels.Add(text);
            LabelTypes[text] = (style.font,style.fontStyle,style.fontSize,rect);
        }
        public static bool Button(Rect rect, string label, GUIStyle style)
        {
            if (label.Length > 0) {
                Buttons.Add(label,rect);
                ButtonTypes[label] = (style.font,style.fontStyle);
            }
            return enabled && Event.current.type == EventType.MouseUp && rect.Contains(Event.current.mousePosition);
        }
        public static string TextField(Rect rect, string draft, int maxLength, GUIStyle style)
        {
            DisplayedDraft = TypedText ?? draft;
            TypedText = null;
            return DisplayedDraft;
        }
    }
}

namespace UnityEngine.UI
{
    public class GraphicRaycaster { }
    public class Image
    {
        public UnityEngine.Color color;
        public bool raycastTarget;
        public readonly UnityEngine.CanvasRenderer canvasRenderer = new();
        public readonly UnityEngine.RectTransform rectTransform = new();
    }
}
