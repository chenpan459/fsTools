# lib/ext2fs 代码功能说明（e2fsprogs 1.46.5）

本文对 `e2fsprogs-1.46.5/lib/ext2fs/` 目录的职责与源文件归类做阅读指引，便于从 **`mke2fs` / `e2fsck` / `debugfs` / `resize2fs`** 等程序跳进库实现。

**路径**：`../e2fsprogs-1.46.5/lib/ext2fs/`

---

## 1. 库的定位

`libext2fs` 提供在用户态操作 **ext2/ext3/ext4（及特性组合）** 的公共能力：

- 打开块设备或镜像，维护 **`ext2_filsys`** 句柄（超级块、块组描述符、位图、I/O 通道等）；
- 在盘上布局与内存结构之间转换：**分配/释放块与 inode、目录与路径、extent、日志、扩展属性、校验和** 等；
- 通过 **`io_manager`** 抽象，同一套逻辑可跑在普通 Unix 块设备、undo、`sparse`、`qcow2`、测试 I/O 等后端上。

依赖的对外头文件主要在：

- `ext2_fs.h`：磁盘布局常量与 `struct ext2_super_block`、inode 等；
- `ext2fs.h`：`ext2_filsys`、位图、`dblist`、`ext2_file_t` 及大量 API 声明；
- `ext2_io.h`：`io_channel` / `io_manager`；
- `ext3_extents.h`、`ext4_acl.h` 等：extent、ACL 等子格式。

---

## 2. 核心抽象：句柄与 I/O

| 概念 | 含义 |
|------|------|
| **`ext2_filsys`** | 已打开文件系统的运行时状态：指向超级块、组描述符表、块/inode 位图、`io`、标志位等。 |
| **`io_channel` / `io_manager`** | 按块读写的设备抽象；`unix_io`、`undo_io`、`sparse_io`、`qcow2`、`test_io` 等见 **`io_manager.c`** 与各 `*_io.c`。 |
| **`struct ext2_super_block`** | 可先作为“参数块”用于 **`ext2fs_initialize`**，再与 **`openfs`** 读出的超级块合并理解。 |

典型调用链：

- **新建文件系统**：`ext2fs_initialize`（**`initialize.c`**）→ … → `ext2fs_allocate_tables`（**`alloc_tables.c`**）→ … → `ext2fs_close` / `ext2fs_close_free`（**`closefs.c`**）。
- **打开已有 FS**：`ext2fs_open` / `ext2fs_open2`（**`openfs.c`**）→ 读位图与组描述符 → 业务操作 → close。

---

## 3. 按功能分类的源文件（`.c`）

下表按**主题**归纳，与 `Makefile.in` 中 **`OBJS`** 及常见阅读顺序一致；**同一文件可能跨多主题**。

### 3.1 打开、初始化、关闭、复制

| 源文件 | 功能概要 |
|--------|----------|
| **`initialize.c`** | `ext2fs_initialize`：按超级块参数算块组数、描述符、位图内存、`group_desc_count` 等，打开 channel。 |
| **`openfs.c`** | `ext2fs_open`：读超级块与组描述符，初始化位图句柄、inode 大小等。 |
| **`closefs.c`** | 把脏元数据写回、释放 `ext2_filsys`。 |
| **`freefs.c`** | 释放内存资源（常与 close 路径配合）。 |
| **`dupfs.c`** | 复制/克隆文件系统句柄（如 resize 流程），**`RESIZE_LIB_OBJS`** 可编入。 |
| **`check_desc.c`** | 块组描述符合法性检查。 |
| **`swapfs.c`** | 字节序交换（非 little-endian 平台相关）。 |
| **`native.c`** | 本机字节序/类型帮助。 |
| **`version.c`** | 库版本信息。 |

### 3.2 I/O 与块层访问

| 源文件 | 功能概要 |
|--------|----------|
| **`io_manager.c`** | 注册/分派 `io_manager`。 |
| **`unix_io.c`**（或配置选中的 **`@OS_IO_FILE@`）** | 常规本地文件/块设备 I/O。 |
| **`undo_io.c`** | 可撤销写入，配合 `e2undo`。 |
| **`sparse_io.c` / `qcow2.c`** | Android sparse 镜像、qcow2 镜像访问。 |
| **`test_io.c`** | 测试用包装 I/O（**`TEST_IO_LIB_OBJS`**）。 |
| **`windows_io.c` / `nt_io.c` / `dosio.c`** | 其他平台后端（按配置）。 |
| **`flushb.c` / `llseek.c`** | 刷新、64 位定位辅助。 |
| **`getsize.c` / `getsectsize.c` / `finddev.c`** | 设备大小、扇区大小、设备发现。 |
| **`blknum.c`** | 块号与块组内索引换算等。 |
| **`valid_blk.c`** | 块号合法性。 |
| **`progress.c`** | 长时间操作的进度回调。 |

### 3.3 超级块、块组、元数据校验

| 源文件 | 功能概要 |
|--------|----------|
| **`alloc_sb.c`** | 超级块相关分配/更新逻辑。 |
| **`alloc_tables.c`** | **`ext2fs_allocate_tables`**：为每组放置 inode/块位图与 inode 表块等。 |
| **`alloc_stats.c`** | 分配统计/记账。 |
| **`res_gdt.c`** | 与在线 resize、GDT 扩展相关的预留与调整。 |
| **`csum.c` / `crc16.c` / `crc32c.c` / `gen_crc32ctable.c`** | 各类元数据与 inode/block **checksum**（如 `metadata_csum`）。 |
| **`sha512.c` / `sha256.c`** | 摘要算法（校验/特性相关路径）。 |

### 3.4 位图与子簇

| 源文件 | 功能概要 |
|--------|----------|
| **`bitmaps.c` / `rw_bitmaps.c`** | 读写 inode/块位图。 |
| **`gen_bitmap.c` / `gen_bitmap64.c`** | 通用位图实现。 |
| **`blkmap64_ba.c` / `blkmap64_rb.c`** | 64 位块位图：位数组与红黑树等实现。 |
| **`bitops.c`** | 位运算辅助。 |

### 3.5 块分配、映射、extent、fallocate

| 源文件 | 功能概要 |
|--------|----------|
| **`alloc.c`** | 新块、新 inode、预分配等核心分配器。 |
| **`block.c` / `bmap.c` / `i_block.c` / `ind_block.c`** | 逻辑块 → 物理块映射（含传统间接块路径）。 |
| **`extent.c`** | **extent** 树：增删改查、与 `ext3_extents.h` 布局一致。 |
| **`fallocate.c` / `punch.c`** | 预分配、打洞释放块范围。 |

### 3.6 Inode 与内联数据

| 源文件 | 功能概要 |
|--------|----------|
| **`inode.c` / `inode_io.c`** | inode 读写、扩展 inode 字段（`extra_isize` 等）。 |
| **`inline.c` / `inline_data.c`** | 内联数据（小文件数据放在 inode 内）。 |

### 3.7 目录与路径

| 源文件 | 功能概要 |
|--------|----------|
| **`namei.c` / `lookup.c`** | 路径解析、目录项查找。 |
| **`dir_iterate.c` / `dirblock.c`** | 遍历目录块、目录块级读写。 |
| **`mkdir.c` / `link.c` / `unlink.c` / `symlink.c`** | 建链、删链、符号链接。 |
| **`newdir.c` / `expanddir.c`** | 新建目录项、扩展目录。 |
| **`dirhash.c`** | 目录哈希（HTree）相关。 |
| **`nls_utf8.c`** | 大小写折叠文件名编码（`casefold` 等特性）。 |

### 3.8 扩展属性、配额、dblist、坏块

| 源文件 | 功能概要 |
|--------|----------|
| **`ext_attr.c`** | 扩展属性块布局与访问。 |
| （配额相关可能在 **`quota.c`** 于其他目录；libext2fs 内若有则配合） | 项目/用户/组配额 inode 等由上层 **`misc`/quotaio** 与库内 inode 操作衔接。 |
| **`dblist.c` / `dblist_dir.c`** | 目录 inode → 所用块列表（加速某些工具）。 |
| **`badblocks.c` / `bb_inode.c` / `read_bb.c` / `read_bb_file.c` / `write_bb_file.c`** | 坏块列表与坏块 inode 更新。 |
| **`bb_compat.c`** | 坏块兼容逻辑，**`DEBUGFS_LIB_OBJS`** 可编入静态库。 |

### 3.9 日志（jbd2 用户态侧）

| 源文件 | 功能概要 |
|--------|----------|
| **`mkjournal.c`** | 创建/附加 journal（含 **`ext2fs_zero_blocks2`** 等，亦为其它模块提供零块写）。 |
| 源码树中 **`e2fsck/revoke.c`、`recovery.c`** 的部分目标会编进 **libext2fs**（见 Makefile 中 **resize/debugfs 相关 OBJ**），用于日志撤销与恢复路径。 |

### 3.10 MMP、镜像、其它

| 源文件 | 功能概要 |
|--------|----------|
| **`mmp.c`** | Multiple Mount Protection 元数据初始化/更新。 |
| **`ismounted.c`** | 挂载检测辅助（与上层工具共用）。 |
| **`imager.c`** | **`E2IMAGE_LIB_OBJS`**：镜像相关（e2image 功能）。 |
| **`fileio.c`** | 类文件的块级缓冲 I/O（`ext2_file_t`）。 |
| **`hashmap.c` / `rbtree.c`** | 库内部数据结构。 |
| **`atexit.c`** | 进程退出钩子（清理）。 |
| **`icount.c`** | inode 引用计数抽象（e2fsck 等大量使用）。 |
| **`get_num_dirs.c` / `get_pathname.c`** | 统计与路径辅助。 |

### 3.11 错误码与 TDB

| 源文件 | 功能概要 |
|--------|----------|
| **`ext2_err`（由 `compile_et` 生成）** | 与 **`com_err`** 搭配的错误码表。 |
| **`tdb.c`** | 轻量键值库（可选 **`CONFIG_TDB`**），用于部分场景持久化。 |

### 3.12 仅测试或独立小程序的 `.c`

目录中存在大量 **`tst_*.c`**：单元/一致性测试，一般不链接进正式 **`libext2fs.so`**，由 **`make check`** 单独编译运行。另有 **`tdbtool.c`** 等工具源。

---

## 4. Makefile 特殊点（为何部分 `.c` 在“别的目录”）

`lib/ext2fs/Makefile.in` 会把下列一类对象编进 **`libext2fs`**，以减少循环依赖、共用静态库目标：

- **`DEBUGFS_LIB_OBJS`**：如 **`bb_compat.o`**、**`inode_io.o`**、**`write_bb_file.o`**（路径可指向上级 `debugfs/`、`misc/`、`e2fsck/` 的源码）。
- **`RESIZE_LIB_OBJS`**：**`dupfs.o`**。
- **`TEST_IO_LIB_OBJS`**：**`test_io.o`**。
- **`E2IMAGE_LIB_OBJS`**：**`imager.o`**。

因此阅读 **`debugfs`**、`**e2fsck/journal**` 相关代码时，可能在 **本目录的 `.o` 列表**里看到“外来”源文件路径。

---

## 5. 推荐阅读顺序

1. **`ext2_io.h` + `io_manager.c` + `unix_io.c`**：理解所有块访问的边界。  
2. **`initialize.c` + `openfs.c` + `alloc_tables.c`**：理解卷如何被“搭起来”。  
3. **`inode.c`、`extent.c`、`bmap.c`**：数据定位主路径。  
4. **`mkjournal.c`、`csum.c`**：日志与校验。  
5. **`closefs.c`**：理解脏数据何时落盘。

更完整的 API 列表见上游 **`doc/libext2fs.info`**（Texinfo）或已安装的 info 页。

---

## 6. 与现有文档的关系

本说明专门对应 **`lib/ext2fs` 目录**；仓库根级的整体说明见同目录下 **[e2fsprogs-1.46.5-源码分析.md](./e2fsprogs-1.46.5-源码分析.md)**。

---

*文档根据工作区 `e2fsprogs-1.46.5/lib/ext2fs/Makefile.in` 的 `OBJS` 列表与文件命名归纳，若本地配置关掉 resize/debugfs/imager，部分 `.o` 不会出现在你的构建产物中，属正常现象。*
