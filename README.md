# GenericMinimapSystem

A simple system to manage minimap development and gameplay.

Is still under development, may update README later.

建议你采用 **C++ 管理瓦片和加载，UMG 管理布局与交互**。第一版使用少量复用的 `UImage`，等功能和资源管理稳定后，再考虑自定义 Slate 绘制。

下面按“可以逐项实现”的顺序列出大纲。假设地图是静态底图、正方形、支持平移缩放，瓦片已经离线导入成 `UTexture2D` 资产。代码是设计骨架，不是可以直接编译的完整类；具体 API 重载以你的 UE 版本为准。

**1. 先确定资源管理方式**

初版建议：

- 每块纹理内容尺寸为 `512 × 512`。
- 使用多分辨率金字塔。
- 瓦片关闭 Virtual Texture Streaming。
- 瓦片设为 `Never Stream`，由你控制整块资产的加载与淘汰。
- 保留适当的 Mip 和平台纹理压缩。

这里的 `Never Stream` **不代表启动时加载所有瓦片**，也不代表永远不能释放。它控制的是：**这块纹理加载后，不再让普通纹理流送系统独立调整其 Mip 驻留。**

你只需要先掌握一个生命周期：

```text
软引用记录路径
    ↓
异步加载一块纹理
    ↓
持有纹理，显示或缓存
    ↓
清除显示引用和缓存引用，释放加载句柄
    ↓
由 UE 在后续 GC 和渲染资源释放过程中回收
```

这样能避免第一版同时处理“资产加载”和“Mip 流送”两套机制。

**2. 拆成三个部分**

| 部分 | 建议类型 | 职责 |
|---|---|---|
| 地图配置 | `UWorldMapDefinition : UDataAsset` | 世界范围、层数、瓦片软引用 |
| 瓦片缓存 | `UWorldMapTileCache : UObject` | 加载队列、加载状态、缓存与淘汰 |
| 地图控件 | `UWorldMapWidget : UUserWidget` | 坐标转换、交互、可见范围、显示 |

第一版让 Widget 用 `UPROPERTY` 持有 Cache 即可。需要让多个地图控件共享缓存时，再移到 Subsystem。

不要让每个瓦片 `Image` 各自决定加载和卸载；集中管理才方便去重、限流和统计。

**3. 定义瓦片 ID 与资源索引**

约定第 `Level` 层有 `2^Level × 2^Level` 块：

```cpp
USTRUCT()
struct FMapTileId
{
    GENERATED_BODY()

    UPROPERTY()
    int32 Level = 0;

    UPROPERTY()
    int32 X = 0;

    UPROPERTY()
    int32 Y = 0;

    bool operator==(const FMapTileId& Other) const
    {
        return Level == Other.Level &&
               X == Other.X &&
               Y == Other.Y;
    }

    friend uint32 GetTypeHash(const FMapTileId& Id)
    {
        return HashCombine(
            HashCombine(::GetTypeHash(Id.Level), ::GetTypeHash(Id.X)),
            ::GetTypeHash(Id.Y));
    }
};
```

配置资产里保存：

```cpp
UPROPERTY(EditAnywhere)
TMap<FMapTileId, TSoftObjectPtr<UTexture2D>> Tiles;
```

`TSoftObjectPtr` 保存资产路径，**不会因为加载配置资产就把所有纹理一起加载，也不会阻止纹理被 GC**。这是按需加载的基础。[Epic：异步资产加载](https://dev.epicgames.com/documentation/unreal-engine/asynchronous-asset-loading-in-unreal-engine)

如果缓存支持多张地图，ID 还需要包含地图标识，或者切换地图时彻底清空旧缓存。

**4. 先实现坐标转换，再实现加载**

统一采用地图 UV：左上角 `(0,0)`，右下角 `(1,1)`。

控件保存：

```cpp
FVector2D CenterUV;      // 视口中心对应的地图 UV
double MapLocalSize;    // 整张地图在当前缩放下的边长，单位为控件局部单位
FVector2D ViewSize;      // 控件局部尺寸
```

转换关系：

```cpp
LocalPos = ViewSize * 0.5 + (MapUV - CenterUV) * MapLocalSize;

MapUV = CenterUV + (LocalPos - ViewSize * 0.5) / MapLocalSize;
```

先编写这些函数：

```cpp
FVector2D WorldToMapUV(FVector WorldPosition) const;
FVector2D MapUVToLocal(FVector2D UV) const;
FVector2D LocalToMapUV(FVector2D LocalPosition) const;

void PanMap(FVector2D LocalDelta);
void ZoomAt(FVector2D LocalMousePosition, double ZoomFactor);
```

拖动时：

```cpp
CenterUV -= LocalDelta / MapLocalSize;
```

以鼠标位置缩放时：

```cpp
const FVector2D AnchorUV = LocalToMapUV(MouseLocal);

MapLocalSize = ClampZoom(MapLocalSize * ZoomFactor);

CenterUV = AnchorUV - (MouseLocal - ViewSize * 0.5) / MapLocalSize;
```

这样缩放前后，鼠标指向的地图位置保持不动。随后再限制地图边界。

输入可从 `NativeOnMouseButtonDown`、`NativeOnMouseMove`、`NativeOnMouseButtonUp`、`NativeOnMouseWheel` 接入；用 `FGeometry::AbsoluteToLocal` 将鼠标屏幕位置转换到控件局部坐标，避免混用 DPI 缩放前后的单位。

**5. 计算显示层级与可见瓦片**

假设整张地图当前如果完整画出来，边长为 `MapScreenPixels` 个实际屏幕像素；每块内容有 `TilePixels` 个纹素。

初始层级选择可以用：

```cpp
Level = Clamp(
    CeilToInt(Log2(MapScreenPixels / TilePixels)),
    0,
    MaxLevel);
```

它倾向于保证纹理分辨率不低于显示需求，随后再添加切换滞回，避免边界附近反复换层。

计算可见范围：

```text
1. 将控件左上角、右下角转换为地图 UV。
2. 与地图范围 [0,1] 求交；没有交集时返回空集合。
3. N = 2^Level。
4. XMin = floor(UVMin.X × N)
   XMax = ceil (UVMax.X × N) - 1
5. Y 同理。
6. 将瓦片索引限制到 [0,N-1]。
```

需要维护三种需求：

| 集合 | 用途 |
|---|---|
| VisibleTiles | 当前应显示的目标层级瓦片 |
| PrefetchTiles | 周围一圈预加载 |
| FallbackTiles | 高清未到时实际使用的低级瓦片 |

这些集合由控件计算，再整体提交给 Cache。**不要每次刷新先卸载全部瓦片再重新请求**，应对新旧集合做差集。

**6. 给缓存建立明确状态**

建议状态：

```text
Unloaded → Queued → Loading → Ready
                         └→ Failed
Ready → 被淘汰 → Unloaded
```

每个运行时记录至少保存：

```cpp
struct FTileRuntime
{
    ETileState State;
    TSharedPtr<FStreamableHandle> Handle;

    uint64 RequestSerial;   // 区分同一瓦片的新旧加载请求
    uint64 LastUsedFrame;   // LRU
    uint64 EstimatedBytes;

    // 当前是否可见、作为回退使用、属于预加载区域等
};
```

纹理对象单独保存在 Cache 的反射属性中：

```cpp
UPROPERTY(Transient)
TMap<FMapTileId, TObjectPtr<UTexture2D>> ResidentTextures;
```

这个 `UPROPERTY` 容器是你的**明确强引用持有者**。普通 C++ 结构体里单独放一个裸指针，不能作为可靠的 GC 保活方式。

建议实现的接口：

```cpp
void UpdateDemand(/* 可见、预加载、回退集合 */);
void PumpLoadQueue();
void StartLoad(FMapTileId Id);
void OnLoadComplete(FMapTileId Id, uint64 RequestSerial);
void EvictTile(FMapTileId Id);
void TrimCache();
void Shutdown();
```

**7. 异步加载的核心写法**

通过全局 Asset Manager 获取 Streamable Manager：

```cpp
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/Texture2D.h"
```

下面省略了队列和统计细节：

```cpp
void UWorldMapTileCache::StartLoad(FMapTileId Id)
{
    const TSoftObjectPtr<UTexture2D>* Soft = Definition->Tiles.Find(Id);
    if (!Soft || Soft->IsNull())
    {
        MarkFailed(Id);
        return;
    }

    FTileRuntime& Entry = Entries.FindOrAdd(Id);

    if (Entry.State == ETileState::Loading ||
        Entry.State == ETileState::Ready)
    {
        return;
    }

    const uint64 Serial = ++NextRequestSerial;

    Entry.State = ETileState::Loading;
    Entry.RequestSerial = Serial;

    Entry.Handle =
        UAssetManager::GetStreamableManager().RequestAsyncLoad(
            Soft->ToSoftObjectPath(),
            FStreamableDelegate::CreateUObject(
                this,
                &UWorldMapTileCache::OnLoadComplete,
                Id,
                Serial));

    if (!Entry.Handle.IsValid())
    {
        MarkFailed(Id);
    }
}
```

加载完成：

```cpp
void UWorldMapTileCache::OnLoadComplete(
    FMapTileId Id, uint64 Serial)
{
    FTileRuntime* Entry = Entries.Find(Id);

    if (!Entry || Entry->RequestSerial != Serial)
    {
        return; // 已取消、已替换或属于旧请求
    }

    const TSoftObjectPtr<UTexture2D>* Soft = Definition->Tiles.Find(Id);
    UTexture2D* Texture = Soft ? Soft->Get() : nullptr;

    // 在这里重新检查：
    // 地图是否关闭？该瓦片是否仍值得显示或缓存？
    if (!Texture || !ShouldRetain(Id))
    {
        DiscardCompletedRequest(Id);
        return;
    }

    // 先建立强引用，再释放加载句柄。
    ResidentTextures.Add(Id, Texture);
    Entry->State = ETileState::Ready;

    if (Entry->Handle.IsValid())
    {
        Entry->Handle->ReleaseHandle();
        Entry->Handle.Reset();
    }

    NotifyTileReady(Id);
    // 标记显示需要刷新，并在调度阶段继续队列和检查预算。
}
```

这里采用的所有权规则很简单：

- **加载期间**：Handle 保活。
- **加载完成后**：`ResidentTextures` 保活。
- **淘汰时**：清理所有显示引用，再移除 `ResidentTextures` 条目。

活动的 `FStreamableHandle` 会保持资产存活；`ReleaseHandle()` 放弃这种持有关系。`CancelHandle()` 用于取消不再需要的请求，但不要将其理解为底层所有读盘工作都会立即停止。[Epic：FStreamableHandle](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Engine/FStreamableHandle?lang=en-US)

另外，**不要在回调里捕获 `TMap` 元素的引用，也不要捕获一个随后可能被复用的 Image**。回调携带瓦片 ID，完成后重新查找状态并刷新显示即可。

**8. 添加加载队列和预算**

初始参数可以用以下值进行实测调优：

| 参数 | 初始值 |
|---|---:|
| 同时加载数量 | 4 |
| 预加载范围 | 外围 1 圈 |
| 缓存预算 | 例如 64 MiB，按平台调整 |
| 常驻回退 | 最低层全图 |

加载顺序建议：

```text
缺失的必要回退 → 可见瓦片（中心优先）→ 外围预加载
```

每次启动请求前，检查它是否仍在需求集合中；过期的排队请求直接删除。

预算要计算：

```text
缓存中保留的纹理
+ 正在加载请求的预计占用
+ 切层期间额外保留的回退纹理
```

纹理加载后，可以用 `CalcTextureMemorySizeEnum(...)` 按平台构建数据估算纹理字节数，例如选择全部 Mip 的计数模式。这是资源预算依据，不是完整的进程显存测量。[Epic：纹理内存计算 API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/Engine/UTexture/CalcTextureMemorySizeEnum)

淘汰顺序为：不再需要的缓存中，最久未使用的优先。当前显示和实际用于回退的纹理不能淘汰。如果这些必要资源本身已经超预算，应降低层级或减少预加载，不能不断卸载又重新加载同一批资源。

**9. UMG 显示和父级回退**

原型布局：

```text
WorldMapWidget
└─ 设置 ClipToBounds 的容器
   └─ CanvasPanel
      ├─ 瓦片 Image 池
      └─ 图标与文字层
```

只为当前需要显示的瓦片分配或复用 Image。

普通完整瓦片可以：

```cpp
Image->SetBrushFromTexture(Texture, false);
```

`false` 表示不使用纹理尺寸自动改变 Brush 尺寸；瓦片布局交给 Canvas Slot 的 `SetPosition` 和 `SetSize`。[Epic：SetBrushFromTexture](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/UMG/Components/UImage/SetBrushFromTexture?application_version=5.5)

高清瓦片未到时，逐级查找已经加载的祖先：

```cpp
Parent.Level = Child.Level - 1;
Parent.X = Child.X / 2;
Parent.Y = Child.Y / 2;
```

对于直接父级，子瓦片在父级纹理中的 UV 为：

```cpp
UVMin = FVector2D(Child.X % 2, Child.Y % 2) * 0.5;
UVMax = UVMin + FVector2D(0.5, 0.5);
```

通过 `FSlateBrush::SetUVRegion()` 选择父级对应区域，再将这个区域拉伸到子瓦片应占的矩形。带边缘扩展像素时，还需要将这个 UV 映射到纹理内容区域。[Epic：FSlateBrush](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/SlateCore/FSlateBrush)

复用 Image 时要重置完整 Brush，包括 UV 区域，避免上一个瓦片的裁剪设置遗留。

**10. 释放资源要显式处理**

淘汰一块瓦片时：

```text
1. 确认它没有被可见瓦片或父级回退使用。
2. 清空对应 Image 的 Brush；清理其他材质、Brush 中的引用。
3. 加载中的请求执行 CancelHandle；完成的句柄执行 ReleaseHandle。
4. Handle.Reset()。
5. 从 ResidentTextures 中移除纹理。
6. 删除或重置运行时记录。
```

Image 放回对象池之前，也要清空 Brush。仅设置 `Collapsed` 仍可能保留纹理引用。

地图关闭时显式调用一个可重复执行的 `CloseMap()`：

```text
停止提交需求 → 清空显示引用 → 取消请求 → 清空缓存
```

再让 `NativeDestruct()` 做兜底清理。仅隐藏控件不会必然触发析构流程。

**释放引用不等于显存当帧下降。** 给预算留出余量，不要在拖动过程中反复调用 `CollectGarbage()`，也不要对这些资产手动 `delete` 或强行释放底层渲染资源。

**11. 按以下顺序完成和验收**

| 阶段 | 实现目标 |
|---|---|
| 第一阶段 | 固定层级，先用几张纹理验证坐标、拖动、裁剪和接缝 |
| 第二阶段 | 接入软引用、异步加载和 Image 复用 |
| 第三阶段 | 添加并发限制、LRU、关闭地图时清理 |
| 第四阶段 | 添加层级切换、父级回退和外围预加载 |
| 第五阶段 | 打包验证缺失资源、快速拖动、反复开关地图和内存趋势 |

瓦片必须被 Cook 包含；特别是运行时拼接资产路径时，不能只依赖编辑器中能加载。可以通过 Asset Manager／`PrimaryAssetLabel` 配置明确的 Cook 规则；**打包包含资产并不等于运行时把它们全部加载**。[Epic：Cooking 与资源规则](https://dev.epicgames.com/documentation/en-us/unreal-engine/cooking-and-chunking?application_version=4.27)

建议做一个简单调试文本，显示当前层级、可见数量、排队数量、加载数量和缓存估算字节数。你会很容易分辨问题究竟来自坐标计算、加载速度，还是资源没有释放。