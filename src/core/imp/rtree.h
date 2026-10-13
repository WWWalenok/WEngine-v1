#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <set>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "data.h"

#if defined(_MSC_VER)
#define _FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define _FORCEINLINE inline __attribute__((always_inline))
#elif defined(__INTEL_COMPILER)
#define _FORCEINLINE __forceinline
#else
#define _FORCEINLINE inline
#endif

template<typename T, size_t N>
struct RTBoundingBox
{
    std::array<T, N> Min;
    std::array<T, N> Max;
    mutable T CachedVolume;
    mutable bool bVolumeValid = false;

    RTBoundingBox()
    {
        for (size_t i = 0; i < N; ++i)
        {
            Min[i] = std::numeric_limits<T>::max();
            Max[i] = std::numeric_limits<T>::lowest();
        }
    }

    RTBoundingBox(const std::array<T, N>& MinCoords, const std::array<T, N>& MaxCoords) : Min(MinCoords), Max(MaxCoords)
    {
    }

    void ExpandToInclude(const RTBoundingBox<T, N>& Other)
    {
        if constexpr (N == 1)
        {
            Min[0] = std::min(Min[0], Other.Min[0]);
            Max[0] = std::max(Max[0], Other.Max[0]);
        }
        else if constexpr (N == 2)
        {
            Min[0] = std::min(Min[0], Other.Min[0]);
            Min[1] = std::min(Min[1], Other.Min[1]);
            Max[0] = std::max(Max[0], Other.Max[0]);
            Max[1] = std::max(Max[1], Other.Max[1]);
        }
        else if constexpr (N == 3)
        {
            Min[0] = std::min(Min[0], Other.Min[0]);
            Min[1] = std::min(Min[1], Other.Min[1]);
            Min[2] = std::min(Min[2], Other.Min[2]);
            Max[0] = std::max(Max[0], Other.Max[0]);
            Max[1] = std::max(Max[1], Other.Max[1]);
            Max[2] = std::max(Max[2], Other.Max[2]);
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                Min[i] = std::min(Min[i], Other.Min[i]);
                Max[i] = std::max(Max[i], Other.Max[i]);
            }
        }
        bVolumeValid = false;
    }

    void ExpandToInclude(const std::array<T, N>& Point)
    {
        if constexpr (N == 1)
        {
            Min[0] = std::min(Min[0], Point[0]);
            Max[0] = std::max(Max[0], Point[0]);
        }
        else if constexpr (N == 2)
        {
            Min[0] = std::min(Min[0], Point[0]);
            Min[1] = std::min(Min[1], Point[1]);
            Max[0] = std::max(Max[0], Point[0]);
            Max[1] = std::max(Max[1], Point[1]);
        }
        else if constexpr (N == 3)
        {
            Min[0] = std::min(Min[0], Point[0]);
            Min[1] = std::min(Min[1], Point[1]);
            Min[2] = std::min(Min[2], Point[2]);
            Max[0] = std::max(Max[0], Point[0]);
            Max[1] = std::max(Max[1], Point[1]);
            Max[2] = std::max(Max[2], Point[2]);
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                Min[i] = std::min(Min[i], Point[i]);
                Max[i] = std::max(Max[i], Point[i]);
            }
        }
        bVolumeValid = false;
    }

    void UpdateVolume() const
    {
        if constexpr (N == 1)
        {
            CachedVolume = Max[0] - Min[0];
        }
        else if constexpr (N == 2)
        {
            CachedVolume = (Max[0] - Min[0]) * (Max[1] - Min[1]);
        }
        else if constexpr (N == 3)
        {
            CachedVolume = (Max[0] - Min[0]) * (Max[1] - Min[1]) * (Max[2] - Min[2]);
        }
        else
        {
            CachedVolume = 1;
            for (size_t i = 0; i < N; ++i)
            {
                CachedVolume *= Max[i] - Min[i];
            }
        }
        bVolumeValid = true;
    }

    _FORCEINLINE const T& GetVolume() const
    {
        if (!bVolumeValid)
        {
            UpdateVolume();
        }
        return CachedVolume;
    }

    _FORCEINLINE T GetExpansionVolume(const RTBoundingBox<T, N>& Other) const
    {
        if constexpr (N == 1)
        {
            T Ds[]{std::max(Max[0], Other.Max[0]) - std::min(Min[0], Other.Min[0])};
            return Ds[0] - GetVolume();
        }
        else if constexpr (N == 2)
        {
            T Ds[]{std::max(Max[0], Other.Max[0]) - std::min(Min[0], Other.Min[0]), std::max(Max[1], Other.Max[1]) - std::min(Min[1], Other.Min[1])};
            return Ds[0] * Ds[1] - GetVolume();
        }
        else if constexpr (N == 3)
        {
            T Ds[]{std::max(Max[0], Other.Max[0]) - std::min(Min[0], Other.Min[0]), std::max(Max[1], Other.Max[1]) - std::min(Min[1], Other.Min[1]), std::max(Max[2], Other.Max[2]) - std::min(Min[2], Other.Min[2])};
            return Ds[0] * Ds[1] * Ds[2] - GetVolume();
        }
        else
        {
            RTBoundingBox<T, N> Expanded = *this;
            Expanded.ExpandToInclude(Other);
            return Expanded.GetVolume() - GetVolume();
        }
    }

    _FORCEINLINE bool Intersects(const RTBoundingBox<T, N>& Other) const
    {
        if constexpr (N == 1)
        {
            return !(Max[0] < Other.Min[0] || Min[0] > Other.Max[0]);
        }
        else if constexpr (N == 2)
        {
            return !(Max[0] < Other.Min[0] || Min[0] > Other.Max[0] || Max[1] < Other.Min[1] || Min[1] > Other.Max[1]);
        }
        else if constexpr (N == 3)
        {
            return !(Max[0] < Other.Min[0] || Min[0] > Other.Max[0] || Max[1] < Other.Min[1] || Min[1] > Other.Max[1] || Max[2] < Other.Min[2] || Min[2] > Other.Max[2]);
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                if (Max[i] < Other.Min[i] || Min[i] > Other.Max[i])
                {
                    return false;
                }
            }
        }
        return true;
    }

    _FORCEINLINE bool Contains(const std::array<T, N>& Point) const
    {
        if constexpr (N == 1)
        {
            return !(Point[0] < Min[0] || Point[0] > Max[0]);
        }
        else if constexpr (N == 2)
        {
            return !(Point[0] < Min[0] || Point[0] > Max[0] || Point[1] < Min[1] || Point[1] > Max[1]);
        }
        else if constexpr (N == 3)
        {
            return !(Point[0] < Min[0] || Point[0] > Max[0] || Point[1] < Min[1] || Point[1] > Max[1] || Point[2] < Min[2] || Point[2] > Max[2]);
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                if (Point[i] < Min[i] || Point[i] > Max[i])
                {
                    return false;
                }
            }
        }
        return true;
    }

    _FORCEINLINE bool Contains(const RTBoundingBox<T, N>& Other) const
    {
        if constexpr (N == 1)
        {
            return !(Other.Min[0] < Min[0] || Other.Max[0] > Max[0]);
        }
        else if constexpr (N == 2)
        {
            return !(Other.Min[0] < Min[0] || Other.Max[0] > Max[0] || Other.Min[1] < Min[1] || Other.Max[1] > Max[1]);
        }
        else if constexpr (N == 3)
        {
            return !(Other.Min[0] < Min[0] || Other.Max[0] > Max[0] || Other.Min[1] < Min[1] || Other.Max[1] > Max[1] || Other.Min[2] < Min[2] || Other.Max[2] > Max[2]);
        }
        else
        {
            for (size_t i = 0; i < N; ++i)
            {
                if (Other.Min[i] < Min[i] || Other.Max[i] > Max[i])
                {
                    return false;
                }
            }
        }
        return true;
    }

    _FORCEINLINE std::array<T, N> GetCenter() const
    {
        if constexpr (std::is_integral_v<T>)
        {
            if constexpr (N == 1)
            {
                return {Min[0] / static_cast<T>(2) + Max[0] / static_cast<T>(2)};
            }
            else if constexpr (N == 2)
            {
                return {Min[0] / static_cast<T>(2) + Max[0] / static_cast<T>(2), Min[1] / static_cast<T>(2) + Max[1] / static_cast<T>(2)};
            }
            else if constexpr (N == 3)
            {
                return {Min[0] / static_cast<T>(2) + Max[0] / static_cast<T>(2), Min[1] / static_cast<T>(2) + Max[1] / static_cast<T>(2), Min[2] / static_cast<T>(2) + Max[2] / static_cast<T>(2)};
            }
            else
            {
                std::array<T, N> Center;
                for (size_t i = 0; i < N; ++i)
                {
                    Center[i] = Min[i] / static_cast<T>(2) + Max[i] / static_cast<T>(2);
                }
                return Center;
            }
        }
        else
        {
            if constexpr (N == 1)
            {
                return {(Min[0] + Max[0]) * static_cast<T>(0.5)};
            }
            else if constexpr (N == 2)
            {
                return {(Min[0] + Max[0]) * static_cast<T>(0.5), (Min[1] + Max[1]) * static_cast<T>(0.5)};
            }
            else if constexpr (N == 3)
            {
                return {(Min[0] + Max[0]) * static_cast<T>(0.5), (Min[1] + Max[1]) * static_cast<T>(0.5), (Min[2] + Max[2]) * static_cast<T>(0.5)};
            }
            else
            {
                std::array<T, N> Center;
                for (size_t i = 0; i < N; ++i)
                {
                    Center[i] = (Min[i] + Max[i]) * static_cast<T>(0.5);
                }
                return Center;
            }
        }
    }

    _FORCEINLINE bool operator==(const RTBoundingBox<T, N>& Other) const
    {
        if constexpr (N == 1)
        {
            return Min[0] == Other.Min[0] && Max[0] == Other.Max[0];
        }
        else if constexpr (N == 2)
        {
            return Min[0] == Other.Min[0] && Max[0] == Other.Max[0] || Min[1] == Other.Min[1] && Max[1] == Other.Max[1];
        }
        else if constexpr (N == 3)
        {
            return Min[0] == Other.Min[0] && Max[0] == Other.Max[0] || Min[1] == Other.Min[1] && Max[1] == Other.Max[1] || Min[2] == Other.Min[2] && Max[2] == Other.Max[2];
        }
        else
        {
            std::array<T, N> Center;
            for (size_t i = 0; i < N; ++i)
            {
                if (Min[i] != Other.Min[i] || Max[i] != Other.Max[i])
                    return false;
            }
            return true;
        }
    }
};

template<typename T, size_t N>
class RTreeNode;

template<typename T, size_t N>
class RTree;

template<typename T, size_t N>
class RTreeObjectBase
{
    size_t TID;
    RTBoundingBox<T, N> CachedBoundingBox;
    mutable bool bDirty = true;
    RTreeObjectBase<T, N>* Parent;

public:
    RTreeObjectBase(size_t TID, RTreeObjectBase<T, N>* InParent = nullptr) : TID(TID), Parent(InParent)
    {
    }

    virtual ~RTreeObjectBase() = default;

    const RTBoundingBox<T, N>& GeRTBoundingBox()
    {
        if (bDirty)
        {
            bDirty = false;
            CachedBoundingBox = GeRTBoundingBoxImpl();
        }
        return CachedBoundingBox;
    }

    void SetDirty() const
    {
        if (bDirty)
        {
            return;
        }
        bDirty = true;
        if (Parent)
        {
            Parent->SetDirty();
        }
    }

    _FORCEINLINE RTreeNode<T, N>* GetParent()
    {
        return static_cast<RTreeNode<T, N>*>(Parent);
    }

    void SetParent(RTreeObjectBase<T, N>* InParent)
    {
        Parent = InParent;
    }

    bool operator==(const RTreeObjectBase<T, N>& Other) const
    {
        return GetId() == Other.GetId();
    }

    virtual size_t GetId() const = 0;
    virtual RTBoundingBox<T, N> GeRTBoundingBoxImpl() = 0;
};

template<typename T, size_t N>
class RTreeNode : public RTreeObjectBase<T, N>
{
    friend class RTree<T, N>;

private:
    std::vector<RTreeObjectBase<T, N>*> Entries;
    size_t Level;

    RTreeNode(size_t NodeLevel = 0) : RTreeObjectBase<T, N>(GID(RTreeNode), nullptr), Level(NodeLevel)
    {
    }

public:
    virtual ~RTreeNode()
    {
        if (Level != 0)
        {
            for (RTreeObjectBase<T, N>* Entry : Entries)
            {
                delete Entry;
            }
        }
    }

    virtual RTBoundingBox<T, N> GeRTBoundingBoxImpl() override
    {
        if (Entries.empty())
        {
            return RTBoundingBox<T, N>();
        }

        RTBoundingBox<T, N> BoundingBox = Entries[0]->GeRTBoundingBox();
        for (size_t i = 1; i < Entries.size(); ++i)
        {
            BoundingBox.ExpandToInclude(Entries[i]->GeRTBoundingBox());
        }
        return BoundingBox;
    }

    virtual size_t GetId() const override
    {
        return reinterpret_cast<size_t>(this);
    }

    size_t GetEntryCount() const
    {
        return Entries.size();
    }

    _FORCEINLINE bool IsOverflowing(size_t MaxEntries) const
    {
        return Entries.size() > MaxEntries;
    }

    _FORCEINLINE bool IsUnderflowing(size_t MinEntries) const
    {
        return Entries.size() < MinEntries;
    }

    void AddEntry(RTreeObjectBase<T, N>* Entry)
    {
        Entry->SetParent(this);
        Entries.push_back(Entry);
        this->SetDirty();
    }

    bool RemoveEntry(size_t Index)
    {
        if (Index >= Entries.size())
        {
            return false;
        }
        Entries.erase(Entries.begin() + Index);
        this->SetDirty();
        return true;
    }

    bool RemoveEntryByObject(RTreeObjectBase<T, N>* Object)
    {
        for (size_t i = 0; i < Entries.size(); ++i)
        {
            if (Entries[i] == Object || *Entries[i] == *Object)
            {
                Entries.erase(Entries.begin() + i);
                this->SetDirty();
                return true;
            }
        }
        return false;
    }

    bool RemoveEntryByLambda(std::function<bool(RTreeObjectBase<T, N>*)> Lambda)
    {
        for (size_t i = 0; i < Entries.size(); ++i)
        {
            if (Lambda(Entries[i]))
            {
                Entries.erase(Entries.begin() + i);
                this->SetDirty();
                return true;
            }
        }
        return false;
    }

    _FORCEINLINE const std::vector<RTreeObjectBase<T, N>*>& GetEntries() const
    {
        return Entries;
    }

    void Clear()
    {
        Entries.clear();
        this->SetDirty();
    }

    _FORCEINLINE size_t GetLevel() const
    {
        return Level;
    }
};

template<typename T, size_t N>
class RTree
{
public:
    enum class ESplitMethod
    {
        Quadratic,
        Linear,
        Exhaustive
    };

private:
    RTreeNode<T, N>* Root;
    size_t MaxEntries;
    size_t MinEntries;
    size_t NumObjects;
    ESplitMethod SplitMethod;

public:
    RTree(size_t MaxEntriesPerNode = 50, ESplitMethod Method = ESplitMethod::Linear) : Root(nullptr), MaxEntries(MaxEntriesPerNode), MinEntries(MaxEntriesPerNode / 2), NumObjects(0), SplitMethod(Method)
    {
        if (MaxEntries < 2)
        {
            throw std::invalid_argument("MaxEntries must be at least 2");
        }
    }

    ~RTree()
    {
        Clear();
    }

    void Clear()
    {
        if (Root)
        {
            delete Root;
            Root = nullptr;
        }
        NumObjects = 0;
    }

    size_t GetSize() const
    {
        return NumObjects;
    }

    bool IsEmpty() const
    {
        return NumObjects == 0;
    }

    bool Insert(RTreeObjectBase<T, N>* Object)
    {
        if (!Object)
        {
            throw std::invalid_argument("Cannot insert null object");
        }


        if (!Root)
        {
            Root = new RTreeNode<T, N>(0);
            Root->AddEntry(Object);
            NumObjects++;
            return true;
        }

        RTreeNode<T, N>* Leaf = ChooseLeaf(Root, Object->GeRTBoundingBox());

        Leaf->AddEntry(Object);
        NumObjects++;

        if (Leaf->IsOverflowing(MaxEntries))
        {
            auto Parent = Leaf->GetParent();
            RTreeNode<T, N>* NewLeaf = SplitNode(Leaf);
            AdjustTree(Leaf, NewLeaf);
        }
        else
        {
            AdjustTree(Leaf, nullptr);
        }

        return true;
    }

    bool Remove(RTreeObjectBase<T, N>* Object)
    {
        if (!Object || !Root)
        {
            return false;
        }


        RTreeObjectBase<T, N>* Element = FindLeaf(Root, Object);

        if (!Element)
        {
            return false;
        }

        RTreeNode<T, N>* Leaf = Element->GetParent();

        if (!Leaf)
        {
            return false;
        }

        if (!Leaf->RemoveEntryByObject(Object))
        {
            return false;
        }

        NumObjects--;

        CondenseTree(Leaf);

        if (Root && Root->GetEntryCount() == 1 && Root->GetLevel() != 0)
        {
            RTreeNode<T, N>* NewRoot = static_cast<RTreeNode<T, N>*>(Root->GetEntries()[0]);
            if (NewRoot)
            {
                Root->Clear();
                delete Root;
                Root = NewRoot;
                Root->SetParent(nullptr);
            }
        }
        else if (Root && Root->GetEntryCount() == 0 && Root->GetLevel() == 0)
        {
            delete Root;
            Root = nullptr;
        }

        return true;
    }

    bool RemoveByLambda(const RTBoundingBox<T, N>& BoundingBox, std::function<bool(RTreeObjectBase<T, N>*)> Lambda)
    {
        if (!Root)
        {
            return false;
        }


        RTreeObjectBase<T, N>* Element = FindLeafByLambda(Root, BoundingBox, Lambda);

        if (!Element)
        {
            return false;
        }

        RTreeNode<T, N>* Leaf = Element->GetParent();
        if (!Leaf)
        {
            return false;
        }

        if (!Leaf->RemoveEntryByObject(Element))
        {
            return false;
        }

        NumObjects--;

        CondenseTree(Leaf);

        if (Root && Root->GetEntryCount() == 1 && Root->GetLevel() != 0)
        {
            RTreeNode<T, N>* NewRoot = static_cast<RTreeNode<T, N>*>(Root->GetEntries()[0]);
            if (NewRoot)
            {
                Root->Clear();
                delete Root;
                Root = NewRoot;
                Root->SetParent(nullptr);
            }
        }
        else if (Root && Root->GetEntryCount() == 0 && Root->GetLevel() == 0)
        {
            delete Root;
            Root = nullptr;
        }

        return true;
    }

    template<typename Target>
    std::vector<Target*> QueryPointCast(const std::array<T, N>& Point) const
    {

        std::vector<Target*> Result;
        if (!Root)
        {
            return Result;
        }

        std::list<RTreeNode<T, N>*> Stack;
        Stack.push_back(Root);

        while (!Stack.empty())
        {
            RTreeNode<T, N>* Node = Stack.back();
            Stack.pop_back();

            for (auto it = Node->GetEntries().rbegin(); it != Node->GetEntries().rend(); ++it)
            {
                auto Ptr = (*it);

                if (Ptr->GeRTBoundingBox().Contains(Point))
                {
                    if (Node->GetLevel() != 0)
                    {
                        Stack.push_back(static_cast<RTreeNode<T, N>*>(Ptr));
                    }
                    else
                    {
                        Result.push_back(static_cast<Target*>(Ptr));
                    }
                }
            }
        }
        return Result;
    }

    std::vector<RTreeObjectBase<T, N>*> QueryPoint(const std::array<T, N>& Point) const
    {
        return QueryPointCast<RTreeObjectBase<T, N>>(Point);
    }

    template<typename Target>
    std::vector<Target*> QueryRangeCast(const RTBoundingBox<T, N>& Range) const
    {
        std::vector<Target*> Result;
        if (!Root)
        {
            return Result;
        }

        std::list<RTreeNode<T, N>*> Stack;
        Stack.push_back(Root);

        while (!Stack.empty())
        {
            RTreeNode<T, N>* Node = Stack.back();
            Stack.pop_back();

            for (auto it = Node->GetEntries().rbegin(); it != Node->GetEntries().rend(); ++it)
            {
                auto Ptr = (*it);

                if (Ptr->GeRTBoundingBox().Intersects(Range))
                {
                    if (Node->GetLevel() != 0)
                    {
                        Stack.push_back(static_cast<RTreeNode<T, N>*>(Ptr));
                    }
                    else
                    {
                        Result.push_back(static_cast<Target*>(Ptr));
                    }
                }
            }
        }
        return Result;
    }

    std::vector<RTreeObjectBase<T, N>*> QueryRange(const RTBoundingBox<T, N>& Range) const
    {
        return QueryRangeCast<RTreeObjectBase<T, N>>(Range);
    }

    std::vector<RTreeObjectBase<T, N>*> KNNSearch(const std::array<T, N>& Point, size_t K) const
    {
        std::vector<RTreeObjectBase<T, N>*> Result;
        if (!Root || K == 0)
        {
            return Result;
        }

        using DistNodePair = std::pair<double, RTreeNode<T, N>*>;
        auto NodeCompare = [](const DistNodePair& A, const DistNodePair& B) {
            return A.first > B.first; // min-heap: ближайшая нода сверху
        };

        using DistEntryPair = std::pair<double, RTreeObjectBase<T, N>*>;
        auto EntryCompare = [](const DistEntryPair& A, const DistEntryPair& B) {
            return A.first < B.first; // max-heap: самый дальний из найденных сверху
        };

        std::priority_queue<DistNodePair, std::vector<DistNodePair>, decltype(NodeCompare)> Queue(NodeCompare);
        std::priority_queue<DistEntryPair, std::vector<DistEntryPair>, decltype(EntryCompare)> Nearest(EntryCompare);

        Queue.push({0.0, Root});

        while (!Queue.empty())
        {
            auto [Dist, Node] = Queue.top();
            Queue.pop();

            // Если у нас уже есть K объектов, и ближайшая нода дальше самого
            // дальнего из них — дальше искать нет смысла.
            if (Nearest.size() == K && Dist > Nearest.top().first)
            {
                break;
            }

            if (Node->GetLevel() == 0)
            {
                for (RTreeObjectBase<T, N>* Entry : Node->GetEntries())
                {
                    double EntryDist = DistanceToBoundingBox(Point, Entry->GeRTBoundingBox());
                    if (Nearest.size() < K)
                    {
                        Nearest.push({EntryDist, Entry});
                    }
                    else if (EntryDist < Nearest.top().first)
                    {
                        Nearest.pop();
                        Nearest.push({EntryDist, Entry});
                    }
                }
            }
            else
            {
                for (RTreeObjectBase<T, N>* Entry : Node->GetEntries())
                {
                    RTreeNode<T, N>* Child = static_cast<RTreeNode<T, N>*>(Entry);
                    if (!Child)
                    {
                        continue;
                    }

                    double ChildDist = DistanceToBoundingBox(Point, Child->GeRTBoundingBox());

                    // Ветку отбрасываем только если у нас уже есть K результатов
                    // и эта ветка заведомо дальше самого дальнего из них.
                    if (Nearest.size() < K || ChildDist < Nearest.top().first)
                    {
                        Queue.push({ChildDist, Child});
                    }
                }
            }
        }

        // Nearest — max-heap, поэтому вынимаем в порядке убывания и разворачиваем,
        // чтобы вернуть от ближайшего к дальнему.
        Result.reserve(Nearest.size());
        while (!Nearest.empty())
        {
            Result.push_back(Nearest.top().second);
            Nearest.pop();
        }
        std::reverse(Result.begin(), Result.end());

        return Result;
    }

    bool Contains(RTreeObjectBase<T, N>* Object) const
    {

        if (!Object || !Root)
        {
            return false;
        }

        return FindLeaf(Root, Object) != nullptr;
    }

    RTreeNode<T, N>* GetRoot() const
    {
        return Root;
    }

    void SetSplitMethod(ESplitMethod Method)
    {
        SplitMethod = Method;
    }

    std::vector<RTreeObjectBase<T, N>*> GetAllObjects() const
    {

        std::vector<RTreeObjectBase<T, N>*> Objects;
        if (Root)
        {
            GetAllObjectsRecursive(Root, Objects);
        }
        return Objects;
    }

private:
    RTreeNode<T, N>* ChooseLeaf(RTreeNode<T, N>* Node, const RTBoundingBox<T, N>& BoundingBox)
    {
        if (Node->GetLevel() == 0)
        {
            return Node;
        }

        T MinExpansion = std::numeric_limits<T>::max();
        double MinDist = std::numeric_limits<double>::max();
        RTreeNode<T, N>* BestChild = nullptr;

        std::array<T, N> BoundingBoxCenter = BoundingBox.GetCenter();

        for (RTreeObjectBase<T, N>* Entry : Node->GetEntries())
        {
            RTreeNode<T, N>* Child = static_cast<RTreeNode<T, N>*>(Entry);
            if (!Child)
            {
                continue;
            }

            RTBoundingBox<T, N> ChildBoundingBox = Child->GeRTBoundingBox();

            auto ChildCenter = ChildBoundingBox.GetCenter();

            double Dist = 0.0;
            for (size_t Axis = 0; Axis < N; ++Axis)
            {
                double D = (BoundingBoxCenter[Axis] > ChildCenter[Axis]) ? BoundingBoxCenter[Axis] - ChildCenter[Axis] : ChildCenter[Axis] - BoundingBoxCenter[Axis];
                Dist += D * D;
            }

            if ((Dist < MinDist))
            {
                BestChild = Child;
                MinDist = Dist;
            }
        }

        if (!BestChild)
        {
            return Node;
        }

        if (BestChild->GetLevel() == 0)
        {
            return BestChild;
        }

        return ChooseLeaf(BestChild, BoundingBox);
    }

    RTreeNode<T, N>* SplitNode(RTreeNode<T, N>* Node)
    {
        switch (SplitMethod)
        {
            case ESplitMethod::Quadratic:
                return QuadraticSplit(Node);
            case ESplitMethod::Linear:
                return LinearSplit(Node);
            case ESplitMethod::Exhaustive:
                return ExhaustiveSplit(Node);
            default:
                return QuadraticSplit(Node);
        }
    }

    RTreeNode<T, N>* QuadraticSplit(RTreeNode<T, N>* Node)
    {
        const auto& Entries = Node->GetEntries();

        size_t Seed1 = 0, Seed2 = 0;
        T MaxWasteArea = -1;

        for (size_t i = 0; i < Entries.size(); ++i)
        {
            for (size_t j = i + 1; j < Entries.size(); ++j)
            {
                RTBoundingBox<T, N> Combined = Entries[i]->GeRTBoundingBox();
                Combined.ExpandToInclude(Entries[j]->GeRTBoundingBox());

                T WasteArea = Combined.GetVolume() - Entries[i]->GeRTBoundingBox().GetVolume() - Entries[j]->GeRTBoundingBox().GetVolume();

                if (WasteArea > MaxWasteArea)
                {
                    MaxWasteArea = WasteArea;
                    Seed1 = i;
                    Seed2 = j;
                }
            }
        }

        RTreeNode<T, N>* NewNode = new RTreeNode<T, N>(Node->GetLevel());

        RTreeObjectBase<T, N>* Seed1Entry = Entries[Seed1];
        RTreeObjectBase<T, N>* Seed2Entry = Entries[Seed2];

        NewNode->AddEntry(Seed2Entry);
        Seed2Entry->SetParent(NewNode);

        std::vector<RTreeObjectBase<T, N>*> RemainingEntries;
        for (size_t i = 0; i < Entries.size(); ++i)
        {
            if (i != Seed1 && i != Seed2)
            {
                RemainingEntries.push_back(Entries[i]);
            }
        }

        Node->Clear();
        Node->AddEntry(Seed1Entry);
        Seed1Entry->SetParent(Node);

        RTBoundingBox<T, N> BoundingBox1 = Seed1Entry->GeRTBoundingBox();
        RTBoundingBox<T, N> BoundingBox2 = Seed2Entry->GeRTBoundingBox();

        while (!RemainingEntries.empty())
        {
            if (Node->GetEntryCount() + RemainingEntries.size() <= MinEntries)
            {
                for (RTreeObjectBase<T, N>* Entry : RemainingEntries)
                {
                    Node->AddEntry(Entry);
                    BoundingBox1.ExpandToInclude(Entry->GeRTBoundingBox());
                }
                RemainingEntries.clear();
                break;
            }

            if (NewNode->GetEntryCount() + RemainingEntries.size() <= MinEntries)
            {
                for (RTreeObjectBase<T, N>* Entry : RemainingEntries)
                {
                    NewNode->AddEntry(Entry);
                    BoundingBox2.ExpandToInclude(Entry->GeRTBoundingBox());
                }
                RemainingEntries.clear();
                break;
            }

            size_t NextIndex = PickNext(RemainingEntries, BoundingBox1, BoundingBox2);
            RTreeObjectBase<T, N>* NextEntry = RemainingEntries[NextIndex];

            RemainingEntries.erase(RemainingEntries.begin() + NextIndex);

            T Expansion1 = BoundingBox1.GetExpansionVolume(NextEntry->GeRTBoundingBox());
            T Expansion2 = BoundingBox2.GetExpansionVolume(NextEntry->GeRTBoundingBox());

            if (Expansion1 < Expansion2 || (Expansion1 == Expansion2 && Node->GetEntryCount() < NewNode->GetEntryCount()))
            {
                Node->AddEntry(NextEntry);
                BoundingBox1.ExpandToInclude(NextEntry->GeRTBoundingBox());
            }
            else
            {
                NewNode->AddEntry(NextEntry);
                BoundingBox2.ExpandToInclude(NextEntry->GeRTBoundingBox());
            }
        }

        return NewNode;
    }

    RTreeNode<T, N>* LinearSplit(RTreeNode<T, N>* Node)
    {
        const auto& Entries = Node->GetEntries();


        auto BoundingBox = Node->GeRTBoundingBox();
        auto MaxSpread = BoundingBox.Max[0] - BoundingBox.Min[0];
        auto SplitAxis = 0;
        for (size_t Axis = 1; Axis < N; ++Axis)
        {
            auto Spread = BoundingBox.Max[0] - BoundingBox.Min[0];
            if (Spread > MaxSpread)
            {
                MaxSpread = Spread;
                SplitAxis = Axis;
            }
        }

        std::vector<RTreeObjectBase<T, N>*> SortedEntries = Entries;
        std::sort(SortedEntries.begin(), SortedEntries.end(), [SplitAxis](RTreeObjectBase<T, N>* A, RTreeObjectBase<T, N>* B) {
            T CenterA = A->GeRTBoundingBox().Min[SplitAxis] / static_cast<T>(2) + A->GeRTBoundingBox().Max[SplitAxis] / static_cast<T>(2);
            T CenterB = B->GeRTBoundingBox().Min[SplitAxis] / static_cast<T>(2) + B->GeRTBoundingBox().Max[SplitAxis] / static_cast<T>(2);
            return CenterA < CenterB;
        });

        const size_t SplitIndex = SortedEntries.size() / 2;

        RTreeNode<T, N>* NewNode = new RTreeNode<T, N>(Node->GetLevel());
        Node->Clear();

        for (size_t i = 0; i < SplitIndex; ++i)
        {
            Node->AddEntry(SortedEntries[i]);
        }

        for (size_t i = SplitIndex; i < SortedEntries.size(); ++i)
        {
            NewNode->AddEntry(SortedEntries[i]);
        }

        return NewNode;
    }

    RTreeNode<T, N>* ExhaustiveSplit(RTreeNode<T, N>* Node)
    {
        return QuadraticSplit(Node);
    }

    size_t PickNext(const std::vector<RTreeObjectBase<T, N>*>& Entries, const RTBoundingBox<T, N>& BoundingBox1, const RTBoundingBox<T, N>& BoundingBox2)
    {
        size_t BestIndex = 0;
        T MaxDifference = -1;

        for (size_t i = 0; i < Entries.size(); ++i)
        {
            T Expansion1 = BoundingBox1.GetExpansionVolume(Entries[i]->GeRTBoundingBox());
            T Expansion2 = BoundingBox2.GetExpansionVolume(Entries[i]->GeRTBoundingBox());

            T Difference = (Expansion1 > Expansion2) ? (Expansion1 - Expansion2) : (Expansion2 - Expansion1);
            if (Difference > MaxDifference)
            {
                MaxDifference = Difference;
                BestIndex = i;
            }
        }

        return BestIndex;
    }

    void AdjustTree(RTreeNode<T, N>* Node, RTreeNode<T, N>* NewNode)
    {
        if (Node == Root && !Node->GetParent())
        {
            if (NewNode)
            {
                RTreeNode<T, N>* NewRoot = new RTreeNode<T, N>(Node->GetLevel() + 1);

                NewRoot->AddEntry(Node);
                NewRoot->AddEntry(NewNode);

                Node->SetParent(NewRoot);
                NewNode->SetParent(NewRoot);
                Root = NewRoot;
            }
            return;
        }

        RTreeNode<T, N>* Parent = Node->GetParent();
        if (!Parent)
        {
            return;
        }

        if (NewNode)
        {
            Parent->AddEntry(NewNode);
            NewNode->SetParent(Parent);
        }

        if (Parent->IsOverflowing(MaxEntries))
        {
            RTreeNode<T, N>* NewParent = SplitNode(Parent);
            AdjustTree(Parent, NewParent);
        }
        else
        {
            AdjustTree(Parent, nullptr);
        }
    }

    RTreeObjectBase<T, N>* FindLeaf(RTreeNode<T, N>* Node, RTreeObjectBase<T, N>* Object) const
    {
        if (Node->GetLevel() == 0)
        {
            for (RTreeObjectBase<T, N>* Entry : Node->GetEntries())
            {
                if (Entry == Object || *Entry == *Object)
                {
                    return Entry;
                }
            }
            return nullptr;
        }

        RTBoundingBox<T, N> ObjecRTBoundingBox = Object->GeRTBoundingBox();
        for (RTreeObjectBase<T, N>* Entry : Node->GetEntries())
        {
            RTreeNode<T, N>* Child = static_cast<RTreeNode<T, N>*>(Entry);
            if (!Child)
            {
                continue;
            }

            if (Child->GeRTBoundingBox().Intersects(ObjecRTBoundingBox) || Child->GeRTBoundingBox().Contains(ObjecRTBoundingBox))
            {
                RTreeObjectBase<T, N>* Result = FindLeaf(Child, Object);
                if (Result)
                {
                    return Result;
                }
            }
        }

        return nullptr;
    }

    RTreeObjectBase<T, N>* FindLeafByLambda(RTreeNode<T, N>* Node, const RTBoundingBox<T, N>& BoundingBox, const std::function<bool(RTreeObjectBase<T, N>*)>& Lambda) const
    {
        if (Node->GetLevel() == 0)
        {
            for (RTreeObjectBase<T, N>* Entry : Node->GetEntries())
            {
                if (Lambda(Entry))
                {
                    return Entry;
                }
            }
            return nullptr;
        }

        for (RTreeObjectBase<T, N>* Entry : Node->GetEntries())
        {
            RTreeNode<T, N>* Child = static_cast<RTreeNode<T, N>*>(Entry);
            if (!Child)
            {
                continue;
            }

            if (Child->GeRTBoundingBox().Intersects(BoundingBox) || Child->GeRTBoundingBox().Contains(BoundingBox))
            {
                RTreeObjectBase<T, N>* Result = FindLeafByLambda(Child, BoundingBox, Lambda);
                if (Result)
                {
                    return Result;
                }
            }
        }

        return nullptr;
    }

    void CondenseTree(RTreeNode<T, N>* Node)
    {
        std::vector<RTreeNode<T, N>*> ReinsertNodes;
        RTreeNode<T, N>* Current = Node;

        while (Current != Root)
        {
            RTreeNode<T, N>* Parent = Current->GetParent();
            if (!Parent)
            {
                break;
            }

            if (Current->IsUnderflowing(MinEntries))
            {
                Parent->RemoveEntryByObject(Current);
                ReinsertNodes.push_back(Current);
            }

            Current = Parent;
        }

        for (RTreeNode<T, N>* NodeToReinsert : ReinsertNodes)
        {
            std::vector<RTreeObjectBase<T, N>*> ObjectsToReinsert = CollectObjectsFromNode(NodeToReinsert);
            for (RTreeObjectBase<T, N>* Obj : ObjectsToReinsert)
            {
                Insert(Obj);
            }
            NodeToReinsert->Clear();
            delete NodeToReinsert;
        }
    }

    std::vector<RTreeObjectBase<T, N>*> CollectObjectsFromNode(RTreeNode<T, N>* Node) const
    {
        std::vector<RTreeObjectBase<T, N>*> Objects;

        if (Node->GetLevel() == 0)
        {
            for (RTreeObjectBase<T, N>* Entry : Node->GetEntries())
            {
                Objects.push_back(Entry);
            }
        }
        else
        {
            for (RTreeObjectBase<T, N>* Entry : Node->GetEntries())
            {
                RTreeNode<T, N>* Child = static_cast<RTreeNode<T, N>*>(Entry);
                if (Child)
                {
                    auto ChildObjects = CollectObjectsFromNode(Child);
                    Objects.insert(Objects.end(), ChildObjects.begin(), ChildObjects.end());
                }
            }
        }

        return Objects;
    }

    double DistanceToBoundingBox(const std::array<T, N>& Point, const RTBoundingBox<T, N>& BoundingBox) const
    {
        double Distance = 0.0;
        for (size_t i = 0; i < N; ++i)
        {
            if (Point[i] < BoundingBox.Min[i])
            {
                double Diff = static_cast<double>(BoundingBox.Min[i] - Point[i]);
                Distance += Diff * Diff;
            }
            else if (Point[i] > BoundingBox.Max[i])
            {
                double Diff = static_cast<double>(Point[i] - BoundingBox.Max[i]);
                Distance += Diff * Diff;
            }
        }
        return std::sqrt(Distance);
    }

    void GetAllObjectsRecursive(RTreeNode<T, N>* Node, std::vector<RTreeObjectBase<T, N>*>& Objects) const
    {
        if (Node->GetLevel() == 0)
        {
            for (RTreeObjectBase<T, N>* Entry : Node->GetEntries())
            {
                Objects.push_back(Entry);
            }
        }
        else
        {
            for (RTreeObjectBase<T, N>* Entry : Node->GetEntries())
            {
                RTreeNode<T, N>* Child = static_cast<RTreeNode<T, N>*>(Entry);
                if (Child)
                {
                    GetAllObjectsRecursive(Child, Objects);
                }
            }
        }
    }
};

template<typename Type, typename T, size_t N>
class TypedRTree
{
public:
    class Wrapper : public RTreeObjectBase<T, N>
    {
        RTBoundingBox<T, N> BoundingBox;
        size_t Id;
        Type Data;

    public:
        Wrapper(const Type& InData, const RTBoundingBox<T, N>& InBoundingBox)
            : RTreeObjectBase<T, N>(GID(Wrapper))
            , BoundingBox(InBoundingBox)
            , Id(0)
            , Data(InData)
        {
            Id = data_core::GETID(&this->Data, sizeof(this->Data));
        }

        Wrapper(const Type& InData, const RTBoundingBox<T, N>& InBoundingBox, size_t InId)
            : BoundingBox(InBoundingBox)
            , Id(InId)
            , Data(InData)
        {
        }

        virtual RTBoundingBox<T, N> GeRTBoundingBoxImpl() override
        {
            return BoundingBox;
        }

        virtual size_t GetId() const override
        {
            return Id;
        }

        const Type& GetData() const { return Data; }
        Type&       GetData()       { return Data; }

        const RTBoundingBox<T, N>& GetBoundingBox() const { return BoundingBox; }

        void SetBoundingBox(const RTBoundingBox<T, N>& InBoundingBox)
        {
            BoundingBox = InBoundingBox;
            this->SetDirty();
        }

        void SetData(const Type& InData)
        {
            Data = InData;
            this->SetDirty();
        }
    };

private:
    RTree<T, N> imp;
    std::unordered_map<size_t, Wrapper*> IdMap;

    Wrapper* FindWrapperByData(const Type& InData) const
    {
        const size_t InId = data_core::GETID(const_cast<Type*>(&InData), sizeof(Type));
        auto It = IdMap.find(InId);
        return It != IdMap.end() ? It->second : nullptr;
    }

public:
    explicit TypedRTree(size_t MaxEntriesPerNode = 50,
                        typename RTree<T, N>::ESplitMethod Method = RTree<T, N>::ESplitMethod::Linear)
        : imp(MaxEntriesPerNode, Method)
    {
    }

    ~TypedRTree()
    {
        Clear();
    }

    TypedRTree(const TypedRTree&) = delete;
    TypedRTree& operator=(const TypedRTree&) = delete;

    // ------------------------------------------------------------------
    // Общие
    // ------------------------------------------------------------------
    void Clear()
    {
        auto Objects = imp.GetAllObjects();
        for (RTreeObjectBase<T, N>* Obj : Objects)
        {
            delete static_cast<Wrapper*>(Obj);
        }
        IdMap.clear();
        imp.Clear();
    }

    size_t GetSize() const { return imp.GetSize(); }
    bool   IsEmpty() const { return imp.IsEmpty(); }

    void SetSplitMethod(typename RTree<T, N>::ESplitMethod Method)
    {
        imp.SetSplitMethod(Method);
    }

    RTreeNode<T, N>* GetRoot() const { return imp.GetRoot(); }

    void SetBoundingBox(const Type& Data, const RTBoundingBox<T, N>& InBoundingBox)
    {
        Wrapper* wrapper = FindWrapperByData(Data);
        if(wrapper)
        {
            wrapper->BoundingBox = InBoundingBox;
            wrapper->SetDirty();
        }
    }

    // ------------------------------------------------------------------
    // Insert
    // ------------------------------------------------------------------
    bool Insert(const Type& InData, const RTBoundingBox<T, N>& InBoundingBox)
    {
        Wrapper* W = new Wrapper(InData, InBoundingBox);
        if (!imp.Insert(W))
        {
            delete W;
            return false;
        }
        IdMap[W->GetId()] = W;
        return true;
    }

    // Вставка с явным ID (переопределяет автоматически сгенерированный)
    bool Insert(const Type& InData, const RTBoundingBox<T, N>& InBoundingBox, size_t InId)
    {
        Wrapper* W = new Wrapper(InData, InBoundingBox, InId);
        if (!imp.Insert(W))
        {
            delete W;
            return false;
        }
        IdMap[InId] = W;
        return true;
    }

    // ------------------------------------------------------------------
    // Remove
    // ------------------------------------------------------------------
    bool Remove(const Type& InData)
    {
        Wrapper* W = FindWrapperByData(InData);
        if (!W)
        {
            return false;
        }
        return RemoveById(W->GetId());
    }

    bool RemoveById(size_t InId)
    {
        auto It = IdMap.find(InId);
        if (It == IdMap.end())
        {
            return false;
        }

        Wrapper* W = It->second;
        if (!imp.Remove(W))
        {
            return false;
        }

        IdMap.erase(It);
        delete W;
        return true;
    }

    // Лямбда принимает сам Type и возвращает true, если объект нужно удалить
    bool RemoveByLambda(const RTBoundingBox<T, N>& InBoundingBox,
                        std::function<bool(const Type&)> Lambda)
    {
        Wrapper* Found = nullptr;

        const bool Removed = imp.RemoveByLambda(
            InBoundingBox,
            [&](RTreeObjectBase<T, N>* Obj) -> bool
            {
                Wrapper* W = static_cast<Wrapper*>(Obj);
                if (Lambda(W->GetData()))
                {
                    Found = W;
                    return true;
                }
                return false;
            });

        if (Removed && Found)
        {
            IdMap.erase(Found->GetId());
            delete Found;
        }
        return Removed;
    }

    // ------------------------------------------------------------------
    // Queries
    // ------------------------------------------------------------------
    std::vector<Type> QueryPoint(const std::array<T, N>& Point) const
    {
        auto Wrappers = imp.QueryPoint(Point);
        std::vector<Type> Result;
        Result.reserve(Wrappers.size());
        for (RTreeObjectBase<T, N>* Obj : Wrappers)
        {
            Result.push_back(static_cast<Wrapper*>(Obj)->GetData());
        }
        return Result;
    }

    std::vector<Wrapper*> QueryPointWrappers(const std::array<T, N>& Point) const
    {
        auto Wrappers = imp.QueryPoint(Point);
        std::vector<Wrapper*> Result;
        Result.reserve(Wrappers.size());
        for (RTreeObjectBase<T, N>* Obj : Wrappers)
        {
            Result.push_back(static_cast<Wrapper*>(Obj));
        }
        return Result;
    }

    std::vector<Type> QueryRange(const RTBoundingBox<T, N>& Range) const
    {
        auto Wrappers = imp.QueryRange(Range);
        std::vector<Type> Result;
        Result.reserve(Wrappers.size());
        for (RTreeObjectBase<T, N>* Obj : Wrappers)
        {
            Result.push_back(static_cast<Wrapper*>(Obj)->GetData());
        }
        return Result;
    }

    std::vector<Wrapper*> QueryRangeWrappers(const RTBoundingBox<T, N>& Range) const
    {
        auto Wrappers = imp.QueryRange(Range);
        std::vector<Wrapper*> Result;
        Result.reserve(Wrappers.size());
        for (RTreeObjectBase<T, N>* Obj : Wrappers)
        {
            Result.push_back(static_cast<Wrapper*>(Obj));
        }
        return Result;
    }

    std::vector<Type> KNNSearch(const std::array<T, N>& Point, size_t K) const
    {
        auto Wrappers = imp.KNNSearch(Point, K);
        std::vector<Type> Result;
        Result.reserve(Wrappers.size());
        for (RTreeObjectBase<T, N>* Obj : Wrappers)
        {
            Result.push_back(static_cast<Wrapper*>(Obj)->GetData());
        }
        return Result;
    }

    std::vector<Wrapper*> KNNSearchWrappers(const std::array<T, N>& Point, size_t K) const
    {
        auto Wrappers = imp.KNNSearch(Point, K);
        std::vector<Wrapper*> Result;
        Result.reserve(Wrappers.size());
        for (RTreeObjectBase<T, N>* Obj : Wrappers)
        {
            Result.push_back(static_cast<Wrapper*>(Obj));
        }
        return Result;
    }

    // ------------------------------------------------------------------
    // ID lookups
    // ------------------------------------------------------------------
    Wrapper* FindById(size_t InId) const
    {
        auto It = IdMap.find(InId);
        return It != IdMap.end() ? It->second : nullptr;
    }

    bool ContainsById(size_t InId) const
    {
        return IdMap.find(InId) != IdMap.end();
    }

    bool Contains(const Type& InData) const
    {
        return FindWrapperByData(InData) != nullptr;
    }

    // ------------------------------------------------------------------
    // Все объекты
    // ------------------------------------------------------------------
    std::vector<Type> GetAllObjects() const
    {
        auto Wrappers = imp.GetAllObjects();
        std::vector<Type> Result;
        Result.reserve(Wrappers.size());
        for (RTreeObjectBase<T, N>* Obj : Wrappers)
        {
            Result.push_back(static_cast<Wrapper*>(Obj)->GetData());
        }
        return Result;
    }

    std::vector<Wrapper*> GetAllWrappers() const
    {
        auto Wrappers = imp.GetAllObjects();
        std::vector<Wrapper*> Result;
        Result.reserve(Wrappers.size());
        for (RTreeObjectBase<T, N>* Obj : Wrappers)
        {
            Result.push_back(static_cast<Wrapper*>(Obj));
        }
        return Result;
    }
};