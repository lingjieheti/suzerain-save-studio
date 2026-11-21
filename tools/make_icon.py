"""Build the app's multi-size Windows archive-book icon. Requires Pillow."""
from pathlib import Path
from PIL import Image, ImageDraw


def archive_icon(size):
    # Coordinates are in a 64-unit square; oversample the small shell icons.
    canvas = Image.new("RGBA", (size * 4, size * 4))
    draw = ImageDraw.Draw(canvas)
    scale = canvas.width / 64

    def points(*coordinates):
        return tuple(round(v * scale) for v in coordinates)

    clay = "#A34F3B"
    draw.rounded_rectangle(points(2, 2, 62, 62), radius=round(14 * scale), fill=clay)
    # Each page keeps its own silhouette so the central fold survives at 16 px.
    draw.polygon(points(12, 18, 21, 18, 30, 22, 30, 47, 21, 43, 12, 43), fill="#FFF9ED")
    draw.polygon(points(33, 22, 42, 18, 52, 18, 52, 43, 43, 43, 33, 47), fill="#F4E4D1")
    if size >= 32:
        draw.polygon(points(42, 18, 47, 18, 47, 30, 44.5, 28, 42, 30), fill="#BC725C")
    return canvas.resize((size, size), Image.Resampling.LANCZOS)


if __name__ == "__main__":
    sizes = (16, 20, 24, 32, 40, 48, 64, 128, 256)
    images = [archive_icon(size) for size in sizes]
    target = Path(__file__).resolve().parents[1] / "src" / "app.ico"
    images[-1].save(target, sizes=[im.size for im in images], append_images=images[:-1])
