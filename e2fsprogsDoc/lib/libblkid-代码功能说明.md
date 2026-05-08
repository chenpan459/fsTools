# lib/blkid 代码功能说明（e2fsprogs 1.46.5）

本文归纳 **`e2fsprogs-1.46.5/lib/blkid/`**：**块设备内容识别库 `libblkid`**（历史上与 **Andreas Dilger / Theodore Ts’o** 一系的 “libblkid 1.0” API 同源），用于通过读磁盘上的 **magic / superblock** 推断 **文件系统或数据卷类型**，并把 **LABEL、UUID、TYPE** 等以 **标签（tag）** 形式挂到 **设备节点** 上；可选地与磁盘上的 **缓存文件** 同步，避免每次全量探测。

**路径**：`../e2fsprogs-1.46.5/lib/blkid/`  

**说明**：发行版中常见的 **`/sbin/blkid`（util-linux）** 是另一条演进线；**本树**内同时提供 **`misc/blkid.c`** 命令行封装，并在本目录 **`Makefile.in`** 里链接 **`libblkid.a`** 生成 **`blkid`** 可执行文件。阅读 API 与行为时以 **`blkid.h.in`**、**`libblkid.3.in`** 与本目录实现为准。

---

## 1. 概念模型

| 概念 | 说明 |
|------|------|
| **`blkid_cache`** | 内存中的 **设备表 + 按标签类型组织的索引**；可关联 **`/etc/blkid.tab`**（宏 **`BLKID_CACHE_FILE`**，见 **`blkidP.h`**）等缓存文件路径。 |
| **`blkid_dev`** | 单个块设备（路径 **`bid_name`**）、**设备号**、**推断的 TYPE**、**标签链表**，以及验证时间等。 |
| **Tag** | **`NAME=value`**，如 **`UUID=…`、`LABEL=…`、`TYPE=ext4`**；支持按类型遍历、按 **NAME+VALUE** 反查设备。 |
| **Probe** | 打开设备，按 **`probe.h`** 中的 **`blkid_magic`** 表（偏移、魔数、probe 函数）识别内容，并填充标签。 |

链接依赖（**`Makefile.in`**）：**`ELF_OTHER_LIBS = -luuid`**（解析/打印 UUID 等需 **`libuuid`**）。

---

## 2. 对外 API（`blkid.h.in`）

主要分组如下（完整声明以 **`blkid.h.in`** 为准）。

| 分组 | 代表函数 | 作用 |
|------|----------|------|
| **缓存** | **`blkid_get_cache`、`blkid_put_cache`、`blkid_gc_cache`** | 创建/释放缓存，可选从文件载入；**`blkid_gc_cache`** 做无效项回收。 |
| **设备枚举** | **`blkid_dev_iterate_begin` / `blkid_dev_set_search` / `blkid_dev_next` / `blkid_dev_iterate_end`** | 在缓存中按 **TYPE 等** 条件遍历设备。 |
| **全量探测** | **`blkid_probe_all`、`blkid_probe_all_new`** | 扫描 **`/proc/partitions`**（及相关逻辑）等设备并探测。 |
| **单设备** | **`blkid_get_dev(cache, devname, flags)`** | **`BLKID_DEV_*`** 控制是否创建条目、是否重新读盘验证等。 |
| **探测与校验** | **`blkid_verify`、`blkid_known_fstype`** | 对已缓存设备再次确认；查询某 **TYPE** 字符串是否被库识别。 |
| **解析** | **`blkid_get_tag_value`、`blkid_get_devname`** | 由 **TAG** 或 **`NAME=value`** 令牌解析出 **设备名** 或某 **tag 值**（**`resolve.c`**）。 |
| **标签遍历** | **`blkid_tag_iterate_*`、`blkid_dev_has_tag`、`blkid_find_dev_with_tag`、`blkid_parse_tag_string`** | 遍历设备上所有 tag；全局按 **TYPE/LABEL/UUID** 查找设备。 |
| **底层辅助** | **`blkid_dev_devname`、`blkid_devno_to_devname`、`blkid_get_dev_size`** | 设备路径、**`dev_t` → 路径**、**`fd` 上查询大小**（**`getsize.c`**）。 |
| **版本** | **`blkid_get_library_version`、`blkid_parse_version_string`** | 库版本号兼容性检查。 |

类型定义：**`blkid_loff_t`**（大设备偏移）、迭代器不透明指针等见 **`blkid_types.h.in`**（配置生成 **`blkid_types.h`**）。

---

## 3. `libblkid` 源文件与 `OBJS`（`Makefile.in`）

| 目标文件 | 源文件 | 功能概要 |
|----------|--------|----------|
| **`cache.o`** | **`cache.c`** | 缓存分配/初始化/释放；**调试掩码**；**`blkid_put_cache`** 时可选 **flush**（写回缓存文件）；安全环境下读 **`getenv`**（**`secure_getenv`** 等）。 |
| **`dev.o`** | **`dev.c`** | **`blkid_dev_*`** 迭代与 **`blkid_dev_devname`**；设备链表维护。 |
| **`devname.o`** | **`devname.c`** | **`blkid_probe_all*`、`blkid_get_dev`**：与 **`/proc`**、打开设备、调用 **probe** 的主流程；与缓存更新策略配合。 |
| **`devno.o`** | **`devno.c`** | **`blkid_devno_to_devname`**：**`stat`**/**`/dev`** 解析 major/minor。 |
| **`getsize.o`** | **`getsize.c`** | **`blkid_get_dev_size`**：ioctl/回退路径取块设备容量。 |
| **`llseek.o`** | **`llseek.c`** | 大偏移 **seek/read** 的移植层（历史 32/64 位 **off_t** 问题）。 |
| **`probe.o`** | **`probe.c`** | 核心 **内容识别**：**`struct blkid_probe`** 缓冲、**`get_buffer`**、各类 **bim_probe** 回调；**ext2/3/4、fat、xfs、jfs、reiser、swap、iso9660、lvm、raid** 等大量 **magic + 偏移** 逻辑（与 **`probe.h`** 中结构体字段对应）。 |
| **`read.o`** | **`read.c`** | **读入缓存文件**（XML 风格 **`<device …>path</device>`** 格式，见文件头注释）；解析 **TIME、TYPE、LABEL、UUID** 等。 |
| **`save.o`** | **`save.c`** | **`blkid_flush_cache`**：把内存缓存写回磁盘（**`blkidP.h`** 声明）；原子替换临时文件等。 |
| **`resolve.o`** | **`resolve.c`** | **`blkid_get_tag_value`、`blkid_get_devname`**：**LABEL/UUID → 设备路径** 类解析。 |
| **`tag.o`** | **`tag.c`** | 标签链表、**`blkid_find_dev_with_tag`**、字符串解析 **NAME=value**。 |
| **`version.o`** | **`version.c`** | 库版本字符串。 |

**内部头文件**（勿当作稳定应用 ABI 使用）：**`blkidP.h`**（**`struct blkid_struct_dev` / `struct blkid_struct_cache` / `struct blkid_struct_tag`**、**`BLKID_PROBE_MIN`** 等）、**`list.h`**（链表宏）、**`probe.h`**（魔数表与各文件系统 **superblock** 布局摘录）。

---

## 4. 与命令行 `blkid` 的关系

**`Makefile.in`** 会编译 **`../../misc/blkid.o`**（源 **`misc/blkid.c`**）并链接 **`libblkid.a`**、**`libuuid`** 得到 **`blkid`** 可执行文件。调试与脚本用法见 **`misc/blkid` 手册页**（若已安装）。

---

## 5. 测试

- **`tst_*`**：各模块带 **`TEST_PROGRAM`** 的独立测例（**`tst_cache`、`tst_probe`** 等）。  
- **`test_probe` + `test_probe.in` + `tests/*.results`**：对 **probe** 输出做**回归对比**（**`fullcheck`/`check`** 会运行 **`./test_probe`**）。  
- **`tst_types.c`**：与 **`blkid_types.h`** 生成的类型/layout 检查。

---

## 6. 推荐阅读顺序

1. **`blkid.h.in`** 与 **`libblkid.3.in`** — 公开契约。  
2. **`blkidP.h`** — 缓存/设备/标签内存模型。  
3. **`read.c` 文件格式注释 + `save.c` 写出格式** — 与 **`/etc/blkid.tab`** 持久化语义。  
4. **`probe.c` 前半 + `probe.h` 中 `blkid_magic`** — 识别顺序与各类魔数位置。

---

## 7. 与其它文档的关系

**`libuuid`** 见 **[libuuid 代码功能说明](./libuuid-代码功能说明.md)**；块设备上的 **ext 系结构**仍以 **[libext2fs 代码功能说明](./libext2fs-代码功能说明.md)** 为准，**`libblkid`** 中的 **ext2_super_block** 摘抄仅服务于 **probe** 字段提取。

---

*文档依据 `lib/blkid/Makefile.in` 的 `OBJS`/`SRCS`、`blkid.h.in`、`blkidP.h` 与代表性源文件归纳。*
