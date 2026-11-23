using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Security;
using Microsoft.Win32;
using UnityEngine;

namespace SuzerainSaveStudio.Live;

internal sealed class PanelAppearance
{
    internal const int Fjord = 2; // Stable index shared with the desktop editor.
    internal readonly string Chinese, English;
    internal readonly Color Background, Surface, Border, Text, Muted, Accent, AccentHover,
        OnAccent, Selection, TextSelection, Hover, Success, SuccessSurface, Warning, WarningSurface;
    internal readonly Color GlassFace, RailFace, Rim;
    private readonly int glintStrength, tint;
    private Texture2D sheen;

    private static Color Shade(int rgb, byte alpha = 255) =>
        new Color32((byte)(rgb >> 16), (byte)(rgb >> 8), (byte)rgb, alpha);

    private PanelAppearance(string chinese, string english, int background,
        int surface, int border, int text, int muted, int accent, int accentHover, int onAccent,
        int selection, int textSelection, int hover, int success, int successSurface, int warning, int warningSurface,
        bool dark = false)
    {
        Chinese = chinese;
        English = english;
        Background = Shade(background);
        Surface = Shade(surface);
        Border = Shade(border);
        Text = Shade(text);
        Muted = Shade(muted);
        Accent = Shade(accent);
        AccentHover = Shade(accentHover);
        OnAccent = Shade(onAccent);
        Selection = Shade(selection);
        TextSelection = Shade(textSelection);
        Hover = Shade(hover);
        Success = Shade(success);
        SuccessSurface = Shade(successSurface);
        Warning = Shade(warning);
        WarningSurface = Shade(warningSurface);
        GlassFace = Shade(background, 238);
        RailFace = Shade(selection, 171);
        Rim = Shade(dark ? 0xA5BCD3 : 0x526575, dark ? (byte)104 : (byte)80);
        glintStrength = dark ? 20 : 184;
        tint = accent;
    }

    // A single filtered texture per palette supplies the broad upper reflection and
    // a small amount of its tint on the opposite edge. No frame capture or blur pass.
    private Texture2D Sheen()
    {
        if (sheen != null) return sheen;
        const int width = 48, height = 32;
        var pixels = new Color32[width * height];
        for (int y = 0; y < height; y++)
        {
            double v = (double)y / (height - 1);
            for (int x = 0; x < width; x++)
            {
                double u = (double)x / (width - 1);
                double top = Math.Exp(-4 * (u - .20) * (u - .20) - 9 * (1 - v));
                double bottom = Math.Exp(-7 * (u - .82) * (u - .82) - 12 * v);
                double light = glintStrength * top, shade = 6 * bottom;
                double mix = light / (light + shade);
                byte Channel(int shift) => (byte)(255 * mix + ((tint >> shift) & 255) * (1 - mix));
                pixels[y * width + x] = new Color32(Channel(16), Channel(8), Channel(0), (byte)(light + shade));
            }
        }
        sheen = new Texture2D(width, height, TextureFormat.RGBA32, false);
        sheen.hideFlags = HideFlags.HideAndDontSave;
        sheen.wrapMode = TextureWrapMode.Clamp;
        sheen.filterMode = FilterMode.Bilinear;
        sheen.SetPixels32(pixels);
        sheen.Apply(false, true);
        return sheen;
    }

    internal static void ReleaseTextures()
    {
        foreach (var theme in Choices)
            if (theme.sheen != null) { UnityEngine.Object.Destroy(theme.sheen); theme.sheen = null; }
    }

    internal void DrawGlass(Rect rect, float radius, float unit, bool glass, bool rail = false)
    {
        if (Event.current.type != EventType.Repaint) return;
        var face = rail ? (glass ? RailFace : Hover) : (glass ? GlassFace : Background);
        GUI.DrawTexture(rect, Texture2D.whiteTexture, ScaleMode.StretchToFill, true, 0, face, 0, radius);
        if (glass) GUI.DrawTexture(rect, Sheen(), ScaleMode.StretchToFill, true, 0, Color.white, 0, radius);
        GUI.DrawTexture(rect, Texture2D.whiteTexture, ScaleMode.StretchToFill, true, 0,
            glass ? Rim : Border, unit, radius);
    }

    // Index is the stable preference shared with the Windows editor.
    internal static readonly PanelAppearance[] Choices =
    {
        new("暖纸", "Paper", 0xFAF9F6, 0xFFFEFC, 0xDEDAD2, 0x2C2926, 0x655E56,
            0xA44D32, 0x89402A, 0xFFFFFF, 0xF4E6DA, 0xC49D8C, 0xF1EDE5, 0x38664C, 0xEDF5EF, 0x865034, 0xFAF0E9),
        new("林间", "Forest", 0xF7FAF8, 0xFFFFFF, 0xDDE7E0, 0x24332C, 0x52645A,
            0x327055, 0x275B44, 0xFFFFFF, 0xE3F1E8, 0x91B09C, 0xEEF4F0, 0x316446, 0xEBF5EE, 0x795332, 0xF8F2E9),
        new("雾蓝", "Fjord", 0xF9FBFC, 0xFFFFFF, 0xDFE6EA, 0x25323B, 0x536570,
            0x336C85, 0x295A72, 0xFFFFFF, 0xE8F1F5, 0x95AFC0, 0xF0F4F6, 0x306658, 0xEAF4F0, 0x805137, 0xFAF0E8),
        new("午夜", "Midnight", 0x1D2228, 0x242A32, 0x3E4753, 0xEBEFF2, 0xBEC8D0,
            0xB8C8E9, 0xCDDAF0, 0x202733, 0x343E4A, 0x566A82, 0x2D353F, 0xACD8BD, 0x283B32, 0xE8BC9B, 0x40342D, dark:true)
    };
}

internal static class PanelPreferences
{
    private const string Location = @"Software\SuzerainSaveStudio";
    private const string WindowsAppearance = @"Software\Microsoft\Windows\CurrentVersion\Themes\Personalize";
    internal static int ReadAppearance() => Read("Appearance", PanelAppearance.Choices.Length, PanelAppearance.Fjord);
    internal static bool WriteAppearance(int index) => index >= 0 && index < PanelAppearance.Choices.Length && Write("Appearance", index);
    internal static bool ReadChinese(bool defaultChinese) => Read("PanelLanguage", 2, defaultChinese ? 1 : 0) == 1;
    internal static bool WriteChinese(bool chinese) => Write("PanelLanguage", chinese ? 1 : 0);
    internal static bool ReadChoiceHints() => Read("ChoiceHints", 2, 0) == 1;
    internal static bool WriteChoiceHints(bool enabled) => Write("ChoiceHints", enabled ? 1 : 0);

    [StructLayout(LayoutKind.Sequential)]
    private struct HighContrast { internal uint Size, Flags; internal IntPtr Scheme; }

    [DllImport("user32.dll", EntryPoint = "SystemParametersInfoW")]
    private static extern bool SystemParametersInfo(uint action, uint size, ref HighContrast value, uint flags);

    internal static bool ReadGlassEffects()
    {
        if (!OperatingSystem.IsWindows()) return true;
        if (Read("GlassEffects", 2, 1) == 0 || Read("EnableTransparency", 2, 1, WindowsAppearance) == 0)
            return false;
        var setting = new HighContrast { Size = (uint)Marshal.SizeOf<HighContrast>() };
        return !SystemParametersInfo(0x0042 /* SPI_GETHIGHCONTRAST */, setting.Size, ref setting, 0) ||
               (setting.Flags & 1) == 0;
    }

    private static int Read(string name, int count, int fallback, string location = Location)
    {
        if (!OperatingSystem.IsWindows()) return fallback;
        try
        {
            using var user = RegistryKey.OpenBaseKey(RegistryHive.CurrentUser, RegistryView.Registry64);
            using var key = user.OpenSubKey(location);
            return key?.GetValue(name) is int index && index >= 0 && index < count ? index : fallback;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException or SecurityException)
        {
            return fallback;
        }
    }

    private static bool Write(string name, int index)
    {
        if (!OperatingSystem.IsWindows()) return false;
        try
        {
            using var user = RegistryKey.OpenBaseKey(RegistryHive.CurrentUser, RegistryView.Registry64);
            using var key = user.CreateSubKey(Location);
            if (key == null) return false;
            key.SetValue(name, index, RegistryValueKind.DWord);
            return true;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException or SecurityException)
        {
            return false;
        }
    }
}
