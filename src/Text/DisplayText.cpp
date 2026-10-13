#include "DisplayText.h"
#include <algorithm>
#include <cfloat>
#include <cstring>

void DisplayText::DecodeUtf8(const std::string& s, std::vector<uint32_t>& out)
{
    const uint8_t* p = (const uint8_t*)s.data();
    const size_t n = s.size();
    size_t i = 0;
    while (i < n)
    {
        uint8_t b = p[i];
        if (b < 0x80) { out.push_back(b); ++i; continue; }

        uint32_t cp = 0; int len = 0;
        if      ((b & 0xE0) == 0xC0) { cp = b & 0x1F; len = 2; }
        else if ((b & 0xF0) == 0xE0) { cp = b & 0x0F; len = 3; }
        else if ((b & 0xF8) == 0xF0) { cp = b & 0x07; len = 4; }
        else { ++i; continue; }

        if (i + len > n) break;
        bool ok = true;
        for (int k = 1; k < len; ++k) {
            if ((p[i+k] & 0xC0) != 0x80) { ok = false; break; }
            cp = (cp << 6) | (p[i+k] & 0x3F);
        }
        if (ok) { out.push_back(cp); i += len; }
        else    { ++i; }
    }
}

void DisplayText::SetText(const std::string& utf8)
{
    Codepoints.clear();
    DecodeUtf8(utf8, Codepoints);
    MarkDirty();
}

// ---------------------------------------------------------------------
// Runs
// ---------------------------------------------------------------------
DisplayText::RunKey DisplayText::AddRun(uint32_t start, uint32_t end,
                                        Run run, const std::string& name)
{
    RunKey key;
    key.Start = start;
    key.End   = end;
    key.Seq   = _nextSeq++;

    _runs.emplace(key, std::move(run));
    if (!name.empty())
        _byName[name] = key;

    MarkDirty();
    return key;
}

bool DisplayText::RemoveRun(const RunKey& key)
{
    auto it = _runs.find(key);
    if (it == _runs.end()) return false;

    // Чистим _byName, если он указывал на этот ключ.
    for (auto bit = _byName.begin(); bit != _byName.end(); )
    {
        if (bit->second == key) bit = _byName.erase(bit);
        else ++bit;
    }

    _runs.erase(it);
    MarkDirty();
    return true;
}

bool DisplayText::RemoveRunByName(const std::string& name)
{
    auto it = _byName.find(name);
    if (it == _byName.end()) return false;
    return RemoveRun(it->second);
}

bool DisplayText::HasRun(const RunKey& key) const
{
    return _runs.find(key) != _runs.end();
}

bool DisplayText::HasRunByName(const std::string& name) const
{
    return _byName.find(name) != _byName.end();
}

DisplayText::Run DisplayText::GetRun(const RunKey& key) const
{
    auto it = _runs.find(key);
    return it != _runs.end() ? it->second : Run{};
}

DisplayText::Run DisplayText::GetRunByName(const std::string& name) const
{
    auto it = _byName.find(name);
    if (it == _byName.end()) return Run{};
    return GetRun(it->second);
}

std::vector<DisplayText::Run> DisplayText::GetRunsAt(uint32_t pos) const
{
    std::vector<Run> out;
    // _runs отсортирован по Start, поэтому пробегаем до первых Start > pos.
    for (auto& [key, run] : _runs)
    {
        if (key.Start > pos) break;

        if (key.Start == key.End)
        {
            // Точечное событие: работает ровно в позиции pos.
            if (key.Start == pos) out.push_back(run);
        }
        else if (key.Start <= pos && pos < key.End)
        {
            out.push_back(run);
        }
    }
    return out;
}

std::vector<DisplayText::RunKey> DisplayText::GetAllRunKeys() const
{
    std::vector<RunKey> out;
    out.reserve(_runs.size());
    for (auto& kv : _runs) out.push_back(kv.first);
    return out;
}

void DisplayText::ClearRuns()
{
    _runs.clear();
    _byName.clear();
    MarkDirty();
}

// ---------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------
const std::vector<DisplayText::Placed>& DisplayText::GetLayout() const
{
    if (_dirty) _Rebuild();
    return _layout;
}

const std::vector<DisplayText::Decoration>& DisplayText::GetDecorations() const
{
    if (_dirty) _Rebuild();
    return _decorations;
}

float DisplayText::GetWidth() const
{
    if (_dirty) _Rebuild();
    return _width;
}

float DisplayText::GetHeight() const
{
    if (_dirty) _Rebuild();
    return _height;
}

void DisplayText::_Rebuild() const
{
    _dirty = false;
    _layout.clear();
    _decorations.clear();
    _indexToPlaced.assign(Codepoints.size(), -1);
    _width = _height = 0.0f;

    // Определяем основной шрифт.
    Ref<Font> primary = _defaultFont;
    if (!primary)
        for (auto& [key, run] : _runs)
            if (run.Font && key.End > key.Start) { primary = run.Font; break; }

    if (!primary && !_runs.empty()) return;   // нет шрифта — нечего строить
    if (Codepoints.empty() || !primary) return;

    const float ascent     = primary->AscentPx();
    const float lineHeight = primary->LineHeightPx();

    float x = 0.0f;
    float y = 0.0f;
    float maxX = 0.0f;

    _layout.reserve(Codepoints.size());

    for (size_t i = 0; i < Codepoints.size(); ++i)
    {
        // --- point-events: переносы ---
        bool doNewline = false;
        for (auto& [key, run] : _runs)
        {
            if (key.Start > i) break;
            if (key.Start == i && key.End == i && run.NewLine)
                doNewline = true;
        }
        if (doNewline)
        {
            maxX = std::max(maxX, x);
            x = 0.0f;
            y += lineHeight;
        }

        // --- эффективный стиль в позиции i ---
        Ref<Font> font;
        uint32_t  color    = 0xFFFFFFFF;
        bool      hasColor = false;
        Modifiers mods;
        float     offX = 0.0f, offY = 0.0f;

        for (auto& [key, run] : _runs)
        {
            if (key.Start > i) break;
            if (key.Start <= i && i < key.End)
            {
                if (run.Font) font = run.Font;
                if (run.bHasColor) { color = run.Color; hasColor = true; }
                if (run.Mods.Underline)     mods.Underline     = true;
                if (run.Mods.Strikethrough) mods.Strikethrough = true;
                if (run.Mods.Invert)        mods.Invert        = true;
                if (run.Mods.Highlight)
                {
                    mods.Highlight = true;
                    mods.HighlightColor = run.Mods.HighlightColor;
                }
                offX += run.OffsetX;
                offY += run.OffsetY;
            }
        }

        if (!font) font = primary;

        auto sym = font->GetSym(Codepoints[i]);
        if (!sym) continue;
        font->EnsurePacked(*sym);

        Placed p;
        p.Index   = (uint32_t)i;
        p.Font    = font;
        p.Sym     = sym;
        p.X       = x + offX + sym->BearingX * font->Size;
        p.Y       = y + ascent + offY - sym->BearingY * font->Size;
        p.Color   = color;
        p.Mods    = mods;

        _indexToPlaced[i] = (int)_layout.size();
        _layout.push_back(p);

        x += sym->Advance * font->Size;
    }

    maxX = std::max(maxX, x);
    _width  = maxX;
    _height = y + lineHeight;

    // -----------------------------------------------------------------
    // Декорации: считаем bbox по диапазону Placed каждого Run'а
    // -----------------------------------------------------------------
    for (auto& [key, run] : _runs)
    {
        if (key.End <= key.Start) continue;
        if (!run.Mods.Underline && !run.Mods.Strikethrough && !run.Mods.Highlight)
            continue;

        const uint32_t start = std::min(key.Start, (uint32_t)Codepoints.size());
        const uint32_t end   = std::min(key.End,   (uint32_t)Codepoints.size());

        float x0 =  FLT_MAX, x1 = -FLT_MAX;
        float baseline = 0.0f, fontSize = 0.0f;
        bool  first = true;

        for (uint32_t i = start; i < end; ++i)
        {
            int pi = _indexToPlaced[i];
            if (pi < 0) continue;
            const Placed& p = _layout[pi];
            if (!p.Sym || !p.Font) continue;

            const float adv = p.Sym->Advance * p.Font->Size;
            const float bx  = p.Sym->BearingX * p.Font->Size;
            const float by  = p.Sym->BearingY * p.Font->Size;

            const float left  = p.X - bx;
            const float right = left + adv;

            x0 = std::min(x0, left);
            x1 = std::max(x1, right);

            if (first)
            {
                baseline = p.Y + by;
                fontSize = p.Font->Size;
                first = false;
            }
        }
        if (first) continue; // ничего не отложено

        const float lineThick = std::max(1.0f, fontSize * 0.06f);

        if (run.Mods.Highlight)
        {
            Decoration d;
            d.type = Decoration::Highlight;
            d.X0 = x0;
            d.Y0 = baseline - ascent;
            d.X1 = x1;
            d.Y1 = baseline + (fontSize - ascent);
            d.Color = run.Mods.HighlightColor;
            _decorations.push_back(d);
        }

        if (run.Mods.Underline)
        {
            Decoration d;
            d.type = Decoration::Underline;
            d.X0 = x0;
            d.Y0 = baseline + fontSize * 0.10f;
            d.X1 = x1;
            d.Y1 = d.Y0 + lineThick;
            d.Color = run.bHasColor ? run.Color : 0xFFFFFFFF;
            _decorations.push_back(d);
        }

        if (run.Mods.Strikethrough)
        {
            Decoration d;
            d.type = Decoration::Strikethrough;
            d.X0 = x0;
            d.Y0 = baseline - fontSize * 0.30f;
            d.X1 = x1;
            d.Y1 = d.Y0 + lineThick;
            d.Color = run.bHasColor ? run.Color : 0xFFFFFFFF;
            _decorations.push_back(d);
        }
    }
}