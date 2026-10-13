#pragma once
#include "../core/core.h"
#include "Font.h"
#include <map>
#include <vector>
#include <string>
#include <unordered_map>

class DisplayText : public CoreObject
{
public:
    struct Modifiers
    {
        bool     Underline        = false;
        bool     Strikethrough    = false;
        bool     Invert           = false;
        bool     Highlight        = false;
        uint32_t HighlightColor   = 0xFFFFFF00;
    };

    // Run — НЕ изменяемый на месте. Только удалить + добавить заново.
    struct Run
    {
        // Позиция
        float OffsetX = 0.0f;
        float OffsetY = 0.0f;
        bool  NewLine = false;   // вставить перенос перед Start

        // Стиль
        Ref<Font> Font;
        bool      bHasColor = false;
        uint32_t  Color     = 0xFFFFFFFF;

        // Модификаторы
        Modifiers Mods;
    };

    struct RunKey
    {
        uint32_t Start = 0;
        uint32_t End   = 0;
        uint64_t Seq   = 0;

        bool operator<(const RunKey& o) const noexcept
        {
            if (Start != o.Start) return Start < o.Start;
            if (End   != o.End)   return End   < o.End;
            return Seq < o.Seq;
        }
        bool operator==(const RunKey& o) const noexcept
        {
            return Start == o.Start && End == o.End && Seq == o.Seq;
        }
    };

    struct Placed
    {
        uint32_t     Index = 0;      // индекс кодпойнта
        Ref<Font>    Font;
        Ref<FontSym> Sym;
        float        X = 0, Y = 0;   // bbox left/top, Y-вниз
        uint32_t     Color = 0xFFFFFFFF;
        Modifiers    Mods;
    };

    struct Decoration
    {
        enum Type { Highlight, Underline, Strikethrough };
        Type     type;
        float    X0, Y0, X1, Y1;
        uint32_t Color;
    };

    // ------------------------------------------------------------------
    // Текст
    // ------------------------------------------------------------------
    std::vector<uint32_t> Codepoints;
    void SetText(const std::string& utf8);

    void SetDefaultFont(Ref<Font> f) { _defaultFont = std::move(f); MarkDirty(); }
    Ref<Font> GetDefaultFont() const { return _defaultFont; }

    // ------------------------------------------------------------------
    // Runs
    // ------------------------------------------------------------------
    // Add — возвращает ключ, по которому потом можно удалить.
    RunKey AddRun(uint32_t start, uint32_t end, Run run,
                  const std::string& name = {});

    bool RemoveRun(const RunKey& key);
    bool RemoveRunByName(const std::string& name);

    bool HasRun(const RunKey& key) const;
    bool HasRunByName(const std::string& name) const;

    // Возвращаем копии, чтобы исключить in-place модификацию.
    Run GetRun(const RunKey& key) const;
    Run GetRunByName(const std::string& name) const;

    // Все Run, действующие в позиции pos:
    //   - [Start, End) содержит pos
    //   - точечные события (Start == End == pos)
    std::vector<Run> GetRunsAt(uint32_t pos) const;

    std::vector<RunKey> GetAllRunKeys() const;

    void ClearRuns();

    // ------------------------------------------------------------------
    // Layout (ленивый)
    // ------------------------------------------------------------------
    const std::vector<Placed>&     GetLayout() const;
    const std::vector<Decoration>& GetDecorations() const;
    float GetWidth() const;
    float GetHeight() const;

    void MarkDirty() { _dirty = true; }

    static void DecodeUtf8(const std::string& s, std::vector<uint32_t>& out);

private:
    // Упорядочены по Start. Уникальность — через Seq.
    std::map<RunKey, Run> _runs;
    std::unordered_map<std::string, RunKey> _byName;
    uint64_t _nextSeq = 0;

    Ref<Font> _defaultFont;

    // Ленивое состояние.
    mutable bool _dirty = true;
    mutable std::vector<Placed>     _layout;
    mutable std::vector<Decoration> _decorations;
    mutable std::vector<int>        _indexToPlaced; // -1 если не отложен
    mutable float _width = 0, _height = 0;

    void _Rebuild() const;
};