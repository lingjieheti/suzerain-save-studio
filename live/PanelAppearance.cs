using System;
using System.IO;
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

    private static Color Shade(int rgb) =>
        new Color32((byte)(rgb >> 16), (byte)(rgb >> 8), (byte)rgb, 255);

    private PanelAppearance(string chinese, string english, int background,
        int surface, int border, int text, int muted, int accent, int accentHover, int onAccent,
        int selection, int textSelection, int hover, int success, int successSurface, int warning, int warningSurface)
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
        new("午夜", "Midnight", 0x1D2228, 0x242A32, 0x3E4753, 0xEBEFF2, 0xB2BBC5,
            0xB8C8E9, 0xCDDAF0, 0x202733, 0x343E4A, 0x566A82, 0x2D353F, 0xACD8BD, 0x283B32, 0xE8BC9B, 0x40342D)
    };
}

internal static class PanelPreferences
{
    private const string Location = @"Software\SuzerainSaveStudio";
    internal static int ReadAppearance() => Read("Appearance", PanelAppearance.Choices.Length, PanelAppearance.Fjord);
    internal static bool WriteAppearance(int index) => index >= 0 && index < PanelAppearance.Choices.Length && Write("Appearance", index);
    internal static bool ReadChinese(bool defaultChinese) => Read("PanelLanguage", 2, defaultChinese ? 1 : 0) == 1;
    internal static bool WriteChinese(bool chinese) => Write("PanelLanguage", chinese ? 1 : 0);
    internal static bool ReadChoiceHints() => Read("ChoiceHints", 2, 0) == 1;
    internal static bool WriteChoiceHints(bool enabled) => Write("ChoiceHints", enabled ? 1 : 0);

    private static int Read(string name, int count, int fallback)
    {
        if (!OperatingSystem.IsWindows()) return fallback;
        try
        {
            using var user = RegistryKey.OpenBaseKey(RegistryHive.CurrentUser, RegistryView.Registry64);
            using var key = user.OpenSubKey(Location);
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
