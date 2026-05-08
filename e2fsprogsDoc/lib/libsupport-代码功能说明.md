# lib/support 代码功能说明（e2fsprogs 1.46.5）

本文归纳 **`e2fsprogs-1.46.5/lib/support/`**：e2fsprogs 内部使用的 **支持库 `libsupport`**，与 **MIT Kerberos 风格的配置文件解析（profile）**、**格式化前设备合理性检查（plausible）**、**ext4 配额（quota）** 及**字符串辅助**等相关。

**路径**：`../e2fsprogs-1.46.5/lib/support/`

---

## 1. 定位与依赖

- **产物**：静态/共享形态 **`libsupport`**（`Makefile.in` 中 `LIBRARY= libsupport`，安装子目录 **`support`**）。
- **主要消费者**：**`mke2fs`**（**`mke2fs.conf`**）、**`e2fsck`**、**`tune2fs`**、**`debugfs`** 等通过 **profile** 读配置；**配额**路径与 **`libext2fs`**、**`misc/quotaio`** 协作（以本目录 **`quotaio*.c`、`mkquota.c`** 为核）。
- **国际化**：**`nls-enable.h`** 在启用 NLS 时包装 **`gettext`**，否则将 `_()` 退化为恒等宏（调试或未启用 NLS 时无翻译）。

---

## 2. 默认编入 `libsupport.a` 的对象（`Makefile.in` 中 `OBJS`）

| 目标文件 | 源文件 | 功能概要 |
|----------|--------|----------|
| `cstring.o` | **`cstring.c`** | 安全/可复用的 C 字符串与缓冲区操作（见 **`cstring.h`**）。 |
| `mkquota.o` | **`mkquota.c`** | 创建/维护配额相关的 inode 与元数据，配合 **`quotaio`**。 |
| `plausible.o` | **`plausible.c`** | **`check_plausibility()`**：在 **`mke2fs`/`tune2fs`** 等写盘前检查设备是否像块设备、是否已有文件系统签名等（见 **`plausible.h`** 标志位）。 |
| `profile.o` | **`profile.c`** | **MIT 衍生 `profile` 库**子集：解析类似 INI 的分层配置（**`[section]`**、**`relation`/`子关系`**），供 **`profile.h`** 声明的 **`profile_init` / `profile_get_*`** 等使用；**`mke2fs.conf`**、**`e2fsck.conf`** 等均依赖此机制。 |
| `parse_qtype.o` | **`parse_qtype.c`** | 解析配额类型字符串（usr/grp/prj 等与 **`quotaio.h`** 内位标志对应）。 |
| `profile_helpers.o` | **`profile_helpers.c`** | profile 的辅助例程（与 **`profile_helpers.h`** 配套）。 |
| `prof_err.o` | **`prof_err.c`**（由 **`prof_err.et`** 经 **`compile_et`** 生成） | **`com_err`** 风格错误表，与 profile 操作返回值配合。 |
| `quotaio.o` | **`quotaio.c`** | 配额上下文、写回配额 inode、与 **`libext2fs`** 的超级块/`quota` 特性协作（见 **`quotaio.h`** 头注释中的典型用法）。 |
| `quotaio_v2.o` | **`quotaio_v2.c`** | **VFS v2** 配额文件格式读写（**`quotaio_v2.h` / `dqblk_v2.h`**）。 |
| `quotaio_tree.o` | **`quotaio_tree.c`** | 配额在内存中的树形索引（**`quotaio_tree.h`**），用于加速按 id 查找。 |
| `dict.o` | **`dict.c`** | 通用字典/哈希表（**`dict.h`**），配额与其它模块可能共享。 |

---

## 3. 源文件存在但未列入默认 `OBJS` 的组件

| 源文件 | 说明 |
|--------|------|
| **`argv_parse.c`** | 命令行风格参数解析辅助（**`argv_parse.h`**）。**不链接进默认 `libsupport.a`**；仅在 Makefile **`test_profile`** 调试目标中与 **`profile.c`** 等一起编译，用于独立测试 profile。 |

---

## 4. 头文件（无对应 `.o` 或作公共包含）

| 头文件 | 作用 |
|--------|------|
| **`profile.h`** | 对外 profile API（**`profile_t`**、`profile_get_string`/`integer`/…）。 |
| **`profile_helpers.h`** | profile 实现辅助声明。 |
| **`plausible.h`** | **`check_plausibility`** 及 **`CHECK_*`** 标志。 |
| **`quotaio.h`** | 配额库主接口：**`quota_ctx_t`**、**`QUOTA_*_BIT`**、**`quota_init_context` / `quota_write_inode`** 等（含 ext4 超级块字段配合说明）。 |
| **`quotaio_v2.h` / `dqblk_v2.h`** | **v2 dqblk / quota 文件**布局与常量。 |
| **`quotaio_tree.h`** | 内存配额树类型。 |
| **`dict.h`** | 字典抽象。 |
| **`common.h`** | 配额等模块公共宏/类型。 |
| **`cstring.h`** | cstring API。 |
| **`argv_parse.h`** | 仅调试用 **`argv_parse`**。 |
| **`nls-enable.h`** | **`_()` / `N_()`** 翻译宏（见上文）。 |
| **`sort_r.h`** | 可移植 **`qsort_r`** 类排序（若其它 `.c` 包含则使用）。 |

---

## 5. 与其它目录的关系（简图）

```
mke2fs / tune2fs / e2fsck
        │
        ├─► lib/support/profile*.c     ← mke2fs.conf、e2fsck.conf
        ├─► lib/support/plausible.c  ← 写盘前“是否合理”
        └─► lib/support/quotaio*.c    ← 配额 + ext2fs 超级块字段 + 配额 inode
                      │
                      ▼
               lib/ext2fs（块/inode 操作）
```

---

## 6. 推荐阅读顺序

1. **`plausible.h` + `plausible.c`**：理解 **`check_plausibility`** 在哪些标志下会提示用户。  
2. **`profile.h` + `profile.c`（及 `profile_helpers.c`）**：理解 **`mke2fs.conf`** 分段如何映射到 **`profile_get_string`** 等。  
3. **`quotaio.h` + `quotaio.c` + `quotaio_v2.c`**：理解配额与 ext4 的衔接。  
4. **`cstring.h` / `dict.h`**：按需查阅实现细节。

---

## 7. 与现有文档的索引关系

同目录下整工程说明见 **[e2fsprogs-1.46.5-源码分析.md](./e2fsprogs-1.46.5-源码分析.md)**；**libext2fs** 专文见 **[libext2fs-代码功能说明.md](./libext2fs-代码功能说明.md)**。`lib/support` 与 **`misc/mke2fs.c`** 中 **`profile`/`check_plausibility`** 调用强相关。

---

*文档依据 `lib/support/Makefile.in` 的 `OBJS`/`SRCS` 与头文件说明归纳；若本地未安装完整 NLS，`_()` 行为以 `nls-enable.h` 为准。*
