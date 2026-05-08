# lib/e2p 代码功能说明（e2fsprogs 1.46.5）

本文归纳 **`e2fsprogs-1.46.5/lib/e2p/`**：**Second extended filesystem utility functions**，发布为 **`libe2p`**（静态/共享库视构建选项而定）与 **`e2p.h`**。它不实现磁盘上的读写逻辑，而是为 **`tune2fs`、`dumpe2fs`、`debugfs`、`misc/mke2fs`** 等工具提供**与 superblock、特性位、挂载默认值、inode 标志、字符串互转及人类可读输出**相关的辅助函数，依赖 **`ext2fs/ext2_fs.h`**（通过 **`ext2fs`** 头）。

**路径**：`../e2fsprogs-1.46.5/lib/e2p/`

---

## 1. 定位与依赖

| 项目 | 说明 |
|------|------|
| **公共头文件** | **`e2p.h`**（安装路径通常为 **`include/e2p/e2p.h`**）。 |
| **对内核/磁盘结构的依赖** | 使用 **`struct ext2_super_block`**、特性常量、**`EXT2_DEFM_*`** 等，与 **`libext2fs`** 所暴露的磁盘布局一致。 |
| **pkg-config** | **`e2p.pc.in`** → **`e2p.pc`**。 |

---

## 2. `libe2p` 的组成（`Makefile.in` 中 `OBJS`）

下表按主题归纳各 **`.o`**，与 **`SRCS`** 一一对应。

### 2.1 Superblock 与人类可读列表

| 目标文件 | 源文件 | 功能概要 |
|----------|--------|----------|
| **`ls.o`** | **`ls.c`** | **`list_super` / `list_super2`**：将 **`ext2_super_block`** 逐项格式化输出到 **`FILE*`**（含 UID/GID 解析、特性摘要、`print_features` 等内部逻辑）。 |
| **`ljs.o`** | **`ljs.c`** | **`e2p_list_journal_super`**：按缓冲区中的 **journal superblock** 结构解码并打印（与 **`kernel-jbd.h`** 类型配合）；可选 **`E2P_LIST_JOURNAL_FLAG_FC`** 等标志。 |
| **`pe.o`** | **`pe.c`** | **`print_fs_errors`**：将 **`s_errors`**（Continue / Remount RO / Panic）译为英文描述。 |
| **`ps.o`** | **`ps.c`** | **`print_fs_state`**：文件系统状态字符串（如 clean / errors 等）。 |

### 2.2 特性、挂载默认、哈希、UUID、OS 类型

| 目标文件 | 源文件 | 功能概要 |
|----------|--------|----------|
| **`feature.o`** | **`feature.c`** | **`e2p_feature_to_string`、`e2p_feature2string`、`e2p_jrnl_feature2string`、`e2p_string2feature*`、`e2p_edit_feature*`**：文件系统 / 日志特性位与 **`has_journal`、`extent`** 等名称互转及批量编辑。 |
| **`mntopts.o`** | **`mntopts.c`** | **`e2p_mntopt2string`、`e2p_string2mntopt`、`e2p_edit_mntopts`**：默认挂载选项（**`EXT2_DEFM_*`、`EXT3_DEFM_*`、`EXT4_DEFM_*`**）与 `mke2fs.conf` / superblock 中字段对应。 |
| **`hashstr.o`** | **`hashstr.c`** | **`e2p_hash2string`、`e2p_string2hash`**：目录哈希算法名（**`legacy`、`half_md4`、`tea`** 等）。 |
| **`uuid.o`** | **`uuid.c`** | **`e2p_is_null_uuid`、`e2p_uuid_to_str`、`e2p_uuid2str`**：16 字节 FS UUID 与打印用字符串。 |
| **`ostype.o`** | **`ostype.c`** | **`e2p_os2string`、`e2p_string2os`**：创建文件系统时的 “OS id”（**`s_creator_os`**）名称；可带 **`TEST_PROGRAM`** 自测目标 **`tst_ostype`**。 |

### 2.3 Inode 标志（chattr/lsattr 语义）

| 目标文件 | 源文件 | 功能概要 |
|----------|--------|----------|
| **`getflags.o` / `setflags.o`** | **`getflags.c` / `setflags.c`** | 对**打开的文件描述符**通过 **`ioctl(FS_IOC_GETFLAGS/SETFLAGS)`**（及 BSD **`st_flags`** 等路径）读写 **ext2 风格 inode 标志**。 |
| **`fgetflags.o` / `fsetflags.o`** | **`fgetflags.c` / `fsetflags.c`** | 按**路径**打开后同上。 |
| **`getversion.o` / `setversion.o`** | **`getversion.c` / `setversion.c`** | **inode 版本号**（**`EXT2_IOC_GETVERSION/SETVERSION`** 系）。 |
| **`fgetversion.o` / `fsetversion.o`** | **`fgetversion.c` / `fsetversion.c`** | 路径方式。 |
| **`pf.o`** | **`pf.c`** | **`print_flags`**：将标志位展开为 **`chattr`** 风格短串或 **`PFOPT_LONG`** 长名称。 |

### 2.4 Project、加密、编码与其它

| 目标文件 | 源文件 | 功能概要 |
|----------|--------|----------|
| **`fgetproject.o` / `fsetproject.o`** | **`fgetproject.c` / `fsetproject.c`** | 经 **`FS_IOC_FSGETXATTR` / `FS_IOC_FSSETXATTR`**（见 **`project.h`**）读写 **project id**；无 ioctl 时返回 **`EOPNOTSUPP`**。 |
| **`crypto_mode.o`** | **`crypto_mode.c`** | **`e2p_encmode2string`、`e2p_string2encmode`**： **`EXT4_ENCRYPTION_MODE_*`** 与可读串。 |
| **`encoding.o`** | **`encoding.c`** | **`e2p_str2encoding`、`e2p_encoding2str`、`e2p_get_encoding_flags`、`e2p_str2encoding_flags`**：区分大小写折叠等字符编码相关元数据。 |
| **`errcode.o`** | **`errcode.c`** | **`e2p_errcode2str`**：将数值错误码映射为短名称（如 **`EIO`、`ENOMEM`**），用于报告路径。 |

### 2.5 通用辅助

| 目标文件 | 源文件 | 功能概要 |
|----------|--------|----------|
| **`iod.o`** | **`iod.c`** | **`iterate_on_dir`**：遍历目录并对每个 **`dirent`** 调用回调（分配足够大的 **`dirent`** 缓冲区以应对长文件名）。 |
| **`parse_num.o`** | **`parse_num.c`** | **`parse_num_blocks` / `parse_num_blocks2`**：解析用户传入的块数/带 suffix 的数字，并结合 **`log_block_size`**。 |
| **`percent.o`** | **`percent.c`** | **`e2p_percent`**：按百分比与基数计算整型结果（工具里进度/阈值类用途）。 |

---

## 3. 附加头文件

| 文件 | 说明 |
|------|------|
| **`project.h`** | **`fgetproject`/`fsetproject`** 使用的 **`fsxattr`** 等 ioctl 相关声明；与 **`e2p.h`** 分担职责。 |

---

## 4. 构建与自检

- **`Makefile.in`**：`LIBRARY=libe2p`，**`fullcheck`/`check`** 会编译并运行 **`tst_ostype`、`tst_feature`**（在 **`ostype.c`、`feature.c`** 中定义 **`TEST_PROGRAM`**）。  
- **`Android.bp`**：Android 树中的对应模块定义（若交叉参考移植场景）。

---

## 5. 在 e2fsprogs 中的关系

- **与 `libext2fs` 分工**：**`libext2fs`** 负责打开卷、读写块与 inode；**`libe2p`** 侧重**用户可见的翻译、打印、路径/ioctl 包装**，二者常一起被上层程序链接。  
- **与 `lib/et`**：错误报告仍走 **`com_err`**；**`e2p`** 自身的 **`e2p_errcode2str`** 是**另一套**简短名字表，不要与 **`error_message()`** 混淆。  

---

## 6. 推荐阅读顺序

1. **`e2p.h`** — 函数列表与宏。  
2. **`feature.c` + `mntopts.c`** — 特性字符串与默认挂载选项（**`dumpe2fs`/`tune2fs`** 最常用）。  
3. **`ls.c`** — superblock 完整 dump 布局。  
4. **`pf.c` + `fgetflags.c`** — 与 **`chattr`/`lsattr`** 行为一致的标志层。

---

## 7. 与其它文档的关系

总览见 **[e2fsprogs-1.46.5-源码分析.md](./e2fsprogs-1.46.5-源码分析.md)**；磁盘数据结构细节仍以 **`libext2fs/ext2_fs.h`** 与 **[libext2fs 代码功能说明](./libext2fs-代码功能说明.md)** 为准。

---

*文档依据 `lib/e2p/Makefile.in` 的 `OBJS`/`SRCS` 与 `e2p.h` 及代表性源文件归纳。*
