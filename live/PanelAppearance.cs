using System;
using System.IO;
using System.Security;
using Microsoft.Win32;
using UnityEngine;

namespace SuzerainSaveStudio.Live;

internal sealed class PanelAppearance
{
    internal readonly string Chinese, English;
    internal readonly Color Background, Surface, Border, Text, Muted, Accent, AccentHover,
        OnAccent, Selection, Hover, Success, SuccessSurface, Warning, WarningSurface;
    internal readonly bool SerifHeadings;

    private static Color Shade(int rgb) =>
        new Color32((byte)(rgb >> 16), (byte)(rgb >> 8), (byte)rgb, 255);

    private PanelAppearance(string chinese, string english, bool serifHeadings, int background,
        int surface, int border, int text, int muted, int accent, int accentHover, int onAccent,
        int selection, int hover, int success, int successSurface, int warning, int warningSurface)
    {
        Chinese = chinese;
        English = english;
        SerifHeadings = serifHeadings;
        Background = Shade(background);
        Surface = Shade(surface);
        Border = Shade(border);
        Text = Shade(text);
        Muted = Shade(muted);
        Accent = Shade(accent);
        AccentHover = Shade(accentHover);
        OnAccent = Shade(onAccent);
        Selection = Shade(selection);
        Hover = Shade(hover);
        Success = Shade(success);
        SuccessSurface = Shade(successSurface);
        Warning = Shade(warning);
        WarningSurface = Shade(warningSurface);
    }

    // Index is the stable preference shared with the Windows editor.
    internal static readonly PanelAppearance[] Choices =
    {
        new("暖纸", "Paper", true, 0xF9F7F1, 0xFFFDF8, 0xD6D0C4, 0x312B25, 0x665E53,
            0x9A442A, 0x863722, 0xFFFAF3, 0xF8E7DA, 0xF0EBE0, 0x375F44, 0xE8F0E5, 0x863E29, 0xF8E7DA),
        new("林间", "Forest", true, 0xEFF4EC, 0xF8FAF4, 0xB5C5B5, 0x20372E, 0x425D4E,
            0x346247, 0x28543A, 0xF5FAF1, 0xD3E5D4, 0xE3ECDD, 0x275A3C, 0xD8EAD7, 0x854121, 0xF3E5D3),
        new("雾蓝", "Fjord", false, 0xEBF1F6, 0xF6F9FC, 0xB6C6D4, 0x1E3346, 0x465E72,
            0x326480, 0x28546E, 0xF6FBFF, 0xD2E5F1, 0xDFE9EF, 0x275C52, 0xDBECE5, 0x7D4229, 0xF7E7D9),
        new("午夜", "Midnight", false, 0x191C25, 0x222631, 0x454C63, 0xE8E9F3, 0xB1B7CA,
            0xBBA8EE, 0xCAB9F9, 0x292235, 0x3B354E, 0x2F3443, 0xABE1B9, 0x273C31, 0xFFBB99, 0x452F2A)
    };
}

internal static class AppearancePreference
{
    private const string Location = @"Software\SuzerainSaveStudio";
    private const string Name = "Appearance";

    internal static int Read()
    {
        if (!OperatingSystem.IsWindows()) return 0;
        try
        {
            using var user = RegistryKey.OpenBaseKey(RegistryHive.CurrentUser, RegistryView.Registry64);
            using var key = user.OpenSubKey(Location);
            return key?.GetValue(Name) is int index && index >= 0 && index < PanelAppearance.Choices.Length
                ? index : 0;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException or SecurityException)
        {
            return 0;
        }
    }

    internal static bool Write(int index)
    {
        if (!OperatingSystem.IsWindows() || index < 0 || index >= PanelAppearance.Choices.Length) return false;
        try
        {
            using var user = RegistryKey.OpenBaseKey(RegistryHive.CurrentUser, RegistryView.Registry64);
            using var key = user.CreateSubKey(Location);
            if (key == null) return false;
            key.SetValue(Name, index, RegistryValueKind.DWord);
            return true;
        }
        catch (Exception e) when (e is IOException or UnauthorizedAccessException or SecurityException)
        {
            return false;
        }
    }
}
