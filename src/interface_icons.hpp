#pragma once

// Common optical bounds keep navigation and file commands aligned with their labels.
namespace icons {
enum class Symbol {
    Economy, Inventory, People, Globe, Sliders, Review, Archive, Book, Heart,
    Reload, Folder, Open, Saves, Undo, Redo, Search, Crown, Pulse, Arrow, Document
};

inline void draw(HDC dc, Symbol symbol, int x, int y, int pixels, COLORREF ink) {
    if (pixels <= 0) return;
    using namespace Gdiplus;
    Graphics graphics(dc);
    graphics.SetPageUnit(UnitPixel);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.TranslateTransform(static_cast<REAL>(x), static_cast<REAL>(y));
    graphics.ScaleTransform(pixels / 20.f, pixels / 20.f);
    Pen pen(Color(255, GetRValue(ink), GetGValue(ink), GetBValue(ink)), 1.65f);
    pen.SetLineCap(LineCapRound, LineCapRound, DashCapRound);
    pen.SetLineJoin(LineJoinRound);
    auto line = [&](std::initializer_list<PointF> points) {
        graphics.DrawLines(&pen, points.begin(), static_cast<INT>(points.size()));
    };
    auto oval = [&](REAL left, REAL top, REAL width, REAL height) {
        graphics.DrawEllipse(&pen, left, top, width, height);
    };
    auto curve = [&](PointF a, PointF b, PointF c, PointF d) {
        graphics.DrawBezier(&pen, a, b, c, d);
    };
    switch (symbol) {
    case Symbol::Economy:
        line({{2,17},{18,17}}); line({{4,13},{4,10}}); line({{10,13},{10,6}}); line({{16,13},{16,2}}); break;
    case Symbol::Inventory:
        line({{2,7},{10,2},{18,7},{2,7}}); line({{4.5f,10},{4.5f,14}});
        line({{10,10},{10,14}}); line({{15.5f,10},{15.5f,14}}); line({{2.5f,17},{17.5f,17}}); break;
    case Symbol::People:
        oval(4,2.5f,6.2f,6.2f); curve({1.5f,16.5f},{2,9},{12,9},{13,16.5f});
        curve({13,3},{17,3.5f},{17.2f,8.2f},{13.5f,9}); curve({14,11.5f},{16,11.7f},{18,13.5f},{18.5f,16}); break;
    case Symbol::Globe:
        oval(2,2,16,16); oval(6.5f,2,7,16); line({{2.5f,10},{17.5f,10}}); break;
    case Symbol::Sliders:
        line({{2,5},{18,5}}); line({{2,10},{18,10}}); line({{2,15},{18,15}});
        line({{7,2.7f},{7,7.3f}}); line({{13,7.7f},{13,12.3f}}); line({{6,12.7f},{6,17.3f}}); break;
    case Symbol::Review:
        line({{8,17},{3,17},{3,2.5f},{14.5f,2.5f},{14.5f,8.5f}}); line({{6,6.5f},{11.5f,6.5f}});
        line({{8.5f,13.5f},{11.5f,16.5f},{18,10}}); break;
    case Symbol::Archive:
        line({{2,3},{18,3},{18,7},{2,7},{2,3}}); line({{4,7},{4,17},{16,17},{16,7}});
        line({{8,10.5f},{12,10.5f}}); break;
    case Symbol::Book:
        curve({2,3.5f},{5,2.5f},{7.7f,3.2f},{10,5}); curve({10,5},{12.3f,3.2f},{15,2.5f},{18,3.5f});
        line({{2,3.5f},{2,15.5f}}); line({{18,3.5f},{18,15.5f}}); line({{10,5},{10,17}});
        curve({2,15.5f},{5,14.5f},{7.7f,15.2f},{10,17}); curve({10,17},{12.3f,15.2f},{15,14.5f},{18,15.5f}); break;
    case Symbol::Heart:
        curve({10,17},{8,15.5f},{2,11.2f},{2,6.8f}); curve({2,6.8f},{2,2.1f},{7.7f,1.5f},{10,6});
        curve({10,6},{12.3f,1.5f},{18,2.1f},{18,6.8f}); curve({18,6.8f},{18,11.2f},{12,15.5f},{10,17}); break;
    case Symbol::Reload:
        graphics.DrawArc(&pen, 3.f, 3.f, 14.f, 14.f, 25.f, 290.f); line({{12,6},{16.5f,6},{16.5f,1.5f}}); break;
    case Symbol::Folder:
        line({{2,16},{2,4},{7.8f,4},{10,6.5f},{17.5f,6.5f},{17.5f,16},{2,16}}); break;
    case Symbol::Open:
        line({{6,17},{3,17},{3,2},{11,2},{15,6},{15,8}}); line({{10.5f,2.5f},{10.5f,6.5f},{14.5f,6.5f}});
        line({{8,13.5f},{18,13.5f},{14.5f,10}}); line({{18,13.5f},{14.5f,17}}); break;
    case Symbol::Saves:
        line({{2.3f,3.5f},{17.7f,3.5f},{17.7f,16.5f},{2.3f,16.5f},{2.3f,3.5f}});
        line({{6,7.5f},{14,7.5f}}); line({{6,12},{10.5f,12}}); break;
    case Symbol::Undo: case Symbol::Redo:
        if (symbol==Symbol::Redo) { graphics.TranslateTransform(20.f,0.f); graphics.ScaleTransform(-1.f,1.f); }
        curve({17,15},{17,9},{14,7},{4,7}); line({{8.5f,2.8f},{4,7},{8.5f,11.2f}}); break;
    case Symbol::Search:
        oval(2,2,11,11); line({{11.8f,11.8f},{17.5f,17.5f}}); break;
    case Symbol::Crown:
        line({{2,5},{6,8},{10,3},{14,8},{18,5},{16,13.5f},{4,13.5f},{2,5}}); line({{4.5f,17},{15.5f,17}}); break;
    case Symbol::Pulse:
        line({{2,11},{5,11},{8,3},{11.7f,17},{14.5f,9},{18,9}}); break;
    case Symbol::Arrow:
        line({{2,10},{17,10},{11.5f,4.5f}}); line({{17,10},{11.5f,15.5f}}); break;
    case Symbol::Document:
        line({{4,2},{11.5f,2},{16,6.5f},{16,18},{4,18},{4,2}}); line({{11,2.5f},{11,7},{15.5f,7}});
        line({{7,11},{13,11}}); break;
    }
}
} // namespace icons
