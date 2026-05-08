# lib/uuid 代码功能说明（e2fsprogs 1.46.5）

本文说明 **`e2fsprogs-1.46.5/lib/uuid/`** 目录：实现 **DCE 兼容的 128 位 UUID** 库（`libuuid`），供 **`mke2fs`、`uuidd`/`uuidgen`、文件系统超级块 UUID** 等使用。

**路径**：`../e2fsprogs-1.46.5/lib/uuid/`

---

## 1. 库的定位与对外 API

- **类型**：`uuid_t` 为 **16 字节无符号字符数组**（`typedef unsigned char uuid_t[16]`），与 RFC 4122 / DCE 盘上字节序一致（见 **`pack`/`unpack`**）。
- **公开头文件**：构建后生成 **`uuid/uuid.h`**（源为 **`uuid.h.in`**）；类型宽度等见 **`uuid_types.h.in`** → 配置生成的 **`uuid_types.h`**。
- **内部头文件**：**`uuidP.h`** — 定义逻辑结构体 **`struct uuid`**（按字段拆分的时间/版本/时钟序列/MAC）及 **`uuid_pack` / `uuid_unpack`** 原型，**不对外**。

**公共 API 与源文件对应**（与 `uuid.h.in` 注释一致）：

| API | 源文件 | 作用 |
|-----|--------|------|
| `uuid_clear` | `clear.c` | 将 `uuid_t` 置为全 0 |
| `uuid_compare` | `compare.c` | 字典序比较两个 UUID |
| `uuid_copy` | `copy.c` | 拷贝 |
| `uuid_generate` / `uuid_generate_random` / `uuid_generate_time` | **`gen_uuid.c`** | 生成 UUID（随机、基于时间等策略，见下节） |
| `uuid_is_null` | `isnull.c` | 是否全 0 |
| `uuid_parse` | `parse.c` | 解析 `xxxxxxxx-xxxx-...` 字符串 → 二进制 |
| `uuid_unparse` / `uuid_unparse_lower` / `uuid_unparse_upper` | `unparse.c` | 二进制 → 字符串 |
| `uuid_time` / `uuid_type` / `uuid_variant` | **`uuid_time.c`** | 从 UUID 取时间戳（对 time-based）、判定类型与 variant |

**字节序**：`pack.c` / `unpack.c` 在 **`struct uuid`（主机字段）** 与 **`uuid_t`（网络/标准字节序列）** 之间转换。

---

## 2. `libuuid` 构建产物（`Makefile.in`）

**`OBJS`** 即链接进静态库/动态库的目标文件：

```
clear.o compare.o copy.o gen_uuid.o isnull.o pack.o parse.o unpack.o unparse.o uuid_time.o
```

- **`gen_uuid.c`** 体积最大：随机源（**`/dev/urandom`、getrandom、jrand48 等**）、可选 **`uuidd` 守护进程**套接字（**`uuidd.h`** 协议）、多平台（含 **`_WIN32`** 分支，另有单独的 **`gen_uuid_nt.c`**，是否编入视配置而定，**不一定在默认 OBJS 中**）。

- **测试**：**`tst_uuid.c`** 编为 **`tst_uuid`**，依赖静态 `libuuid`。

- **调试小程序**：**`uuid_time.c`** 在开启 DEBUG 时可编为 **`uuid_time`**，用于开发时打印时间信息（见 Makefile `uuid_time` 目标）。

- **手册页**：`*.3.in` 经替换生成 **`uuid.3`、`uuid_generate.3`** 等；**`uuid.pc.in`** 生成 **pkg-config** 文件。

---

## 3. 各源文件功能概述

| 文件 | 功能 |
|------|------|
| **`clear.c`** | `memset` 为 0。 |
| **`compare.c`** | `memcmp(uu1, uu2, sizeof(uuid_t))`。 |
| **`copy.c`** | `memcpy`。 |
| **`pack.c` / `unpack.c`** | **`struct uuid` ↔ `uuid_t`**，按 UUID 标准排布 16 字节。 |
| **`parse.c`** | 固定 36 字符标准串 + 4 个 `-`，十六进制解析；失败返回非 0。 |
| **`unparse.c`** | `sprintf` 输出标准格式；lower/upper 控制 A–F 大小写。 |
| **`isnull.c`** | 与全 0 比较。 |
| **`uuid_time.c`** | time-based UUID：从 **`time_low/mid/hi`** 等还原 **1582–1970 时间偏移**（见 **`uuidP.h`** 中 `TIME_OFFSET_*`）；`uuid_type`、`uuid_variant` 读版本与 variant 位。 |
| **`gen_uuid.c`** | **`uuid_generate`**：通常优先高质量随机；**`uuid_generate_time`**：时间 + 时钟序列 + 节点（MAC/随机）；可与 **`uuidd`** 通信批量取号以降低文件描述符占用（Linux 常见）。 |

---

## 4. 与 e2fsprogs 其它部分的关系

- **`mke2fs`**：创建/指定文件系统 **`s_uuid`**（`uuid_generate` / `uuid_parse` 等）。
- **`libext2fs`**：超级块、**`metadata_csum_seed`** 等与 UUID 字节交互。
- **`misc/uuidd.c`**：**守护进程**与本库 **`uuidd.h`** 协议配合（非 `lib/uuid` 目录内，但依赖本库 API）。

发行版中系统可能已提供 **`libuuid`（util-linux）**；e2fsprogs 自带副本用于 **自包含构建** 与版本一致性。

---

## 5. 推荐阅读顺序

1. **`uuid.h.in`** — 公共类型与全部函数声明。  
2. **`uuidP.h` + `pack.c`** — 理解 16 字节布局与 `struct uuid`。  
3. **`gen_uuid.c`** — 只读 **`uuid_generate` / `uuid_generate_time`** 主路径即可。  
4. **`parse.c` / `unparse.c`** — 字符串互转。  
5. **`uuid_time.c`** — 与 time-based UUID 调试相关。

---

*文档依据 `lib/uuid/Makefile.in` 的 `OBJS`/`SRCS` 与源文件头注释归纳。*
