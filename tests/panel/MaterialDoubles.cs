namespace UnityEngine;

public enum TextureFormat { RGBA32 }
public enum TextureWrapMode { Clamp }
public enum FilterMode { Bilinear }
public enum HideFlags { HideAndDontSave }

public class Texture2D
{
    public static readonly Texture2D whiteTexture = new(1, 1, TextureFormat.RGBA32, false);
    public HideFlags hideFlags;
    public TextureWrapMode wrapMode;
    public FilterMode filterMode;
    public Color32[] Pixels;
    public readonly int Width, Height;
    public Texture2D(int width, int height, TextureFormat format, bool mipChain) { Width=width; Height=height; }
    public void SetPixels32(Color32[] pixels) { Pixels=pixels; }
    public void Apply(bool updateMipmaps, bool makeNoLongerReadable) { }
}
