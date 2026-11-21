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
        OnAccent, Selection, TextSelection, Hover, Success, SuccessSurface, Warning, WarningSurface;
    internal readonly bool SerifHeadings;

    private static Color Shade(int rgb) =>
        new Color32((byte)(rgb >> 16), (byte)(rgb >> 8), (byte)rgb, 255);

    private PanelAppearance(string chinese, string english, bool serifHeadings, int background,
        int surface, int border, int text, int muted, int accent, int accentHover, int onAccent,
        int selection, int textSelection, int hover, int success, int successSurface, int warning, int warningSurface)
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
        new("暖纸", "Paper", true, 0xFAF9F6, 0xFFFEFC, 0xDEDAD2, 0x2C2926, 0x655E56,
            0xA44D32, 0x89402A, 0xFFFFFF, 0xF4E6DA, 0xC49D8C, 0xF1EDE5, 0x38664C, 0xEDF5EF, 0x865034, 0xFAF0E9),
        new("林间", "Forest", true, 0xF7FAF8, 0xFFFFFF, 0xDDE7E0, 0x24332C, 0x52645A,
            0x327055, 0x275B44, 0xFFFFFF, 0xE3F1E8, 0x91B09C, 0xEEF4F0, 0x316446, 0xEBF5EE, 0x795332, 0xF8F2E9),
        new("雾蓝", "Fjord", false, 0xF7FAFC, 0xFFFFFF, 0xDCE6EC, 0x263642, 0x506371,
            0x356C8D, 0x2B5974, 0xFFFFFF, 0xE5F1F8, 0x95AFC0, 0xEEF4F7, 0x306658, 0xEAF4F0, 0x805137, 0xFAF0E8),
        new("午夜", "Midnight", false, 0x1D2228, 0x242A32, 0x3E4753, 0xEBEFF2, 0xB2BBC5,
            0xB8C8E9, 0xCDDAF0, 0x202733, 0x343E4A, 0x566A82, 0x2D353F, 0xACD8BD, 0x283B32, 0xE8BC9B, 0x40342D)
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
